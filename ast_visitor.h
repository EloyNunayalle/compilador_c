#ifndef AST_VISITOR_H
#define AST_VISITOR_H

// =============================================================================
// ast_visitor.h — Visitor para generar JSON del AST completo
// =============================================================================

#include "ast.h"
#include "visitor.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

using json = nlohmann::json;

class ASTJsonVisitor : public Visitor {
public:
  json ast;

  // Expresiones
  int visit(IntLit *e) override {
    if (!e) return 0;
    ast = json{{"type", "IntLit"}, {"value", e->value}};
    return 0;
  }

  int visit(FloatLit *e) override {
    if (!e) return 0;
    ast = json{{"type", "FloatLit"}, {"value", e->value}};
    return 0;
  }

  int visit(StringLit *e) override {
    if (!e) return 0;
    ast = json{{"type", "StringLit"}, {"value", e->value}};
    return 0;
  }

  int visit(CastExp *e) override {
    if (!e) return 0;
    ast = json{
        {"type", "CastExp"},
        {"targetType", typeToJson(e->target)},
        {"expr", expToJson(e->operand)}};
    return 0;
  }

  int visit(IdExp *e) override {
    if (!e) return 0;
    ast = json{{"type", "IdExp"}, {"name", e->name}};
    return 0;
  }

  int visit(UnaryExp *e) override {
    if (!e) return 0;
    std::string opStr = unaryOpToString(e->op);
    ast = json{{"type", "UnaryExp"},
               {"op", opStr},
                               {"expr", expToJson(e->operand)}};
    return 0;
  }

  int visit(BinaryExp *e) override {
    if (!e) return 0;
    std::string opStr = binaryOpToString(e->op);
    ast = json{{"type", "BinaryExp"},
               {"op", opStr},
               {"left", expToJson(e->left)},
               {"right", expToJson(e->right)}};
    return 0;
  }

  int visit(CallExp *e) override {
    if (!e) return 0;
    json args = json::array();
    for (auto arg : e->args) {
      args.push_back(expToJson(arg));
    }
    ast = json{{"type", "CallExp"}, {"function", e->name}, {"args", args}};
    return 0;
  }

  int visit(AddrExp *e) override {
    if (!e) return 0;
    ast = json{{"type", "AddrExp"}, {"expr", expToJson(e->operand)}};
    return 0;
  }

  int visit(DerefExp *e) override {
    if (!e) return 0;
    ast = json{{"type", "DerefExp"}, {"expr", expToJson(e->operand)}};
    return 0;
  }

  int visit(IndexExp *e) override {
    if (!e) return 0;
    ast = json{{"type", "IndexExp"},
               {"array", expToJson(e->base)},
               {"index", expToJson(e->index)}};
    return 0;
  }

  int visit(FieldExp *e) override {
    if (!e) return 0;
    ast = json{{"type", "FieldExp"},
               {"expr", expToJson(e->obj)},
               {"field", e->field}};
    return 0;
  }

  int visit(SizeofExp *e) override {
    if (!e) return 0;
    ast = json{{"type", "SizeofExp"}, {"exprType", typeToJson(e->queryType)}};
    return 0;
  }

  // Sentencias
  int visit(VarDecStm *s) override {
    if (!s) return 0;
    json vars = json::array();
    for (const auto &v : s->vars) {
      json var = json{{"name", v.name},
                      {"type", typeToJson(v.type)}};
      if (v.init) {
        var["init"] = expToJson(v.init);
      }
      vars.push_back(var);
    }
    ast = json{{"type", "VarDecStm"}, {"declarations", vars}};
    return 0;
  }

  int visit(AssignStm *s) override {
    if (!s) return 0;
    std::string opStr = assignOpToString(s->op);
    ast = json{{"type", "AssignStm"},
               {"lvalue", expToJson(s->target)},
               {"op", opStr},
               {"rvalue", expToJson(s->value)}};
    return 0;
  }

  int visit(ExprStm *s) override {
    if (!s) return 0;
    ast = json{{"type", "ExprStm"}, {"expr", expToJson(s->e)}};
    return 0;
  }

  int visit(ReturnStm *s) override {
    if (!s) return 0;
    if (s->e) {
      ast = json{{"type", "ReturnStm"}, {"expr", expToJson(s->e)}};
    } else {
      ast = json{{"type", "ReturnStm"}, {"expr", nullptr}};
    }
    return 0;
  }

  int visit(IfStm *s) override {
    if (!s) return 0;
    json ifNode = json{{"type", "IfStm"},
                       {"condition", expToJson(s->cond)},
                       {"then", stmToJson(s->thenB)}};
    if (s->elseB) {
      ifNode["else"] = stmToJson(s->elseB);
    }
    ast = ifNode;
    return 0;
  }

  int visit(WhileStm *s) override {
    if (!s) return 0;
    ast = json{{"type", "WhileStm"},
               {"condition", expToJson(s->cond)},
               {"body", stmToJson(s->body)}};
    return 0;
  }

  int visit(DoWhileStm *s) override {
    if (!s) return 0;
    ast = json{{"type", "DoWhileStm"},
               {"body", stmToJson(s->body)},
               {"condition", expToJson(s->cond)}};
    return 0;
  }

  int visit(ForStm *s) override {
    if (!s) return 0;
    json forNode = json{{"type", "ForStm"}, {"condition", expToJson(s->cond)}};
    if (s->init) {
      forNode["init"] = stmToJson(s->init);
    }
    if (s->update) {
      forNode["update"] = stmToJson(s->update);
    }
    forNode["body"] = stmToJson(s->body);
    ast = forNode;
    return 0;
  }

  int visit(BreakStm *s) override {
    if (!s) return 0;
    ast = json{{"type", "BreakStm"}};
    return 0;
  }

  int visit(ContinueStm *s) override {
    if (!s) return 0;
    ast = json{{"type", "ContinueStm"}};
    return 0;
  }

  int visit(Block *s) override {
    if (!s) return 0;
    json stms = json::array();
    for (auto stmt : s->stms) {
      stms.push_back(stmToJson(stmt));
    }
    ast = json{{"type", "Block"}, {"statements", stms}};
    return 0;
  }

  int visit(FunDef *f) override {
    if (!f) return 0;
    json params = json::array();
    for (const auto &param : f->params) {
      params.push_back(json{{"name", param.name}, {"type", typeToJson(param.type)}});
    }
    ast = json{{"type", "FunDef"},
               {"name", f->name},
               {"returnType", typeToJson(f->retType)},
               {"params", params},
               {"body", stmToJson(f->body)}};
    return 0;
  }

  int visit(Program *p) override {
    if (!p) return 0;
    json funcs = json::array();
    for (auto func : p->functions) {
      func->accept(this);
      funcs.push_back(ast);
    }
    ast = json{{"type", "Program"}, {"functions", funcs}};
    return 0;
  }

  json getAst() { return ast; }

private:
  json expToJson(Exp *e) {
    if (!e) return nullptr;
    e->accept(this);
    return ast;
  }

  json stmToJson(Stm *s) {
    if (!s) return nullptr;
    s->accept(this);
    return ast;
  }

  json typeToJson(const Type &t) {
    return json{{"base", typeBaseToString(t.base)},
                {"pointer", t.pointer},
                {"isArray", t.isArray},
                {"arrayLen", t.arrayLen},
                {"cols", t.cols}};
  }

  std::string typeBaseToString(Type::Base base) {
    switch (base) {
    case Type::INT:
      return "int";
    case Type::DOUBLE:
      return "double";
    case Type::CHAR:
      return "char";
    case Type::VOID:
      return "void";
    case Type::BOOL:
      return "bool";
    case Type::STRUCT:
      return "struct";
    default:
      return "unknown";
    }
  }

  std::string unaryOpToString(UnaryOp op) {
    switch (op) {
    case NEG_OP:
      return "-";
    case NOT_OP:
      return "!";
    default:
      return "?";
    }
  }

  std::string binaryOpToString(BinaryOp op) {
    switch (op) {
    case PLUS_OP:
      return "+";
    case MINUS_OP:
      return "-";
    case MUL_OP:
      return "*";
    case DIV_OP:
      return "/";
    case MOD_OP:
      return "%";
    case LT_OP:
      return "<";
    case GT_OP:
      return ">";
    case LE_OP:
      return "<=";
    case GE_OP:
      return ">=";
    case EQ_OP:
      return "==";
    case NE_OP:
      return "!=";
    case AND_OP:
      return "&&";
    case OR_OP:
      return "||";
    default:
      return "?";
    }
  }

  std::string assignOpToString(AssignOp op) {
    switch (op) {
    case ASSIGN_EQ:
      return "=";
    case ASSIGN_ADD:
      return "+=";
    case ASSIGN_SUB:
      return "-=";
    case ASSIGN_MUL:
      return "*=";
    case ASSIGN_DIV:
      return "/=";
    default:
      return "?";
    }
  }
};

#endif
