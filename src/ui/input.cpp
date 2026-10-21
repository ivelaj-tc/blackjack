#include "ui/input.h"

#include <cctype>
#include <cmath>
#include <cstddef>
#include <exception>
#include <iostream>

namespace bj {
namespace {

std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    std::size_t start = s.find_first_not_of(ws);
    if (start == std::string::npos) return "";
    std::size_t end = s.find_last_not_of(ws);
    return s.substr(start, end - start + 1);
}

}  // namespace

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) throw InputClosed();
    return trim(line);
}

bool parseInt(const std::string& s, int& out) {
    try {
        std::size_t used = 0;
        int value = std::stoi(s, &used);
        if (used != s.size()) return false;
        out = value;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool parseDouble(const std::string& s, double& out) {
    try {
        std::size_t used = 0;
        double value = std::stod(s, &used);
        if (used != s.size() || !std::isfinite(value)) return false;
        out = value;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool isQuit(const std::string& line) { return !line.empty() && (line[0] == 'q' || line[0] == 'Q'); }

char promptChoice(const std::string& prompt, const std::string& allowed, char dflt) {
    while (true) {
        std::string line = readLine(prompt);
        if (line.empty() && dflt != 0) return dflt;
        if (line.size() == 1) {
            char c = static_cast<char>(std::tolower(static_cast<unsigned char>(line[0])));
            if (allowed.find(c) != std::string::npos) return c;
        }
        std::cout << "Please enter one of:";
        for (char c : allowed) std::cout << ' ' << c;
        std::cout << "\n";
    }
}

}  // namespace bj
