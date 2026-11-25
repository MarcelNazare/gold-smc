#pragma once
#include "goldsmc/types.hpp"
#include "goldsmc/config.hpp"
#include <optional>
#include <vector>

namespace goldsmc {

std::optional<SweepResult> detectLiquiditySweep(
    const std::vector<Candle>& candles,
    const std::vector<Swing>&  swings,
    bool bullish,
    int  maxLookbackCandles);

std::optional<int> detectCHoCH(
    const std::vector<Candle>& candles,
    const std::vector<Swing>&  swings,
    int  sweepIndex,
    bool bullish,
    int  maxCandles);

std::optional<FVG> findFVG(
    const std::vector<Candle>& candles,
    int    startIndex,
    int    endIndex,
    bool   bullish,
    double minSize);

std::optional<Signal> runStrategy(
    const std::vector<Candle>& candles,
    const StrategyConfig& cfg);

} // namespace goldsmc