// Tell Crow to use Boost.Asio instead of standalone Asio
#define CROW_USE_BOOST

// Tell Crow to generate main() for us
#define CROW_MAIN

#include "crow_all.h"

// app.cpp  
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

// Very simple keyword-based "AI" to find important clauses.
// Later you can replace this with a real LLM/API call.
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

    // Catch last sentence if no period
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

int main() {
    crow::SimpleApp app;

    // POST /analyze
    CROW_ROUTE(app, "/analyze").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("tosText")) {
            crow::response res(400);
            res.set_header("Access-Control-Allow-Origin", "*");
            res.write("Invalid JSON, expected { \"tosText\": \"...\" }");
            return res;
        }

        string tosText = body["tosText"].s();

        auto highlights = extractImportantClauses(tosText);

        crow::json::wvalue resJson;
        resJson["summary"] = "Auto-detected important clauses based on legal/privacy keywords.";

        crow::json::wvalue highlightArr;
        for (size_t i = 0; i < highlights.size(); ++i) {
            highlightArr[i] = highlights[i];
        }
        resJson["highlights"] = std::move(highlightArr);

        crow::response res(resJson);
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        return res;
    });

    // OPTIONS preflight handler
    CROW_ROUTE(app, "/analyze").methods(crow::HTTPMethod::Options)
    ([](const crow::request&) {
        crow::response res;
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        return res;
    });

    app.port(8080).multithreaded().run();
}