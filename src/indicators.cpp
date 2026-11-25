#include "goldsmc/indicators.hpp"

namespace goldsmc {

std::vector<Swing> findSwings(const std::vector<Candle>& candles, int lookback) {
    std::vector<Swing> swings;
    if (lookback < 1) lookback = 1;
    if (static_cast<int>(candles.size()) < 2 * lookback + 1) return swings;

    for (int i = lookback; i < static_cast<int>(candles.size()) - lookback; ++i) {
        bool isHigh = true, isLow = true;
        for (int j = i - lookback; j <= i + lookback; ++j) {
            if (j == i) continue;
            if (candles[j].high >= candles[i].high) isHigh = false;
            if (candles[j].low  <= candles[i].low)  isLow  = false;
        }
        if (isHigh && isLow) {
            // Both? Only possible with flat — treat as neither.
        } else if (isHigh) {
            swings.push_back({i, candles[i].high, true});
        } else if (isLow) {
            swings.push_back({i, candles[i].low, false});
        }
    }
    return swings;
}

} // namespace goldsmc