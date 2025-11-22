#define BOOST_ASIO_NO_DEPRECATED
#define BOOST_ASIO_STANDALONE
#define CROW_USE_BOOST
#define CROW_MAIN
#include "crow_all.h"
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <cstdlib>   // getenv
#include <cstdio>    // popen, pclose
#include <iostream>  // cout

using namespace std;

// ===========================
//  Helper: escape for JSON
// ===========================
string escapeForJson(const string& s) {
    string out;
    out.reserve(s.size() * 2);
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

// ===========================
//  Call OpenAI via curl
//  (writes JSON to /tmp file)
// ===========================
string callOpenAI(const string& tosText) {
    const char* key = getenv("OPENAI_API_KEY");
    if (!key) {
        return R"({"summary":"Error: OPENAI_API_KEY not set","highlights":[]})";
    }

    // 1) Build JSON payload as a string (no crow::json::dump needed)
    string safeTos = escapeForJson(tosText);

    string systemPrompt =
        "You are a Terms of Service analyzer. "
        "Given raw TOS text, respond ONLY with strict JSON with keys: "
        "summary (string) and highlights (array of strings). "
        "No extra keys, no explanations outside JSON.";

    string safeSystem = escapeForJson(systemPrompt);

    string jsonPayload =
        "{"
          "\"model\":\"gpt-4o-mini\","
          "\"messages\":["
            "{"
              "\"role\":\"system\","
              "\"content\":\"" + safeSystem + "\""
            "},"
            "{"
              "\"role\":\"user\","
              "\"content\":\"" + safeTos + "\""
            "}"
          "]"
        "}";

    // 2) Write JSON to a temp file so curl can send it with -d @file
    string tmpPath = "openai_payload.json";  // <--- changed from /tmp/...
    {
        ofstream out(tmpPath);
        if (!out.good()) {
            return R"({"summary":"Error: could not write temp JSON file","highlights":[]})";
        }
        out << jsonPayload;
    }

    // 3) Build curl command using -d @file (no quoting issues)
    string cmd =
        "curl -s https://api.openai.com/v1/chat/completions "
        "-H \"Content-Type: application/json\" "
        "-H \"Authorization: Bearer " + string(key) + "\" "
        "-d @" + tmpPath;

    cout << "CURL CMD:\n" << cmd << "\n\n";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return R"({"summary":"Error: failed to run curl","highlights":[]})";
    }

    char buffer[4096];
    string response;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        response += buffer;
    }
    pclose(pipe);

    cout << "OpenAI raw response:\n" << response << "\n\n";
    return response;
}

// ===========================
//          MAIN
// ===========================
int main() {
    crow::SimpleApp app;
    
    // Serve index.html from frontend directory
    CROW_ROUTE(app, "/")([]() {
        ifstream file("frontend/index.html");
        if (!file.good())
            return crow::response(404, "index.html not found");

        string content(
            (istreambuf_iterator<char>(file)),
            istreambuf_iterator<char>()
        );

        crow::response res(content);
        res.add_header("Content-Type", "application/javascript");
        return res;
    });

    // ===========================
    //   SERVE main.js
    // ===========================
    CROW_ROUTE(app, "/main.js")([]() {
        ifstream file("frontend/main.js");
        if (!file.good())
            return crow::response(404, "main.js not found");

        string content(
            (istreambuf_iterator<char>(file)),
            istreambuf_iterator<char>()
        );

        crow::response res(content);
        res.add_header("Content-Type", "application/javascript");
        return res;
    });

    // ===========================
    //   POST /analyze  (AI-powered)
// ===========================
    CROW_ROUTE(app, "/analyze").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);

        if (!body || !body.has("tosText"))
            return crow::response(400, "Invalid JSON: expected { \"tosText\": \"...\" }");

        string text = body["tosText"].s();

        // 1) Call OpenAI
        string apiResponse = callOpenAI(text);

        // 2) Parse outer JSON
        auto outer = crow::json::load(apiResponse);
        if (!outer) {
            crow::json::wvalue res;
            res["summary"] = "OpenAI returned non-JSON response";
            res["highlights"] = crow::json::wvalue::list();
            res["raw"] = apiResponse;
            return crow::response(res);
        }

        // 3) Handle OpenAI error object
        if (outer.has("error") && outer["error"].has("message")) {
            string msg = outer["error"]["message"].s();

            crow::json::wvalue res;
            res["summary"] = "OpenAI error: " + msg;
            res["highlights"] = crow::json::wvalue::list();
            return crow::response(res);
        }

        // 4) Normal case: expect choices[0].message.content
        if (!outer.has("choices")) {
            crow::json::wvalue res;
            res["summary"] = "OpenAI response missing 'choices'";
            res["highlights"] = crow::json::wvalue::list();
            res["raw"] = apiResponse;
            return crow::response(res);
        }

        auto& choices = outer["choices"];
        if (choices.size() == 0 ||
            !choices[0].has("message") ||
            !choices[0]["message"].has("content")) {

            crow::json::wvalue res;
            res["summary"] = "OpenAI response missing message.content";
            res["highlights"] = crow::json::wvalue::list();
            res["raw"] = apiResponse;
            return crow::response(res);
        }

        string contentJson = choices[0]["message"]["content"].s();

        // 5) Parse the inner JSON produced by the model
        auto inner = crow::json::load(contentJson);
        if (!inner) {
            crow::json::wvalue res;
            res["summary"] = "Failed to parse inner JSON from OpenAI.";
            res["highlights"] = crow::json::wvalue::list();
            res["raw"] = contentJson;
            return crow::response(res);
        }

        // 6) Build final response
        crow::json::wvalue res;
        if (inner.has("summary"))
            res["summary"] = inner["summary"].s();
        else
            res["summary"] = "No summary returned from OpenAI.";

        res["highlights"] = crow::json::wvalue::list();
        if (inner.has("highlights")) {
            auto& arr = inner["highlights"];
            for (size_t i = 0; i < arr.size(); i++) {
                res["highlights"][i] = arr[i].s();
            }
        }

        return crow::response(res);
    });

    app.port(8080).multithreaded().run();
}