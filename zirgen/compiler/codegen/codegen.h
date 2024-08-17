// Copyright (c) 2024 RISC Zero, Inc.
//
// All rights reserved.

#pragma once

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/PassManager.h"
#include "zirgen/Dialect/Zll/IR/Codegen.h"
#include "zirgen/compiler/codegen/protocol_info_const.h"

#include <memory>
#include <string>

namespace zirgen {
namespace recursion {
struct EncodeStats;
}

class RustStreamEmitter {
public:
  virtual ~RustStreamEmitter() = default;
  virtual void emitStepFunc(const std::string& name, mlir::func::FuncOp func) = 0;
  virtual void emitPolyFunc(const std::string& fn, mlir::func::FuncOp func) = 0;
  virtual void emitPolyExtFunc(mlir::func::FuncOp func) = 0;
  virtual void emitTaps(mlir::func::FuncOp func) = 0;
  virtual void emitInfo(mlir::func::FuncOp func, ProtocolInfo version) = 0;
};

class GpuStreamEmitter {
public:
  virtual ~GpuStreamEmitter() = default;
  virtual void emitPoly(mlir::func::FuncOp func) = 0;
  virtual void emitStepFunc(const std::string& name, mlir::func::FuncOp func) = 0;
};

class CppStreamEmitter {
public:
  virtual ~CppStreamEmitter() = default;
  virtual void emitPoly(mlir::func::FuncOp func) = 0;
  virtual void emitTaps(mlir::func::FuncOp func) = 0;
  virtual void emitHeader(mlir::func::FuncOp func) = 0;
};

// Options relating to a specific stage.
struct StageOptions {
  /* implicit */ StageOptions(const char name[]) : name(name) {}

  // Name of this stage
  std::string name;

  // Add any extra passes for this stage
  std::function<void(mlir::OpPassManager& opm)> addExtraPasses;
};

struct EmitCodeOptions {
  // An identifier string indicating which version of what circuit is being omitted
  ProtocolInfo info;
  // Stages to generate step functions for, indexed by stage ID.
  std::vector<StageOptions> stages;
};

namespace codegen {

class RustLanguageSyntax : public LanguageSyntax {
public:
  // Mark the given macro as being invoked using curly braces instead of parentheses.
  // This makes it so it can expand in non-expression contexts.
  void addItemsMacro(llvm::StringRef macroName);

private:
  LanguageKind getLanguageKind() override { return LanguageKind::Rust; }
  std::string canonIdent(llvm::StringRef ident, IdentKind idt) override;
  void emitClone(CodegenEmitter& cg, CodegenIdent<IdentKind::Var> value) override;
  void emitTakeReference(CodegenEmitter& cg, EmitPart emitTarget) override;
  void fallbackEmitLiteral(CodegenEmitter& cg, mlir::Type ty, mlir::Attribute value) override;

  void emitConditional(CodegenEmitter& cg, CodegenValue condition, EmitPart emitThen) override;
  void emitSwitchStatement(CodegenEmitter& cg,
                           CodegenIdent<IdentKind::Var> resultNames,
                           mlir::Type resultType,
                           llvm::ArrayRef<CodegenValue> conditions,
                           llvm::ArrayRef<EmitArmPartFunc> emitArms) override;

  void emitFuncDefinition(CodegenEmitter& cg,
                          CodegenIdent<IdentKind::Func> funcName,
                          llvm::ArrayRef<CodegenIdent<IdentKind::Var>> argNames,
                          mlir::FunctionType funcType,
                          mlir::Region* body) override;

  void emitReturn(CodegenEmitter& cg, llvm::ArrayRef<CodegenValue> values) override;

  void emitSaveResults(CodegenEmitter& cg,
                       llvm::ArrayRef<CodegenIdent<IdentKind::Var>> names,
                       llvm::ArrayRef<mlir::Type> types,
                       EmitPart emitExpression) override;

  void emitSaveConst(CodegenEmitter& cg,
                     CodegenIdent<IdentKind::Const> name,
                     CodegenValue value) override;

  void emitCall(CodegenEmitter& cg,
                CodegenIdent<IdentKind::Func> callee,
                llvm::ArrayRef<llvm::StringRef> contextArgs,
                llvm::ArrayRef<CodegenValue> args) override;

  void emitInvokeMacro(CodegenEmitter& cg,
                       CodegenIdent<IdentKind::Macro> callee,
                       llvm::ArrayRef<llvm::StringRef> contextArgs,
                       llvm::ArrayRef<EmitPart> emitArgs) override;

  void emitStructDef(CodegenEmitter& cg,
                     mlir::Type ty,
                     llvm::ArrayRef<CodegenIdent<IdentKind::Field>> fields,
                     llvm::ArrayRef<mlir::Type> types) override;
  void emitStructConstruct(CodegenEmitter& cg,
                           mlir::Type ty,
                           llvm::ArrayRef<CodegenIdent<IdentKind::Field>> names,
                           llvm::ArrayRef<CodegenValue> values) override;
  void
  emitArrayDef(CodegenEmitter& cg, mlir::Type ty, mlir::Type elemType, size_t numElems) override;
  void emitArrayConstruct(CodegenEmitter& cg,
                          mlir::Type ty,
                          mlir::Type elemType,
                          llvm::ArrayRef<CodegenValue> values) override;
  void emitMapConstruct(CodegenEmitter& cg,
                        CodegenValue array,
                        std::optional<CodegenValue> layout,
                        llvm::ArrayRef<CodegenIdent<IdentKind::Var>> argNames,
                        mlir::Region& body) override;
  void emitReduceConstruct(CodegenEmitter& cg,
                           CodegenValue array,
                           CodegenValue init,
                           std::optional<CodegenValue> layout,
                           llvm::ArrayRef<CodegenIdent<IdentKind::Var>> argNames,
                           mlir::Region& body) override;
  void emitLayoutDef(CodegenEmitter& cg,
                     mlir::Type ty,
                     llvm::ArrayRef<CodegenIdent<IdentKind::Field>> fields,
                     llvm::ArrayRef<mlir::Type> types) override;

  llvm::StringSet<> itemsMacros;

private:
  void emitValueWithReferenceIfNeeded(CodegenEmitter& cg, CodegenValue value);
};

struct CppLanguageSyntax : public LanguageSyntax {
  LanguageKind getLanguageKind() override { return LanguageKind::Cpp; }

  std::string canonIdent(llvm::StringRef ident, IdentKind idt) override;
  void fallbackEmitLiteral(CodegenEmitter& cg, mlir::Type ty, mlir::Attribute value) override;

  void emitConditional(CodegenEmitter& cg, CodegenValue condition, EmitPart emitThen) override;
  void emitSwitchStatement(CodegenEmitter& cg,
                           CodegenIdent<IdentKind::Var> resultName,
                           mlir::Type resultType,
                           llvm::ArrayRef<CodegenValue> conditions,
                           llvm::ArrayRef<EmitArmPartFunc> emitArms) override;

  void emitFuncDefinition(CodegenEmitter& cg,
                          CodegenIdent<IdentKind::Func> funcName,
                          llvm::ArrayRef<CodegenIdent<IdentKind::Var>> argNames,
                          mlir::FunctionType funcType,
                          mlir::Region* body) override;

  void emitReturn(CodegenEmitter& cg, llvm::ArrayRef<CodegenValue> values) override;

  void emitSaveResults(CodegenEmitter& cg,
                       llvm::ArrayRef<CodegenIdent<IdentKind::Var>> names,
                       llvm::ArrayRef<mlir::Type> types,
                       EmitPart emitExpression) override;

  void emitSaveConst(CodegenEmitter& cg,
                     CodegenIdent<IdentKind::Const> name,
                     CodegenValue value) override;

  void emitCall(CodegenEmitter& cg,
                CodegenIdent<IdentKind::Func> callee,
                llvm::ArrayRef<llvm::StringRef> contextArgs,
                llvm::ArrayRef<CodegenValue> args) override;

  void emitInvokeMacro(CodegenEmitter& cg,
                       CodegenIdent<IdentKind::Macro> callee,
                       llvm::ArrayRef<llvm::StringRef> contextArgs,
                       llvm::ArrayRef<EmitPart> emitArgs) override;

  void emitStructDef(CodegenEmitter& cg,
                     mlir::Type ty,
                     llvm::ArrayRef<CodegenIdent<IdentKind::Field>> fields,
                     llvm::ArrayRef<mlir::Type> types) override;
  void emitStructConstruct(CodegenEmitter& cg,
                           mlir::Type ty,
                           llvm::ArrayRef<CodegenIdent<IdentKind::Field>> names,
                           llvm::ArrayRef<CodegenValue> values) override;
  void
  emitArrayDef(CodegenEmitter& cg, mlir::Type ty, mlir::Type elemType, size_t numElems) override;
  void emitArrayConstruct(CodegenEmitter& cg,
                          mlir::Type ty,
                          mlir::Type elemType,
                          llvm::ArrayRef<CodegenValue> values) override;
  void emitMapConstruct(CodegenEmitter& cg,
                        CodegenValue array,
                        std::optional<CodegenValue> layout,
                        llvm::ArrayRef<CodegenIdent<IdentKind::Var>> argNames,
                        mlir::Region& body) override;
  void emitReduceConstruct(CodegenEmitter& cg,
                           CodegenValue array,
                           CodegenValue init,
                           std::optional<CodegenValue> layout,
                           llvm::ArrayRef<CodegenIdent<IdentKind::Var>> argNames,
                           mlir::Region& body) override;
  void emitLayoutDef(CodegenEmitter& cg,
                     mlir::Type ty,
                     llvm::ArrayRef<CodegenIdent<IdentKind::Field>> fields,
                     llvm::ArrayRef<mlir::Type> types) override;
};

struct CudaLanguageSyntax : public CppLanguageSyntax {
  void emitSaveConst(CodegenEmitter& cg,
                     CodegenIdent<IdentKind::Const> name,
                     CodegenValue value) override;
  void
  emitArrayDef(CodegenEmitter& cg, mlir::Type ty, mlir::Type elemType, size_t numElems) override;
  void emitArrayConstruct(CodegenEmitter& cg,
                          mlir::Type ty,
                          mlir::Type elemType,
                          llvm::ArrayRef<CodegenValue> values) override;
};

} // namespace codegen

std::unique_ptr<RustStreamEmitter> createRustStreamEmitter(llvm::raw_ostream& ofs);
std::unique_ptr<GpuStreamEmitter> createGpuStreamEmitter(llvm::raw_ostream& ofs,
                                                         const std::string& suffix);
std::unique_ptr<CppStreamEmitter> createCppStreamEmitter(llvm::raw_ostream& ofs);

void registerCodegenCLOptions();
void emitCode(mlir::ModuleOp module, const EmitCodeOptions& opts);
void emitRecursion(const std::string& path,
                   mlir::func::FuncOp func,
                   recursion::EncodeStats* stats = nullptr);

struct FileContext {
  llvm::DenseMap<mlir::Value, std::string> vars;
  size_t next = 0;

  std::string use(mlir::Value value) const {
    auto it = vars.find(value);
    if (it == vars.end()) {
      throw std::runtime_error("Missing use");
    }
    return it->second;
  }

  std::string def(mlir::Value value, const std::string& prefix = "x") {
    std::string name = prefix + std::to_string(next++);
    vars[value] = name;
    return name;
  }
};

std::string escapeString(llvm::StringRef str);

} // namespace zirgen