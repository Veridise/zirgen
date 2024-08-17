// Copyright (c) 2024 RISC Zero, Inc.
//
// All rights reserved.

#include "zirgen/Dialect/ZStruct/IR/ZStruct.h"

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/IR/DialectImplementation.h"
#include "mlir/Transforms/InliningUtils.h"
#include "llvm/ADT/TypeSwitch.h"

#include "zirgen/Dialect/ZStruct/IR/Dialect.cpp.inc"

#define GET_TYPEDEF_CLASSES
#include "zirgen/Dialect/ZStruct/IR/Types.cpp.inc"
#define GET_ATTRDEF_CLASSES
#include "zirgen/Dialect/ZStruct/IR/Attrs.cpp.inc"

using namespace mlir;

namespace zirgen::ZStruct {

class ZStructInlinerInterface : public DialectInlinerInterface {
  using DialectInlinerInterface::DialectInlinerInterface;

  bool
  isLegalToInline(Region* op, Region* dest, bool wouldBeCloned, IRMapping& valueMapping) const {
    // All regions in this dialect are inlinable
    return true;
  }
  bool
  isLegalToInline(Operation* op, Region* dest, bool wouldBeCloned, IRMapping& valueMapping) const {
    // All ops in this dialect are inlinable
    return true;
  }
};

class ZStructOpAsmInterface : public OpAsmDialectInterface {
public:
  using OpAsmDialectInterface::OpAsmDialectInterface;

  // Introduce a type alias for each Zirgen component, because these types are
  // often very deeply nested and therefore have very large text representations
  // in the IR. These aliases make the IR much more readable.
  AliasResult getAlias(Type type, raw_ostream& os) const final {
    if (auto component = type.dyn_cast<StructType>()) {
      os << "zstruct$" << component.getId();
      return AliasResult::FinalAlias;
    } else if (auto component = type.dyn_cast<LayoutType>()) {
      os << "zlayout$" << component.getId();
      return AliasResult::FinalAlias;
    } else if (auto component = type.dyn_cast<UnionType>()) {
      os << "zunion$" << component.getId();
      return AliasResult::FinalAlias;
    }
    return AliasResult::NoAlias;
  }
};

void ZStructDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "zirgen/Dialect/ZStruct/IR/Ops.cpp.inc"
      >();

  addTypes<
#define GET_TYPEDEF_LIST
#include "zirgen/Dialect/ZStruct/IR/Types.cpp.inc"
      >();

  addAttributes<
#define GET_ATTRDEF_LIST
#include "zirgen/Dialect/ZStruct/IR/Attrs.cpp.inc"
      >();

  addInterface<ZStructOpAsmInterface>();
  addInterface<ZStructInlinerInterface>();
}

// Constant names for generated constants.
std::string getLayoutConstName(StringRef origName) {
  origName.consume_front("@");
  return "layout$" + origName.str();
}

int extractIntAttr(mlir::Attribute attr) {
  if (auto intAttr = llvm::dyn_cast<mlir::IntegerAttr>(attr)) {
    return intAttr.getValue().getZExtValue();
  }

  if (auto polyAttr = llvm::dyn_cast<PolynomialAttr>(attr)) {
    return polyAttr[0];
  }

  llvm::errs() << "Unable to extract an integer value from " << attr << "\n";
  abort();
}

} // namespace zirgen::ZStruct