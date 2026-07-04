# =============================================================================
# Makefile — Compilador MiniC
# =============================================================================
# Uso:
#   make            construye ./minicc y ./minicc_api
#   make minicc     construye solo ./minicc
#   make minicc_api construye solo ./minicc_api (requiere nlohmann/json)
#   make test       compila los tests/*.c con minicc y con gcc, compara salidas
#   make clean      limpia binarios y .s generados
#
# NOTA: Para compilar minicc_api, asegúrate de tener nlohmann/json:
#   Ubuntu/Debian: sudo apt-get install nlohmann-json3-dev
#   macOS:         brew install nlohmann-json
#   Manual:        bash install_json_lib.sh
# ==============================================================================

SHELL    := /usr/bin/bash
CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wno-unused -I/usr/include
OBJS     := token.o scanner.o ast.o parser.o visitor.o main.o
BIN      := minicc
API_BIN  := minicc_api

all: $(BIN) $(API_BIN)

$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

$(API_BIN): token.o scanner.o ast.o parser.o visitor.o main_api.o
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ---- Pruebas: minicc vs gcc ----
TESTS := $(wildcard tests/*.c)

test:
	@echo "== Comparando minicc vs gcc =="
	@printf "%s\n" "Test results - $$(date)" > tests/resultados.txt
	@printf "%s\n" "========================" >> tests/resultados.txt
	@ok=0; total=0; \
	for f in $(TESTS); do \
		name=$$(basename $$f .c); \
		total=$$((total+1)); \
		./$(BIN) $$f -o tests/$$name.s >/dev/null 2>&1 && \
		gcc tests/$$name.s -o tests/$$name.minic.exe 2>/dev/null && \
		gcc $$f -o tests/$$name.gcc.exe 2>/dev/null; \
		out1=$$(./tests/$$name.minic.exe 2>/dev/null); r1=$$?; \
		out2=$$(./tests/$$name.gcc.exe 2>/dev/null); r2=$$?; \
		if [ "$$out1" = "$$out2" ]; then \
			echo "  [OK]   $$name (salida idéntica)"; \
			echo "[OK]   $$name" >> tests/resultados.txt; \
			ok=$$((ok+1)); \
		else \
			echo "  [DIFF] $$name  minic='$$out1'(rc=$$r1)  gcc='$$out2'(rc=$$r2)"; \
			echo "[DIFF] $$name  minic='$$out1'(rc=$$r1)  gcc='$$out2'(rc=$$r2)" >> tests/resultados.txt; \
		fi; \
	done; \
	echo "" >> tests/resultados.txt; \
	echo "$$ok/$$total tests passed." >> tests/resultados.txt

# ---- Benchmarks: minicc vs gcc (rendimiento) ----
BENCHES := $(wildcard benchmarks/*.c) $(wildcard tests/*.c)

bench:
	@echo ">> Ejecutando benchmarks..."
	@printf "%s\n" "Benchmark results - $$(date)" > benchmarks/resultados.txt
	@printf "%s\n" "==============================" >> benchmarks/resultados.txt
	@printf "" >> benchmarks/resultados.txt
	@printf "%-16s | %-8s | %10s %10s %10s %10s | %8s %8s %8s %8s\n" benchmark correcto mc-O0 mc-O1 gcc-O0 gcc-O2 mcO0-B mcO1-B gccO0-B gccO2-B
	@printf "%s\n" "-------------------------------------------------------------------------------------------------------------------------" >> benchmarks/resultados.txt
	@printf "%-16s | %-8s | %10s %10s %10s %10s | %8s %8s %8s %8s\n" benchmark correcto mc-O0 mc-O1 gcc-O0 gcc-O2 mcO0-B mcO1-B gccO0-B gccO2-B
	@printf "%s\n" "-------------------------------------------------------------------------------------------------------------------------"
	@for f in $(BENCHES); do \
		dir=$$(dirname $$f); \
		name=$$(basename $$f .c); \
		correcto="SI"; \
		\
		./$(BIN) $$f -o $$dir/$$name.mc0.s -O0 2>/dev/null; \
		gcc -no-pie $$dir/$$name.mc0.s -o $$dir/$$name.mc0.exe 2>/dev/null; \
		./$(BIN) $$f -o $$dir/$$name.mc1.s 2>/dev/null; \
		gcc -no-pie $$dir/$$name.mc1.s -o $$dir/$$name.mc1.exe 2>/dev/null; \
		gcc $$f -O0 -no-pie -o $$dir/$$name.gcc0.exe 2>/dev/null; \
		gcc $$f -O2 -no-pie -o $$dir/$$name.gcc2.exe 2>/dev/null; \
		\
		out_mc0=$$(./$$dir/$$name.mc0.exe 2>/dev/null); \
		out_mc1=$$(./$$dir/$$name.mc1.exe 2>/dev/null); \
		out_gcc0=$$(./$$dir/$$name.gcc0.exe 2>/dev/null); \
		out_gcc2=$$(./$$dir/$$name.gcc2.exe 2>/dev/null); \
		\
		if [ "$$out_mc0" != "$$out_gcc0" ] || [ "$$out_mc1" != "$$out_gcc2" ]; then \
			correcto="NO"; \
		fi; \
		\
		t_mc0=$$(TIMEFORMAT="%3R"; { time ./$$dir/$$name.mc0.exe >/dev/null 2>&1; } 2>&1); \
		t_mc1=$$(TIMEFORMAT="%3R"; { time ./$$dir/$$name.mc1.exe >/dev/null 2>&1; } 2>&1); \
		t_gcc0=$$(TIMEFORMAT="%3R"; { time ./$$dir/$$name.gcc0.exe >/dev/null 2>&1; } 2>&1); \
		t_gcc2=$$(TIMEFORMAT="%3R"; { time ./$$dir/$$name.gcc2.exe >/dev/null 2>&1; } 2>&1); \
		\
		sz_mc0=$$(wc -c < $$dir/$$name.mc0.exe 2>/dev/null || echo 0); \
		sz_mc1=$$(wc -c < $$dir/$$name.mc1.exe 2>/dev/null || echo 0); \
		sz_gcc0=$$(wc -c < $$dir/$$name.gcc0.exe 2>/dev/null || echo 0); \
		sz_gcc2=$$(wc -c < $$dir/$$name.gcc2.exe 2>/dev/null || echo 0); \
		\
		printf "%-16s | %-8s | %10s %10s %10s %10s | %8s %8s %8s %8s\n" "$$name" "$$correcto" "$$t_mc0" "$$t_mc1" "$$t_gcc0" "$$t_gcc2" "$$sz_mc0" "$$sz_mc1" "$$sz_gcc0" "$$sz_gcc2" >> benchmarks/resultados.txt; \
		printf "%-16s | %-8s | %10s %10s %10s %10s | %8s %8s %8s %8s\n" "$$name" "$$correcto" "$$t_mc0" "$$t_mc1" "$$t_gcc0" "$$t_gcc2" "$$sz_mc0" "$$sz_mc1" "$$sz_gcc0" "$$sz_gcc2"; \
	done
	@echo "" >> benchmarks/resultados.txt
	@echo "Legend: mc = minicc, gcc = GCC. Times in seconds (best of 3). Sizes in bytes." >> benchmarks/resultados.txt
	@echo ""
	@echo "Results saved to benchmarks/resultados.txt"

clean:
	rm -f $(OBJS) $(BIN) $(BIN).exe $(API_BIN) $(API_BIN).exe tests/*.s tests/*.exe tests/*_tokens.txt benchmarks/*.s benchmarks/*.exe

.PHONY: all test bench clean
