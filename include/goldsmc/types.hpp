#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace goldsmc {

struct Candle {
    std::string datetime;
    double open{0}, high{0}, low{0}, close{0};
};

struct Swing {
    int index{0};
    double price{0};
    bool isHigh{false};
};

struct FVG {
    int startIndex{0};
    double top{0};
    double bottom{0};
    bool bullish{false};
};

struct SweepResult {
    int sweepIndex{-1};
    int sweptSwingIndex{-1};
    double sweptLevel{0};
};

struct Signal {
    enum class Type { LONG, SHORT };
    Type type{Type::LONG};
    double entry{0};
    double stop{0};
    double target{0};
    double sweepLevel{0};
    int sweepIndex{-1};
    int chochIndex{-1};
    int fvgIndex{-1};
    std::string reason;
};

} // namespace goldsmc