// =============================================================================
// parser.cpp — Parser recursivo-descendente de MiniC
// =============================================================================

#include "parser.h"
#include <stdexcept>

Parser::Parser(Scanner *scanner) : scanner(scanner), previous(nullptr) {
  current = scanner->nextToken();
}

// ---- Primitivas ----
bool Parser::check(Token::Type t) { return current->type == t; }
bool Parser::isAtEnd() { return current->type == Token::END; }

void Parser::advance() {
  if (previous) delete previous;
  previous = current;
  current = scanner->nextToken();
}

bool Parser::match(Token::Type t) {
  if (check(t)) { advance(); return true; }
  return false;
}

void Parser::error(const std::string &expected) {
  throw std::runtime_error("Error sintáctico: se esperaba " + expected +
                           " pero se encontró " +
                           Token::typeName(current->type) +
                           (current->text.empty() ? "" : " ('" + current->text + "')"));
}

void Parser::expect(Token::Type t, const std::string &what) {
  if (!match(t)) error(what);
}

// ---- Tipos ----
bool Parser::isTypeStart() {
  return check(Token::KW_INT) || check(Token::KW_DOUBLE) ||
         check(Token::KW_CHAR) || check(Token::KW_VOID) ||
         check(Token::KW_BOOL) || check(Token::STRUCT);
}

Type Parser::parseType() {
  Type t;
  if (match(Token::KW_INT)) t.base = Type::INT;
  else if (match(Token::KW_DOUBLE)) t.base = Type::DOUBLE;
  else if (match(Token::KW_CHAR)) t.base = Type::CHAR;
  else if (match(Token::KW_VOID)) t.base = Type::VOID;
  else if (match(Token::KW_BOOL)) t.base = Type::BOOL;
  else if (match(Token::STRUCT)) {
    t.base = Type::STRUCT;
    if (!check(Token::ID)) error("nombre de struct");
    t.structName = current->text;
    advance();
  } else error("un tipo (int, double, char, void, bool, struct)");
  while (match(Token::STAR)) t.pointer++;
  return t;
}

// ---- Parámetro (soporta punteros a función: retType (*name)(tipos...)) ----
// Devuelve false si el "parámetro" era realmente 'void' (lista vacía).
bool Parser::parseParam(Param &out) {
  Type base = parseType();
  if (base.base == Type::VOID && base.pointer == 0 && !check(Token::LPAREN)) {
    return false; // (void)
  }
  // Puntero a función:  retType (*name)(tipos...)
  if (check(Token::LPAREN)) {
    advance(); // (
    expect(Token::STAR, "'*'");
    std::string pname;
    if (check(Token::ID)) { pname = current->text; advance(); }
    expect(Token::RPAREN, "')'");
    expect(Token::LPAREN, "'('");
    if (!check(Token::RPAREN)) {
      do {
        if (isTypeStart()) parseType();
        if (check(Token::ID)) advance();
      } while (match(Token::COMMA));
    }
    expect(Token::RPAREN, "')'");
    Type fp = base;
    fp.isFuncPtr = true;
    fp.pointer = 1;
    out.type = fp;
    out.name = pname;
    return true;
  }
  // Parámetro normal (permite arreglo 'int a[]' tratado como puntero)
  if (!check(Token::ID)) error("nombre de parámetro");
  out.name = current->text;
  advance();
  if (match(Token::LBRACKET)) {
    // 'int a[]' o 'int a[N]' => puntero
    if (check(Token::INT_LIT)) advance();
    expect(Token::RBRACKET, "']'");
    base.pointer++;
  }
  out.type = base;
  return true;
}

// ---- Programa ----
Program *Parser::parseProgram() {
  Program *prog = new Program();
  while (!isAtEnd()) {
    // ---- Posible definición de struct: struct Nombre { campos }; ----
    if (check(Token::STRUCT)) {
      advance(); // struct
      if (!check(Token::ID)) error("nombre de struct");
      std::string sname = current->text;
      advance();
      if (check(Token::LBRACE)) {
        StructDef *sd = new StructDef();
        sd->name = sname;
        advance(); // {
        while (!check(Token::RBRACE) && !isAtEnd()) {
          Type ft = parseType();
          do {
            StructField fld;
            Type vt = ft;
            while (match(Token::STAR)) vt.pointer++;
            if (!check(Token::ID)) error("nombre de campo");
            fld.name = current->text;
            advance();
            fld.type = vt;
            sd->fields.push_back(fld);
          } while (match(Token::COMMA));
          expect(Token::SEMICOL, "';'");
        }
        expect(Token::RBRACE, "'}'");
        expect(Token::SEMICOL, "';'");
        prog->structs.push_back(sd);
        continue;
      }
      // No es definición: es un tipo 'struct Nombre' para var/función.
      Type t;
      t.base = Type::STRUCT;
      t.structName = sname;
      while (match(Token::STAR)) t.pointer++;
      if (!check(Token::ID)) error("un identificador");
      std::string name = current->text;
      advance();
      // Reutiliza la lógica común de var/función mediante un salto controlado:
      // (procesamos aquí mismo el caso struct para no duplicar demasiado)
      if (check(Token::LPAREN)) {
        FunDef *fn = new FunDef();
        fn->retType = t; fn->name = name;
        advance();
        if (!check(Token::RPAREN)) {
          do {
            Param p;
            if (!parseParam(p)) break;
            fn->params.push_back(p);
          } while (match(Token::COMMA));
        }
        expect(Token::RPAREN, "')'");
        fn->body = parseBlock();
        prog->functions.push_back(fn);
      } else {
        VarDecStm *vd = new VarDecStm();
        vd->type = t;
        VarInit vi; vi.name = name; vi.type = t;
        if (match(Token::ASSIGN)) vi.init = parseExpr();
        vd->vars.push_back(vi);
        expect(Token::SEMICOL, "';'");
        prog->globals.push_back(vd);
      }
      continue;
    }

    Type t = parseType();
    if (!check(Token::ID)) error("un identificador");
    std::string name = current->text;
    advance();

    if (check(Token::LPAREN)) {
      // ---- Definición de función ----
      FunDef *fn = new FunDef();
      fn->retType = t;
      fn->name = name;
      advance(); // (
      if (!check(Token::RPAREN)) {
        do {
          Param p;
          if (!parseParam(p)) break; // (void)
          fn->params.push_back(p);
        } while (match(Token::COMMA));
      }
      expect(Token::RPAREN, "')'");
      fn->body = parseBlock();
      prog->functions.push_back(fn);
    } else {
      // ---- Declaración de variable global ----
      VarDecStm *vd = new VarDecStm();
      vd->type = t;
      VarInit vi;
      vi.name = name;
      vi.type = t;
      if (match(Token::LBRACKET)) {
        if (!check(Token::INT_LIT)) error("tamaño de arreglo");
        vi.arraySize = std::stoi(current->text);
        advance();
        expect(Token::RBRACKET, "']'");
        vi.type.isArray = true; vi.type.arrayLen = vi.arraySize;
        if (match(Token::LBRACKET)) { // segunda dimensión global
          if (!check(Token::INT_LIT)) error("tamaño de 2ª dimensión");
          vi.type.cols = std::stoi(current->text);
          advance();
          expect(Token::RBRACKET, "']'");
        }
      }
      if (match(Token::ASSIGN)) vi.init = parseExpr();
      vd->vars.push_back(vi);
      while (match(Token::COMMA)) {
        VarInit vi2;
        Type vt2 = t;
        while (match(Token::STAR)) vt2.pointer++;
        if (!check(Token::ID)) error("un identificador");
        vi2.name = current->text;
        advance();
        if (match(Token::LBRACKET)) {
          if (!check(Token::INT_LIT)) error("tamaño de arreglo");
          vi2.arraySize = std::stoi(current->text);
          advance();
          expect(Token::RBRACKET, "']'");
          vt2.isArray = true; vt2.arrayLen = vi2.arraySize;
        }
        vi2.type = vt2;
        if (match(Token::ASSIGN)) vi2.init = parseExpr();
        vd->vars.push_back(vi2);
      }
      expect(Token::SEMICOL, "';'");
      prog->globals.push_back(vd);
    }
  }
  return prog;
}

// ---- Bloque ----
Block *Parser::parseBlock() {
  expect(Token::LBRACE, "'{'");
  Block *b = new Block();
  while (!check(Token::RBRACE) && !isAtEnd())
    b->stms.push_back(parseStm());
  expect(Token::RBRACE, "'}'");
  return b;
}

// ---- Declaración de variable local (consume ';') ----
VarDecStm *Parser::parseVarDecl(Type baseType) {
  VarDecStm *vd = new VarDecStm();
  vd->type = baseType;
  do {
    VarInit vi;
    // Puntero a función:  retType (*name)(params...)
    if (check(Token::LPAREN)) {
      advance();               // (
      expect(Token::STAR, "'*'");
      if (!check(Token::ID)) error("nombre de puntero a función");
      std::string fpname = current->text;
      advance();
      expect(Token::RPAREN, "')'");
      expect(Token::LPAREN, "'('");
      // consumir la lista de tipos de parámetros (no la usamos para el codegen)
      if (!check(Token::RPAREN)) {
        do {
          if (isTypeStart()) parseType();
          // permitir nombres de parámetro opcionales
          if (check(Token::ID)) advance();
        } while (match(Token::COMMA));
      }
      expect(Token::RPAREN, "')'");
      Type fp = baseType;
      fp.isFuncPtr = true;
      fp.pointer = 1; // se comporta como un puntero (8 bytes)
      vi.name = fpname;
      vi.type = fp;
      if (match(Token::ASSIGN)) vi.init = parseExpr();
      vd->vars.push_back(vi);
      continue;
    }
    // '*' extra pertenece a esta variable: int *p, x;
    Type varT = baseType;
    while (match(Token::STAR)) varT.pointer++;
    if (!check(Token::ID)) error("un identificador");
    vi.name = current->text;
    advance();
    if (match(Token::LBRACKET)) {
      if (!check(Token::INT_LIT)) error("tamaño de arreglo");
      vi.arraySize = std::stoi(current->text);
      advance();
      expect(Token::RBRACKET, "']'");
      varT.isArray = true;
      varT.arrayLen = vi.arraySize;
      // Segunda dimensión: int m[R][C]
      if (match(Token::LBRACKET)) {
        if (!check(Token::INT_LIT)) error("tamaño de 2ª dimensión");
        varT.cols = std::stoi(current->text);
        advance();
        expect(Token::RBRACKET, "']'");
      }
    }
    vi.type = varT;
    if (match(Token::ASSIGN)) vi.init = parseExpr();
    vd->vars.push_back(vi);
  } while (match(Token::COMMA));
  expect(Token::SEMICOL, "';'");
  return vd;
}

// Convierte una expresión con posible operador de asignación en Stm.
// (No consume ';'.)
static AssignOp assignOpFromToken(Token::Type t) {
  switch (t) {
  case Token::PLUS_ASSIGN: return ASSIGN_ADD;
  case Token::MINUS_ASSIGN: return ASSIGN_SUB;
  case Token::STAR_ASSIGN: return ASSIGN_MUL;
  case Token::SLASH_ASSIGN: return ASSIGN_DIV;
  default: return ASSIGN_EQ;
  }
}

Stm *Parser::parseSimpleStm() {
  // Usado en el init/update del for (sin ';'). Puede ser declaración,
  // asignación, ++/--, o expresión.
  if (isTypeStart()) {
    Type t = parseType();
    VarDecStm *vd = new VarDecStm();
    vd->type = t;
    do {
      VarInit vi;
      Type vt = t;
      while (match(Token::STAR)) vt.pointer++;
      if (!check(Token::ID)) error("un identificador");
      vi.name = current->text;
      vi.type = vt;
      advance();
      if (match(Token::ASSIGN)) vi.init = parseExpr();
      vd->vars.push_back(vi);
    } while (match(Token::COMMA));
    return vd;
  }
  Exp *lhs = parseExpr();
  if (check(Token::ASSIGN) || check(Token::PLUS_ASSIGN) ||
      check(Token::MINUS_ASSIGN) || check(Token::STAR_ASSIGN) ||
      check(Token::SLASH_ASSIGN)) {
    AssignOp op = assignOpFromToken(current->type);
    advance();
    Exp *rhs = parseExpr();
    return new AssignStm(lhs, op, rhs);
  }
  // ++ / --  (postfijo; como sentencia equivale a += 1 / -= 1)
  // Nota: usamos ASSIGN_ADD/SUB para no reutilizar el nodo 'lhs' dos veces
  // (evita doble liberación en los destructores del AST).
  if (check(Token::INC) || check(Token::DEC)) {
    AssignOp aop = check(Token::INC) ? ASSIGN_ADD : ASSIGN_SUB;
    advance();
    return new AssignStm(lhs, aop, new IntLit(1));
  }
  return new ExprStm(lhs);
}

// ---- Sentencias ----
Stm *Parser::parseStm() {
  // Declaración local
  if (isTypeStart())
    return parseVarDecl(parseType());

  if (check(Token::LBRACE)) return parseBlock();

  if (match(Token::IF)) {
    expect(Token::LPAREN, "'('");
    Exp *cond = parseExpr();
    expect(Token::RPAREN, "')'");
    Block *thenB = check(Token::LBRACE) ? parseBlock() : nullptr;
    if (!thenB) { // permite sentencia única sin llaves
      thenB = new Block();
      thenB->stms.push_back(parseStm());
    }
    Block *elseB = nullptr;
    if (match(Token::ELSE)) {
      if (check(Token::LBRACE)) elseB = parseBlock();
      else { elseB = new Block(); elseB->stms.push_back(parseStm()); }
    }
    return new IfStm(cond, thenB, elseB);
  }

  if (match(Token::WHILE)) {
    expect(Token::LPAREN, "'('");
    Exp *cond = parseExpr();
    expect(Token::RPAREN, "')'");
    Block *body = check(Token::LBRACE) ? parseBlock() : nullptr;
    if (!body) { body = new Block(); body->stms.push_back(parseStm()); }
    return new WhileStm(cond, body);
  }

  if (match(Token::DO)) {
    Block *body = parseBlock();
    expect(Token::WHILE, "'while'");
    expect(Token::LPAREN, "'('");
    Exp *cond = parseExpr();
    expect(Token::RPAREN, "')'");
    expect(Token::SEMICOL, "';'");
    return new DoWhileStm(body, cond);
  }

  if (match(Token::FOR)) {
    expect(Token::LPAREN, "'('");
    Stm *init = nullptr;
    if (!check(Token::SEMICOL)) init = parseSimpleStm();
    expect(Token::SEMICOL, "';'");
    Exp *cond = nullptr;
    if (!check(Token::SEMICOL)) cond = parseExpr();
    expect(Token::SEMICOL, "';'");
    Stm *update = nullptr;
    if (!check(Token::RPAREN)) update = parseSimpleStm();
    expect(Token::RPAREN, "')'");
    Block *body = check(Token::LBRACE) ? parseBlock() : nullptr;
    if (!body) { body = new Block(); body->stms.push_back(parseStm()); }
    return new ForStm(init, cond, update, body);
  }

  if (match(Token::RETURN)) {
    Exp *e = nullptr;
    if (!check(Token::SEMICOL)) e = parseExpr();
    expect(Token::SEMICOL, "';'");
    return new ReturnStm(e);
  }

  if (match(Token::BREAK)) {
    expect(Token::SEMICOL, "';'");
    return new BreakStm();
  }
  if (match(Token::CONTINUE)) {
    expect(Token::SEMICOL, "';'");
    return new ContinueStm();
  }

  // Asignación / expresión / ++/--
  Stm *s = parseSimpleStm();
  expect(Token::SEMICOL, "';'");
  return s;
}

// =============================================================================
// Expresiones (precedencia estándar de C, sin ternario)
// =============================================================================

Exp *Parser::parseExpr() { return parseLogicalOr(); }

Exp *Parser::parseLogicalOr() {
  Exp *e = parseLogicalAnd();
  while (match(Token::OR))
    e = new BinaryExp(e, OR_OP, parseLogicalAnd());
  return e;
}

Exp *Parser::parseLogicalAnd() {
  Exp *e = parseEquality();
  while (match(Token::AND))
    e = new BinaryExp(e, AND_OP, parseEquality());
  return e;
}

Exp *Parser::parseEquality() {
  Exp *e = parseRelational();
  while (check(Token::EQ) || check(Token::NE)) {
    BinaryOp op = check(Token::EQ) ? EQ_OP : NE_OP;
    advance();
    e = new BinaryExp(e, op, parseRelational());
  }
  return e;
}

Exp *Parser::parseRelational() {
  Exp *e = parseAdditive();
  while (check(Token::LT) || check(Token::GT) || check(Token::LE) ||
         check(Token::GE)) {
    BinaryOp op = check(Token::LT)   ? LT_OP
                  : check(Token::GT) ? GT_OP
                  : check(Token::LE) ? LE_OP
                                     : GE_OP;
    advance();
    e = new BinaryExp(e, op, parseAdditive());
  }
  return e;
}

Exp *Parser::parseAdditive() {
  Exp *e = parseMultiplicative();
  while (check(Token::PLUS) || check(Token::MINUS)) {
    BinaryOp op = check(Token::PLUS) ? PLUS_OP : MINUS_OP;
    advance();
    e = new BinaryExp(e, op, parseMultiplicative());
  }
  return e;
}

Exp *Parser::parseMultiplicative() {
  Exp *e = parseUnary();
  while (check(Token::STAR) || check(Token::SLASH) || check(Token::PERCENT)) {
    BinaryOp op = check(Token::STAR)    ? MUL_OP
                  : check(Token::SLASH) ? DIV_OP
                                        : MOD_OP;
    advance();
    e = new BinaryExp(e, op, parseUnary());
  }
  return e;
}

Exp *Parser::parseUnary() {
  if (match(Token::MINUS)) return new UnaryExp(NEG_OP, parseUnary());
  if (match(Token::NOT)) return new UnaryExp(NOT_OP, parseUnary());
  if (match(Token::PLUS)) return parseUnary(); // +x == x
  if (match(Token::AMP)) return new AddrExp(parseUnary());   // &x
  if (match(Token::STAR)) return new DerefExp(parseUnary()); // *p
  if (match(Token::SIZEOF)) {
    // sizeof(type)  |  sizeof(expr)  |  sizeof expr
    if (match(Token::LPAREN)) {
      if (isTypeStart()) {
        Type t = parseType();
        expect(Token::RPAREN, "')'");
        return new SizeofExp(t);
      }
      Exp *inner = parseExpr();
      expect(Token::RPAREN, "')'");
      return new SizeofExp(inner);
    }
    return new SizeofExp(parseUnary());
  }
  return parsePostfix();
}

Exp *Parser::parsePostfix() {
  Exp *e = parsePrimary();
  while (true) {
    if (match(Token::LBRACKET)) {
      Exp *idx = parseExpr();
      expect(Token::RBRACKET, "']'");
      e = new IndexExp(e, idx);
    } else if (match(Token::DOT)) {
      if (!check(Token::ID)) error("nombre de campo");
      std::string f = current->text;
      advance();
      e = new FieldExp(e, f, false);
    } else if (match(Token::ARROW)) {
      if (!check(Token::ID)) error("nombre de campo");
      std::string f = current->text;
      advance();
      e = new FieldExp(e, f, true);
    } else {
      break;
    }
  }
  return e;
}

Exp *Parser::parsePrimary() {
  if (check(Token::INT_LIT)) {
    long v = std::stol(current->text);
    advance();
    return new IntLit(v);
  }
  if (check(Token::FLOAT_LIT)) {
    double v = std::stod(current->text);
    advance();
    return new FloatLit(v);
  }
  if (check(Token::CHAR_LIT)) {
    // interpreta escapes básicos
    std::string s = current->text;
    advance();
    long v = 0;
    if (s.size() >= 2 && s[0] == '\\') {
      switch (s[1]) {
      case 'n': v = '\n'; break;
      case 't': v = '\t'; break;
      case 'r': v = '\r'; break;
      case '0': v = '\0'; break;
      case '\\': v = '\\'; break;
      case '\'': v = '\''; break;
      default: v = s[1]; break;
      }
    } else if (!s.empty()) {
      v = (unsigned char)s[0];
    }
    return new IntLit(v);
  }
  if (check(Token::STRING_LIT)) {
    std::string s = current->text;
    advance();
    return new StringLit(s);
  }
  if (check(Token::TRUE)) { advance(); return new IntLit(1); }
  if (check(Token::FALSE)) { advance(); return new IntLit(0); }

  if (check(Token::ID)) {
    std::string name = current->text;
    advance();
    if (match(Token::LPAREN)) {
      CallExp *call = new CallExp(name);
      if (!check(Token::RPAREN)) {
        do {
          call->args.push_back(parseExpr());
        } while (match(Token::COMMA));
      }
      expect(Token::RPAREN, "')'");
      return call;
    }
    return new IdExp(name);
  }

  if (match(Token::LPAREN)) {
    // ¿Conversión explícita?  (tipo) expr
    if (isTypeStart()) {
      Type t = parseType();
      expect(Token::RPAREN, "')'");
      return new CastExp(t, parseUnary());
    }
    Exp *e = parseExpr();
    expect(Token::RPAREN, "')'");
    return e;
  }

  error("una expresión");
  return nullptr; // inalcanzable
}
