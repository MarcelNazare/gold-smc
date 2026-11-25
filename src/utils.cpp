#include "goldsmc/utils.hpp"

#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <algorithm>

namespace {

// Trim leading/trailing whitespace.
std::string trim(const std::string& s) {
    auto l = s.find_first_not_of(" \t\r\n");
    if (l == std::string::npos) return "";
    auto r = s.find_last_not_of(" \t\r\n");
    return s.substr(l, r - l + 1);
}

} // namespace

namespace goldsmc::utils {

std::string getEnv(const std::string& name) {
    const char* v = std::getenv(name.c_str());
    return v ? std::string(v) : std::string{};
}

void loadDotEnv(const std::string& path) {
    auto tryLoad = [&](const std::string& p) -> bool {
        std::ifstream f(p);
        if (!f) return false;

        std::string line;
        int lineNo = 0;
        int loaded = 0;

        while (std::getline(f, line)) {
            ++lineNo;

            // Strip UTF-8 BOM on the first line.
            if (lineNo == 1 && line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line.erase(0, 3);
            }

            line = trim(line);
            if (line.empty() || line[0] == '#') continue;

            auto eq = line.find('=');
            if (eq == std::string::npos) continue;

            std::string key   = trim(line.substr(0, eq));
            std::string value = trim(line.substr(eq + 1));

            // Strip matching single or double quotes.
            if (value.size() >= 2 &&
                ((value.front() == '"'  && value.back() == '"') ||
                 (value.front() == '\'' && value.back() == '\''))) {
                value = value.substr(1, value.size() - 2);
            }
            if (key.empty()) continue;

#if defined(_WIN32)
            if (std::getenv(key.c_str()) == nullptr) {
                _putenv_s(key.c_str(), value.c_str());
                ++loaded;
            }
#else
            if (std::getenv(key.c_str()) == nullptr) {
                setenv(key.c_str(), value.c_str(), 0);
                ++loaded;
            }
#endif
        }

        logInfo("Loaded " + std::to_string(loaded) + " env var(s) from " + p);
        return loaded > 0;
    };

    // Try the requested path first.
    if (tryLoad(path)) return;

    // Fall back to one directory up (handles running from build/).
    if (tryLoad("../" + path)) return;

    logWarn("No .env file at " + path + " or ../" + path);
}

void logInfo (const std::string& m) { std::cout << "[INFO] "  << m << "\n"; }
void logWarn (const std::string& m) { std::cerr << "[WARN] "  << m << "\n"; }
void logError(const std::string& m) { std::cerr << "[ERROR] " << m << "\n"; }

std::string urlEncode(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += static_cast<char>(c);
        } else {
            char buf[4];
            std::snprintf(buf, sizeof(buf), "%%%02X", c);
            out += buf;
        }
    }
    return out;
}

std::string nowIsoUtc() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buf;
}

} // namespace goldsmc::utils