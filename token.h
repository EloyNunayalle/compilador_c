#ifndef TOKEN_H
#define TOKEN_H

// =============================================================================
// token.h — Token de MiniC (subconjunto de C)
// =============================================================================
// Unidad léxica mínima producida por el Scanner. Cada Token tiene un tipo y el
// texto literal del lexema. Cubre enteros, flotantes, punteros, cadenas,
// structs, y todas las palabras clave y operadores de C soportados.
// Nota: SWITCH/CASE/DEFAULT son reconocidos por el scanner pero el parser
// todavía no implementa la sentencia switch.
// =============================================================================

#include <ostream>
#include <string>

class Token {
public:
  enum Type {
    // ---- Operadores aritméticos ----
    PLUS,    // +
    MINUS,   // -
    STAR,    // *  (multiplicación / puntero / desreferencia)
    SLASH,   // /
    PERCENT, // %

    // ---- Incremento / decremento ----
    INC, // ++
    DEC, // --

    // ---- Asignación compuesta ----
    PLUS_ASSIGN,  // +=
    MINUS_ASSIGN, // -=
    STAR_ASSIGN,  // *=
    SLASH_ASSIGN, // /=

    // ---- Delimitadores ----
    LPAREN,   // (
    RPAREN,   // )
    LBRACKET, // [
    RBRACKET, // ]
    LBRACE,   // {
    RBRACE,   // }
    SEMICOL,  // ;
    COMMA,    // ,
    DOT,      // .
    ARROW,    // ->

    // ---- Relacionales ----
    LT, // <
    GT, // >
    LE, // <=
    GE, // >=
    EQ, // ==
    NE, // !=

    // ---- Lógicos / bit ----
    AND, // &&
    OR,  // ||
    NOT, // !
    AMP, // &  (address-of / bitand)

    // ---- Asignación ----
    ASSIGN, // =

    // ---- Literales ----
    INT_LIT,    // 123
    FLOAT_LIT,  // 1.5
    CHAR_LIT,   // 'a'
    STRING_LIT, // "hola"
    TRUE,       // true
    FALSE,      // false

    // ---- Identificadores y palabras clave ----
    ID,

    // Tipos
    KW_INT,    // int
    KW_DOUBLE, // double
    KW_CHAR,   // char
    KW_VOID,   // void
    KW_BOOL,   // bool

    // Declaraciones de tipos de usuario
    STRUCT,  // struct
    TYPEDEF, // typedef

    // Control de flujo
    IF,       // if
    ELSE,     // else
    WHILE,    // while
    FOR,      // for
    DO,       // do
    RETURN,   // return
    BREAK,    // break
    CONTINUE, // continue
    SWITCH,   // switch
    CASE,     // case
    DEFAULT,  // default

    // Operadores de palabra clave
    SIZEOF, // sizeof

    // ---- Especiales ----
    ERR, // carácter no reconocido (error léxico)
    END  // fin de la entrada
  };

  Type type;
  std::string text;

  Token(Type type);
  Token(Type type, char c);
  Token(Type type, const std::string &source, int first, int length);

  static std::string typeName(Type t);

  friend std::ostream &operator<<(std::ostream &outs, const Token &tok);
  friend std::ostream &operator<<(std::ostream &outs, const Token *tok);
};

#endif // TOKEN_H
