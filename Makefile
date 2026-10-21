CXX       ?= g++
CXXFLAGS  ?= -std=c++14 -Wall -Wextra -O2
# LDFLAGS is left to the caller; the Dockerfile uses it to link the C++ runtime statically.

# This machine's Command Line Tools have an incomplete libc++ header dir and a mismatched default SDK, so
# <iostream> isn't found without pointing at the 26.5 SDK. The workaround only applies on macOS and only if that
# SDK folder exists, so Linux and Docker builds are unaffected. After reinstalling the CLT, build with
# `make SDK_FLAGS=` (or just delete these lines).
UNAME_S   := $(shell uname -s)
SDK       := /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk
ifeq ($(UNAME_S),Darwin)
ifneq ($(wildcard $(SDK)),)
SDK_FLAGS ?= -isysroot $(SDK) -isystem $(SDK)/usr/include/c++/v1
endif
endif

SRCS := main.cpp $(wildcard src/*/*.cpp)
INCLUDES := -Isrc
OBJS := $(SRCS:%.cpp=build/%.o)
BIN  := blackjack

.PHONY: all run clean

all: $(BIN)

run: $(BIN)
	./$(BIN)

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $(SDK_FLAGS) $^ $(LDFLAGS) -o $@

build/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SDK_FLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf build $(BIN)

-include $(OBJS:.o=.d)
