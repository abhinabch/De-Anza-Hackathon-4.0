#define CROW_MAIN
#define CROW_USE_BOOST
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
//  Call OpenAI via curl
//  (using Crow's JSON library for proper escaping)
// ===========================
string callOpenAI(const string& tosText) {
    const char* key = getenv("OPENAI_API_KEY");
    if (!key) {
        return R"({"summary":"Error: OPENAI_API_KEY not set","highlights":[]})";
    }

    // Build JSON payload
    crow::json::wvalue payload;
    payload["model"] = "gpt-4o-mini";
    payload["max_tokens"] = 60000;
    
    crow::json::wvalue system_msg;
    system_msg["role"] = "system";
    system_msg["content"] = "You are a Terms of Service analyzer. "
        "Respond ONLY with valid JSON with keys: "
        "summary (string) and highlights (array of strings).";
    
    crow::json::wvalue user_msg;
    user_msg["role"] = "user";
    user_msg["content"] = tosText;
    
    crow::json::wvalue msgs;
    msgs[0] = std::move(system_msg);
    msgs[1] = std::move(user_msg);
    payload["messages"] = std::move(msgs);

    string jsonPayload = payload.dump();
    
    // Write to file
    string tmpPath = "openai_payload.json";
    {
        ofstream out(tmpPath, ios::out | ios::binary);
        if (!out.good()) {
            return R"({"summary":"Error: could not write temp JSON file","highlights":[]})";
        }
        out.write(jsonPayload.c_str(), jsonPayload.length());
        out.close();
    }

    // Use PowerShell with better error handling
    string psCmd = 
        "powershell -NoProfile -Command \""
        "try { "
        "$jsonBody = Get-Content -Raw -Encoding UTF8 openai_payload.json; "
        "$headers = @{'Content-Type'='application/json'; 'Authorization'='Bearer " + string(key) + "'}; "
        "$response = Invoke-WebRequest -Uri 'https://api.openai.com/v1/chat/completions' "
        "-Method Post -Headers $headers -Body ([System.Text.Encoding]::UTF8.GetBytes($jsonBody)); "
        "$response.Content; "
        "} catch { "
        "Write-Output ('{\\\"error\\\":{\\\"message\\\":\\\"' + $_.Exception.Message + '\\\"}}'); "
        "}\"";

    cout << "Calling OpenAI API...\n";

    FILE* pipe = _popen(psCmd.c_str(), "r");
    if (!pipe) {
        return R"({"summary":"Error: failed to run PowerShell","highlights":[]})";
    }

    char buffer[8192];
    string response;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        response += buffer;
    }
    _pclose(pipe);

    // Remove any PowerShell formatting/whitespace
    size_t start = response.find('{');
    size_t end = response.rfind('}');
    if (start != string::npos && end != string::npos) {
        response = response.substr(start, end - start + 1);
    }

    cout << "OpenAI response:\n" << response << "\n\n";
    return response;
}

// ===========================
//          MAIN
// ===========================
int main() {
    crow::SimpleApp app;

    // ===========================
    //   SERVE index.html at "/"
    // ===========================
    CROW_ROUTE(app, "/")([]() {
        ifstream file("frontend/index.html");
        if (!file.good())
            return crow::response(404, "index.html not found");

        string content(
            (istreambuf_iterator<char>(file)),
            istreambuf_iterator<char>()
        );

        crow::response res(content);
        res.add_header("Content-Type", "text/html");
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

        if (inner.has("summary")) {
            res["summary"] = inner["summary"].s();
        } else {
            res["summary"] = "No summary returned.";
        }

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