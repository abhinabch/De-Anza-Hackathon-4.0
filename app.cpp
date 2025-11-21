#define BOOST_ASIO_NO_DEPRECATED
#define BOOST_ASIO_STANDALONE


#define CROW_USE_BOOST
#define CROW_MAIN
#include "crow_all.h"

#include <string>
#include <vector>
#include <algorithm>

using namespace std;

// Dumb keyword-based “AI” for now
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

int main() {
    crow::SimpleApp app;

    // GET /  -> test
    CROW_ROUTE(app, "/")([]() {
        return "Crow server is running on port 8080";
    });

    // GET /analyze -> simple message so browser GET doesn't 405
    CROW_ROUTE(app, "/analyze")([]() {
        return "Use POST /analyze with JSON { \"tosText\": \"...\" }";
    });

    // POST /analyze -> real API
    CROW_ROUTE(app, "/analyze").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("tosText")) {
            return crow::response(400, "Invalid JSON, expected { \"tosText\": \"...\" }");
        }

        std::string tosText = body["tosText"].s();
        auto highlights = extractImportantClauses(tosText);

        crow::json::wvalue res;
        res["summary"] = "Auto-detected important clauses based on legal/privacy keywords.";

        crow::json::wvalue arr;
        for (size_t i = 0; i < highlights.size(); ++i) {
            arr[i] = highlights[i];
        }
        res["highlights"] = std::move(arr);

        return crow::response{res};
    });

    app.bindaddr("0.0.0.0").port(8080).run();

}