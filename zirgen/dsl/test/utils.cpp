// Copyright (c) 2024 RISC Zero, Inc.
//
// All rights reserved.

#include "zirgen/dsl/test/utils.h"

namespace zirgen {
namespace dsl {
namespace utils {

static llvm::SMLoc loc;

ast::Parameter::Ptr Parameter(string name, Expr&& type, bool isVariadic) {
  return std::make_unique<ast::Parameter>(loc, name, std::move(type), isVariadic);
}

Expr Literal(uint64_t value) {
  return std::make_unique<ast::Literal>(loc, value);
}

Expr StringLiteral(string value) {
  return std::make_unique<ast::StringLiteral>(loc, value);
}

Expr Ident(string name) {
  return std::make_unique<ast::Ident>(loc, name);
}

Expr Lookup(Expr&& component, string member) {
  return std::make_unique<ast::Lookup>(loc, std::move(component), member);
}

Expr Subscript(Expr&& array, Expr&& element) {
  return std::make_unique<ast::Subscript>(loc, std::move(array), std::move(element));
}

Expr Specialize(Expr&& type, ExprVec&& args) {
  return std::make_unique<ast::Specialize>(loc, std::move(type), std::move(args));
}

Stmt Definition(string name, Expr&& value, bool isGlobal) {
  return std::make_unique<ast::Definition>(loc, name, std::move(value), isGlobal);
}

Stmt Declaration(string name, Expr&& type, bool isGlobal) {
  return std::make_unique<ast::Declaration>(loc, name, std::move(type), isGlobal);
}

Stmt Constraint(Expr&& lhs, Expr&& rhs) {
  return std::make_unique<ast::Constraint>(loc, std::move(lhs), std::move(rhs));
}

Stmt Void(Expr&& expression) {
  return std::make_unique<ast::Void>(loc, std::move(expression));
}

Expr Block(StmtVec&& body, Expr value) {
  return std::make_unique<ast::Block>(loc, std::move(body), std::move(value));
}

Expr Construct(Expr&& type, ExprVec&& args) {
  return std::make_unique<ast::Construct>(loc, std::move(type), std::move(args));
}

Expr Construct(string ident, ExprVec&& args) {
  return Construct(Ident(ident), std::move(args));
}

Expr Map(Expr&& array, string element, Expr&& function) {
  return make_unique<ast::Map>(loc, std::move(array), element, std::move(function));
}

Expr Reduce(Expr&& array, Expr&& init, Expr&& type) {
  return make_unique<ast::Reduce>(loc, std::move(array), std::move(init), std::move(type));
}

Expr Switch(Expr&& selector, ExprVec&& cases) {
  return make_unique<ast::Switch>(loc, std::move(selector), std::move(cases));
}

Expr Range(Expr&& start, Expr&& end) {
  return make_unique<ast::Range>(loc, std::move(start), std::move(end));
}

Expr ArrayLiteral(ExprVec&& elements) {
  return make_unique<ast::ArrayLiteral>(loc, std::move(elements));
}

Comp Component(string name, ParamVec&& tp, ParamVec&& p, Expr&& body) {
  auto kind = ast::Component::Kind::Object;
  return make_unique<ast::Component>(loc, kind, name, std::move(tp), std::move(p), std::move(body));
}

Comp Function(string name, ParamVec&& tp, ParamVec&& p, Expr&& body) {
  auto kind = ast::Component::Kind::Function;
  return make_unique<ast::Component>(loc, kind, name, std::move(tp), std::move(p), std::move(body));
}

Comp Extern(string name, ParamVec&& p, Expr&& resultType) {
  auto kind = ast::Component::Kind::Extern;
  return make_unique<ast::Component>(
      loc, kind, name, ParamVec{}, std::move(p), std::move(resultType));
}

Mod Module(CompVec&& components) {
  return make_unique<ast::Module>(loc, std::move(components));
}

Expr Back(Expr&& body, Expr&& distance) {
  return make_unique<ast::Back>(loc, std::move(body), std::move(distance));
}

Expr Isz(Expr&& value) {
  return Construct(Ident("Isz"), arr(std::move(value)));
}

} // namespace utils
} // namespace dsl
} // namespace zirgen