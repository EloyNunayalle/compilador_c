#ifndef AST_H
#define AST_H

// =============================================================================
// ast.h — Árbol de Sintaxis Abstracta de MiniC
// =============================================================================
// Jerarquía:
//
//   Exp (abstracta)
//     ├── IntLit      — literal entero (también char/bool tras plegado)
//     ├── FloatLit    — literal double
//     ├── StringLit   — literal de cadena
//     ├── CastExp     — (tipo) expr
//     ├── IdExp       — variable (lvalue)
//     ├── UnaryExp    — -x, !x
//     ├── BinaryExp   — a op b  (+, -, *, /, %, <, >, <=, >=, ==, !=, &&, ||)
//     ├── CallExp     — f(args)   (incluye printf builtin)
//     ├── AddrExp     — &lvalue
//     ├── DerefExp    — *ptr  (lvalue)
//     ├── IndexExp    — base[index]  (lvalue)
//     ├── FieldExp    — obj.field / ptr->field  (lvalue)
//     └── SizeofExp   — sizeof(tipo) / sizeof(expr)
//
//   Stm (abstracta)
//     ├── VarDecStm   — int a = 1, b;  (locales y globales)
//     ├── AssignStm   — lvalue (=|+=|-=|*=|/=) exp
//     ├── ExprStm     — exp ;
//     ├── ReturnStm   — return exp? ;
//     ├── IfStm       — if (c) {..} else {..}
//     ├── WhileStm    — while (c) {..}
//     ├── DoWhileStm  — do {..} while (c);
//     ├── ForStm      — for (init; c; upd) {..}
//     ├── BreakStm / ContinueStm
//     └── Block       — { Stm* }   (define un scope)
//
//   StructDef, Param, FunDef, Program
// =============================================================================

#include <string>
#include <vector>

class Visitor;
class BinaryExp;
class UnaryExp;
class IntLit;
class FloatLit;
class IdExp;
class CallExp;
class CastExp;
class AddrExp;
class DerefExp;
class IndexExp;
class FieldExp;
class ReturnStm;
class VarDecStm;
class IfStm;
class WhileStm;
class DoWhileStm;
class ForStm;
class Block;

// =============================================================================
// Sistema de tipos
// =============================================================================

struct Type {
  enum Base { INT, DOUBLE, CHAR, VOID, BOOL, STRUCT, UNKNOWN };
  Base base = UNKNOWN;
  int pointer = 0;         // niveles de puntero (int** => pointer==2)
  std::string structName;  // válido cuando base==STRUCT
  bool isArray = false;    // arreglo en línea (int a[N] o int m[R][C])
  int arrayLen = 0;        // filas (dimensión externa) si isArray
  int cols = 0;            // columnas (2ª dimensión) si es matriz 2D; 0 = 1D
  bool isFuncPtr = false;  // puntero a función (para lambdas/callbacks estilo C)

  Type() = default;
  Type(Base b, int p = 0) : base(b), pointer(p) {}

  // Bytes del escalar base de ESTE tipo (válido cuando pointer==0).
  int baseElemSize() const {
    if (pointer == 0 && base == CHAR) return 1;
    if (pointer == 0 && base == BOOL) return 1;
    if (pointer == 0 && base == INT) return 4;
    return 8;
  }

  // Tipo resultante de indexar UNA vez este arreglo/puntero.
  Type pointee() const {
    Type t = *this;
    if (t.cols > 0) {              // matriz 2D -> fila (arreglo 1D de 'cols')
      t.isArray = true; t.arrayLen = t.cols; t.cols = 0;
      return t;
    }
    if (t.isArray) { t.isArray = false; t.arrayLen = 0; }
    else if (t.pointer > 0) t.pointer--;
    return t;
  }

  // Bytes que ocupa este tipo como VALOR almacenado.
  //   char        -> 1
  //   bool        -> 1
  //   int         -> 4
  //   puntero     -> 8
  //   double      -> 8
  //   int[N]      -> N*4  ;  char[N] -> N   ;  int[R][C] -> R*C*4
  int storageSize() const {
    if (isArray) {
      long cells = (cols > 0) ? (long)arrayLen * cols : arrayLen;
      return (int)(cells * baseElemSize());
    }
    if (pointer > 0) return 8;
    return sizeBytes();
  }

  // Bytes que avanza el puntero/arreglo al sumar 1 al índice (stride).
  // = tamaño del elemento apuntado (el pointee).
  //   char*    -> 1     int*  -> 8     int[R][C] -> C*8 (una fila)
  int elemSize() const { return pointee().storageSize(); }

  bool isPointer() const { return pointer > 0; }
  bool isFloating() const { return pointer == 0 && base == DOUBLE; }
  bool isInteger() const {
    return pointer == 0 && (base == INT || base == CHAR || base == BOOL);
  }
  bool isVoid() const { return pointer == 0 && base == VOID; }

  // Tamaño lógico del tipo según C (int=4, char=1, bool=1, double=8, puntero=8).
  int sizeBytes() const {
    if (pointer > 0) return 8;
    switch (base) {
    case CHAR: return 1;
    case BOOL: return 1;
    case INT: return 4;
    case DOUBLE: return 8;
    default: return 8;
    }
  }

  std::string toString() const {
    std::string s;
    switch (base) {
    case INT: s = "int"; break;
    case DOUBLE: s = "double"; break;
    case CHAR: s = "char"; break;
    case VOID: s = "void"; break;
    case BOOL: s = "bool"; break;
    case STRUCT: s = "struct " + structName; break;
    default: s = "?"; break;
    }
    for (int i = 0; i < pointer; i++) s += "*";
    return s;
  }
};

// =============================================================================
// Operadores
// =============================================================================

enum BinaryOp {
  PLUS_OP, MINUS_OP, MUL_OP, DIV_OP, MOD_OP,
  LT_OP, GT_OP, LE_OP, GE_OP, EQ_OP, NE_OP,
  AND_OP, OR_OP
};

enum UnaryOp { NEG_OP, NOT_OP };

enum AssignOp { ASSIGN_EQ, ASSIGN_ADD, ASSIGN_SUB, ASSIGN_MUL, ASSIGN_DIV };

// =============================================================================
// Expresiones
// =============================================================================

class Exp {
public:
  Type exprType;              // rellenado por el TypeChecker
  bool isConstant = false;    // usado por el plegado de constantes
  long constantValue = 0;
  int label = 0;              // etiqueta Sethi-Ullman
  bool ishoja = false;        // usado por Sethi-Ullman

  virtual int accept(Visitor *v) = 0;
  virtual bool isLValue() const { return false; }
  virtual ~Exp() {}
  static std::string binopToStr(BinaryOp op);

  // helpers para evitar dynamic_cast
  virtual BinaryExp *asBinary() { return nullptr; }
  virtual UnaryExp *asUnary() { return nullptr; }
  virtual IntLit *asIntLit() { return nullptr; }
  virtual FloatLit *asFloatLit() { return nullptr; }
  virtual IdExp *asId() { return nullptr; }
  virtual CallExp *asCall() { return nullptr; }
  virtual CastExp *asCast() { return nullptr; }
  virtual AddrExp *asAddr() { return nullptr; }
  virtual DerefExp *asDeref() { return nullptr; }
  virtual IndexExp *asIndex() { return nullptr; }
  virtual FieldExp *asField() { return nullptr; }
  virtual Exp *clone() const { return nullptr; }
};

class IntLit : public Exp {
public:
  long value;
  IntLit(long v) : value(v) {}
  IntLit *asIntLit() override { return this; }
  int accept(Visitor *v) override;
  Exp *clone() const override { auto *c = new IntLit(value); c->exprType = exprType; return c; }
};

// ---- Literal de punto flotante (double) ----
class FloatLit : public Exp {
public:
  double value;
  FloatLit(double v) : value(v) {}
  FloatLit *asFloatLit() override { return this; }
  int accept(Visitor *v) override;
  Exp *clone() const override { auto *c = new FloatLit(value); c->exprType = exprType; return c; }
};

class StringLit : public Exp {
public:
  std::string value; // contenido con escapes tal cual (\n, \t, ...)
  int labelId = -1;  // índice de etiqueta .LC asignado en codegen
  StringLit(const std::string &v) : value(v) {}
  int accept(Visitor *v) override;
  Exp *clone() const override { auto *c = new StringLit(value); c->exprType = exprType; return c; }
};

class IdExp : public Exp {
public:
  std::string name;
  IdExp(const std::string &n) : name(n) {}
  bool isLValue() const override { return true; }
  IdExp *asId() override { return this; }
  int accept(Visitor *v) override;
  Exp *clone() const override { auto *c = new IdExp(name); c->exprType = exprType; return c; }
};

class UnaryExp : public Exp {
public:
  UnaryOp op;
  Exp *operand;
  UnaryExp(UnaryOp op, Exp *e) : op(op), operand(e) {}
  UnaryExp *asUnary() override { return this; }
  int accept(Visitor *v) override;
  Exp *clone() const override { auto *c = new UnaryExp(op, operand->clone()); c->exprType = exprType; return c; }
  ~UnaryExp() { delete operand; }
};

class BinaryExp : public Exp {
public:
  BinaryOp op;
  Exp *left;
  Exp *right;
  BinaryExp(Exp *l, BinaryOp op, Exp *r) : op(op), left(l), right(r) {}
  BinaryExp *asBinary() override { return this; }
  int accept(Visitor *v) override;
  Exp *clone() const override { auto *c = new BinaryExp(left->clone(), op, right->clone()); c->exprType = exprType; return c; }
  ~BinaryExp() { delete left; delete right; }
};

class CallExp : public Exp {
public:
  std::string name;
  std::vector<Exp *> args;
  CallExp(const std::string &n) : name(n) {}
  CallExp *asCall() override { return this; }
  int accept(Visitor *v) override;
  Exp *clone() const override {
    auto *c = new CallExp(name);
    for (auto a : args) c->args.push_back(a->clone());
    c->exprType = exprType;
    return c;
  }
  ~CallExp() { for (auto a : args) delete a; }
};

// ---- &lvalue (dirección) ----
class AddrExp : public Exp {
public:
  Exp *operand;
  AddrExp(Exp *e) : operand(e) {}
  AddrExp *asAddr() override { return this; }
  int accept(Visitor *v) override;
  Exp *clone() const override { auto *c = new AddrExp(operand->clone()); c->exprType = exprType; return c; }
  ~AddrExp() { delete operand; }
};

// ---- *ptr (desreferencia; es lvalue) ----
class DerefExp : public Exp {
public:
  Exp *operand;
  DerefExp(Exp *e) : operand(e) {}
  bool isLValue() const override { return true; }
  DerefExp *asDeref() override { return this; }
  int accept(Visitor *v) override;
  Exp *clone() const override { auto *c = new DerefExp(operand->clone()); c->exprType = exprType; return c; }
  ~DerefExp() { delete operand; }
};

// ---- base[index] (es lvalue) ----
class IndexExp : public Exp {
public:
  Exp *base;
  Exp *index;
  IndexExp(Exp *b, Exp *i) : base(b), index(i) {}
  bool isLValue() const override { return true; }
  IndexExp *asIndex() override { return this; }
  int accept(Visitor *v) override;
  Exp *clone() const override { auto *c = new IndexExp(base->clone(), index->clone()); c->exprType = exprType; return c; }
  ~IndexExp() { delete base; delete index; }
};

// ---- obj.field / ptr->field (es lvalue) ----
class FieldExp : public Exp {
public:
  Exp *obj;
  std::string field;
  bool arrow; // true => ptr->field ; false => obj.field
  FieldExp(Exp *o, const std::string &f, bool arrow)
      : obj(o), field(f), arrow(arrow) {}
  bool isLValue() const override { return true; }
  FieldExp *asField() override { return this; }
  int accept(Visitor *v) override;
  Exp *clone() const override { auto *c = new FieldExp(obj->clone(), field, arrow); c->exprType = exprType; return c; }
  ~FieldExp() { delete obj; }
};

// ---- (tipo) expr  — conversión explícita (cast) ----
class CastExp : public Exp {
public:
  Type target;
  Exp *operand;
  CastExp(Type t, Exp *e) : target(t), operand(e) {}
  CastExp *asCast() override { return this; }
  int accept(Visitor *v) override;
  Exp *clone() const override { auto *c = new CastExp(target, operand->clone()); c->exprType = exprType; return c; }
  ~CastExp() { delete operand; }
};

// ---- sizeof(type) o sizeof(expr) ----
class SizeofExp : public Exp {
public:
  Type queryType;         // usado cuando queryExpr==nullptr
  Exp *queryExpr = nullptr; // usado para sizeof(expr)
  SizeofExp(Type t) : queryType(t) {}
  SizeofExp(Exp *e) : queryExpr(e) {}
  int accept(Visitor *v) override;
  ~SizeofExp() { delete queryExpr; }
};

// =============================================================================
// Sentencias
// =============================================================================

class Stm {
public:
  virtual int accept(Visitor *v) = 0;
  virtual VarDecStm *asVarDecStm() { return nullptr; }
  virtual ReturnStm *asReturn() { return nullptr; }
  virtual IfStm *asIfStm() { return nullptr; }
  virtual WhileStm *asWhileStm() { return nullptr; }
  virtual DoWhileStm *asDoWhileStm() { return nullptr; }
  virtual ForStm *asForStm() { return nullptr; }
  virtual Block *asBlock() { return nullptr; }
  virtual ~Stm() {}
};

// Una declaración local: tipo + varios (nombre, init opcional)
struct VarInit {
  std::string name;
  Type type;           // tipo específico de esta variable (incluye punteros)
  Exp *init = nullptr; // puede ser nullptr
  int arraySize = 0;   // 0 = no arreglo; >0 = arreglo de tamaño fijo
};

class VarDecStm : public Stm {
public:
  Type type;
  std::vector<VarInit> vars;
  VarDecStm *asVarDecStm() override { return this; }
  int accept(Visitor *v) override;
  ~VarDecStm() { for (auto &vi : vars) delete vi.init; }
};

class AssignStm : public Stm {
public:
  Exp *target; // lvalue
  AssignOp op;
  Exp *value;
  AssignStm(Exp *t, AssignOp op, Exp *v) : target(t), op(op), value(v) {}
  int accept(Visitor *v) override;
  ~AssignStm() { delete target; delete value; }
};

class ExprStm : public Stm {
public:
  Exp *e;
  ExprStm(Exp *e) : e(e) {}
  int accept(Visitor *v) override;
  ~ExprStm() { delete e; }
};

class ReturnStm : public Stm {
public:
  Exp *e; // puede ser nullptr (return;)
  ReturnStm(Exp *e) : e(e) {}
  ReturnStm *asReturn() override { return this; }
  int accept(Visitor *v) override;
  ~ReturnStm() { delete e; }
};

class IfStm : public Stm {
public:
  Exp *cond;
  Block *thenB;
  Block *elseB; // puede ser nullptr
  IfStm(Exp *c, Block *t, Block *e) : cond(c), thenB(t), elseB(e) {}
  IfStm *asIfStm() override { return this; }
  int accept(Visitor *v) override;
  ~IfStm();
};

class WhileStm : public Stm {
public:
  Exp *cond;
  Block *body;
  WhileStm(Exp *c, Block *b) : cond(c), body(b) {}
  WhileStm *asWhileStm() override { return this; }
  int accept(Visitor *v) override;
  ~WhileStm();
};

class DoWhileStm : public Stm {
public:
  Block *body;
  Exp *cond;
  DoWhileStm(Block *b, Exp *c) : body(b), cond(c) {}
  DoWhileStm *asDoWhileStm() override { return this; }
  int accept(Visitor *v) override;
  ~DoWhileStm();
};

class ForStm : public Stm {
public:
  Stm *init;   // puede ser nullptr
  Exp *cond;   // puede ser nullptr
  Stm *update; // puede ser nullptr
  Block *body;
  ForStm(Stm *i, Exp *c, Stm *u, Block *b)
      : init(i), cond(c), update(u), body(b) {}
  ForStm *asForStm() override { return this; }
  int accept(Visitor *v) override;
  ~ForStm();
};

class BreakStm : public Stm {
public:
  int accept(Visitor *v) override;
};

class ContinueStm : public Stm {
public:
  int accept(Visitor *v) override;
};

class Block : public Stm {
public:
  std::vector<Stm *> stms;
  Block *asBlock() override { return this; }
  int accept(Visitor *v) override;
  ~Block() { for (auto s : stms) delete s; }
};

inline IfStm::~IfStm() { delete cond; delete thenB; delete elseB; }
inline WhileStm::~WhileStm() { delete cond; delete body; }
inline DoWhileStm::~DoWhileStm() { delete body; delete cond; }
inline ForStm::~ForStm() { delete init; delete cond; delete update; delete body; }

// =============================================================================
// Estructura del programa
// =============================================================================

struct Param {
  Type type;
  std::string name;
};

struct StructField {
  Type type;
  std::string name;
};

// ---- struct Nombre { campos... }; ----
class StructDef {
public:
  std::string name;
  std::vector<StructField> fields;
};

class FunDef {
public:
  Type retType;
  std::string name;
  std::vector<Param> params;
  Block *body = nullptr;
  bool isInlined = false;
  int accept(Visitor *v);
  ~FunDef() { delete body; }
};

class Program {
public:
  std::vector<StructDef *> structs;
  std::vector<VarDecStm *> globals;
  std::vector<FunDef *> functions;
  int accept(Visitor *v);
  ~Program() {
    for (auto s : structs) delete s;
    for (auto g : globals) delete g;
    for (auto f : functions) delete f;
  }
};

#endif // AST_H
