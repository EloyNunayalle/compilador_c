#ifndef VISITOR_H
#define VISITOR_H

// =============================================================================
// visitor.h — Visitantes del AST de MiniC
// =============================================================================
//   TypeCheckerVisitor — análisis semántico + verificación de tipos:
//     · variables declaradas, funciones existentes, aridad de llamadas
//     · verificación/propagación de tipos, promoción int->double
//     · cálculo de layout de structs (tamaño y offset por campo)
//     · cuenta el tamaño del frame de cada función
//
//   Optimizer — plegado de constantes, identidades algebraicas, reducción
//               de fuerza (se ejecuta entre el parser y el typechecker).
//
//   GenCodeVisitor — generación de x86-64 (AT&T, System V):
//     · emite ensamblador ensamblable con gcc
// =============================================================================

#include "ast.h"
#include "environment.h"
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations de todos los nodos visitables
class IntLit;
class FloatLit;
class StringLit;
class CastExp;
class IdExp;
class UnaryExp;
class BinaryExp;
class CallExp;
class AddrExp;
class DerefExp;
class IndexExp;
class FieldExp;
class SizeofExp;
class VarDecStm;
class AssignStm;
class ExprStm;
class ReturnStm;
class IfStm;
class WhileStm;
class DoWhileStm;
class ForStm;
class BreakStm;
class ContinueStm;
class Block;
class FunDef;
class Program;

// =============================================================================
// Visitor base
// =============================================================================

class Visitor {
public:
  virtual int visit(IntLit *e) = 0;
  virtual int visit(FloatLit *e) = 0;
  virtual int visit(StringLit *e) = 0;
  virtual int visit(CastExp *e) = 0;
  virtual int visit(IdExp *e) = 0;
  virtual int visit(UnaryExp *e) = 0;
  virtual int visit(BinaryExp *e) = 0;
  virtual int visit(CallExp *e) = 0;
  virtual int visit(AddrExp *e) = 0;
  virtual int visit(DerefExp *e) = 0;
  virtual int visit(IndexExp *e) = 0;
  virtual int visit(FieldExp *e) = 0;
  virtual int visit(SizeofExp *e) = 0;
  virtual int visit(VarDecStm *s) = 0;
  virtual int visit(AssignStm *s) = 0;
  virtual int visit(ExprStm *s) = 0;
  virtual int visit(ReturnStm *s) = 0;
  virtual int visit(IfStm *s) = 0;
  virtual int visit(WhileStm *s) = 0;
  virtual int visit(DoWhileStm *s) = 0;
  virtual int visit(ForStm *s) = 0;
  virtual int visit(BreakStm *s) = 0;
  virtual int visit(ContinueStm *s) = 0;
  virtual int visit(Block *b) = 0;
  virtual int visit(FunDef *f) = 0;
  virtual int visit(Program *p) = 0;
  virtual ~Visitor() {}
};

// =============================================================================
// Información de funciones (compartida entre fases)
// =============================================================================

struct FunInfo {
  Type retType;
  std::vector<Type> paramTypes;
};

// =============================================================================
// Información de layout de un struct
// =============================================================================

struct StructLayout {
  int size = 0;                                     // tamaño total (bytes)
  std::unordered_map<std::string, int> fieldOffset; // campo -> offset
  std::unordered_map<std::string, Type> fieldType;  // campo -> tipo
};

// =============================================================================
// TypeCheckerVisitor
// =============================================================================

class TypeCheckerVisitor : public Visitor {
public:
  // Entorno de tipos por variable, con scopes anidados
  Environment<Type> env;

  // Firma de cada función
  std::unordered_map<std::string, FunInfo> functions;

  // Layout de cada struct declarado
  std::unordered_map<std::string, StructLayout> structs;

  // función -> número de slots locales (params + locales) para el frame
  std::unordered_map<std::string, int> frameSlots;

  std::string currentFun;
  Type currentRet;
  int localCount = 0; // acumulador durante el análisis de una función

  bool ok = true; // false si hubo error semántico
  std::vector<std::string> errors; // mensajes de error

  void check(Program *program);
  void error(const std::string &msg);

  int visit(IntLit *e) override;
  int visit(FloatLit *e) override;
  int visit(StringLit *e) override;
  int visit(CastExp *e) override;
  int visit(IdExp *e) override;
  int visit(UnaryExp *e) override;
  int visit(BinaryExp *e) override;
  int visit(CallExp *e) override;
  int visit(AddrExp *e) override;
  int visit(DerefExp *e) override;
  int visit(IndexExp *e) override;
  int visit(FieldExp *e) override;
  int visit(SizeofExp *e) override;
  int visit(VarDecStm *s) override;
  int visit(AssignStm *s) override;
  int visit(ExprStm *s) override;
  int visit(ReturnStm *s) override;
  int visit(IfStm *s) override;
  int visit(WhileStm *s) override;
  int visit(DoWhileStm *s) override;
  int visit(ForStm *s) override;
  int visit(BreakStm *s) override;
  int visit(ContinueStm *s) override;
  int visit(Block *b) override;
  int visit(FunDef *f) override;
  int visit(Program *p) override;

private:
  Type checkExp(Exp *e); // visita y devuelve el tipo calculado
};

// =============================================================================
// Optimizer — optimización sobre el AST (independiente del codegen)
//   · Plegado de constantes (constant folding): 2+3 -> 5, 4*5-1 -> 19
//   · Identidades algebraicas: x+0, x*1, x*0, x-0, x/1
//   · Reducción de fuerza: x*2 -> x+x
// Se activa con -O1 (por defecto). Con -O0 se omite para comparar.
// =============================================================================

class Optimizer {
public:
  int foldCount = 0;      // nº de plegados de constantes realizados
  int algebraCount = 0;   // nº de simplificaciones algebraicas
  int strengthCount = 0;  // nº de reducciones de fuerza

  void run(Program *p);

private:
  void fold(Exp *&e);       // reescribe 'e' en su lugar
  void optStm(Stm *s);
  void optBlock(Block *b);
};

// =============================================================================
// GenCodeVisitor — x86-64 AT&T / System V
// =============================================================================

class GenCodeVisitor : public Visitor {
public:
  std::ostream &out;

  // metadatos del TypeChecker
  std::unordered_map<std::string, FunInfo> functions;
  std::unordered_map<std::string, int> frameSlots;
  std::unordered_map<std::string, StructLayout> structs;

  // variable local -> offset desde %rbp
  std::unordered_map<std::string, int> memoria;
  // variable local -> tipo (para elegir instrucciones int/double/puntero)
  std::unordered_map<std::string, Type> varType;
  // variables globales declaradas en .data/.bss
  std::unordered_map<std::string, Type> globals;

  int offset = -8;      // próximo slot libre en el frame
  int labelCount = 0;   // etiquetas únicas
  std::string currentFun;
  Type currentRet;      // tipo de retorno de la función actual
  std::string epilogueLabel;

  // pila de etiquetas para break/continue
  std::vector<std::string> breakLabels;
  std::vector<std::string> continueLabels;

  // literales de cadena a emitir en .rdata
  std::vector<std::pair<std::string, std::string>> stringPool; // (label, texto)
  // literales double a emitir en .rdata (label, bits IEEE-754 de 64 bits)
  std::vector<std::pair<std::string, unsigned long long>> doublePool;

  bool usesPrintf = false;

  GenCodeVisitor(std::ostream &out) : out(out) {}

  void generate(Program *program, TypeCheckerVisitor &tc);

  std::string newLabel(const std::string &base);
  std::string internString(const std::string &text);

  int visit(IntLit *e) override;
  int visit(FloatLit *e) override;
  int visit(StringLit *e) override;
  int visit(CastExp *e) override;
  int visit(IdExp *e) override;
  int visit(UnaryExp *e) override;
  int visit(BinaryExp *e) override;
  int visit(CallExp *e) override;
  int visit(AddrExp *e) override;
  int visit(DerefExp *e) override;
  int visit(IndexExp *e) override;
  int visit(FieldExp *e) override;
  int visit(SizeofExp *e) override;
  int visit(VarDecStm *s) override;
  int visit(AssignStm *s) override;
  int visit(ExprStm *s) override;
  int visit(ReturnStm *s) override;
  int visit(IfStm *s) override;
  int visit(WhileStm *s) override;
  int visit(DoWhileStm *s) override;
  int visit(ForStm *s) override;
  int visit(BreakStm *s) override;
  int visit(ContinueStm *s) override;
  int visit(Block *b) override;
  int visit(FunDef *f) override;
  int visit(Program *p) override;

private:
  // Deja en %rax la DIRECCIÓN del lvalue (para asignaciones / &)
  void genAddress(Exp *lvalue);
  void collectLocals(Block *b); // recorre y asigna offsets a VarDecStm
  void assignLocal(const VarInit &vi); // asigna offset a una variable local
  int structSizeOf(const Type &t); // tamaño en bytes de un tipo (para sizeof)
  // Con la dirección en %rax, carga el valor: entero->%rax, double->%xmm0
  void loadFromAddr(const Type &t);
  // Guarda el valor: entero en %rax / double en %xmm0; dirección en %rcx
  void storeToAddr(const Type &t);
  // Convierte el valor recién evaluado de 'from' a 'to' (int<->double)
  void emitConvert(const Type &from, const Type &to);
  // Evalúa 'e' y deja el resultado como double en %xmm0 (promoviendo si es int)
  void evalAsDouble(Exp *e);
};

#endif // VISITOR_H
