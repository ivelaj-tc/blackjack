#pragma once

#include <string>

namespace bj {

// 5 -> "+5", 0 -> "0", -3 -> "-3"
std::string signedInt(int n);

// One decimal place; values that would print as +/-0.0 are shown as 0.0.
std::string tenths(double v, bool withSign);

// A fraction as a percentage with two decimals: 0.0058 -> "0.58%" (or "+0.58%" with a sign).
std::string percent(double fraction, bool withSign);

}  // namespace bj
