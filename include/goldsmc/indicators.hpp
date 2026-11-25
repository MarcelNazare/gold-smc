#pragma once
#include "goldsmc/types.hpp"
#include <vector>

namespace goldsmc {

std::vector<Swing> findSwings(const std::vector<Candle>& candles, int lookback = 2);

} // namespace goldsmc