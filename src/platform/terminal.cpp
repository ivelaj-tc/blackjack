#include "platform/terminal.h"

#ifndef _WIN32
#include <unistd.h>
#endif

namespace bj {

bool isInteractive() {
#ifdef _WIN32
    return false;
#else
    return isatty(STDIN_FILENO) != 0 && isatty(STDOUT_FILENO) != 0;
#endif
}

}  // namespace bj
