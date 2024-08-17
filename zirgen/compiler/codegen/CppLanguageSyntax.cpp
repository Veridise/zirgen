// Copyright (c) 2024 RISC Zero, Inc.
//
// All rights reserved.

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"
#include "zirgen/Dialect/Zll/IR/Codegen.h"
#include "zirgen/Dialect/Zll/IR/IR.h"
#include "zirgen/compiler/codegen/codegen.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/TypeSwitch.h"

using namespace mlir;
using namespace zirgen::Zll;

namespace zirgen::codegen {

void CppLanguageSyntax::fallbackEmitLiteral(CodegenEmitter& cg,
                                            mlir::Type ty,
                                            mlir::Attribute value) {
  TypeSwitch<Attribute>(value)
      .Case<IntegerAttr>([&](auto intAttr) { cg << intAttr.getValue().getZExtValue(); })
      .Case<StringAttr>([&](auto strAttr) { cg.emitEscapedString(strAttr); })
      .Case<PolynomialAttr>([&](auto polyAttr) {
        cg << "(" << ty.cast<ValType>().getTypeName(cg) << " {";
        cg.interleaveComma(polyAttr.asArrayRef());
        cg << "})";
      })
      .Default([&](auto) {
        llvm::errs() << "Don't know how to emit type " << ty << " into C++ with value " << value
                     << "\n";
        abort();
      });
}

void CppLanguageSyntax::emitConditional(CodegenEmitter& cg,
                                        CodegenValue condition,
                                        EmitPart emitThen) {
  cg << "if (to_size_t(" << condition << ")) {\n";
  cg << emitThen;
  cg << "}\n";
}

void CppLanguageSyntax::emitSwitchStatement(CodegenEmitter& cg,
                                            CodegenIdent<IdentKind::Var> resultName,
                                            mlir::Type resultType,
                                            llvm::ArrayRef<CodegenValue> conditions,
                                            llvm::ArrayRef<EmitArmPartFunc> emitArms) {
  cg << cg.getTypeName(resultType) << " " << resultName << ";\n";
  for (const auto& [cond, emitArm] : llvm::zip(conditions, emitArms)) {
    cg << "if (to_size_t(" << cond << ")) {\n";
    auto result = emitArm();
    cg << resultName << " = " << result << ";\n";
    cg << "} else ";
  }
  cg << "{\n";
  cg << "   assert(0 && \"Reached unreachable mux arm\");\n";
  cg << "}\n";
}

void CppLanguageSyntax::emitFuncDefinition(CodegenEmitter& cg,
                                           CodegenIdent<IdentKind::Func> funcName,
                                           llvm::ArrayRef<CodegenIdent<IdentKind::Var>> argNames,
                                           mlir::FunctionType funcType,
                                           mlir::Region* body) {
  auto returnTypes = funcType.getResults();
  if (returnTypes.size() == 0) {
    cg << "void";
  } else if (returnTypes.size() == 1) {
    cg << cg.getTypeName(returnTypes[0]);
  } else {
    cg << "std::tuple<";
    cg.interleaveComma(returnTypes, [&](auto ty) { cg << cg.getTypeName(ty); });
    cg << ">";
  }

  cg << " " << funcName << "(";
  if (!contextArgDecls.empty()) {
    cg.interleaveComma(contextArgDecls, [&](auto contextArg) { cg << EmitPart(contextArg); });
    if (!argNames.empty())
      cg << ",";
  }
  cg.interleaveComma(zip(argNames, funcType.getInputs()), [&](auto vt) {
    auto argName = std::get<0>(vt);
    Type argType = std::get<1>(vt);

    if (argType.hasTrait<CodegenLayoutTypeTrait>())
      cg << "BoundLayout<" << cg.getTypeName(argType) << ">";
    else
      cg << cg.getTypeName(argType);
    cg << " " << argName;
  });

  cg << ") {\n";
  cg.emitRegion(*body);
  cg << "}\n";
}

void CppLanguageSyntax::emitReturn(CodegenEmitter& cg, llvm::ArrayRef<CodegenValue> values) {

  cg << "return ";
  if (values.size() > 1) {
    cg << "std::make_tuple(";
  }
  cg.interleaveComma(values);
  if (values.size() > 1) {
    cg << ")";
  }
  cg << ";\n";
}

void CppLanguageSyntax::emitSaveResults(CodegenEmitter& cg,
                                        ArrayRef<CodegenIdent<IdentKind::Var>> names,
                                        llvm::ArrayRef<mlir::Type> types,
                                        EmitPart emitExpression) {
  if (names.empty()) {
    cg << emitExpression << ";\n";
  } else if (names.size() == 1) {
    if (Type(types[0]).hasTrait<CodegenLayoutTypeTrait>()) {
      cg << "BoundLayout<" << cg.getTypeName(types[0]) << ">";
    } else {
      cg << cg.getTypeName(types[0]);
    }
    cg << " " << names[0] << " = " << emitExpression << ";\n";
  } else {
    cg << "auto [";
    cg.interleaveComma(names);
    cg << "] = " << emitExpression << ";\n";
  }
}

void CppLanguageSyntax::emitSaveConst(CodegenEmitter& cg,
                                      CodegenIdent<IdentKind::Const> name,
                                      CodegenValue value) {
  cg << "constexpr " << cg.getTypeName(value.getType()) << " " << name << " = " << value << ";\n";
}

void CppLanguageSyntax::emitCall(CodegenEmitter& cg,
                                 CodegenIdent<IdentKind::Func> callee,
                                 llvm::ArrayRef<StringRef> contextArgs,
                                 llvm::ArrayRef<CodegenValue> args) {
  cg << callee << "(";
  if (!contextArgs.empty()) {
    cg.interleaveComma(contextArgs, [&](auto contextArg) { cg << EmitPart(contextArg) << ","; });
    if (!args.empty())
      cg << ",";
  }
  cg.interleaveComma(args);
  cg << ")";
}

void CppLanguageSyntax::emitInvokeMacro(CodegenEmitter& cg,
                                        CodegenIdent<IdentKind::Macro> callee,
                                        llvm::ArrayRef<StringRef> contextArgs,
                                        llvm::ArrayRef<EmitPart> emitArgs) {
  cg << callee;
  if (contextArgs.empty() && emitArgs.empty())
    return;

  cg << "(";
  if (!contextArgs.empty()) {
    cg.interleaveComma(contextArgs, [&](auto contextArg) { cg << EmitPart(contextArg) << ","; });
    if (!emitArgs.empty())
      cg << ",";
  }
  cg.interleaveComma(emitArgs);
  cg << ")";
}

std::string CppLanguageSyntax::canonIdent(llvm::StringRef ident, IdentKind kind) {
  switch (kind) {
  case IdentKind::Var:
  case IdentKind::Field:
  case IdentKind::Func: {
    std::string str = convertToCamelFromSnakeCase(ident);
    if (!str.empty())
      str[0] = llvm::toLower(str[0]);
    return str;
  }
  case IdentKind::Type:
    return convertToCamelFromSnakeCase(ident, /*capitalizeFirst=*/true);
  case IdentKind::Const:
    return "k" + convertToCamelFromSnakeCase(ident, /*capitalizeFirst=*/true);
  case IdentKind::Macro:
    return llvm::StringRef(convertToSnakeFromCamelCase(ident)).upper();
  }
  throw(std::runtime_error("Unknown ident kind"));
}

void CppLanguageSyntax::emitStructDef(CodegenEmitter& cg,
                                      mlir::Type ty,
                                      llvm::ArrayRef<CodegenIdent<IdentKind::Field>> names,
                                      llvm::ArrayRef<mlir::Type> types) {
  cg << "struct " << cg.getTypeName(ty) << " {\n";
  assert(names.size() == types.size());
  for (size_t i = 0; i != names.size(); i++) {
    cg << "  " << cg.getTypeName(types[i]) << " " << names[i] << ";\n";
  }
  cg << "};\n";
}

void CppLanguageSyntax::emitStructConstruct(CodegenEmitter& cg,
                                            mlir::Type ty,
                                            llvm::ArrayRef<CodegenIdent<IdentKind::Field>> names,
                                            llvm::ArrayRef<CodegenValue> values) {
  cg << cg.getTypeName(ty) << "{\n";
  assert(names.size() == values.size());

  cg.interleaveComma(zip(names, values), [&](auto zipped) {
    auto [name, value] = zipped;
    cg << "  ." << name << " = " << value;
  });
  cg << "}";
}

void CppLanguageSyntax::emitArrayDef(CodegenEmitter& cg,
                                     mlir::Type ty,
                                     mlir::Type elemType,
                                     size_t numElems) {
  cg << "using " << cg.getTypeName(ty) << " = std::array<" << cg.getTypeName(elemType) << ", "
     << numElems << ">;\n";
}

void CppLanguageSyntax::emitArrayConstruct(CodegenEmitter& cg,
                                           mlir::Type ty,
                                           mlir::Type elemType,
                                           llvm::ArrayRef<CodegenValue> values) {
  cg << cg.getTypeName(ty);
  cg << "{";
  cg.interleaveComma(values);
  cg << "}";
}

void CppLanguageSyntax::emitMapConstruct(CodegenEmitter& cg,
                                         CodegenValue array,
                                         std::optional<CodegenValue> layout,
                                         llvm::ArrayRef<CodegenIdent<IdentKind::Var>> argNames,
                                         mlir::Region& body) {
  cg << "map(" << array << ", ";
  if (layout)
    cg << *layout << ", ";
  cg << "std::function([&](";
  cg << cg.getTypeName(array.getType()) << "::value_type " << argNames[0];
  if (layout)
    cg << ", BoundLayout<" << cg.getTypeName(layout->getType()) << "::value_type> " << argNames[1];
  cg << ") {\n";
  cg.emitRegion(body);
  cg << "\n}))";
}

void CppLanguageSyntax::emitReduceConstruct(CodegenEmitter& cg,
                                            CodegenValue array,
                                            CodegenValue init,
                                            std::optional<CodegenValue> layout,
                                            llvm::ArrayRef<CodegenIdent<IdentKind::Var>> argNames,
                                            mlir::Region& body) {
  cg << "reduce(" << array << ", " << init << ", ";
  if (layout) {
    cg << *layout << ", ";
  }
  cg << "std::function([&](";
  cg << cg.getTypeName(init.getType()) << " " << argNames[0] << ", "
     << cg.getTypeName(array.getType()) << "::value_type " << argNames[1];
  if (layout) {
    cg << ", BoundLayout<" << cg.getTypeName(layout->getType()) << "::value_type> " << argNames[2];
  }
  cg << ") {\n";
  cg.emitRegion(body);
  cg << "\n}))";
}

void CppLanguageSyntax::emitLayoutDef(CodegenEmitter& cg,
                                      mlir::Type ty,
                                      llvm::ArrayRef<CodegenIdent<IdentKind::Field>> names,
                                      llvm::ArrayRef<mlir::Type> types) {
  // In C++, layouts are just regular structures.
  emitStructDef(cg, ty, names, types);
}

// ----------------------------------------------------------------------
// CUDA variant of C++ that needs things slightly different than regular C++
void CudaLanguageSyntax::emitSaveConst(CodegenEmitter& cg,
                                       CodegenIdent<IdentKind::Const> name,
                                       CodegenValue value) {
  // Make constants available on the device
  cg << "__device__ ";
  CppLanguageSyntax::emitSaveConst(cg, name, value);
}

void CudaLanguageSyntax::emitArrayDef(CodegenEmitter& cg,
                                      mlir::Type ty,
                                      mlir::Type elemType,
                                      size_t numElems) {
  // Cuda doesn't support std::array constexpr on the device, so use a C array type.
  cg << "using " << cg.getTypeName(ty) << " = " << cg.getTypeName(elemType) << "[" << numElems
     << "];\n";
}

void CudaLanguageSyntax::emitArrayConstruct(CodegenEmitter& cg,
                                            mlir::Type ty,
                                            mlir::Type elemType,
                                            llvm::ArrayRef<CodegenValue> values) {
  // Leave off the type name when constructing arrays; otherwise it coerces it into a pointer.
  cg << "{";
  cg.interleaveComma(values);
  cg << "}";
}

} // namespace zirgen::codegen