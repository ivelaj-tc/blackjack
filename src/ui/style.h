#pragma once

#include <string>

namespace bj {
namespace style {

// True if stdout is a color-capable terminal. NO_COLOR disables color; CLICOLOR_FORCE=1 forces it even when piped.
bool supportsColor();

// True if the NO_COLOR environment variable asks for no color at all.
bool colorForbidden();

void setEnabled(bool on);
bool enabled();

// Wraps `text` in ANSI SGR codes (for example "1;32") when color is enabled; returns it unchanged otherwise.
// Styled pieces must not be nested: the inner reset would end the outer style early.
std::string paint(const std::string& text, const std::string& sgr);

std::string bold(const std::string& text);
std::string dim(const std::string& text);
std::string good(const std::string& text);    // bold green: wins and positive numbers
std::string bad(const std::string& text);     // bold red: losses and negative numbers
std::string warn(const std::string& text);    // yellow: pushes, chip counts, cautions
std::string accent(const std::string& text);  // bold cyan: keys and highlights

// "[k]" with the key highlighted, for menu entries and prompts.
std::string tag(const std::string& key);

// good() for positive values, bad() for negative ones, unstyled for zero.
std::string bySign(const std::string& text, double value);

}  // namespace style
}  // namespace bj
