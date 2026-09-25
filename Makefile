CXX      = g++
CXXFLAGS = -O3 -march=native -mbmi2 -Wall -Wextra

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
ASAN_FLAGS = -O1 -g -fno-omit-frame-pointer -march=native -mbmi2 \
             -fsanitize=address,undefined -fsanitize-recover=address,undefined

asan: $(OBJ:.o=.cpp) $(HEADERS)
	$(CXX) $(ASAN_FLAGS) $(OBJ:.o=.cpp) -o buster_asan

clean:
	rm -f $(OBJ) buster buster_asan

.PHONY: check asan clean
