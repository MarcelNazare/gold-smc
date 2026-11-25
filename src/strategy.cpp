#include "goldsmc/indicators.hpp"
#include "goldsmc/strategy.hpp"
#include <algorithm>
#include <cmath>

namespace goldsmc {

std::optional<SweepResult> detectLiquiditySweep(
    const std::vector<Candle>& candles,
    const std::vector<Swing>&  swings,
    bool bullish,
    int  maxLookbackCandles)
{
    if (candles.empty() || swings.empty()) return std::nullopt;

    int lastIdx = static_cast<int>(candles.size()) - 1;
    int start   = std::max(0, lastIdx - maxLookbackCandles);

    for (int i = lastIdx; i >= start; --i) {
        const Candle& c = candles[i];
        for (auto it = swings.rbegin(); it != swings.rend(); ++it) {
            if (it->index >= i) continue;
            if (bullish && !it->isHigh) {
                // Bullish sweep: wick below swing low, close above.
                if (c.low < it->price && c.close > it->price)
                    return SweepResult{i, it->index, it->price};
            } else if (!bullish && it->isHigh) {
                // Bearish sweep: wick above swing high, close below.
                if (c.high > it->price && c.close < it->price)
                    return SweepResult{i, it->index, it->price};
            }
        }
    }
    return std::nullopt;
}

std::optional<int> detectCHoCH(
    const std::vector<Candle>& candles,
    const std::vector<Swing>&  swings,
    int  sweepIndex,
    bool bullish,
    int  maxCandles)
{
    if (sweepIndex < 0) return std::nullopt;

    // Most recent opposite swing BEFORE the sweep becomes the reference level.
    std::optional<Swing> reference;
    for (auto it = swings.rbegin(); it != swings.rend(); ++it) {
        if (it->index >= sweepIndex) continue;
        if (bullish && it->isHigh) { reference = *it; break; }
        if (!bullish && !it->isHigh){ reference = *it; break; }
    }
    if (!reference) return std::nullopt;

    int end = std::min(static_cast<int>(candles.size()) - 1, sweepIndex + maxCandles);
    for (int i = sweepIndex + 1; i <= end; ++i) {
        if (bullish  && candles[i].close > reference->price) return i;
        if (!bullish && candles[i].close < reference->price) return i;
    }
    return std::nullopt;
}

std::optional<FVG> findFVG(
    const std::vector<Candle>& candles,
    int    startIndex,
    int    endIndex,
    bool   bullish,
    double minSize)
{
    int n = static_cast<int>(candles.size());
    int s = std::max(0, startIndex);
    int e = std::min(n - 3, endIndex);
    for (int i = s; i <= e; ++i) {
        if (bullish) {
            if (candles[i].high < candles[i + 2].low) {
                double size = candles[i + 2].low - candles[i].high;
                if (size >= minSize) {
                    return FVG{i, candles[i + 2].low, candles[i].high, true};
                }
            }
        } else {
            if (candles[i].low > candles[i + 2].high) {
                double size = candles[i].low - candles[i + 2].high;
                if (size >= minSize) {
                    return FVG{i, candles[i].low, candles[i + 2].high, false};
                }
            }
        }
    }
    return std::nullopt;
}

std::optional<Signal> runStrategy(
    const std::vector<Candle>& candles,
    const StrategyConfig& cfg)
{
    if (candles.size() < 20) return std::nullopt;

    auto swings = findSwings(candles, cfg.swing_lookback);
    if (swings.empty()) return std::nullopt;

    auto build = [&](bool bullish, const SweepResult& sweep) -> std::optional<Signal> {
        auto chochIdx = detectCHoCH(candles, swings, sweep.sweepIndex, bullish,
                                    cfg.max_candles_sweep_to_choch);
        if (!chochIdx) return std::nullopt;

        int fvgStart = sweep.sweepIndex;
        int fvgEnd   = std::min(static_cast<int>(candles.size()) - 3,
                                *chochIdx + cfg.max_candles_choch_to_fvg);
        auto fvg = findFVG(candles, fvgStart, fvgEnd, bullish, cfg.min_fvg_size);
        if (!fvg) return std::nullopt;

        Signal s;
        s.sweepLevel  = sweep.sweptLevel;
        s.sweepIndex  = sweep.sweepIndex;
        s.chochIndex  = *chochIdx;
        s.fvgIndex    = fvg->startIndex;

        if (bullish) {
            s.type   = Signal::Type::LONG;
            s.entry  = fvg->top;                              // retrace into top of bullish FVG
            s.stop   = sweep.sweptLevel - cfg.stop_buffer_usd;
            double risk = s.entry - s.stop;
            if (risk <= 0) return std::nullopt;
            s.target = s.entry + cfg.risk_reward * risk;
            s.reason = "Bullish sweep + 5m CHoCH + bullish FVG";
        } else {
            s.type   = Signal::Type::SHORT;
            s.entry  = fvg->bottom;                           // retrace into bottom of bearish FVG
            s.stop   = sweep.sweptLevel + cfg.stop_buffer_usd;
            double risk = s.stop - s.entry;
            if (risk <= 0) return std::nullopt;
            s.target = s.entry - cfg.risk_reward * risk;
            s.reason = "Bearish sweep + 5m CHoCH + bearish FVG";
        }
        return s;
    };

    // Prefer the most recent sweep — whichever direction had the freshest one.
    auto bullSweep = detectLiquiditySweep(candles, swings, true,
                                          cfg.sweep_lookback_candles);
    auto bearSweep = detectLiquiditySweep(candles, swings, false,
                                          cfg.sweep_lookback_candles);

    bool bullFirst = true;
    if (bullSweep && bearSweep)
        bullFirst = bullSweep->sweepIndex >= bearSweep->sweepIndex;
    else if (!bullSweep) bullFirst = false;

    if (bullFirst) {
        if (bullSweep) if (auto s = build(true,  *bullSweep)) return s;
        if (bearSweep) if (auto s = build(false, *bearSweep)) return s;
    } else {
        if (bearSweep) if (auto s = build(false, *bearSweep)) return s;
        if (bullSweep) if (auto s = build(true,  *bullSweep)) return s;
    }
    return std::nullopt;
}

} // namespace goldsmc