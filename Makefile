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

# Dependencias de cabeceras
token.o:   token.h
scanner.o: scanner.h token.h
ast.o:     ast.h visitor.h
parser.o:  parser.h scanner.h ast.h
visitor.o: visitor.h ast.h environment.h
main.o:    parser.h scanner.h visitor.h ast.h
main_api.o: parser.h scanner.h visitor.h ast.h ast_visitor.h

# ---- Pruebas: minicc vs gcc ----
TESTS := $(wildcard tests/*.c)

test: $(BIN)
	@echo "== Comparando minicc vs gcc =="
	@for f in $(TESTS); do \
		name=$$(basename $$f .c); \
		./$(BIN) $$f -o tests/$$name.s >/dev/null 2>&1 && \
		gcc tests/$$name.s -o tests/$$name.minic.exe 2>/dev/null && \
		gcc $$f -o tests/$$name.gcc.exe 2>/dev/null; \
		out1=$$(./tests/$$name.minic.exe 2>/dev/null); r1=$$?; \
		out2=$$(./tests/$$name.gcc.exe 2>/dev/null); r2=$$?; \
		if [ "$$out1" = "$$out2" ]; then \
			echo "  [OK]   $$name (salida idéntica)"; \
		else \
			echo "  [DIFF] $$name  minic='$$out1'(rc=$$r1)  gcc='$$out2'(rc=$$r2)"; \
		fi; \
	done

clean:
	rm -f $(OBJS) $(BIN) $(BIN).exe $(API_BIN) $(API_BIN).exe tests/*.s tests/*.exe tests/*_tokens.txt

.PHONY: all test clean
