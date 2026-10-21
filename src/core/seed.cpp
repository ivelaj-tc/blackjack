#include "core/seed.h"

#include <random>

namespace bj {
namespace seed {
namespace {

bool fixed = false;
std::uint32_t fixedSeed = 0;
std::uint32_t drillCounter = 0;

}  // namespace

void setFixed(std::uint32_t value) {
    fixed = true;
    fixedSeed = value;
}

bool hasFixed() { return fixed; }
std::uint32_t fixedValue() { return fixedSeed; }

std::uint32_t forShoe() { return fixed ? fixedSeed : std::random_device{}(); }

std::uint32_t forDrill() {
    if (!fixed) return std::random_device{}();
    return fixedSeed * 2654435761u + ++drillCounter;  // spread consecutive drills apart
}

}  // namespace seed
}  // namespace bj
