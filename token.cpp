// =============================================================================
// token.cpp — Implementación de Token
// =============================================================================

#include "token.h"

Token::Token(Type type) : type(type), text("") {}

Token::Token(Type type, char c) : type(type), text(std::string(1, c)) {}

Token::Token(Type type, const std::string &source, int first, int length)
    : type(type), text(source.substr(first, length)) {}

std::string Token::typeName(Type t) {
  switch (t) {
  case PLUS:         return "'+'";
  case MINUS:        return "'-'";
  case STAR:         return "'*'";
  case SLASH:        return "'/'";
  case PERCENT:      return "'%'";
  case INC:          return "'++'";
  case DEC:          return "'--'";
  case PLUS_ASSIGN:  return "'+='";
  case MINUS_ASSIGN: return "'-='";
  case STAR_ASSIGN:  return "'*='";
  case SLASH_ASSIGN: return "'/='";
  case LPAREN:       return "'('";
  case RPAREN:       return "')'";
  case LBRACKET:     return "'['";
  case RBRACKET:     return "']'";
  case LBRACE:       return "'{'";
  case RBRACE:       return "'}'";
  case SEMICOL:      return "';'";
  case COMMA:        return "','";
  case DOT:          return "'.'";
  case ARROW:        return "'->'";
  case LT:           return "'<'";
  case GT:           return "'>'";
  case LE:           return "'<='";
  case GE:           return "'>='";
  case EQ:           return "'=='";
  case NE:           return "'!='";
  case AND:          return "'&&'";
  case OR:           return "'||'";
  case NOT:          return "'!'";
  case AMP:          return "'&'";
  case ASSIGN:       return "'='";
  case INT_LIT:      return "literal entero";
  case FLOAT_LIT:    return "literal flotante";
  case CHAR_LIT:     return "literal de carácter";
  case STRING_LIT:   return "literal de cadena";
  case TRUE:         return "'true'";
  case FALSE:        return "'false'";
  case ID:           return "identificador";
  case KW_INT:       return "'int'";
  case KW_DOUBLE:    return "'double'";
  case KW_CHAR:      return "'char'";
  case KW_VOID:      return "'void'";
  case KW_BOOL:      return "'bool'";
  case STRUCT:       return "'struct'";
  case TYPEDEF:      return "'typedef'";
  case IF:           return "'if'";
  case ELSE:         return "'else'";
  case WHILE:        return "'while'";
  case FOR:          return "'for'";
  case DO:           return "'do'";
  case RETURN:       return "'return'";
  case BREAK:        return "'break'";
  case CONTINUE:     return "'continue'";
  case SWITCH:       return "'switch'";
  case CASE:         return "'case'";
  case DEFAULT:      return "'default'";
  case SIZEOF:       return "'sizeof'";
  case ERR:          return "<error léxico>";
  case END:          return "fin de entrada";
  default:           return "<desconocido>";
  }
}

std::ostream &operator<<(std::ostream &outs, const Token &tok) {
  outs << "TOKEN(" << Token::typeName(tok.type);
  if (!tok.text.empty())
    outs << ", \"" << tok.text << "\"";
  outs << ")";
  return outs;
}

std::ostream &operator<<(std::ostream &outs, const Token *tok) {
  if (!tok)
    return outs << "TOKEN(NULL)";
  return outs << *tok;
}
