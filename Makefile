CXX      = g++
CXXFLAGS = -std=c++20 -O3 -march=native -mbmi2 -Wall -Wextra

OBJ     = initiation_support.o fen_input.o main.o testharness.o uci.o eval.o search.o movepicker.o \
          movegen.o position.o search_state.o tt.o tables.o
HEADERS = global_header.h global_constants.h support_files.h piece_table.h search_state.h tt.h tables.h \
          uci.h testharness.h movegen.h position.h search.h movepicker.h eval.h fen_input.h

buster: $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o buster

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# make check — confirms a build is correct, in a few seconds:
#   * the built-in self-tests (hashing, make/unmake, evaluation symmetry, ...) over every position
#     three moves deep from the start position;
#   * perft 5 from the start position, which must be exactly 4 865 609.
check: buster
	@out=$$(printf 'position startpos\ntest 3\nperft depth 5\nquit\n' | ./buster); \
	echo "$$out" | grep -E "tests:|FAIL|nodes found"; \
	echo "$$out" | awk '/^tests:/ { split($$2, n, "/"); ok_tests = (n[1] == n[2] && n[2] > 0) } \
	                    /nodes found: 4865609/ { ok_perft = 1 } \
	                    END { exit !(ok_tests && ok_perft) }' \
	&& echo "make check: OK" || { echo "make check: FAILED"; exit 1; }

# make asan — a separate binary, buster_asan, with AddressSanitizer and UndefinedBehaviorSanitizer.
# Built straight from the sources, so it never mixes with the optimised object files.
ASAN_FLAGS = -std=c++20 -O1 -g -fno-omit-frame-pointer -march=native -mbmi2 \
             -fsanitize=address,undefined -fsanitize-recover=address,undefined

asan: $(OBJ:.o=.cpp) $(HEADERS)
	$(CXX) $(ASAN_FLAGS) $(OBJ:.o=.cpp) -o buster_asan

# ---- Release binaries --------------------------------------------------------------------
# Portable builds for publishing: -march=haswell (any CPU with BMI2), not -march=native.
#   make release-linux     fully static x86-64 Linux binary (runs on any distribution)
#   make release-windows   x86-64 Windows .exe, cross-compiled with MinGW-w64 (static, no DLLs)
#   make release           both, into release/, named with the engine version
VERSION       := $(shell grep -oP 'engine_version = "\K[^"]+' global_constants.h)
RELEASE_FLAGS  = -std=c++20 -O3 -march=haswell -mbmi2 -Wall -Wextra -static -s
MINGW         ?= x86_64-w64-mingw32-g++

release-linux: $(OBJ:.o=.cpp) $(HEADERS)
	mkdir -p release
	$(CXX) $(RELEASE_FLAGS) $(OBJ:.o=.cpp) -o release/buster-$(VERSION)-linux-x86_64

release-windows: $(OBJ:.o=.cpp) $(HEADERS)
	mkdir -p release
	$(MINGW) $(RELEASE_FLAGS) $(OBJ:.o=.cpp) -o release/buster-$(VERSION)-windows-x86_64.exe

release: release-linux release-windows

clean:
	rm -f $(OBJ) buster buster_asan
	rm -rf release

.PHONY: check asan clean release release-linux release-windows
