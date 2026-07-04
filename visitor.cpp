#include "visitor.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

static const char *ARG_REGS[6] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};

// =============================================================================
// TypeCheckerVisitor
// =============================================================================

void TypeCheckerVisitor::error(const std::string &msg) {
  ok = false;
  errors.push_back(msg);
  std::cerr << "Error semántico: " << msg << "\n";
}

Type TypeCheckerVisitor::checkExp(Exp *e) {
  e->accept(this);
  return e->exprType;
}

// Tamaño en bytes de un tipo (usa sizeBytes() de Type para tipos primitivos).
static int sizeOfType(const Type &t,
                      const std::unordered_map<std::string, StructLayout> &S) {
  if (t.pointer > 0) return 8;
  if (t.base == Type::STRUCT) {
    auto it = S.find(t.structName);
    return it != S.end() ? it->second.size : 0;
  }
  return t.sizeBytes();
}

// Nombre de función de la libc que aceptamos sin declaración previa.
bool isExternalName(const std::string &name) {
  static const char *ext[] = {
      "malloc", "calloc", "realloc", "printf", "puts", "putchar", "free",
      "scanf", "strlen", "strcmp", "abs", "atoi", "exit", "memset", "memcpy",
      "strcpy", "rand", "srand"};
  for (auto s : ext) if (name == s) return true;
  return false;
}

// Igual que arriba pero devuelve además el tipo de retorno asumido.
static bool isKnownExternal(const std::string &name, Type &retOut) {
  if (name == "malloc" || name == "calloc" || name == "realloc") {
    retOut = Type(Type::VOID, 1); // void*
    return true;
  }
  if (isExternalName(name)) {
    retOut = Type(Type::INT);
    return true;
  }
  return false;
}

void TypeCheckerVisitor::check(Program *program) {
  // layouts de structs (tamaño real por campo con alineación natural)
  for (StructDef *sd : program->structs) {
    StructLayout lay;
    int off = 0;
    for (auto &f : sd->fields) {
      int sz = sizeOfType(f.type, structs);
      int align = (sz > 8) ? 8 : sz; // alineación natural, máx 8
      off = (off + align - 1) & ~(align - 1);
      lay.fieldOffset[f.name] = off;
      lay.fieldType[f.name] = f.type;
      off += sz;
    }
    if (off > 0) {
      int maxAlign = 8;
      off = (off + maxAlign - 1) & ~(maxAlign - 1);
    }
    lay.size = off;
    structs[sd->name] = lay;
  }
  // firmas de funciones (permite recursión mutua)
  for (FunDef *f : program->functions) {
    FunInfo info;
    info.retType = f->retType;
    for (auto &p : f->params) info.paramTypes.push_back(p.type);
    functions[f->name] = info;
  }
  program->accept(this);
}

int TypeCheckerVisitor::visit(Program *p) {
  env.add_level(); // scope global
  for (auto g : p->globals) g->accept(this);
  for (auto f : p->functions) f->accept(this);
  env.remove_level();
  return 0;
}

int TypeCheckerVisitor::visit(FunDef *f) {
  currentFun = f->name;
  currentRet = f->retType;
  localCount = 0;
  env.add_level();
  for (auto &p : f->params) {
    env.add_var(p.name, p.type);
    localCount++;
  }
  f->body->accept(this);
  env.remove_level();
  frameSlots[f->name] = localCount;
  return 0;
}

int TypeCheckerVisitor::visit(Block *b) {
  env.add_level();
  for (auto s : b->stms) s->accept(this);
  env.remove_level();
  return 0;
}

int TypeCheckerVisitor::visit(VarDecStm *s) {
  for (auto &vi : s->vars) {
    if (vi.init) checkExp(vi.init);
    env.add_var(vi.name, vi.type); // el tipo por-variable ya lo puso el parser
    localCount++;
  }
  return 0;
}

int TypeCheckerVisitor::visit(AssignStm *s) {
  Type tt = checkExp(s->target);
  Type vt = checkExp(s->value);
  if (!s->target->isLValue())
    error("el lado izquierdo de una asignación debe ser un lvalue");
  (void)tt; (void)vt;
  return 0;
}

int TypeCheckerVisitor::visit(ExprStm *s) { checkExp(s->e); return 0; }

int TypeCheckerVisitor::visit(ReturnStm *s) {
  if (s->e) {
    Type t = checkExp(s->e);
    (void)t; // conversión al tipo de retorno se maneja en codegen
  }
  return 0;
}

int TypeCheckerVisitor::visit(IfStm *s) {
  checkExp(s->cond);
  s->thenB->accept(this);
  if (s->elseB) s->elseB->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(WhileStm *s) {
  checkExp(s->cond);
  s->body->accept(this);
  return 0;
}

int TypeCheckerVisitor::visit(DoWhileStm *s) {
  s->body->accept(this);
  checkExp(s->cond);
  return 0;
}

int TypeCheckerVisitor::visit(ForStm *s) {
  env.add_level();
  if (s->init) s->init->accept(this);
  if (s->cond) checkExp(s->cond);
  if (s->update) s->update->accept(this);
  s->body->accept(this);
  env.remove_level();
  return 0;
}

int TypeCheckerVisitor::visit(BreakStm *) { return 0; }
int TypeCheckerVisitor::visit(ContinueStm *) { return 0; }

int TypeCheckerVisitor::visit(IntLit *e) {
  e->exprType = Type(Type::INT);
  return 0;
}
int TypeCheckerVisitor::visit(FloatLit *e) {
  e->exprType = Type(Type::DOUBLE);
  return 0;
}
int TypeCheckerVisitor::visit(CastExp *e) {
  checkExp(e->operand);
  e->exprType = e->target;
  return 0;
}
int TypeCheckerVisitor::visit(StringLit *e) {
  e->exprType = Type(Type::CHAR, 1); // char*
  return 0;
}
int TypeCheckerVisitor::visit(IdExp *e) {
  Type t;
  if (env.lookup(e->name, t)) {
    e->exprType = t;
  } else if (functions.count(e->name)) {
    // Nombre de función usado como valor => puntero a función.
    Type fp = functions[e->name].retType;
    fp.isFuncPtr = true;
    fp.pointer = 1;
    e->exprType = fp;
  } else {
    error("variable no declarada: '" + e->name + "'");
    e->exprType = Type(Type::INT);
  }
  return 0;
}
int TypeCheckerVisitor::visit(UnaryExp *e) {
  Type t = checkExp(e->operand);
  e->exprType = (e->op == NOT_OP) ? Type(Type::INT) : t;
  return 0;
}
int TypeCheckerVisitor::visit(BinaryExp *e) {
  Type l = checkExp(e->left);
  Type r = checkExp(e->right);
  // aritmética de punteros: puntero +/- entero => puntero
  if ((e->op == PLUS_OP || e->op == MINUS_OP)) {
    bool lp = l.isPointer() || l.isArray;
    bool rp = r.isPointer() || r.isArray;
    if (lp && !rp) { e->exprType = l; if (e->exprType.isArray) { e->exprType.isArray=false; e->exprType.pointer++; } return 0; }
    if (rp && !lp && e->op == PLUS_OP) { e->exprType = r; if (e->exprType.isArray) { e->exprType.isArray=false; e->exprType.pointer++; } return 0; }
  }
  // promoción numérica: si alguno es double, el resultado es double
  if (l.isFloating() || r.isFloating())
    e->exprType = Type(Type::DOUBLE);
  else
    e->exprType = Type(Type::INT);
  // los relacionales/lógicos devuelven int
  if (e->op >= LT_OP)
    e->exprType = Type(Type::INT);
  return 0;
}
int TypeCheckerVisitor::visit(CallExp *e) {
  for (auto a : e->args) checkExp(a);
  Type extRet;
  if (isKnownExternal(e->name, extRet)) {
    e->exprType = extRet;
    return 0;
  }
  // Llamada indirecta: el nombre es una variable puntero-a-función.
  // El tipo de retorno se aproxima con el tipo base declarado (int/double/...).
  Type vt;
  if (env.lookup(e->name, vt) && vt.isFuncPtr) {
    e->exprType = Type(vt.base);
    return 0;
  }
  auto it = functions.find(e->name);
  if (it == functions.end()) {
    error("función no declarada: '" + e->name + "'");
    e->exprType = Type(Type::INT);
    return 0;
  }
  if (e->args.size() != it->second.paramTypes.size())
    error("la función '" + e->name + "' espera " +
          std::to_string(it->second.paramTypes.size()) + " argumentos, se dieron " +
          std::to_string(e->args.size()));
  e->exprType = it->second.retType;
  return 0;
}

int TypeCheckerVisitor::visit(AddrExp *e) {
  Type t = checkExp(e->operand);
  Type r = t;
  if (r.isArray) { r.isArray = false; r.arrayLen = 0; }
  r.pointer++;
  e->exprType = r;
  return 0;
}

int TypeCheckerVisitor::visit(DerefExp *e) {
  Type t = checkExp(e->operand);
  e->exprType = t.pointee();
  return 0;
}

int TypeCheckerVisitor::visit(IndexExp *e) {
  Type bt = checkExp(e->base);
  checkExp(e->index);
  e->exprType = bt.pointee();
  return 0;
}

int TypeCheckerVisitor::visit(FieldExp *e) {
  Type ot = checkExp(e->obj);
  std::string sname = e->arrow ? ot.pointee().structName : ot.structName;
  auto it = structs.find(sname);
  if (it == structs.end()) {
    error("acceso a campo sobre un tipo que no es struct ('" + sname + "')");
    e->exprType = Type(Type::INT);
    return 0;
  }
  auto ft = it->second.fieldType.find(e->field);
  if (ft == it->second.fieldType.end()) {
    error("el struct '" + sname + "' no tiene campo '" + e->field + "'");
    e->exprType = Type(Type::INT);
    return 0;
  }
  e->exprType = ft->second;
  return 0;
}

int TypeCheckerVisitor::visit(SizeofExp *e) {
  Type target = e->queryType;
  if (e->queryExpr) target = checkExp(e->queryExpr);
  int sz = sizeOfType(target, structs);
  e->isConstant = true;
  e->constantValue = sz;
  e->exprType = Type(Type::INT);
  return 0;
}

// =============================================================================
// GenCodeVisitor
// =============================================================================

std::string GenCodeVisitor::newLabel(const std::string &base) {
  return "." + base + std::to_string(labelCount++);
}

std::string GenCodeVisitor::internString(const std::string &text) {
  std::string label = ".LC" + std::to_string(stringPool.size());
  stringPool.push_back({label, text});
  return label;
}

void GenCodeVisitor::generate(Program *program, TypeCheckerVisitor &tc) {
  functions = tc.functions;
  frameSlots = tc.frameSlots;
  structs = tc.structs;
  program->accept(this);
}

// Tamaño en bytes de un tipo (para sizeof / reservas).
int GenCodeVisitor::structSizeOf(const Type &t) {
  if (t.pointer > 0) return 8;
  if (t.base == Type::STRUCT) {
    auto it = structs.find(t.structName);
    return it != structs.end() ? it->second.size : 0;
  }
  return t.sizeBytes();
}

// Con la dirección en %rax, carga el valor:
//   double     -> %xmm0
//   char/bool  -> %rax (con signo, 1 byte)
//   int        -> %eax (4 bytes, zero-extiende)
//   puntero/otro -> %rax (8 bytes)
void GenCodeVisitor::loadFromAddr(const Type &t) {
  if (t.isFloating())
    out << "\tmovsd (%rax), %xmm0\n";
  else if (t.pointer == 0 && (t.base == Type::CHAR || t.base == Type::BOOL))
    out << "\tmovsbq (%rax), %rax\n";
  else if (t.pointer == 0 && t.base == Type::INT)
    out << "\tmovl (%rax), %eax\n";
  else
    out << "\tmovq (%rax), %rax\n";
}

// Guarda el valor en la dirección de %rcx:
//   double     -> desde %xmm0
//   char/bool  -> %al (1 byte)
//   int        -> %eax (4 bytes)
//   puntero/otro -> %rax (8 bytes)
void GenCodeVisitor::storeToAddr(const Type &t) {
  if (t.isFloating())
    out << "\tmovsd %xmm0, (%rcx)\n";
  else if (t.pointer == 0 && (t.base == Type::CHAR || t.base == Type::BOOL))
    out << "\tmovb %al, (%rcx)\n";
  else if (t.pointer == 0 && t.base == Type::INT)
    out << "\tmovl %eax, (%rcx)\n";
  else
    out << "\tmovq %rax, (%rcx)\n";
}

// Convierte el valor recién evaluado de tipo 'from' al tipo 'to'.
// Entero vive en %rax, double en %xmm0.
void GenCodeVisitor::emitConvert(const Type &from, const Type &to) {
  bool fF = from.isFloating(), tF = to.isFloating();
  if (fF == tF) {
    if (!fF && to.base == Type::BOOL && from.base != Type::BOOL) {
      out << "\ttest %rax, %rax\n";
      out << "\tsetne %al\n";
      out << "\tmovsbq %al, %rax\n";
    }
    return;
  }
  if (!fF && tF)                    // int -> double
    out << "\tcvtsi2sdq %rax, %xmm0\n";
  else                             // double -> int (truncando, como C)
    out << "\tcvttsd2siq %xmm0, %rax\n";
}

// Evalúa 'e' y garantiza el resultado como double en %xmm0.
void GenCodeVisitor::evalAsDouble(Exp *e) {
  e->accept(this);
  if (!e->exprType.isFloating())
    out << "\tcvtsi2sdq %rax, %xmm0\n"; // promueve int -> double
}

int GenCodeVisitor::visit(Program *p) {
  // ---- Sección de datos ----
  out << "\t.data\n";
  out << "print_int_fmt: .string \"%ld\\n\"\n";
  out << "print_str_fmt: .string \"%s\\n\"\n";
  // variables globales a .data
  for (auto g : p->globals) {
    for (auto &vi : g->vars) {
      globals[vi.name] = vi.type;
      if (vi.type.isArray) {
        out << vi.name << ": .zero " << vi.type.storageSize() << "\n";
      } else if (vi.type.base == Type::STRUCT && vi.type.pointer == 0) {
        out << vi.name << ": .zero " << structSizeOf(vi.type) << "\n";
      } else if (vi.type.isFloating()) {
        double dv = 0.0;
        if (auto fl = vi.init->asFloatLit()) dv = fl->value;
        else if (vi.init && vi.init->isConstant) dv = (double)vi.init->constantValue;
        unsigned long long bits;
        __builtin_memcpy(&bits, &dv, 8);
        out << vi.name << ": .quad " << bits << "  # double " << dv << "\n";
      } else {
        long initv = 0;
        if (vi.init && vi.init->isConstant) initv = vi.init->constantValue;
        out << vi.name << ": .quad " << initv << "\n";
      }
    }
  }

  out << "\n\t.text\n";
  for (auto f : p->functions) if (!f->isInlined) f->accept(this);

  // ---- Constantes de solo-lectura en .rdata (PE/COFF) ----
  if (!stringPool.empty() || !doublePool.empty()) {
    out << "\n\t.section .rodata\n";
    for (auto &dp : doublePool)
      out << "\t.align 8\n" << dp.first << ": .quad " << dp.second << "\n";
    for (auto &sp : stringPool)
      out << sp.first << ": .string \"" << sp.second << "\"\n";
  }
  return 0;
}

// Reserva un bloque de 'bytes' (redondeado a 8) y devuelve el offset base
// (dirección más baja del bloque). 'offset' apunta al último slot asignado.
static int allocBlock(int &offset, int bytes) {
  int b = (bytes + 7) & ~7;
  int base = offset - b + 8;
  offset -= b;
  return base;
}

// Asigna un offset de frame a una variable local según su tipo.
void GenCodeVisitor::assignLocal(const VarInit &vi) {
  if (memoria.count(vi.name)) return;
  Type vt = vi.type;
  int bytes;
  if (vt.isArray)
    bytes = vt.storageSize();
  else if (vt.base == Type::STRUCT && vt.pointer == 0)
    bytes = structSizeOf(vt);
  else
    bytes = vt.sizeBytes();
  memoria[vi.name] = allocBlock(offset, bytes);
  varType[vi.name] = vt;
}

// Asigna offsets a locales recorriendo el cuerpo (incluye bloques anidados).
void GenCodeVisitor::collectLocals(Block *b) {
  for (Stm *s : b->stms) {
    if (auto vd = s->asVarDecStm()) {
      for (auto &vi : vd->vars) assignLocal(vi);
    } else if (auto ifs = s->asIfStm()) {
      collectLocals(ifs->thenB);
      if (ifs->elseB) collectLocals(ifs->elseB);
    } else if (auto ws = s->asWhileStm()) {
      collectLocals(ws->body);
    } else if (auto dw = s->asDoWhileStm()) {
      collectLocals(dw->body);
    } else if (auto fs = s->asForStm()) {
      if (auto vd = fs->init->asVarDecStm())
        for (auto &vi : vd->vars) assignLocal(vi);
      collectLocals(fs->body);
    } else if (auto blk = s->asBlock()) {
      collectLocals(blk);
    }
  }
}

int GenCodeVisitor::visit(FunDef *f) {
  currentFun = f->name;
  currentRet = f->retType;
  memoria.clear();
  varType.clear();
  epilogueLabel = ".end_" + f->name;

  // Slot -8(%rbp) reservado para guardar %rbx (callee-saved) que usamos como
  // scratch de alineación en cada call. Las variables empiezan en -16.
  offset = -16;

  out << "\n\t.globl " << f->name << "\n";
  out << f->name << ":\n";
  out << "\tpushq %rbp\n";
  out << "\tmovq %rsp, %rbp\n";

  // asignar offsets: primero params, luego locales
  for (size_t i = 0; i < f->params.size(); i++) {
    memoria[f->params[i].name] = offset;
    varType[f->params[i].name] = f->params[i].type;
    offset -= 8;
  }
  collectLocals(f->body);

  // tamaño de frame alineado a 16 (rbp queda 16-alineado tras el prólogo)
  int frameSize = -offset - 8;
  if (frameSize < 0) frameSize = 0;
  int aligned = (frameSize + 15) & ~15;
  out << "\tsubq $" << aligned << ", %rsp\n";

  // preservar %rbx del llamador (lo usamos como scratch de alineación)
  out << "\tmovq %rbx, -8(%rbp)\n";

  // Guardar params entrantes en sus slots. SysV x86-64:
  // RDI=0, RSI=1, RDX=2, RCX=3, R8=4, R9=5; XMM0‑XMM7 para floats.
  static const char *XMM_ARG[8] = {"%xmm0", "%xmm1", "%xmm2", "%xmm3",
                                    "%xmm4", "%xmm5", "%xmm6", "%xmm7"};
  for (size_t i = 0; i < f->params.size() && i < 6; i++) {
    int slot = memoria[f->params[i].name];
    if (f->params[i].type.isFloating())
      out << "\tmovsd " << XMM_ARG[i] << ", " << slot << "(%rbp)\n";
    else
      out << "\tmovq " << ARG_REGS[i] << ", " << slot << "(%rbp)\n";
  }

  f->body->accept(this);

  // epílogo (return implícito con 0)
  out << "\tmovq $0, %rax\n";
  out << epilogueLabel << ":\n";
  out << "\tmovq -8(%rbp), %rbx\n"; // restaurar %rbx del llamador
  out << "\tleave\n";
  out << "\tret\n";
  return 0;
}

int GenCodeVisitor::visit(Block *b) {
  for (auto s : b->stms) s->accept(this);
  return 0;
}

int GenCodeVisitor::visit(VarDecStm *s) {
  for (auto &vi : s->vars) {
    // Arreglos y structs en línea: la variable ES el bloque (no hay init aquí).
    if (vi.type.isArray || (vi.type.base == Type::STRUCT && vi.type.pointer == 0))
      continue;
    if (vi.init) {
      vi.init->accept(this);                    // en %rax o %xmm0
      emitConvert(vi.init->exprType, vi.type);  // promoción/conversión implícita
      out << "\tleaq " << memoria[vi.name] << "(%rbp), %rcx\n"; // no toca rax/xmm0
      storeToAddr(vi.type);
    }
  }
  return 0;
}

// Deja la DIRECCIÓN del lvalue en %rax
void GenCodeVisitor::genAddress(Exp *lvalue) {
  if (auto id = lvalue->asId()) {
    if (memoria.count(id->name))
      out << "\tleaq " << memoria[id->name] << "(%rbp), %rax\n";
    else if (globals.count(id->name))
      out << "\tleaq " << id->name << "(%rip), %rax\n";
    else
      throw std::runtime_error("variable no encontrada en codegen: " + id->name);
    return;
  }
  // *p  =>  la dirección es el valor de p
  if (auto de = lvalue->asDeref()) {
    de->operand->accept(this); // valor del puntero en %rax
    return;
  }
  // base[index]  =>  dir = base + index*elemSize
  if (auto ix = lvalue->asIndex()) {
    ix->base->accept(this);            // base (dirección/puntero) en %rax
    out << "\tpushq %rax\n";
    ix->index->accept(this);           // índice en %rax
    int es = ix->base->exprType.elemSize();
    if (es != 1) out << "\timulq $" << es << ", %rax\n";
    out << "\tmovq %rax, %rcx\n";
    out << "\tpopq %rax\n";
    out << "\taddq %rcx, %rax\n";      // dir = base + index*es
    return;
  }
  // obj.field  |  ptr->field
  if (auto fe = lvalue->asField()) {
    std::string sname;
    if (fe->arrow) {
      fe->obj->accept(this);           // valor del puntero en %rax
      sname = fe->obj->exprType.pointee().structName;
    } else {
      genAddress(fe->obj);             // dirección del struct en %rax
      sname = fe->obj->exprType.structName;
    }
    int off = 0;
    auto it = structs.find(sname);
    if (it != structs.end() && it->second.fieldOffset.count(fe->field))
      off = it->second.fieldOffset[fe->field];
    if (off != 0) out << "\taddq $" << off << ", %rax\n";
    return;
  }
  throw std::runtime_error("lvalue no soportado");
}

int GenCodeVisitor::visit(AssignStm *s) {
  Type tt = s->target->exprType;

  if (s->op == ASSIGN_EQ) {
    s->value->accept(this);
    emitConvert(s->value->exprType, tt); // valor final en %rax o %xmm0 según tt
  } else if (tt.isFloating()) {
    // x op= v  con x double
    evalAsDouble(s->target);
    out << "\tsubq $8, %rsp\n\tmovsd %xmm0, (%rsp)\n";
    evalAsDouble(s->value);
    out << "\tmovsd %xmm0, %xmm1\n";
    out << "\tmovsd (%rsp), %xmm0\n\taddq $8, %rsp\n";
    switch (s->op) {
    case ASSIGN_ADD: out << "\taddsd %xmm1, %xmm0\n"; break;
    case ASSIGN_SUB: out << "\tsubsd %xmm1, %xmm0\n"; break;
    case ASSIGN_MUL: out << "\tmulsd %xmm1, %xmm0\n"; break;
    case ASSIGN_DIV: out << "\tdivsd %xmm1, %xmm0\n"; break;
    default: break;
    }
  } else {
    // x op= v  con x entero
    s->target->accept(this);
    out << "\tpushq %rax\n";
    s->value->accept(this);
    emitConvert(s->value->exprType, tt);
    out << "\tmovq %rax, %rcx\n";
    out << "\tpopq %rax\n";
    switch (s->op) {
    case ASSIGN_ADD: out << "\taddq %rcx, %rax\n"; break;
    case ASSIGN_SUB: out << "\tsubq %rcx, %rax\n"; break;
    case ASSIGN_MUL: out << "\timulq %rcx, %rax\n"; break;
    case ASSIGN_DIV: out << "\tcqto\n\tidivq %rcx\n"; break;
    default: break;
    }
  }

  // Guardar el resultado (en %rax o %xmm0) en la dirección del target.
  if (tt.isFloating()) {
    genAddress(s->target);            // %rax = dir (no toca %xmm0)
    out << "\tmovq %rax, %rcx\n";
    storeToAddr(tt);
  } else {
    out << "\tpushq %rax\n";
    genAddress(s->target);
    out << "\tmovq %rax, %rcx\n";
    out << "\tpopq %rax\n";
    storeToAddr(tt);
  }
  return 0;
}

int GenCodeVisitor::visit(ExprStm *s) { s->e->accept(this); return 0; }

int GenCodeVisitor::visit(ReturnStm *s) {
  if (s->e) {
    s->e->accept(this);
    emitConvert(s->e->exprType, currentRet); // convierte al tipo de retorno
  } else {
    out << "\tmovq $0, %rax\n";
  }
  out << "\tjmp " << epilogueLabel << "\n";
  return 0;
}

int GenCodeVisitor::visit(IfStm *s) {
  std::string elseL = newLabel("else");
  std::string endL = newLabel("endif");
  s->cond->accept(this);
  out << "\tcmpq $0, %rax\n";
  out << "\tje " << (s->elseB ? elseL : endL) << "\n";
  s->thenB->accept(this);
  if (s->elseB) {
    out << "\tjmp " << endL << "\n";
    out << elseL << ":\n";
    s->elseB->accept(this);
  }
  out << endL << ":\n";
  return 0;
}

int GenCodeVisitor::visit(WhileStm *s) {
  std::string startL = newLabel("while");
  std::string endL = newLabel("endwhile");
  breakLabels.push_back(endL);
  continueLabels.push_back(startL);
  out << startL << ":\n";
  s->cond->accept(this);
  out << "\tcmpq $0, %rax\n";
  out << "\tje " << endL << "\n";
  s->body->accept(this);
  out << "\tjmp " << startL << "\n";
  out << endL << ":\n";
  breakLabels.pop_back();
  continueLabels.pop_back();
  return 0;
}

int GenCodeVisitor::visit(DoWhileStm *s) {
  std::string startL = newLabel("do");
  std::string endL = newLabel("enddo");
  breakLabels.push_back(endL);
  continueLabels.push_back(startL);
  out << startL << ":\n";
  s->body->accept(this);
  s->cond->accept(this);
  out << "\tcmpq $0, %rax\n";
  out << "\tjne " << startL << "\n";
  out << endL << ":\n";
  breakLabels.pop_back();
  continueLabels.pop_back();
  return 0;
}

int GenCodeVisitor::visit(ForStm *s) {
  std::string startL = newLabel("for");
  std::string updL = newLabel("forupd");
  std::string endL = newLabel("endfor");
  if (s->init) s->init->accept(this);
  breakLabels.push_back(endL);
  continueLabels.push_back(updL);
  out << startL << ":\n";
  if (s->cond) {
    s->cond->accept(this);
    out << "\tcmpq $0, %rax\n";
    out << "\tje " << endL << "\n";
  }
  s->body->accept(this);
  out << updL << ":\n";
  if (s->update) s->update->accept(this);
  out << "\tjmp " << startL << "\n";
  out << endL << ":\n";
  breakLabels.pop_back();
  continueLabels.pop_back();
  return 0;
}

int GenCodeVisitor::visit(BreakStm *) {
  if (!breakLabels.empty())
    out << "\tjmp " << breakLabels.back() << "\n";
  return 0;
}
int GenCodeVisitor::visit(ContinueStm *) {
  if (!continueLabels.empty())
    out << "\tjmp " << continueLabels.back() << "\n";
  return 0;
}

int GenCodeVisitor::visit(IntLit *e) {
  out << "\tmovq $" << e->value << ", %rax\n";
  return 0;
}

int GenCodeVisitor::visit(FloatLit *e) {
  // Materializa el double desde una constante en .rdata (bits IEEE-754).
  unsigned long long bits;
  double v = e->value;
  static_assert(sizeof(bits) == sizeof(v), "double debe ser 64 bits");
  __builtin_memcpy(&bits, &v, sizeof(bits));
  std::string label = ".LD" + std::to_string(doublePool.size());
  doublePool.push_back({label, bits});
  out << "\tmovsd " << label << "(%rip), %xmm0\n";
  return 0;
}

int GenCodeVisitor::visit(StringLit *e) {
  std::string label = internString(e->value);
  out << "\tleaq " << label << "(%rip), %rax\n";
  return 0;
}

int GenCodeVisitor::visit(CastExp *e) {
  Type from = e->operand->exprType;
  e->operand->accept(this);
  if (e->target.isFloating() && !from.isFloating()) {
    out << "\tcvtsi2sdq %rax, %xmm0\n";
  } else if (!e->target.isFloating() && from.isFloating()) {
    out << "\tcvttsd2siq %xmm0, %rax\n";
  }
  if (!e->target.isFloating() && e->target.base == Type::BOOL && from.base != Type::BOOL) {
    out << "\ttest %rax, %rax\n";
    out << "\tsetne %al\n";
    out << "\tmovsbq %al, %rax\n";
  }
  return 0;
}

int GenCodeVisitor::visit(IdExp *e) {
  // Nombre de función usado como valor => su dirección (para punteros a función).
  if (functions.count(e->name) && !memoria.count(e->name) &&
      !globals.count(e->name)) {
    out << "\tleaq " << e->name << "(%rip), %rax\n";
    return 0;
  }
  // Determinar el tipo de la variable (local, param o global).
  Type t;
  bool known = false;
  if (varType.count(e->name)) { t = varType[e->name]; known = true; }
  else if (globals.count(e->name)) { t = globals[e->name]; known = true; }

  // Arreglos y structs "decaen" a su dirección base (semántica de C).
  if (known && (t.isArray || (t.base == Type::STRUCT && t.pointer == 0))) {
    genAddress(e);
    return 0;
  }
  // Escalares: cargar el valor con el ancho/registro correcto.
  std::string loc;
  if (memoria.count(e->name)) loc = std::to_string(memoria[e->name]) + "(%rbp)";
  else if (globals.count(e->name)) loc = e->name + "(%rip)";
  else throw std::runtime_error("variable no encontrada en codegen: " + e->name);

  if (t.isFloating())
    out << "\tmovsd " << loc << ", %xmm0\n";
  else if (t.pointer == 0 && (t.base == Type::CHAR || t.base == Type::BOOL))
    out << "\tmovsbq " << loc << ", %rax\n";
  else if (t.pointer == 0 && t.base == Type::INT)
    out << "\tmovl " << loc << ", %eax\n";
  else
    out << "\tmovq " << loc << ", %rax\n";
  return 0;
}

int GenCodeVisitor::visit(AddrExp *e) {
  genAddress(e->operand); // &lvalue: la dirección es el valor
  return 0;
}

int GenCodeVisitor::visit(DerefExp *e) {
  genAddress(e);              // deja la dirección (valor del puntero) en %rax
  loadFromAddr(e->exprType);  // carga el valor apuntado
  return 0;
}

int GenCodeVisitor::visit(IndexExp *e) {
  genAddress(e);
  // Si el resultado es a su vez un arreglo (fila de una matriz 2D), decae a su
  // dirección: no se carga ningún valor. Igual para structs por valor.
  if (e->exprType.isArray ||
      (e->exprType.base == Type::STRUCT && e->exprType.pointer == 0))
    return 0;
  loadFromAddr(e->exprType);
  return 0;
}

int GenCodeVisitor::visit(FieldExp *e) {
  genAddress(e);
  loadFromAddr(e->exprType);
  return 0;
}

int GenCodeVisitor::visit(SizeofExp *e) {
  out << "\tmovq $" << e->constantValue << ", %rax\n";
  return 0;
}

int GenCodeVisitor::visit(UnaryExp *e) {
  e->operand->accept(this);
  if (e->op == NEG_OP) {
    if (e->operand->exprType.isFloating()) {
      // negar un double: restar de 0.0  (0.0 - x)
      out << "\tmovsd %xmm0, %xmm1\n";
      out << "\txorpd %xmm0, %xmm0\n";
      out << "\tsubsd %xmm1, %xmm0\n";
    } else {
      out << "\tnegq %rax\n";
    }
  } else { // NOT_OP (siempre entero)
    out << "\tcmpq $0, %rax\n";
    out << "\tsete %al\n";
    out << "\tmovzbq %al, %rax\n";
  }
  return 0;
}

int GenCodeVisitor::visit(BinaryExp *e) {
  // cortocircuito para && y ||
  if (e->op == AND_OP) {
    std::string falseL = newLabel("andfalse");
    std::string endL = newLabel("andend");
    e->left->accept(this);
    out << "\tcmpq $0, %rax\n\tje " << falseL << "\n";
    e->right->accept(this);
    out << "\tcmpq $0, %rax\n\tje " << falseL << "\n";
    out << "\tmovq $1, %rax\n\tjmp " << endL << "\n";
    out << falseL << ":\n\tmovq $0, %rax\n";
    out << endL << ":\n";
    return 0;
  }
  if (e->op == OR_OP) {
    std::string trueL = newLabel("ortrue");
    std::string endL = newLabel("orend");
    e->left->accept(this);
    out << "\tcmpq $0, %rax\n\tjne " << trueL << "\n";
    e->right->accept(this);
    out << "\tcmpq $0, %rax\n\tjne " << trueL << "\n";
    out << "\tmovq $0, %rax\n\tjmp " << endL << "\n";
    out << trueL << ":\n\tmovq $1, %rax\n";
    out << endL << ":\n";
    return 0;
  }

  // ---- Operaciones en punto flotante (double / SSE) ----
  bool floatOp = e->left->exprType.isFloating() || e->right->exprType.isFloating();
  if (floatOp && e->op != AND_OP && e->op != OR_OP) {
    // evalúa left como double, guarda en pila; right como double en %xmm1
    evalAsDouble(e->left);
    out << "\tsubq $8, %rsp\n\tmovsd %xmm0, (%rsp)\n"; // push xmm0
    evalAsDouble(e->right);
    out << "\tmovsd %xmm0, %xmm1\n";                  // right en xmm1
    out << "\tmovsd (%rsp), %xmm0\n\taddq $8, %rsp\n"; // pop -> left en xmm0
    switch (e->op) {
    case PLUS_OP:  out << "\taddsd %xmm1, %xmm0\n"; break;
    case MINUS_OP: out << "\tsubsd %xmm1, %xmm0\n"; break;
    case MUL_OP:   out << "\tmulsd %xmm1, %xmm0\n"; break;
    case DIV_OP:   out << "\tdivsd %xmm1, %xmm0\n"; break;
    case LT_OP: case GT_OP: case LE_OP:
    case GE_OP: case EQ_OP: case NE_OP: {
      // ucomisd fija CF/ZF/PF; usamos set* sin signo. Resultado int en %rax.
      const char *set = "sete";
      // Para < y <= comparamos al revés (xmm0 ? xmm1) con seta/setae.
      switch (e->op) {
      case GT_OP: out << "\tucomisd %xmm1, %xmm0\n"; set = "seta"; break;
      case GE_OP: out << "\tucomisd %xmm1, %xmm0\n"; set = "setae"; break;
      case LT_OP: out << "\tucomisd %xmm0, %xmm1\n"; set = "seta"; break;
      case LE_OP: out << "\tucomisd %xmm0, %xmm1\n"; set = "setae"; break;
      case EQ_OP: out << "\tucomisd %xmm1, %xmm0\n"; set = "sete"; break;
      case NE_OP: out << "\tucomisd %xmm1, %xmm0\n"; set = "setne"; break;
      default: break;
      }
      out << "\t" << set << " %al\n\tmovzbq %al, %rax\n";
      break;
    }
    default: break;
    }
    return 0;
  }

  // aritmética de punteros: escala el operando entero por el tamaño del elemento
  if ((e->op == PLUS_OP || e->op == MINUS_OP) && e->exprType.isPointer()) {
    int es = e->exprType.elemSize();
    bool leftIsPtr = e->left->exprType.isPointer() || e->left->exprType.isArray;
    if (leftIsPtr) {
      e->left->accept(this);
      out << "\tpushq %rax\n";
      e->right->accept(this);
      if (es != 1) out << "\timulq $" << es << ", %rax\n";
      out << "\tmovq %rax, %rcx\n";
      out << "\tpopq %rax\n";
      out << (e->op == PLUS_OP ? "\taddq %rcx, %rax\n" : "\tsubq %rcx, %rax\n");
    } else {
      // entero + puntero
      e->left->accept(this);
      if (es != 1) out << "\timulq $" << es << ", %rax\n";
      out << "\tpushq %rax\n";
      e->right->accept(this);
      out << "\tmovq %rax, %rcx\n";
      out << "\tpopq %rax\n";
      out << "\taddq %rcx, %rax\n";
    }
    return 0;
  }

  // ---- Enteros (default) ----
  // Orden Sethi-Ullmann: evaluar primero el subárbol con label mayor
  if (e->left->label >= e->right->label) {
    e->left->accept(this);
    out << "\tpushq %rax\n";
    e->right->accept(this);
    out << "\tmovq %rax, %rcx\n";
    out << "\tpopq %rax\n";
  } else {
    e->right->accept(this);
    out << "\tpushq %rax\n";
    e->left->accept(this);
    out << "\tmovq %rax, %rcx\n";
    out << "\tpopq %rax\n";
    out << "\txchgq %rax, %rcx\n";
  }

  switch (e->op) {
  case PLUS_OP: out << "\taddq %rcx, %rax\n"; break;
  case MINUS_OP: out << "\tsubq %rcx, %rax\n"; break;
  case MUL_OP: out << "\timulq %rcx, %rax\n"; break;
  case DIV_OP: out << "\tcqto\n\tidivq %rcx\n"; break;
  case MOD_OP: out << "\tcqto\n\tidivq %rcx\n\tmovq %rdx, %rax\n"; break;
  case LT_OP: case GT_OP: case LE_OP:
  case GE_OP: case EQ_OP: case NE_OP: {
    out << "\tcmpq %rcx, %rax\n";
    const char *set = "sete";
    switch (e->op) {
    case LT_OP: set = "setl"; break;
    case GT_OP: set = "setg"; break;
    case LE_OP: set = "setle"; break;
    case GE_OP: set = "setge"; break;
    case EQ_OP: set = "sete"; break;
    case NE_OP: set = "setne"; break;
    default: break;
    }
    out << "\t" << set << " %al\n";
    out << "\tmovzbq %al, %rax\n";
    break;
  }
  default: break;
  }
  return 0;
}

int GenCodeVisitor::visit(CallExp *e) {
  // SysV x86-64: hasta 6 enteros (RDI,RSI,RDX,RCX,R8,R9) y 8 XMM (XMM0‑XMM7)

  // ---- print builtin: print(x) imprime un entero con salto de línea ----
  if (e->name == "print" && e->args.size() == 1) {
    e->args[0]->accept(this);
    out << "\tmovq %rax, %rsi\n";                  // 2º arg
    out << "\tleaq print_int_fmt(%rip), %rdi\n";   // 1er arg (formato)
    out << "\tmovq %rsp, %rbx\n";
    out << "\tandq $-16, %rsp\n";
    out << "\tcall printf\n";
    out << "\tmovq %rbx, %rsp\n";
    return 0;
  }

  int n = (int)e->args.size();
  bool variadic = (e->name == "printf" || e->name == "scanf");

  // ¿Llamada indirecta? El nombre es una variable (puntero a función), no una
  // función/externa conocida. Cargamos el puntero y lo apilamos primero.
  Type extRetTmp;
  bool isDirect = functions.count(e->name) ||
                  (e->name == "print") ||
                  (e->name == "printf") || (e->name == "scanf") ||
                  isExternalName(e->name);
  bool indirect = !isDirect &&
                  (memoria.count(e->name) || globals.count(e->name));
  if (indirect) {
    IdExp tmp(e->name);
    visit(&tmp);                 // valor del puntero en %rax
    out << "\tpushq %rax\n";     // guárdalo bajo los args
  }

  // ¿El parámetro i es double? (para funciones propias con firma conocida)
  auto paramIsDouble = [&](int i) -> bool {
    auto it = functions.find(e->name);
    if (it != functions.end() && i < (int)it->second.paramTypes.size())
      return it->second.paramTypes[i].isFloating();
    return false;
  };

  // 1) Evalúa cada argumento y guarda su valor en la pila (8 bytes c/u).
  //    Enteros/punteros desde %rax; doubles desde %xmm0.
  for (int i = 0; i < n; i++) {
    e->args[i]->accept(this);
    // conversión implícita a double si el parámetro formal es double
    if (!variadic && paramIsDouble(i))
      emitConvert(e->args[i]->exprType, Type(Type::DOUBLE));
    if (e->args[i]->exprType.isFloating() || (!variadic && paramIsDouble(i)))
      out << "\tsubq $8, %rsp\n\tmovsd %xmm0, (%rsp)\n";
    else
      out << "\tpushq %rax\n";
  }

  // Contar cuántos args SSE hay para %al y los XMM registers.
  int sseCount = 0;
  for (int i = 0; i < n; i++)
    if (e->args[i]->exprType.isFloating() || (!variadic && paramIsDouble(i)))
      sseCount++;

  // 2) Precompute GP register index for each argument.
  //    SysV x86-64: floats use XMM registers and do NOT consume a GP slot.
  //    Integers advance GP independently: RDI=0 → RSI=1 → RDX=2 → RCX=3 → R8=4 → R9=5.
  int gpForArg[32];
  int gpIdx = 0;
  for (int i = 0; i < n && i < 32; i++) {
    bool isF = e->args[i]->exprType.isFloating() || (!variadic && paramIsDouble(i));
    if (isF) gpForArg[i] = -1;
    else gpForArg[i] = gpIdx++;
  }
  // El tope de pila tiene el último arg; recorremos de n-1 a 0.
  static const char *XMM[8] = {"%xmm0","%xmm1","%xmm2","%xmm3",
                                "%xmm4","%xmm5","%xmm6","%xmm7"};
  int xmmIdx = sseCount - 1;
  int stackArgs = 0;
  for (int i = n - 1; i >= 0; i--) {
    bool isF = e->args[i]->exprType.isFloating() || (!variadic && paramIsDouble(i));
    int g = gpForArg[i];  // -1 for float, 0-5 for GP register index
    if (g >= 0 && g < 6) {
      // integer arg that fits in a GP register
      out << "\tpopq " << ARG_REGS[g] << "\n";
    } else if (isF && xmmIdx >= 0) {
      // float arg → XMM register (and NO GP save area for variadic)
      out << "\tmovsd (%rsp), " << XMM[xmmIdx] << "\n\taddq $8, %rsp\n";
      xmmIdx--;
    } else {
      // stack arg (beyond 6 GP regs) → pop into scratch register
      if (stackArgs == 0)
        out << "\tpopq %r10\n";
      else if (stackArgs == 1)
        out << "\tpopq %r11\n";
      else
        out << "\tpopq %rax\n";
      stackArgs++;
    }
  }

  if (indirect) out << "\tpopq %r10\n";

  out << "\tmovq %rsp, %rbx\n";
  out << "\tandq $-16, %rsp\n";
  // keep RSP 16-byte aligned after pushq of stack args (SysV: no shadow space)
  // odd count → pre-adjust 8 so total offset stays multiple of 16
  if ((stackArgs & 1) != 0)
    out << "\tsubq $8, %rsp\n";

  // push back stack args in correct order after alignment
  if (stackArgs >= 1)
    out << "\tpushq %r10\n";   // r10 = last stack arg (highest index)
  if (stackArgs >= 2)
    out << "\tpushq %r11\n";   // r11 = second-to-last stack arg

  if (variadic) out << "\tmovb $" << sseCount << ", %al\n";
  if (indirect) out << "\tcall *%r10\n";
  else out << "\tcall " << e->name << "\n";
  out << "\tmovq %rbx, %rsp\n";
  return 0;
}

// =============================================================================
// InlineVisitor — inline de funciones pequeñas
// =============================================================================

bool InlineVisitor::isInlineable(FunDef *fd) {
  if (!fd || !fd->body) return false;
  // Debe tener exactamente una sentencia (un return)
  if (fd->body->stms.size() != 1) return false;
  ReturnStm *ret = fd->body->stms[0]->asReturn();
  if (!ret || !ret->e) return false;
  return true;
}

Exp *InlineVisitor::cloneReturn(FunDef *fd) {
  for (auto s : fd->body->stms) {
    if (auto r = s->asReturn())
      return r->e ? r->e->clone() : nullptr;
  }
  return nullptr;
}

void InlineVisitor::substituteParameters(Exp *&exp, FunDef *fd, CallExp *call) {
  if (!exp) return;
  if (auto bin = exp->asBinary()) {
    substituteParameters(bin->left, fd, call);
    substituteParameters(bin->right, fd, call);
    return;
  }
  if (auto un = exp->asUnary()) {
    substituteParameters(un->operand, fd, call);
    return;
  }
  if (auto fc = exp->asCall()) {
    for (auto &arg : fc->args) substituteParameters(arg, fd, call);
    return;
  }
  if (auto cast = exp->asCast()) {
    substituteParameters(cast->operand, fd, call);
    return;
  }
  if (auto addr = exp->asAddr()) {
    substituteParameters(addr->operand, fd, call);
    return;
  }
  if (auto der = exp->asDeref()) {
    substituteParameters(der->operand, fd, call);
    return;
  }
  if (auto idx = exp->asIndex()) {
    substituteParameters(idx->base, fd, call);
    substituteParameters(idx->index, fd, call);
    return;
  }
  if (auto fld = exp->asField()) {
    substituteParameters(fld->obj, fd, call);
    return;
  }
  if (auto id = exp->asId()) {
    for (size_t i = 0; i < fd->params.size(); i++) {
      if (id->name == fd->params[i].name) {
        delete exp;
        exp = call->args[i]->clone();
        return;
      }
    }
  }
}

void InlineVisitor::inlineExp(Exp *&exp) {
  if (!exp) return;
  if (auto bin = exp->asBinary()) {
    inlineExp(bin->left);
    inlineExp(bin->right);
    return;
  }
  if (auto un = exp->asUnary()) {
    inlineExp(un->operand);
    return;
  }
  auto fc = exp->asCall();
  if (!fc) return;
  for (auto &arg : fc->args) inlineExp(arg);
  auto it = inlineFunctions.find(fc->name);
  if (it == inlineFunctions.end()) return;
  Exp *newTree = cloneReturn(it->second);
  if (!newTree) return;
  substituteParameters(newTree, it->second, fc);
  inlineExp(newTree);
  inlineCount++;
  delete exp;
  exp = newTree;
}

int InlineVisitor::Inline(Program *program) {
  inlineFunctions.clear();
  for (auto fd : program->functions) {
    if (isInlineable(fd) && fd->name != "main") {
      inlineFunctions[fd->name] = fd;
      fd->isInlined = true;
    }
  }
  program->accept(this);
  return 0;
}

int InlineVisitor::visit(IntLit *) { return 0; }
int InlineVisitor::visit(FloatLit *) { return 0; }
int InlineVisitor::visit(StringLit *) { return 0; }
int InlineVisitor::visit(CastExp *e) { inlineExp(e->operand); return 0; }
int InlineVisitor::visit(IdExp *) { return 0; }
int InlineVisitor::visit(UnaryExp *e) { inlineExp(e->operand); return 0; }
int InlineVisitor::visit(BinaryExp *e) { inlineExp(e->left); inlineExp(e->right); return 0; }
int InlineVisitor::visit(CallExp *) { return 0; }
int InlineVisitor::visit(AddrExp *e) { inlineExp(e->operand); return 0; }
int InlineVisitor::visit(DerefExp *e) { inlineExp(e->operand); return 0; }
int InlineVisitor::visit(IndexExp *e) { inlineExp(e->base); inlineExp(e->index); return 0; }
int InlineVisitor::visit(FieldExp *e) { inlineExp(e->obj); return 0; }
int InlineVisitor::visit(SizeofExp *) { return 0; }
int InlineVisitor::visit(VarDecStm *s) { for (auto &vi : s->vars) if (vi.init) inlineExp(vi.init); return 0; }
int InlineVisitor::visit(AssignStm *s) { inlineExp(s->target); inlineExp(s->value); return 0; }
int InlineVisitor::visit(ExprStm *s) { inlineExp(s->e); return 0; }
int InlineVisitor::visit(ReturnStm *s) { if (s->e) inlineExp(s->e); return 0; }
int InlineVisitor::visit(IfStm *s) {
  inlineExp(s->cond);
  if (s->thenB) s->thenB->accept(this);
  if (s->elseB) s->elseB->accept(this);
  return 0;
}
int InlineVisitor::visit(WhileStm *s) {
  inlineExp(s->cond);
  if (s->body) s->body->accept(this);
  return 0;
}
int InlineVisitor::visit(DoWhileStm *s) {
  if (s->body) s->body->accept(this);
  inlineExp(s->cond);
  return 0;
}
int InlineVisitor::visit(ForStm *s) {
  if (s->init) s->init->accept(this);
  if (s->cond) inlineExp(s->cond);
  if (s->update) s->update->accept(this);
  if (s->body) s->body->accept(this);
  return 0;
}
int InlineVisitor::visit(BreakStm *) { return 0; }
int InlineVisitor::visit(ContinueStm *) { return 0; }
int InlineVisitor::visit(Block *b) { for (auto s : b->stms) s->accept(this); return 0; }
int InlineVisitor::visit(FunDef *f) { if (f->body) f->body->accept(this); return 0; }
int InlineVisitor::visit(Program *p) {
  for (auto f : p->functions) if (!f->isInlined) f->accept(this);
  return 0;
}

// =============================================================================
// FoldVisitor — plegado de constantes + algebraicas + reducción de fuerza
// =============================================================================

static bool isIntConst(Exp *e, long &v) {
  IntLit *n = e->asIntLit();
  if (n) { v = n->value; return true; }
  return false;
}

static void foldExp(Exp *&e, int &foldCount, int &algebraCount, int &strengthCount) {
  if (!e) return;

  // BinaryExp
  BinaryExp *be = e->asBinary();
  if (be) {
    foldExp(be->left, foldCount, algebraCount, strengthCount);
    foldExp(be->right, foldCount, algebraCount, strengthCount);
    long lv, rv;
    bool lc = isIntConst(be->left, lv);
    bool rc = isIntConst(be->right, rv);

    // (1) Constant folding
    if (lc && rc) {
      long r = 0; bool ok = true;
      switch (be->op) {
      case PLUS_OP: r = lv + rv; break;
      case MINUS_OP: r = lv - rv; break;
      case MUL_OP: r = lv * rv; break;
      case DIV_OP: if (rv == 0) { ok = false; break; } r = lv / rv; break;
      case MOD_OP: if (rv == 0) { ok = false; break; } r = lv % rv; break;
      case LT_OP: r = lv < rv; break;
      case GT_OP: r = lv > rv; break;
      case LE_OP: r = lv <= rv; break;
      case GE_OP: r = lv >= rv; break;
      case EQ_OP: r = lv == rv; break;
      case NE_OP: r = lv != rv; break;
      case AND_OP: r = (lv && rv); break;
      case OR_OP: r = (lv || rv); break;
      default: ok = false; break;
      }
      if (ok) { delete e; e = new IntLit(r); foldCount++; return; }
    }

    // (2) Algebraic identities
    if (rc && (be->op == PLUS_OP || be->op == MINUS_OP) && rv == 0) {
      Exp *keep = be->left; be->left = nullptr; delete be; e = keep; algebraCount++; return;
    }
    if (lc && be->op == PLUS_OP && lv == 0) {
      Exp *keep = be->right; be->right = nullptr; delete be; e = keep; algebraCount++; return;
    }
    if (rc && be->op == MUL_OP && rv == 1) {
      Exp *keep = be->left; be->left = nullptr; delete be; e = keep; algebraCount++; return;
    }
    if (lc && be->op == MUL_OP && lv == 1) {
      Exp *keep = be->right; be->right = nullptr; delete be; e = keep; algebraCount++; return;
    }
    if (rc && be->op == DIV_OP && rv == 1) {
      Exp *keep = be->left; be->left = nullptr; delete be; e = keep; algebraCount++; return;
    }
    // x*0 -> 0, 0*x -> 0
    if (rc && be->op == MUL_OP && rv == 0) {
      delete be->left; delete be->right; be->left = be->right = nullptr;
      delete e; e = new IntLit(0); algebraCount++; return;
    }
    if (lc && be->op == MUL_OP && lv == 0) {
      delete be->left; delete be->right; be->left = be->right = nullptr;
      delete e; e = new IntLit(0); algebraCount++; return;
    }

    // (3) Strength reduction: x*2 -> x+x
    if (be->op == MUL_OP && rc && rv == 2) {
      IdExp *id = be->left->asId();
      if (id) {
        Exp *a = new IdExp(id->name), *b = new IdExp(id->name);
        delete e; e = new BinaryExp(a, PLUS_OP, b); strengthCount++; return;
      }
    }
    if (be->op == MUL_OP && lc && lv == 2) {
      IdExp *id = be->right->asId();
      if (id) {
        Exp *a = new IdExp(id->name), *b = new IdExp(id->name);
        delete e; e = new BinaryExp(a, PLUS_OP, b); strengthCount++; return;
      }
    }
    return;
  }

  // UnaryExp
  UnaryExp *ue = e->asUnary();
  if (ue) {
    foldExp(ue->operand, foldCount, algebraCount, strengthCount);
    long v;
    if (isIntConst(ue->operand, v)) {
      long r = (ue->op == NEG_OP) ? -v : !v;
      delete e; e = new IntLit(r); foldCount++; return;
    }
    return;
  }

  // Other expression types — just recurse into children
  if (auto ce = e->asCast()) { foldExp(ce->operand, foldCount, algebraCount, strengthCount); return; }
  if (auto ae = e->asAddr()) { foldExp(ae->operand, foldCount, algebraCount, strengthCount); return; }
  if (auto de = e->asDeref()) { foldExp(de->operand, foldCount, algebraCount, strengthCount); return; }
  if (auto ix = e->asIndex()) { foldExp(ix->base, foldCount, algebraCount, strengthCount); foldExp(ix->index, foldCount, algebraCount, strengthCount); return; }
  if (auto fe = e->asField()) { foldExp(fe->obj, foldCount, algebraCount, strengthCount); return; }
  if (auto call = e->asCall()) {
    for (auto &a : call->args) foldExp(a, foldCount, algebraCount, strengthCount);
    return;
  }
}

int FoldVisitor::Fold(Program *p) {
  for (auto g : p->globals)
    for (auto &vi : g->vars)
      if (vi.init) foldExp(vi.init, foldCount, algebraCount, strengthCount);
  p->accept(this);
  return 0;
}

int FoldVisitor::visit(IntLit *) { return 0; }
int FoldVisitor::visit(FloatLit *) { return 0; }
int FoldVisitor::visit(StringLit *) { return 0; }
int FoldVisitor::visit(CastExp *) { return 0; }
int FoldVisitor::visit(IdExp *) { return 0; }
int FoldVisitor::visit(UnaryExp *) { return 0; }
int FoldVisitor::visit(BinaryExp *) { return 0; }
int FoldVisitor::visit(CallExp *) { return 0; }
int FoldVisitor::visit(AddrExp *) { return 0; }
int FoldVisitor::visit(DerefExp *) { return 0; }
int FoldVisitor::visit(IndexExp *) { return 0; }
int FoldVisitor::visit(FieldExp *) { return 0; }
int FoldVisitor::visit(SizeofExp *) { return 0; }
int FoldVisitor::visit(VarDecStm *s) {
  for (auto &vi : s->vars) if (vi.init) foldExp(vi.init, foldCount, algebraCount, strengthCount);
  return 0;
}
int FoldVisitor::visit(AssignStm *s) { foldExp(s->target, foldCount, algebraCount, strengthCount); foldExp(s->value, foldCount, algebraCount, strengthCount); return 0; }
int FoldVisitor::visit(ExprStm *s) { foldExp(s->e, foldCount, algebraCount, strengthCount); return 0; }
int FoldVisitor::visit(ReturnStm *s) { if (s->e) foldExp(s->e, foldCount, algebraCount, strengthCount); return 0; }
int FoldVisitor::visit(IfStm *s) {
  foldExp(s->cond, foldCount, algebraCount, strengthCount);
  if (s->thenB) s->thenB->accept(this);
  if (s->elseB) s->elseB->accept(this);
  return 0;
}
int FoldVisitor::visit(WhileStm *s) {
  foldExp(s->cond, foldCount, algebraCount, strengthCount);
  if (s->body) s->body->accept(this);
  return 0;
}
int FoldVisitor::visit(DoWhileStm *s) {
  if (s->body) s->body->accept(this);
  foldExp(s->cond, foldCount, algebraCount, strengthCount);
  return 0;
}
int FoldVisitor::visit(ForStm *s) {
  if (s->init) s->init->accept(this);
  if (s->cond) foldExp(s->cond, foldCount, algebraCount, strengthCount);
  if (s->update) s->update->accept(this);
  if (s->body) s->body->accept(this);
  return 0;
}
int FoldVisitor::visit(BreakStm *) { return 0; }
int FoldVisitor::visit(ContinueStm *) { return 0; }
int FoldVisitor::visit(Block *b) { for (auto s : b->stms) s->accept(this); return 0; }
int FoldVisitor::visit(FunDef *f) { if (f->body) f->body->accept(this); return 0; }

int FoldVisitor::visit(Program *p) {
  for (auto f : p->functions) if (!f->isInlined) f->accept(this);
  return 0;
}

// =============================================================================
// SethiVisitor — etiquetado Sethi-Ullman
// =============================================================================

int SethiVisitor::Sethi(Program *p) {
  p->accept(this);
  return 0;
}

int SethiVisitor::visit(IntLit *e) { e->ishoja = true; e->label = 1; return 0; }
int SethiVisitor::visit(FloatLit *e) { e->ishoja = true; e->label = 1; return 0; }
int SethiVisitor::visit(StringLit *e) { e->label = 1; return 0; }
int SethiVisitor::visit(IdExp *e) { e->ishoja = true; e->label = 1; return 0; }
int SethiVisitor::visit(UnaryExp *e) { e->operand->accept(this); e->label = e->operand->label; return 0; }
int SethiVisitor::visit(BinaryExp *e) {
  e->left->accept(this);
  e->right->accept(this);
  if (e->left->label == e->right->label)
    e->label = e->left->label + 1;
  else
    e->label = std::max(e->left->label, e->right->label);
  return 0;
}
int SethiVisitor::visit(CallExp *e) {
  int mx = 0;
  for (auto a : e->args) { a->accept(this); if (a->label > mx) mx = a->label; }
  e->label = mx;
  return 0;
}
int SethiVisitor::visit(CastExp *e) { e->operand->accept(this); e->label = e->operand->label; return 0; }
int SethiVisitor::visit(AddrExp *e) { e->operand->accept(this); e->label = e->operand->label; return 0; }
int SethiVisitor::visit(DerefExp *e) { e->operand->accept(this); e->label = e->operand->label; return 0; }
int SethiVisitor::visit(IndexExp *e) {
  e->base->accept(this);
  e->index->accept(this);
  e->label = std::max(e->base->label, e->index->label);
  return 0;
}
int SethiVisitor::visit(FieldExp *e) { e->obj->accept(this); e->label = e->obj->label; return 0; }
int SethiVisitor::visit(SizeofExp *) { return 0; }
int SethiVisitor::visit(VarDecStm *s) { for (auto &vi : s->vars) if (vi.init) vi.init->accept(this); return 0; }
int SethiVisitor::visit(AssignStm *s) { s->target->accept(this); s->value->accept(this); return 0; }
int SethiVisitor::visit(ExprStm *s) { s->e->accept(this); return 0; }
int SethiVisitor::visit(ReturnStm *s) { if (s->e) s->e->accept(this); return 0; }
int SethiVisitor::visit(IfStm *s) {
  s->cond->accept(this);
  if (s->thenB) s->thenB->accept(this);
  if (s->elseB) s->elseB->accept(this);
  return 0;
}
int SethiVisitor::visit(WhileStm *s) {
  s->cond->accept(this);
  if (s->body) s->body->accept(this);
  return 0;
}
int SethiVisitor::visit(DoWhileStm *s) {
  if (s->body) s->body->accept(this);
  s->cond->accept(this);
  return 0;
}
int SethiVisitor::visit(ForStm *s) {
  if (s->init) s->init->accept(this);
  if (s->cond) s->cond->accept(this);
  if (s->update) s->update->accept(this);
  if (s->body) s->body->accept(this);
  return 0;
}
int SethiVisitor::visit(BreakStm *) { return 0; }
int SethiVisitor::visit(ContinueStm *) { return 0; }
int SethiVisitor::visit(Block *b) { for (auto s : b->stms) s->accept(this); return 0; }
int SethiVisitor::visit(FunDef *f) { if (f->body) f->body->accept(this); return 0; }
int SethiVisitor::visit(Program *p) {
  for (auto f : p->functions) if (!f->isInlined) f->accept(this);
  return 0;
}
