#include "test_framework.hpp"
#include "goldsmc/indicators.hpp"
#include "goldsmc/strategy.hpp"

using namespace goldsmc;

namespace {

Candle c(double o, double h, double l, double cl) {
    return Candle{"", o, h, l, cl};
}

} // namespace

TEST(findSwings_detects_a_local_high_and_low) {
    std::vector<Candle> candles = {
        c(1, 2, 1, 2),
        c(2, 3, 2, 3),
        c(3, 5, 3, 4),     // index 2: local high (5)
        c(4, 4, 3, 3),
        c(3, 3, 1, 2),     // index 4: local low (1)
        c(2, 3, 2, 3),
        c(3, 4, 3, 4),
    };
    auto swings = findSwings(candles, 1);

    bool foundHigh = false, foundLow = false;
    for (auto& s : swings) {
        if (s.index == 2 && s.isHigh) { foundHigh = true; ASSERT_NEAR(s.price, 5, 1e-9); }
        if (s.index == 4 && !s.isHigh){ foundLow  = true; ASSERT_NEAR(s.price, 1, 1e-9); }
    }
    ASSERT_TRUE(foundHigh);
    ASSERT_TRUE(foundLow);
}

TEST(findFVG_detects_bullish_imbalance) {
    std::vector<Candle> candles = {
        c(100, 101,  99, 100),
        c(101, 102, 100, 101),
        c(110, 115, 109, 114),  // impulse up
        c(115, 118, 114, 117),
        c(118, 120, 117, 119),
    };
    // Candle 0 high = 101, candle 2 low = 109 → bullish FVG 101..109
    auto fvg = findFVG(candles, 0, 2, true, 0.0);
    ASSERT_TRUE(fvg.has_value());
    ASSERT_TRUE(fvg->bullish);
    ASSERT_NEAR(fvg->bottom, 101.0, 1e-9);
    ASSERT_NEAR(fvg->top,    109.0, 1e-9);
    ASSERT_EQ(fvg->startIndex, 0);
}

TEST(findFVG_detects_bearish_imbalance) {
    std::vector<Candle> candles = {
        c(120, 121, 119, 120),
        c(119, 120, 118, 119),
        c(110, 111, 105, 106),   // impulse down
        c(106, 108, 104, 105),
        c(105, 107, 103, 104),
    };
    // Candle 0 low = 119, candle 2 high = 111 → bearish FVG 111..119
    auto fvg = findFVG(candles, 0, 2, false, 0.0);
    ASSERT_TRUE(fvg.has_value());
    ASSERT_FALSE(fvg->bullish);
    ASSERT_NEAR(fvg->top,    119.0, 1e-9);
    ASSERT_NEAR(fvg->bottom, 111.0, 1e-9);
}

TEST(findFVG_rejects_tiny_gap_below_min_size) {
    std::vector<Candle> candles = {
        c(100, 101,   99, 100),
        c(101, 102,  100, 101),
        c(102, 103,  101.05, 102.5), // gap 101 → 101.05
        c(103, 104,  103, 104),
    };
    auto fvg = findFVG(candles, 0, 1, true, /*minSize*/ 1.0);
    ASSERT_FALSE(fvg.has_value());
}