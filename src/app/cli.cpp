#include "app/cli.h"

#include <iostream>

namespace bj {
namespace {

bool parseSeed(const std::string& text, std::uint32_t& out) {
    if (text.empty() || text.size() > 10) return false;
    unsigned long long value = 0;
    for (char c : text) {
        if (c < '0' || c > '9') return false;
        value = value * 10 + static_cast<unsigned long long>(c - '0');
    }
    if (value > 4294967295ULL) return false;
    out = static_cast<std::uint32_t>(value);
    return true;
}

}  // namespace

bool parseArgs(int argc, char** argv, CliOptions& options, std::string& error) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            options.help = true;
        } else if (arg == "--seed" || arg.rfind("--seed=", 0) == 0) {
            std::string value;
            if (arg == "--seed") {
                if (i + 1 >= argc) {
                    error = "--seed needs a number";
                    return false;
                }
                value = argv[++i];
            } else {
                value = arg.substr(7);
            }
            if (!parseSeed(value, options.seed)) {
                error = "--seed needs a whole number from 0 to 4294967295, not '" + value + "'";
                return false;
            }
            options.hasSeed = true;
        } else {
            error = "unknown option '" + arg + "'";
            return false;
        }
    }
    return true;
}

void printUsage(std::ostream& out) {
    out << "Usage: blackjack [--seed N] [--help]\n"
        << "\n"
        << "  --seed N   Shuffle every shoe (and the drills' questions) from the number N, so a game repeats exactly\n"
        << "             for the same plays. The challenge uses it as its shoe, so friends can compete on one seed.\n"
        << "             Without it the challenge plays today's shoe, the same for everyone that day.\n"
        << "  -h, --help Show this help.\n";
}

}  // namespace bj
