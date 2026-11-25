#include "test_framework.hpp"
#include "goldsmc/strategy.hpp"
#include "goldsmc/indicators.hpp"

using namespace goldsmc;

namespace {

Candle c(double o, double h, double l, double cl) {
    return Candle{"", o, h, l, cl};
}

StrategyConfig cfg() {
    StrategyConfig cfg;
    cfg.swing_lookback = 1;
    cfg.sweep_lookback_candles = 50;
    cfg.max_candles_sweep_to_choch = 10;
    cfg.max_candles_choch_to_fvg = 10;
    cfg.min_fvg_size = 0.0;
    cfg.stop_buffer_usd = 0.0;
    cfg.risk_reward = 2.0;
    return cfg;
}

// Build a small bullish scenario:
//  - swing low at ~index 6
//  - sweep at index 10 (wick below, close above)
//  - CHoCH: close above swing high at index 12
//  - FVG forms during the impulse leg
std::vector<Candle> bullishSetup() {
    return {
        c(100.0, 101.0,  99.5, 100.5), // 0
        c(100.5, 102.0, 100.0, 101.5), // 1  (swing high candidate ~ index 1)
        c(101.5, 101.8, 100.0, 100.2), // 2
        c(100.2, 100.8,  99.0,  99.4), // 3
        c( 99.4, 100.0,  98.5,  98.8), // 4
        c( 98.8,  99.5,  98.0,  98.3), // 5
        c( 98.3,  98.6,  97.0,  97.8), // 6  swing low = 97.0
        c( 97.8,  98.5,  97.5,  98.2), // 7
        c( 98.2,  99.0,  98.0,  98.8), // 8
        c( 98.8,  99.0,  97.5,  98.5), // 9
        c( 98.5,  99.0,  96.5,  98.8), // 10 sweep below 97.0 → close 98.8 above
        c( 98.8, 100.0,  98.7,  99.8), // 11 up
        c( 99.8, 101.5,  99.7, 101.3), // 12 CHoCH close > swing high 102? not yet
        c(101.3, 103.0, 101.2, 102.8), // 13 close > 102 → CHoCH here
        c(102.8, 105.0, 102.5, 104.8), // 14 impulse
        c(104.8, 106.0, 104.5, 105.8), // 15
    };
}

} // namespace

TEST(detectLiquiditySweep_finds_bullish_sweep) {
    auto candles = bullishSetup();
    auto swings  = findSwings(candles, 1);
    auto sweep   = detectLiquiditySweep(candles, swings, true, 50);
    ASSERT_TRUE(sweep.has_value());
    ASSERT_EQ(sweep->sweepIndex, 10);
    ASSERT_NEAR(sweep->sweptLevel, 97.0, 1e-9);
}

TEST(detectCHoCH_triggers_on_bullish_break) {
    auto candles = bullishSetup();
    auto swings  = findSwings(candles, 1);
    auto sweep   = detectLiquiditySweep(candles, swings, true, 50);
    ASSERT_TRUE(sweep.has_value());

    auto choch = detectCHoCH(candles, swings, sweep->sweepIndex, true, 10);
    ASSERT_TRUE(choch.has_value());
    ASSERT_TRUE(candles[*choch].close > 102.0); // broke above a prior swing high
}

TEST(runStrategy_returns_long_signal) {
    auto candles = bullishSetup();
    auto sig = runStrategy(candles, cfg());
    ASSERT_TRUE(sig.has_value());
    ASSERT_TRUE(sig->type == Signal::Type::LONG);
    ASSERT_TRUE(sig->entry  > 0);
    ASSERT_TRUE(sig->stop   < sig->entry);
    ASSERT_TRUE(sig->target > sig->entry);
    ASSERT_NEAR(sig->target - sig->entry,
                2.0 * (sig->entry - sig->stop), 1e-6); // 1:2 RR
}

TEST(runStrategy_returns_nothing_without_sweep) {
    std::vector<Candle> candles;
    for (int i = 0; i < 30; ++i)
        candles.push_back(c(100 + i, 101 + i, 99 + i, 100 + i));
    auto sig = runStrategy(candles, cfg());
    ASSERT_FALSE(sig.has_value());
}