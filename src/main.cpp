#include <filesystem> 
#include "goldsmc/config.hpp"
#include "goldsmc/twelvedata.hpp"
#include "goldsmc/strategy.hpp"
#include "goldsmc/utils.hpp"
#include <exception>
#include <iomanip>
#include <iostream>

using namespace goldsmc;

int main(int argc, char** argv) {
    std::string configPath = (argc > 1) ? argv[1] : "config/config.json";

    try {
        utils::loadDotEnv(".env");
        StrategyConfig cfg = loadConfig(configPath);

        std::string apiKey = utils::getEnv("TWELVEDATA_API_KEY");
        if (apiKey.empty()) {
            utils::logError("TWELVEDATA_API_KEY is not set (env or .env).");
            return 1;
        }

        utils::logInfo("Fetching " + cfg.symbol + " " + cfg.interval +
                       " (outputsize=" + std::to_string(cfg.output_size) + ")");
        auto candles = fetchTimeSeries(apiKey, cfg.symbol, cfg.interval, cfg.output_size);

        if (candles.size() < 20) {
            utils::logWarn("Too few candles returned: " + std::to_string(candles.size()));
            return 2;
        }

        utils::logInfo("Fetched " + std::to_string(candles.size()) +
                       " candles. Last close = " +
                       std::to_string(candles.back().close));

        auto signal = runStrategy(candles, cfg);
        if (!signal) {
            std::cout << "\nNo sweep + CHoCH + FVG setup found on the latest data.\n";
            return 0;
        }

        std::cout << "\n=== SIGNAL (" << utils::nowIsoUtc() << ") ===\n";
        std::cout << (signal->type == Signal::Type::LONG ? "LONG" : "SHORT") << "  "
                  << cfg.symbol << "  " << cfg.interval << "\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Entry   : " << signal->entry  << "\n";
        std::cout << "Stop    : " << signal->stop   << "\n";
        std::cout << "Target  : " << signal->target << "\n";
        std::cout << "R:R     : 1:" << cfg.risk_reward << "\n";
        std::cout << "Sweep   : level " << signal->sweepLevel
                  << " at candle #"     << signal->sweepIndex << "\n";
        std::cout << "CHoCH   : candle #"     << signal->chochIndex << "\n";
        std::cout << "FVG     : candle #"     << signal->fvgIndex   << "\n";
        std::cout << "Reason  : " << signal->reason << "\n";

        if (signal->type == Signal::Type::LONG && candles.back().close < signal->entry)
            utils::logWarn("Price is already below the FVG entry — retest may have passed.");
        if (signal->type == Signal::Type::SHORT && candles.back().close > signal->entry)
            utils::logWarn("Price is already above the FVG entry — retest may have passed.");

        return 0;
    } catch (const std::exception& e) {
        utils::logError(std::string("Fatal: ") + e.what());
        return 1;
    }
}