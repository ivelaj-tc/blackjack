#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>

namespace bj {

struct CliOptions {
    bool hasSeed = false;
    std::uint32_t seed = 0;
    bool help = false;
};

// Parses the command line: --seed N (or --seed=N) and -h/--help. Returns false and sets `error` for anything else.
bool parseArgs(int argc, char** argv, CliOptions& options, std::string& error);

void printUsage(std::ostream& out);

}  // namespace bj
