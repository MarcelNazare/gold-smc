#pragma once
#include "goldsmc/types.hpp"
#include <string>
#include <vector>

namespace goldsmc {

std::vector<Candle> fetchTimeSeries(
    const std::string& apiKey,
    const std::string& symbol,
    const std::string& interval,
    int outputSize = 300);

} // namespace goldsmc