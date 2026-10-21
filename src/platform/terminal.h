#pragma once

namespace bj {

// True when both stdin and stdout are terminals, i.e. a person is playing rather than a pipe or a script.
bool isInteractive();

}  // namespace bj
