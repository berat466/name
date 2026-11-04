CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -fno-fast-math -ffp-contract=off
SRC := src/main.cpp src/asm.cpp src/ikili.cpp src/vm.cpp src/dump.cpp src/native_x64.cpp src/sablon.cpp src/yapisal.cpp src/liste.cpp src/ifade.cpp

pm: $(SRC) src/ops.h src/ops_gen.h src/program.h src/ifade_ops_gen.h src/anlam.h src/anlam_wfp.h src/anlam_wfp_gen.h src/anlam_wcx.h src/anlam_wcx_gen.h src/anlam_wi_gen.h src/host.h
	$(CXX) $(CXXFLAGS) -o $@ $(SRC) -lquadmath

pm_san: $(SRC) src/ops.h src/ops_gen.h src/program.h src/ifade_ops_gen.h src/anlam.h src/anlam_wfp.h src/anlam_wfp_gen.h src/anlam_wcx.h src/anlam_wcx_gen.h src/anlam_wi_gen.h src/host.h
	$(CXX) -std=c++17 -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=undefined -Wall -Wextra -ffp-contract=off -o $@ $(SRC) -lquadmath

pm_steps: $(SRC) src/ops.h src/ops_gen.h src/program.h src/ifade_ops_gen.h src/anlam.h src/anlam_wfp.h src/anlam_wfp_gen.h src/anlam_wcx.h src/anlam_wcx_gen.h src/anlam_wi_gen.h src/host.h
	$(CXX) $(CXXFLAGS) -DPM_STEPS -o $@ $(SRC) -lquadmath

test: pm
	./tests/run_tests.sh ./pm
	./uygulamalar/kontrol.sh ./pm

test_san: pm_san
	./tests/run_tests.sh ./pm_san
	./uygulamalar/kontrol.sh ./pm_san

clean:
	rm -f pm pm_san tests/*.pmb
