#pragma once

namespace bj {

struct Settings;

// Waits a moment between consecutive cards when sound effects or the dealing animation are on, so each card is
// heard and seen on its own. Does nothing otherwise, so piped and silent runs aren't slowed down.
void pauseBetweenCards(const Settings& settings);

}  // namespace bj
