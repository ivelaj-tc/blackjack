#include "ui/pacing.h"

#include <chrono>
#include <iostream>
#include <thread>

#include "config/settings.h"
#include "platform/audio.h"

namespace bj {

void pauseBetweenCards(const Settings& settings) {
    if (!audio::effectsEnabled() && !animationEnabled(settings)) return;
    std::cout.flush();  // make sure what has been drawn so far is visible during the pause
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

}  // namespace bj
