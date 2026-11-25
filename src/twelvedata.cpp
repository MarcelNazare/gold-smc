#include "goldsmc/twelvedata.hpp"
#include "goldsmc/types.hpp"          
#include "goldsmc/utils.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <stdexcept>
#include <string>

using json = nlohmann::json;

namespace {

std::size_t writeCallback(char* ptr, std::size_t size, std::size_t nmemb, void* userdata) {
    auto* s = static_cast<std::string*>(userdata);
    s->append(ptr, size * nmemb);
    return size * nmemb;
}

struct CurlInit {
    CurlInit()  { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlInit() { curl_global_cleanup(); }
};

} // namespace

namespace goldsmc {

std::vector<Candle> fetchTimeSeries(
    const std::string& apiKey,
    const std::string& symbol,
    const std::string& interval,
    int outputSize)
{
    static CurlInit curlInitOnce;

    std::string url =
        "https://api.twelvedata.com/time_series"
        "?symbol="   + utils::urlEncode(symbol) +
        "&interval=" + utils::urlEncode(interval) +
        "&outputsize=" + std::to_string(outputSize) +
        "&order=ASC" +
        "&apikey="   + utils::urlEncode(apiKey) +
        "&format=JSON";

    std::string body;
    CURL* h = curl_easy_init();
    if (!h) throw std::runtime_error("curl_easy_init failed");

    curl_easy_setopt(h, CURLOPT_URL, url.c_str());
    curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(h, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(h, CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(h, CURLOPT_USERAGENT, "gold-smc/0.1");

    CURLcode rc = curl_easy_perform(h);
    long httpCode = 0;
    curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(h);

    if (rc != CURLE_OK)
        throw std::runtime_error(std::string("curl error: ") + curl_easy_strerror(rc));
    if (httpCode != 200)
        throw std::runtime_error("HTTP " + std::to_string(httpCode) + ": " + body);

    json j = json::parse(body);

    if (j.contains("status") && j["status"] == "error") {
        std::string msg = j.value("message", "unknown");
        throw std::runtime_error("TwelveData error: " + msg);
    }
    if (j.contains("code") && j["code"].is_number()) {
        // Rate limit or similar top-level error.
        std::string msg = j.value("message", "unknown");
        throw std::runtime_error("TwelveData API code " + std::to_string(j["code"].get<int>()) + ": " + msg);
    }

    if (!j.contains("values") || !j["values"].is_array())
        throw std::runtime_error("Unexpected TwelveData response: no 'values' array");

    std::vector<Candle> out;
    out.reserve(j["values"].size());

    for (const auto& v : j["values"]) {
        Candle c;
        c.datetime = v.value("datetime", "");
        auto toD = [&](const char* key) -> double {
            if (!v.contains(key)) return 0.0;
            if (v[key].is_number()) return v[key].get<double>();
            return std::stod(v[key].get<std::string>());
        };
        c.open  = toD("open");
        c.high  = toD("high");
        c.low   = toD("low");
        c.close = toD("close");
        out.push_back(std::move(c));
    }

    // We requested order=ASC but be defensive.
    if (out.size() >= 2 && out.front().datetime > out.back().datetime)
        std::reverse(out.begin(), out.end());

    return out;
}

} // namespace goldsmc