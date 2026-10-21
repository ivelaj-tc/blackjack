#include <iostream>

#include "app/cli.h"
#include "app/menu.h"
#include "config/settings.h"
#include "core/seed.h"
#include "platform/audio.h"
#include "progress/profile.h"
#include "progress/save.h"
#include "ui/input.h"

int main(int argc, char** argv) {
    bj::CliOptions cli;
    std::string error;
    if (!bj::parseArgs(argc, argv, cli, error)) {
        std::cerr << "blackjack: " << error << "\n";
        bj::printUsage(std::cerr);
        return 2;
    }
    if (cli.help) {
        bj::printUsage(std::cout);
        return 0;
    }
    if (cli.hasSeed) bj::seed::setFixed(cli.seed);

    bj::Settings settings;
    bj::Profile profile;
    bj::loadProgress(settings, profile);  // does nothing when saving is off, as in piped runs

    bj::applyColors(settings);
    bj::audio::init(settings.music, settings.effects);

    try {
        bj::runMainMenu(settings, profile);
    } catch (const bj::InputClosed&) {
        std::cout << "\nInput closed. Goodbye!\n";
    }
    bj::saveProgress(settings, profile);
    return 0;
}
