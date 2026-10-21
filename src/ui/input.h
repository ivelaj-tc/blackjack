#pragma once

#include <string>

namespace bj {

// Thrown by readLine when the input ends (Ctrl-D, a closed pipe). It unwinds to the top level, letting a session in
// progress show its summary and save before the program stops.
struct InputClosed {};

// Prints the prompt and returns the trimmed reply. Throws InputClosed when there is no more input.
std::string readLine(const std::string& prompt);

// Parses a whole string as an int (no trailing characters). Leaves `out` untouched on failure.
bool parseInt(const std::string& s, int& out);

// Parses a whole string as a finite decimal number. Leaves `out` untouched on failure.
bool parseDouble(const std::string& s, double& out);

bool isQuit(const std::string& line);

// Asks until the player types exactly one of the allowed characters (case-insensitive).
// An empty answer returns dflt when one is given.
char promptChoice(const std::string& prompt, const std::string& allowed, char dflt = 0);

}  // namespace bj
