#ifndef PARSER_H
#define PARSER_H

// =============================================================================
// parser.h — Parser recursivo-descendente para MiniC (gramática C)
// =============================================================================
// Gramática (M1):
//   Program    → (VarDecl | FunDef)*
//   FunDef     → Type ID '(' Params ')' Block
//   Params     → ε | Param (',' Param)*
//   Param      → Type ID
//   Type       → ('int'|'double'|'char'|'void'|'bool') '*'*
//   Block      → '{' Stm* '}'
//   Stm        → VarDecl | If | While | For | DoWhile | Return
//              | Break | Continue | Block | ExprOrAssign ';'
//   VarDecl    → Type InitDecl (',' InitDecl)* ';'
//   InitDecl   → ID ('[' INT ']')? ('=' Expr)?
//   Expr       → assignment
//   (precedencia estándar de C hacia abajo)
// =============================================================================

#include "ast.h"
#include "scanner.h"

class Parser {
private:
  Scanner *scanner;
  Token *current;
  Token *previous;

  bool match(Token::Type t);
  bool check(Token::Type t);
  void advance();
  bool isAtEnd();
  void error(const std::string &expected);
  void expect(Token::Type t, const std::string &what);

  // ---- Reglas ----
  bool isTypeStart();
  Type parseType();
  bool parseParam(Param &out); // parsea un parámetro; false si era (void)
  Block *parseBlock();
  Stm *parseStm();
  VarDecStm *parseVarDecl(Type baseType); // consume hasta ';'
  Stm *parseSimpleStm();                  // para init/update de for (sin ';')

  // Expresiones por precedencia
  Exp *parseExpr();       // = asignación gestionada en parseStm; aquí: ternario/lógico
  Exp *parseLogicalOr();
  Exp *parseLogicalAnd();
  Exp *parseEquality();
  Exp *parseRelational();
  Exp *parseAdditive();
  Exp *parseMultiplicative();
  Exp *parseUnary();
  Exp *parsePostfix();
  Exp *parsePrimary();

public:
  Parser(Scanner *scanner);
  Program *parseProgram();
};

#endif // PARSER_H
