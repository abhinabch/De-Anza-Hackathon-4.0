#define CROW_MAIN
#define CROW_USE_BOOST
#include "crow_all.h"

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <iterator>

using namespace std;

// ===========================
//   SIMPLE TOS ANALYZER
// ===========================
vector<string> extractImportantClauses(const string& tosText) {
    static const vector<string> keywords = {
        "privacy", "data", "personal information", "cookies",
        "third party", "tracking", "fees", "billing", "liability"
    };

    vector<string> results;
    string sentence;

    for (char c : tosText) {
        sentence.push_back(c);

        if (c == '.') {
            string lower = sentence;
            transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            for (const auto& kw : keywords) {
                if (lower.find(kw) != string::npos) {
                    results.push_back(sentence);
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
                results.push_back(sentence);
                break;
            }
        }
    }

    return results;
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
    //   GET /analyze (test only)
    // ===========================
    CROW_ROUTE(app, "/analyze")([]() {
        return "Use POST /analyze with JSON { \"tosText\": \"...\" }";
    });

    // ===========================
    //   POST /analyze (real API)
    // ===========================
    CROW_ROUTE(app, "/analyze").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);

        if (!body || !body.has("tosText"))
            return crow::response(400, "Invalid JSON: expected { \"tosText\": \"...\" }");

        string text = body["tosText"].s();
        auto highlights = extractImportantClauses(text);

        crow::json::wvalue res;
        res["summary"] = "Detected important TOS clauses.";
        res["highlights"] = crow::json::wvalue::list();

        // assign by index (push_back does NOT exist)
        for (size_t i = 0; i < highlights.size(); i++) {
            res["highlights"][i] = highlights[i];
        }

        return crow::response(res);
    });

    // ===========================
    //   START SERVER
    // ===========================
    app.port(8080).multithreaded().run();
}