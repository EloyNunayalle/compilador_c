// =============================================================================
// scanner.cpp — Analizador léxico de MiniC (subconjunto de C)
// =============================================================================
// Reconoce:
//   · Comentarios de línea (//) y de bloque (/* */)
//   · Enteros y flotantes (123, 1.5, 3.14, .5, 10.)
//   · Literales de carácter ('a', '\n') y de cadena ("hola\n")
//   · Identificadores y palabras clave de C
//   · Operadores de C incluyendo -> ++ -- += -= *= /= && || == != <= >=
// =============================================================================

#include "scanner.h"
#include "token.h"
#include <cctype>
#include <fstream>
#include <iostream>

Scanner::Scanner(const char *s) : input(s), first(0), current(0) {}
Scanner::~Scanner() {}

static bool is_white_space(char c) {
  return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

// Salta espacios en blanco y comentarios. Devuelve false si encuentra un
// comentario de bloque sin cerrar (error léxico).
bool Scanner::skipTrivia() {
  while (current < (int)input.length()) {
    char c = input[current];
    if (is_white_space(c)) {
      current++;
    } else if (c == '/' && current + 1 < (int)input.length() &&
               input[current + 1] == '/') {
      // comentario de línea
      current += 2;
      while (current < (int)input.length() && input[current] != '\n')
        current++;
    } else if (c == '#') {
      // Directiva de preprocesador (#include, #define): se ignora la línea.
      // Permite que los mismos .c compilen con gcc y con minicc.
      while (current < (int)input.length() && input[current] != '\n')
        current++;
    } else if (c == '/' && current + 1 < (int)input.length() &&
               input[current + 1] == '*') {
      // comentario de bloque
      current += 2;
      bool closed = false;
      while (current + 1 < (int)input.length()) {
        if (input[current] == '*' && input[current + 1] == '/') {
          current += 2;
          closed = true;
          break;
        }
        current++;
      }
      if (!closed)
        return false; // comentario sin cerrar
    } else {
      break;
    }
  }
  return true;
}

Token *Scanner::nextToken() {
  if (!skipTrivia())
    return new Token(Token::ERR, "/*", 0, 2);

  if (current >= (int)input.length())
    return new Token(Token::END);

  char c = input[current];
  first = current;

  // ---- Números: enteros y flotantes ----
  if (isdigit(c) || (c == '.' && current + 1 < (int)input.length() &&
                     isdigit(input[current + 1]))) {
    bool isFloat = false;
    while (current < (int)input.length() && isdigit(input[current]))
      current++;
    if (current < (int)input.length() && input[current] == '.') {
      isFloat = true;
      current++;
      while (current < (int)input.length() && isdigit(input[current]))
        current++;
    }
    // exponente opcional: e[+/-]dígitos
    if (current < (int)input.length() &&
        (input[current] == 'e' || input[current] == 'E')) {
      isFloat = true;
      current++;
      if (current < (int)input.length() &&
          (input[current] == '+' || input[current] == '-'))
        current++;
      while (current < (int)input.length() && isdigit(input[current]))
        current++;
    }
    Token::Type t = isFloat ? Token::FLOAT_LIT : Token::INT_LIT;
    return new Token(t, input, first, current - first);
  }

  // ---- Identificadores y palabras clave ----
  if (isalpha(c) || c == '_') {
    current++;
    while (current < (int)input.length() &&
           (isalnum(input[current]) || input[current] == '_'))
      current++;
    std::string lex = input.substr(first, current - first);

    if (lex == "int")      return new Token(Token::KW_INT, input, first, current - first);
    if (lex == "long")     return new Token(Token::KW_INT, input, first, current - first);
    if (lex == "double")   return new Token(Token::KW_DOUBLE, input, first, current - first);
    if (lex == "float")    return new Token(Token::KW_DOUBLE, input, first, current - first);
    if (lex == "char")     return new Token(Token::KW_CHAR, input, first, current - first);
    if (lex == "void")     return new Token(Token::KW_VOID, input, first, current - first);
    if (lex == "bool")     return new Token(Token::KW_BOOL, input, first, current - first);
    if (lex == "_Bool")    return new Token(Token::KW_BOOL, input, first, current - first);
    if (lex == "struct")   return new Token(Token::STRUCT, input, first, current - first);
    if (lex == "typedef")  return new Token(Token::TYPEDEF, input, first, current - first);
    if (lex == "if")       return new Token(Token::IF, input, first, current - first);
    if (lex == "else")     return new Token(Token::ELSE, input, first, current - first);
    if (lex == "while")    return new Token(Token::WHILE, input, first, current - first);
    if (lex == "for")      return new Token(Token::FOR, input, first, current - first);
    if (lex == "do")       return new Token(Token::DO, input, first, current - first);
    if (lex == "return")   return new Token(Token::RETURN, input, first, current - first);
    if (lex == "break")    return new Token(Token::BREAK, input, first, current - first);
    if (lex == "continue") return new Token(Token::CONTINUE, input, first, current - first);
    if (lex == "switch")   return new Token(Token::SWITCH, input, first, current - first);
    if (lex == "case")     return new Token(Token::CASE, input, first, current - first);
    if (lex == "default")  return new Token(Token::DEFAULT, input, first, current - first);
    if (lex == "sizeof")   return new Token(Token::SIZEOF, input, first, current - first);
    if (lex == "true")     return new Token(Token::TRUE, input, first, current - first);
    if (lex == "false")    return new Token(Token::FALSE, input, first, current - first);

    return new Token(Token::ID, input, first, current - first);
  }

  // ---- Literal de carácter: 'a' o '\n' ----
  if (c == '\'') {
    current++; // consume '
    std::string val;
    if (current < (int)input.length() && input[current] == '\\') {
      val += input[current++]; // backslash
      if (current < (int)input.length())
        val += input[current++]; // carácter escapado
    } else if (current < (int)input.length()) {
      val += input[current++];
    }
    if (current < (int)input.length() && input[current] == '\'')
      current++; // consume '
    else
      return new Token(Token::ERR, "'", 0, 1);
    Token *tok = new Token(Token::CHAR_LIT);
    tok->text = val;
    return tok;
  }

  // ---- Literal de cadena: "..." ----
  if (c == '"') {
    current++; // consume "
    std::string val;
    while (current < (int)input.length() && input[current] != '"') {
      if (input[current] == '\\' && current + 1 < (int)input.length()) {
        val += input[current++]; // backslash
        val += input[current++]; // escapado
      } else {
        val += input[current++];
      }
    }
    if (current < (int)input.length() && input[current] == '"')
      current++; // consume "
    else
      return new Token(Token::ERR, "\"", 0, 1);
    Token *tok = new Token(Token::STRING_LIT);
    tok->text = val;
    return tok;
  }

  // ---- Operadores y delimitadores ----
  auto peek = [&](int k) -> char {
    return (current + k < (int)input.length()) ? input[current + k] : '\0';
  };

  switch (c) {
  case '+':
    if (peek(1) == '+') { current += 2; return new Token(Token::INC, "++", 0, 2); }
    if (peek(1) == '=') { current += 2; return new Token(Token::PLUS_ASSIGN, "+=", 0, 2); }
    current++; return new Token(Token::PLUS, c);
  case '-':
    if (peek(1) == '-') { current += 2; return new Token(Token::DEC, "--", 0, 2); }
    if (peek(1) == '=') { current += 2; return new Token(Token::MINUS_ASSIGN, "-=", 0, 2); }
    if (peek(1) == '>') { current += 2; return new Token(Token::ARROW, "->", 0, 2); }
    current++; return new Token(Token::MINUS, c);
  case '*':
    if (peek(1) == '=') { current += 2; return new Token(Token::STAR_ASSIGN, "*=", 0, 2); }
    current++; return new Token(Token::STAR, c);
  case '/':
    if (peek(1) == '=') { current += 2; return new Token(Token::SLASH_ASSIGN, "/=", 0, 2); }
    current++; return new Token(Token::SLASH, c);
  case '%': current++; return new Token(Token::PERCENT, c);
  case '(': current++; return new Token(Token::LPAREN, c);
  case ')': current++; return new Token(Token::RPAREN, c);
  case '[': current++; return new Token(Token::LBRACKET, c);
  case ']': current++; return new Token(Token::RBRACKET, c);
  case '{': current++; return new Token(Token::LBRACE, c);
  case '}': current++; return new Token(Token::RBRACE, c);
  case ';': current++; return new Token(Token::SEMICOL, c);
  case ',': current++; return new Token(Token::COMMA, c);
  case '.': current++; return new Token(Token::DOT, c);
  case '<':
    if (peek(1) == '=') { current += 2; return new Token(Token::LE, "<=", 0, 2); }
    current++; return new Token(Token::LT, c);
  case '>':
    if (peek(1) == '=') { current += 2; return new Token(Token::GE, ">=", 0, 2); }
    current++; return new Token(Token::GT, c);
  case '=':
    if (peek(1) == '=') { current += 2; return new Token(Token::EQ, "==", 0, 2); }
    current++; return new Token(Token::ASSIGN, c);
  case '!':
    if (peek(1) == '=') { current += 2; return new Token(Token::NE, "!=", 0, 2); }
    current++; return new Token(Token::NOT, c);
  case '&':
    if (peek(1) == '&') { current += 2; return new Token(Token::AND, "&&", 0, 2); }
    current++; return new Token(Token::AMP, c);
  case '|':
    if (peek(1) == '|') { current += 2; return new Token(Token::OR, "||", 0, 2); }
    current++; return new Token(Token::ERR, c);
  }

  // ---- Carácter no reconocido ----
  Token *err = new Token(Token::ERR, c);
  current++;
  return err;
}

int ejecutar_scanner(Scanner *scanner, const std::string &InputFile) {
  std::string outputName = InputFile;
  size_t pos = outputName.find_last_of('.');
  if (pos != std::string::npos)
    outputName = outputName.substr(0, pos);
  outputName += "_tokens.txt";

  std::ofstream outFile(outputName);
  if (!outFile.is_open()) {
    std::cerr << "Error: no se pudo abrir el archivo de salida: " << outputName
              << std::endl;
    return 1;
  }

  outFile << "Scanner\n" << std::endl;
  Token *tok;
  while (true) {
    tok = scanner->nextToken();
    if (tok->type == Token::ERR) {
      outFile << *tok << std::endl;
      outFile << "Error léxico: token inválido '" << tok->text << "'\n";
      outFile << "\nScanner no exitoso\n";
      delete tok;
      outFile.close();
      return 1;
    }
    outFile << *tok << std::endl;
    if (tok->type == Token::END) {
      outFile << "\nScanner exitoso\n";
      delete tok;
      outFile.close();
      return 0;
    }
    delete tok;
  }
}
