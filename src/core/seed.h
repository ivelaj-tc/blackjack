#pragma once

#include <cstdint>

namespace bj {
namespace seed {

// Pins every shuffle to one number (the --seed option), so a game repeats exactly for the same plays.
void setFixed(std::uint32_t value);
bool hasFixed();
std::uint32_t fixedValue();

// Seed for a new shoe: the fixed seed when one is set, otherwise a random one.
std::uint32_t forShoe();

// Seed for a drill's random questions. With a fixed seed each call differs, so drills still vary from one run to
// the next but a whole run can be repeated; otherwise it is random.
std::uint32_t forDrill();

}  // namespace seed
}  // namespace bj
