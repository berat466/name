CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -fno-fast-math -ffp-contract=off
SRC := src/main.cpp src/asm.cpp src/ikili.cpp src/vm.cpp src/dump.cpp

pm: $(SRC) src/ops.h src/program.h
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

pm_san: $(SRC) src/ops.h src/program.h
	$(CXX) -std=c++17 -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=undefined -Wall -Wextra -ffp-contract=off -o $@ $(SRC)

test: pm
	./tests/run_tests.sh ./pm

test_san: pm_san
	./tests/run_tests.sh ./pm_san

clean:
	rm -f pm pm_san tests/*.pmb
