#define CROW_MAIN
#define CROW_USE_BOOST
#include "crow_all.h"

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <sstream>

using namespace std;

// -------------------------------
// Cross-platform popen
// -------------------------------
#ifdef _WIN32
    #define POPEN  _popen
    #define PCLOSE _pclose
#else
    #define POPEN  popen
    #define PCLOSE pclose
#endif

// -------------------------------
// Convert Crow detail::r_string to std::string
// -------------------------------
string toStdString(const crow::json::detail::r_string& s) {
    return string(s.begin(), s.end());
}

// ===========================
//  Split text into chunks
// ===========================
vector<string> chunkText(const string& text, size_t maxChars = 3000) {
    vector<string> chunks;
    size_t pos = 0;
    
    while (pos < text.length()) {
        size_t chunkSize = min(maxChars, text.length() - pos);
        
        // Try to break at sentence boundary
        if (pos + chunkSize < text.length()) {
            size_t lastPeriod = text.rfind('.', pos + chunkSize);
            if (lastPeriod != string::npos && lastPeriod > pos) {
                chunkSize = lastPeriod - pos + 1;
            }
        }
        
        chunks.push_back(text.substr(pos, chunkSize));
        pos += chunkSize;
    }
    
    return chunks;
}

// ===========================
//  Call OpenAI with a chunk (cross-platform)
// ===========================
string callOpenAIChunk(const string& tosChunk, int chunkNum, int totalChunks) {
    const char* key = getenv("OPENAI_API_KEY");
    if (!key) {
        return R"({"highlights":[]})";
    }

    // Build JSON payload
    crow::json::wvalue payload;
    payload["model"] = "gpt-4o-mini";
    payload["max_tokens"] = 800;
    
    crow::json::wvalue system_msg;
    system_msg["role"] = "system";
    system_msg["content"] = "You are analyzing Terms of Service. Extract important clauses about privacy, data collection, liability, fees, and user rights. "
        "Respond ONLY with valid JSON: {\"highlights\": [\"clause1\", \"clause2\", ...]}";
    
    crow::json::wvalue user_msg;
    user_msg["role"] = "user";
    
    stringstream content;
    content << "This is part " << chunkNum << " of " << totalChunks << " of a Terms of Service. "
            << "Extract important clauses:\n\n" << tosChunk;
    user_msg["content"] = content.str();
    
    crow::json::wvalue msgs;
    msgs[0] = std::move(system_msg);
    msgs[1] = std::move(user_msg);
    payload["messages"] = std::move(msgs);

    string jsonPayload = payload.dump();
    
    // Write to file
    string tmpPath = "openai_payload_" + to_string(chunkNum) + ".json";
    {
        ofstream out(tmpPath, ios::out | ios::binary);
        if (!out.good()) {
            return R"({"highlights":[]})";
        }
        out.write(jsonPayload.c_str(), jsonPayload.length());
        out.close();
    }

    // Cross-platform API call
    string cmd;

#ifdef _WIN32
    // Windows: Use PowerShell
    cmd = 
        "powershell -NoProfile -Command \""
        "try { "
        "$jsonBody = Get-Content -Raw -Encoding UTF8 " + tmpPath + "; "
        "$headers = @{'Content-Type'='application/json'; 'Authorization'='Bearer " + string(key) + "'}; "
        "$response = Invoke-WebRequest -Uri 'https://api.openai.com/v1/chat/completions' "
        "-Method Post -Headers $headers -Body ([System.Text.Encoding]::UTF8.GetBytes($jsonBody)); "
        "$response.Content; "
        "} catch { "
        "Write-Output '{\\\"highlights\\\":[]}'; "
        "}\"";
#else
    // Mac/Linux: Use curl
    cmd =
        "curl -s -X POST https://api.openai.com/v1/chat/completions "
        "-H \"Content-Type: application/json\" "
        "-H \"Authorization: Bearer " + string(key) + "\" "
        "--data-binary @" + tmpPath;
#endif

    FILE* pipe = POPEN(cmd.c_str(), "r");
    if (!pipe) {
        return R"({"highlights":[]})";
    }

    char buffer[8192];
    string response;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        response += buffer;
    }
    PCLOSE(pipe);

    // Extract JSON
    size_t start = response.find('{');
    size_t end = response.rfind('}');
    if (start != string::npos && end != string::npos) {
        response = response.substr(start, end - start + 1);
    }

    return response;
}

// ===========================
//  Analyze entire TOS in chunks
// ===========================
crow::json::wvalue analyzeFullTOS(const string& tosText) {
    crow::json::wvalue result;
    vector<string> allHighlights;
    
    // Split into chunks
    vector<string> chunks = chunkText(tosText, 3000);
    
    cout << "Splitting TOS into " << chunks.size() << " chunks...\n";
    
    // Analyze each chunk
    for (size_t i = 0; i < chunks.size(); i++) {
        cout << "Analyzing chunk " << (i+1) << "/" << chunks.size() << "...\n";
        
        string response = callOpenAIChunk(chunks[i], i + 1, chunks.size());
        
        // Parse response
        auto parsed = crow::json::load(response);
        if (!parsed) continue;
        
        // Handle error
        if (parsed.has("error")) {
            cout << "Error in chunk " << (i+1) << "\n";
            continue;
        }
        
        // Extract highlights from choices[0].message.content
        if (parsed.has("choices") && parsed["choices"].size() > 0) {
            auto& choice = parsed["choices"][0];
            if (choice.has("message") && choice["message"].has("content")) {
                string content = toStdString(choice["message"]["content"].s());
                
                // Parse the inner JSON
                auto inner = crow::json::load(content);
                if (inner && inner.has("highlights")) {
                    auto& highlights = inner["highlights"];
                    for (size_t j = 0; j < highlights.size(); j++) {
                        allHighlights.push_back(toStdString(highlights[j].s()));
                    }
                }
            }
        }
    }
    
    // Build final response
    result["summary"] = "AI analyzed " + to_string(chunks.size()) + 
                       " sections of your Terms of Service and found " + 
                       to_string(allHighlights.size()) + 
                       " important clauses regarding privacy, liability, fees, and user rights.";
    
    for (size_t i = 0; i < allHighlights.size(); i++) {
        result["highlights"][i] = allHighlights[i];
    }
    
    cout << "Analysis complete. Found " << allHighlights.size() << " total highlights.\n";
    
    return result;
}

// ===========================
//          MAIN
// ===========================
int main() {
    crow::SimpleApp app;

    // Serve index.html
    CROW_ROUTE(app, "/")([]() {
        ifstream file("frontend/index.html");
        if (!file.good())
            return crow::response(404, "index.html not found");

        string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        crow::response res(content);
        res.add_header("Content-Type", "text/html");
        return res;
    });

    // Serve main.js
    CROW_ROUTE(app, "/main.js")([]() {
        ifstream file("frontend/main.js");
        if (!file.good())
            return crow::response(404, "main.js not found");

        string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        crow::response res(content);
        res.add_header("Content-Type", "application/javascript");
        return res;
    });

    // CORS headers
    CROW_ROUTE(app, "/analyze").methods(crow::HTTPMethod::Options)
    ([]() {
        auto res = crow::response(200);
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");
        return res;
    });

    // POST /analyze
    CROW_ROUTE(app, "/analyze").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);

        if (!body || !body.has("tosText")) {
            auto res = crow::response(400, "Invalid JSON");
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }

        string text = toStdString(body["tosText"].s());
        
        cout << "Received TOS with " << text.length() << " characters\n";

        // Analyze with chunking
        crow::json::wvalue result = analyzeFullTOS(text);
        
        auto response = crow::response(result);
        response.add_header("Access-Control-Allow-Origin", "*");
        return response;
    });

    cout << "Starting TOS Analyzer with OpenAI chunking on port 8080...\n";
    app.port(8080).multithreaded().run();
}