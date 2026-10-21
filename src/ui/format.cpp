#include "ui/format.h"

#include <cmath>
#include <cstdio>

namespace bj {

std::string signedInt(int n) { return (n > 0 ? "+" : "") + std::to_string(n); }

std::string tenths(double v, bool withSign) {
    if (std::fabs(v) < 0.05) v = 0.0;
    char buf[32];
    std::snprintf(buf, sizeof(buf), withSign ? "%+.1f" : "%.1f", v);
    return buf;
}

std::string percent(double fraction, bool withSign) {
    double pct = fraction * 100.0;
    if (std::fabs(pct) < 0.005) pct = 0.0;
    char buf[32];
    std::snprintf(buf, sizeof(buf), withSign ? "%+.2f%%" : "%.2f%%", pct);
    return buf;
}

}  // namespace bj
