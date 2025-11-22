#define BOOST_ASIO_NO_DEPRECATED
#define BOOST_ASIO_STANDALONE
#define CROW_USE_BOOST
#define CROW_MAIN
#include "crow_all.h"
#include <string>
#include <vector>
#include <fstream>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <cstdlib>   // for getenv
#include <cstdio>    // for popen / pclose
<<<<<<< HEAD

#include <cstdlib>   // getenv
#include <cstdio>    // popen, pclose
#include <iostream>  // cout
=======
>>>>>>> parent of 9dbfadd (AI works)

using namespace std;

// Simple TOS keyword analyzer
vector<string> extractImportantClauses(const string& tosText) {
    static const vector<string> keywords = {
        "privacy", "data", "personal information", "personal info",
        "liability", "arbitration", "dispute", "termination",
        "fees", "billing", "third party", "third-party",
        "tracking", "cookies"
    };
    vector<string> importantSentences;
    string sentence;
    for (char c : tosText) {
        sentence.push_back(c);
        if (c == '.') {
            string lower = sentence;
            transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            for (const auto& kw : keywords) {
                if (lower.find(kw) != string::npos) {
                    importantSentences.push_back(sentence);
                    break;
                }
            }
            sentence.clear();
        }
    }
    if (!sentence.empty()) {
        string lower = sentence;
        transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        for (const auto& kw : keywords) {
            if (lower.find(kw) != string::npos) {
                importantSentences.push_back(sentence);
                break;
            }
        }
    }
    return importantSentences;
}

// Sanitize text so it can be safely embedded in JSON string for curl
string sanitizeForJson(string s) {
    for (char& c : s) {
        if (c == '"')  c = '\'';  // replace " with '
        if (c == '\n') c = ' ';   // flatten newlines
        if (c == '\r') c = ' ';
    }
    return s;
}

// Call OpenAI Chat Completions API using curl
string callOpenAI(const string& tosText) {
    const char* key = getenv("OPENAI_API_KEY");
    if (!key) {
        // Fallback JSON if key is missing
        return R"({"summary":"Error: OPENAI_API_KEY not set on server","highlights":[]})";
    }

    string safeText = sanitizeForJson(tosText);

    // Build JSON payload for OpenAI
    // We ask it to return EXACTLY the JSON shape our frontend expects.
    string jsonPayload =
        "{"
          "\"model\":\"gpt-4o-mini\","        // cheap, fast model :contentReference[oaicite:0]{index=0}
          "\"messages\":["
            "{"
              "\"role\":\"system\","
              "\"content\":\"You are a Terms of Service analyzer. Given raw TOS text, respond ONLY with strict JSON: {"
                "\\\"summary\\\": string, "
                "\\\"highlights\\\": array of strings. "
                "Each highlight must be a short important clause. No extra keys, no explanations outside JSON."
              "\""
            "},"
            "{"
              "\"role\":\"user\","
              "\"content\":\"" + safeText + "\""
            "}"
          "]"
        "}";

    // Build curl command
    string cmd =
        "curl -s https://api.openai.com/v1/chat/completions "
        "-H \"Content-Type: application/json\" "
        "-H \"Authorization: Bearer " + string(key) + "\" "
        "-d \"" + jsonPayload + "\"";

    // Run curl and capture output
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return R"({"summary":"Error running curl","highlights":[]})";
    }

    char buffer[4096];
    string response;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        response += buffer;
    }
    pclose(pipe);

    // The OpenAI Chat API returns an object with choices[0].message.content,
    // which itself will be a JSON string. To keep it simple, we just return
    // the raw API response and let the frontend parse content, OR you can
    // later parse it with crow::json if you want.
    return response;
}


int main() {
    crow::SimpleApp app;
    
    // Serve index.html from frontend directory
    CROW_ROUTE(app, "/")([]() {
        ifstream file("frontend/index.html");
        if (!file.good())
            return crow::response(404, "index.html not found in frontend/ directory");
        
        string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        crow::response res(content);
        res.add_header("Content-Type", "text/html");
        return res;
    });
    
    // OPTIONS /analyze -> Handle CORS preflight
    CROW_ROUTE(app, "/analyze").methods(crow::HTTPMethod::Post)
([](const crow::request& req) {
    auto body = crow::json::load(req.body);

    if (!body || !body.has("tosText"))
        return crow::response(400, "Invalid JSON: expected { \"tosText\": \"...\" }");

    string text = body["tosText"].s();

    // Call OpenAI
    string apiResponse = callOpenAI(text);

    // Parse OpenAI's outer JSON
    auto outer = crow::json::load(apiResponse);
    if (!outer || !outer.has("choices")) {
        // If OpenAI responded weirdly, just return some basic error JSON
        return crow::response(
            R"({"summary":"OpenAI error or bad response","highlights":[]})"
        );
    }

    // Extract choices[0].message.content (this should be our inner JSON as string)
    auto& choices = outer["choices"];
    if (choices.size() == 0 || !choices[0].has("message") || !choices[0]["message"].has("content")) {
        return crow::response(
            R"({"summary":"OpenAI response missing content","highlights":[]})"
        );
    }

    string contentJson = choices[0]["message"]["content"].s();

    // Now parse the inner JSON that the model produced
    auto inner = crow::json::load(contentJson);
    if (!inner) {
        return crow::response(
            R"({"summary":"Failed to parse inner JSON from OpenAI","highlights":[]})"
        );
    }

    // Return exactly what frontend expects
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
    
    app.bindaddr("0.0.0.0").port(8080).multithreaded().run();
}