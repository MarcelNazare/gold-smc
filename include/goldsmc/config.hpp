#pragma once
#include <string>
#include <vector>
#include <utility>

namespace goldsmc {

struct StrategyConfig {
    std::string symbol         = "XAU/USD";
    std::string interval       = "5min";
    int    output_size                    = 300;
    int    swing_lookback                 = 2;
    double risk_reward                    = 2.0;
    double min_fvg_size                   = 0.20;
    double stop_buffer_usd                = 0.30;
    int    sweep_lookback_candles         = 30;
    int    max_candles_sweep_to_choch     = 15;
    int    max_candles_choch_to_fvg       = 10;
    bool   session_filter_enabled         = false;
    std::vector<std::pair<int,int>> allowed_utc_hours;
};

StrategyConfig loadConfig(const std::string& path);

} // namespace goldsmc