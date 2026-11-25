#include "goldsmc/config.hpp"
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace goldsmc {

StrategyConfig loadConfig(const std::string& path) {
    StrategyConfig cfg;
    std::ifstream f(path);
    if (!f) {
        // No config file → use defaults.
        return cfg;
    }

    json j;
    f >> j;

    cfg.symbol                    = j.value("symbol",                    cfg.symbol);
    cfg.interval                  = j.value("interval",                  cfg.interval);
    cfg.output_size               = j.value("output_size",               cfg.output_size);
    cfg.swing_lookback            = j.value("swing_lookback",            cfg.swing_lookback);
    cfg.risk_reward               = j.value("risk_reward",               cfg.risk_reward);
    cfg.min_fvg_size              = j.value("min_fvg_size",              cfg.min_fvg_size);
    cfg.stop_buffer_usd           = j.value("stop_buffer_usd",           cfg.stop_buffer_usd);
    cfg.sweep_lookback_candles    = j.value("sweep_lookback_candles",    cfg.sweep_lookback_candles);
    cfg.max_candles_sweep_to_choch= j.value("max_candles_sweep_to_choch",cfg.max_candles_sweep_to_choch);
    cfg.max_candles_choch_to_fvg  = j.value("max_candles_choch_to_fvg",  cfg.max_candles_choch_to_fvg);
    cfg.session_filter_enabled    = j.value("session_filter_enabled",    cfg.session_filter_enabled);

    if (j.contains("allowed_utc_hours")) {
        for (auto& pair : j["allowed_utc_hours"]) {
            cfg.allowed_utc_hours.emplace_back(pair[0].get<int>(), pair[1].get<int>());
        }
    }

    if (cfg.swing_lookback < 1) cfg.swing_lookback = 1;
    if (cfg.output_size < 20)   cfg.output_size = 20;
    return cfg;
}

} // namespace goldsmc