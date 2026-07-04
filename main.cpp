// =============================================================================
// main.cpp — Driver del compilador MiniC
// =============================================================================
// Flujo:
//   1. Leer archivo fuente (.c)
//   2. Scanner  → tokens
//   3. Parser   → AST
//   4. InlineVisitor → FoldVisitor → SethiVisitor (-O1 por defecto)
//   5. TypeChecker (análisis semántico + verificación de tipos)
//   6. GenCode  → x86-64 AT&T en <archivo>.s
//
// Uso:  ./minicc <archivo.c> [-o salida.s] [-O0|-O1]
// =============================================================================

#include "parser.h"
#include "scanner.h"
#include "visitor.h"
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Uso: " << argv[0] << " <archivo.c> [-o salida.s]\n";
    return 1;
  }

  std::string inputFile = argv[1];
  std::string outputFile;
  bool optimize = true; // -O1 por defecto; -O0 lo desactiva
  for (int i = 2; i < argc; i++) {
    std::string a = argv[i];
    if (a == "-o" && i + 1 < argc)
      outputFile = argv[++i];
    else if (a == "-O0")
      optimize = false;
    else if (a == "-O1")
      optimize = true;
  }
  if (outputFile.empty()) {
    size_t dot = inputFile.find_last_of('.');
    outputFile = (dot == std::string::npos ? inputFile : inputFile.substr(0, dot)) + ".s";
  }

  std::ifstream infile(inputFile);
  if (!infile.is_open()) {
    std::cerr << "Error: no se pudo abrir '" << inputFile << "'\n";
    return 1;
  }
  std::string input, line;
  while (std::getline(infile, line)) input += line + '\n';
  infile.close();

  // ---- Léxico + sintáctico ----
  Scanner scanner(input.c_str());
  Parser parser(&scanner);
  Program *program = nullptr;
  try {
    program = parser.parseProgram();
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << "\n";
    return 1;
  }

  // ---- Optimización sobre el AST (antes del análisis semántico) ----
  if (optimize) {
    InlineVisitor inl;
    inl.Inline(program);

    FoldVisitor fold;
    fold.Fold(program);
    std::cout << "Optimizacion -O1: " << fold.foldCount
              << " plegados de constantes, " << fold.algebraCount
              << " identidades algebraicas, " << fold.strengthCount
              << " reducciones de fuerza.\n";

    SethiVisitor sethi;
    sethi.Sethi(program);
  }

  // ---- Semántico ----
  TypeCheckerVisitor tc;
  tc.check(program);
  if (!tc.ok) {
    std::cerr << "Compilación abortada por errores semánticos.\n";
    delete program;
    return 1;
  }

  // ---- Generación de código ----
  std::ofstream outfile(outputFile);
  if (!outfile.is_open()) {
    std::cerr << "Error: no se pudo crear '" << outputFile << "'\n";
    delete program;
    return 1;
  }
  try {
    GenCodeVisitor codegen(outfile);
    codegen.generate(program, tc);
  } catch (const std::exception &ex) {
    std::cerr << "Error de generación: " << ex.what() << "\n";
    outfile.close();
    delete program;
    return 1;
  }
  outfile.close();
  std::cout << "Compilación exitosa: " << outputFile << "\n";
  delete program;
  return 0;
}
