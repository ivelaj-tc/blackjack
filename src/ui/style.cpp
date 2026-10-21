#include "ui/style.h"

#include <cstdlib>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace bj {
namespace style {
namespace {

bool colorEnabled = false;

bool envSet(const char* name) {
    const char* value = std::getenv(name);
    return value != nullptr && *value != '\0';
}

}  // namespace

bool colorForbidden() { return envSet("NO_COLOR"); }

bool supportsColor() {
    if (colorForbidden()) return false;

    const char* force = std::getenv("CLICOLOR_FORCE");
    if (force != nullptr && *force != '\0' && std::string(force) != "0") return true;

#ifdef _WIN32
    return false;  // the classic Windows console doesn't interpret ANSI codes by default
#else
    const char* term = std::getenv("TERM");
    if (term != nullptr && std::string(term) == "dumb") return false;
    return isatty(STDOUT_FILENO) != 0;
#endif
}

void setEnabled(bool on) { colorEnabled = on; }
bool enabled() { return colorEnabled; }

std::string paint(const std::string& text, const std::string& sgr) {
    if (!colorEnabled) return text;
    return "\x1b[" + sgr + "m" + text + "\x1b[0m";
}

std::string bold(const std::string& text) { return paint(text, "1"); }
std::string dim(const std::string& text) { return paint(text, "2"); }
std::string good(const std::string& text) { return paint(text, "1;32"); }
std::string bad(const std::string& text) { return paint(text, "1;31"); }
std::string warn(const std::string& text) { return paint(text, "33"); }
std::string accent(const std::string& text) { return paint(text, "1;36"); }

std::string tag(const std::string& key) { return "[" + accent(key) + "]"; }

std::string bySign(const std::string& text, double value) {
    if (value > 0) return good(text);
    if (value < 0) return bad(text);
    return text;
}

}  // namespace style
}  // namespace bj
