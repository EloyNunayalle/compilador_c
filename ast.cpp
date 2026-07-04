// =============================================================================
// ast.cpp — Implementación de accept() (patrón Visitor) y utilidades del AST
// =============================================================================

#include "ast.h"
#include "visitor.h"

std::string Exp::binopToStr(BinaryOp op) {
  switch (op) {
  case PLUS_OP: return "+";
  case MINUS_OP: return "-";
  case MUL_OP: return "*";
  case DIV_OP: return "/";
  case MOD_OP: return "%";
  case LT_OP: return "<";
  case GT_OP: return ">";
  case LE_OP: return "<=";
  case GE_OP: return ">=";
  case EQ_OP: return "==";
  case NE_OP: return "!=";
  case AND_OP: return "&&";
  case OR_OP: return "||";
  }
  return "?";
}

// ---- Expresiones ----
int IntLit::accept(Visitor *v) { return v->visit(this); }
int FloatLit::accept(Visitor *v) { return v->visit(this); }
int StringLit::accept(Visitor *v) { return v->visit(this); }
int CastExp::accept(Visitor *v) { return v->visit(this); }
int IdExp::accept(Visitor *v) { return v->visit(this); }
int UnaryExp::accept(Visitor *v) { return v->visit(this); }
int BinaryExp::accept(Visitor *v) { return v->visit(this); }
int CallExp::accept(Visitor *v) { return v->visit(this); }
int AddrExp::accept(Visitor *v) { return v->visit(this); }
int DerefExp::accept(Visitor *v) { return v->visit(this); }
int IndexExp::accept(Visitor *v) { return v->visit(this); }
int FieldExp::accept(Visitor *v) { return v->visit(this); }
int SizeofExp::accept(Visitor *v) { return v->visit(this); }

// ---- Sentencias ----
int VarDecStm::accept(Visitor *v) { return v->visit(this); }
int AssignStm::accept(Visitor *v) { return v->visit(this); }
int ExprStm::accept(Visitor *v) { return v->visit(this); }
int ReturnStm::accept(Visitor *v) { return v->visit(this); }
int IfStm::accept(Visitor *v) { return v->visit(this); }
int WhileStm::accept(Visitor *v) { return v->visit(this); }
int DoWhileStm::accept(Visitor *v) { return v->visit(this); }
int ForStm::accept(Visitor *v) { return v->visit(this); }
int BreakStm::accept(Visitor *v) { return v->visit(this); }
int ContinueStm::accept(Visitor *v) { return v->visit(this); }
int Block::accept(Visitor *v) { return v->visit(this); }

// ---- Estructura ----
int FunDef::accept(Visitor *v) { return v->visit(this); }
int Program::accept(Visitor *v) { return v->visit(this); }
