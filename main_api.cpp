// =============================================================================
// main_api.cpp — Compilador MiniC para API
// =============================================================================
// Uso:
//   ./minicc_api --mode json     (lee stdin, retorna AST en JSON a stdout)
//   ./minicc_api --mode asm      (lee stdin, retorna x86-64 AT&T a stdout)
//   ./minicc_api --mode both     (retorna AST y ensamblador)
//   [-O0|-O1]                    (desactivar/activar optimizaciones)
//
// Salida: JSON con estructura unificada
// =============================================================================

#include "ast_visitor.h"
#include "parser.h"
#include "scanner.h"
#include "visitor.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

using json = nlohmann::json;

int main(int argc, char *argv[]) {
  // Valores por defecto
  std::string mode = "asm"; // "json" o "asm"
  bool optimize = true;

  // Parsear argumentos
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--mode" && i + 1 < argc) {
      mode = argv[++i];
    } else if (arg == "-O0") {
      optimize = false;
    } else if (arg == "-O1") {
      optimize = true;
    }
  }

  // Leer entrada desde stdin
  std::string input;
  std::string line;
  while (std::getline(std::cin, line)) {
    input += line + '\n';
  }

  try {
    // ---- Léxico + sintáctico ----
    Scanner scanner(input.c_str());
    Parser parser(&scanner);
    Program *program = parser.parseProgram();

    // ---- Optimización sobre el AST (opcional) ----
    if (optimize) {
      Optimizer opt;
      opt.run(program);
    }

    // ---- Semántico ----
    TypeCheckerVisitor tc;
    tc.check(program);
    if (!tc.ok) {
      json errorResponse = json{
          {"success", false},
          {"error", "Semantic errors during type checking"},
          {"details", tc.errors}};
      std::cout << errorResponse.dump() << std::endl;
      delete program;
      return 1;
    }

    // ---- Generar salida según modo ----
    json response;

    if (mode == "json") {
      // Modo JSON: retornar AST completo
      ASTJsonVisitor astVisitor;
      program->accept(&astVisitor);

      response = json{{"success", true},
                      {"mode", "ast"},
                      {"ast", astVisitor.getAst()},
                      {"types", json::object()}};

    } else if (mode == "asm") {
      // Modo x86-64: retornar ensamblador
      std::ostringstream asmStream;
      GenCodeVisitor codegen(asmStream);
      codegen.generate(program, tc);

      response = json{{"success", true},
                      {"mode", "asm"},
                      {"assembly", asmStream.str()},
                      {"types", json::object()}};

    } else if (mode == "both") {
      // Modo both: retornar AST y ensamblador
      ASTJsonVisitor astVisitor;
      program->accept(&astVisitor);

      std::ostringstream asmStream;
      GenCodeVisitor codegen(asmStream);
      codegen.generate(program, tc);

      response = json{{"success", true},
                      {"mode", "both"},
                      {"ast", astVisitor.getAst()},
                      {"assembly", asmStream.str()},
                      {"types", json::object()}};

    } else {
      response = json{{"success", false},
                      {"error", "Unknown mode: " + mode}};
      std::cout << response.dump() << std::endl;
      delete program;
      return 1;
    }

    std::cout << response.dump() << std::endl;
    delete program;
    return 0;

  } catch (const std::exception &ex) {
    json errorResponse = json{
        {"success", false},
        {"error", ex.what()}};
    std::cout << errorResponse.dump() << std::endl;
    return 1;
  }
}
