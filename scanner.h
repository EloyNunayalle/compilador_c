#ifndef SCANNER_H
#define SCANNER_H

// =============================================================================
// scanner.h — Analizador léxico de MiniC
// =============================================================================

#include "token.h"
#include <string>

class Scanner {
private:
  std::string input;
  int first;   // inicio del lexema actual
  int current; // posición de lectura

  bool skipTrivia(); // salta espacios y comentarios; false si error

public:
  Scanner(const char *s);
  ~Scanner();
  Token *nextToken();
};

// Utilidad de depuración: tokeniza un archivo y escribe <archivo>_tokens.txt
int ejecutar_scanner(Scanner *scanner, const std::string &InputFile);

#endif // SCANNER_H
