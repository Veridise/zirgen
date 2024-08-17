// Copyright (c) 2024 RISC Zero, Inc.
//
// All rights reserved.

#include "zirgen/Dialect/ZHLT/IR/TypeUtils.h"
#include "zirgen/Dialect/ZHLT/IR/ZHLT.h"
#include "zirgen/Dialect/ZStruct/Analysis/BufferAnalysis.h"
#include "zirgen/Dialect/ZStruct/Analysis/DegreeAnalysis.h"
#include "zirgen/Dialect/ZStruct/IR/ZStruct.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Attributes.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Matchers.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Interfaces/FunctionImplementation.h"
#include "llvm/ADT/TypeSwitch.h"

#define GET_OP_CLASSES
#include "zirgen/Dialect/ZHLT/IR/ComponentOps.cpp.inc"

namespace zirgen::Zhlt {

using namespace mlir;

namespace {

LogicalResult verifyRegion(Operation* origOp,
                           Region& region,
                           const std::function<LogicalResult(Operation*)>& verifyInnerOp) {
  if (region
          .walk([&](Operation* op) {
            if (failed(verifyInnerOp(op))) {
              op->emitError() << "Invalid operation inside " << origOp->getName();
              return WalkResult::interrupt();
            }
            return WalkResult::advance();
          })
          .wasInterrupted()) {
    return failure();
  }
  return success();
}

} // namespace

mlir::LogicalResult CheckFuncOp::verifyRegions() {
  return verifyRegion(*this, getBody(), [&](auto op) -> LogicalResult {
    if (llvm::isa<Zll::PolyOp,
                  Zll::IfOp,
                  Zll::TerminateOp,
                  Zll::EqualZeroOp,
                  Zhlt::ReturnOp,
                  Zhlt::GetGlobalLayoutOp,
                  ZStruct::GetBufferOp,
                  ZStruct::BindLayoutOp,
                  ZStruct::LookupOp,
                  ZStruct::LoadOp,
                  ZStruct::SubscriptOp,
                  arith::ConstantOp>(op))
      return success();
    return failure();
  });
}

namespace {

// Add context to a maximum degree error by attempting to provide
// enough context to diagnose the problem.
void addDegreeContext(InFlightDiagnostic& diag,
                      DataFlowSolver& solver,
                      Value val,
                      DenseSet<std::pair<Location, /*degree=*/unsigned>>& seenLocs) {
  auto degree = solver.lookupState<ZStruct::DegreeAnalysis::Element>(val)->getValue();
  if (!degree.isDefined()) {
    diag.attachNote(val.getLoc()) << "Unknown degree producer";
    return;
  }

  bool degreeIncreased = true;
  SmallVector<unsigned> contribDegrees;
  for (Value contrib : degree.contributions) {
    auto contribDegree = solver.lookupState<ZStruct::DegreeAnalysis::Element>(contrib)->getValue();
    assert(contribDegree.get() <= degree.get());
    if (contribDegree.get() == degree.get())
      degreeIncreased = false;
    contribDegrees.push_back(contribDegree.get());
  }

  // Deduplicate pairs of (location, degree)
  if (!llvm::isa<UnknownLoc>(val.getLoc()) &&
      !seenLocs.contains(std::make_pair(val.getLoc(), degree.get()))) {
    auto& note = diag.attachNote(val.getLoc())
                 << "Degree " << degree.get() << " produced by " << val;
    if (!contribDegrees.empty()) {
      note << " from input degrees ";
      interleaveComma(contribDegrees, note);
    }
    seenLocs.insert(std::make_pair(val.getLoc(), degree.get()));
  }

  for (Value contrib : degree.contributions) {
    auto contribDegree = solver.lookupState<ZStruct::DegreeAnalysis::Element>(contrib)->getValue();
    // Try to prune contributions intelligently for maximum signal-to-noise ratio.
    if (degreeIncreased || contribDegree.get() == degree.get()) {
      addDegreeContext(diag, solver, contrib, seenLocs);
    }
  }
}

} // namespace

mlir::LogicalResult CheckFuncOp::verifyMaxDegree(size_t maxDegree) {
  DataFlowSolver solver;
  solver.load<ZStruct::DegreeAnalysis>();
  if (failed(solver.initializeAndRun(*this)))
    return failure();

  LogicalResult res = success();
  this->walk([&](Zll::EqualZeroOp op) {
    auto degree = solver.lookupState<ZStruct::DegreeAnalysis::Element>(op)->getValue();
    assert(degree.isDefined());
    if (degree.get() <= maxDegree)
      return;

    auto diag = op->emitError() << "Constraint degree " << degree.get()
                                << " exceeds maximum degree " << maxDegree;

    // Note the degree contribution of enclosing muxes
    Operation* parent = op->getParentOp();
    auto parentDegree = solver.lookupState<ZStruct::DegreeAnalysis::Element>(parent)->getValue();
    assert(parentDegree.isDefined());
    if (parentDegree.get() != 0) {
      diag.attachNote(parent->getLoc()) << "At mux depth " << parentDegree.get();
    }

    DenseSet<std::pair<Location, /*degree=*/unsigned>> seenLocs;
    addDegreeContext(diag, solver, op.getIn(), seenLocs);
    res = failure();
  });
  return res;
}

mlir::LogicalResult ValidityRegsFuncOp::verifyRegions() {
  return verifyRegion(*this, getBody(), [&](auto op) -> LogicalResult {
    if (llvm::isa<Zll::PolyOp,
                  Zhlt::ReturnOp,
                  ZStruct::GetBufferOp,
                  ZStruct::BindLayoutOp,
                  ZStruct::LookupOp,
                  ZStruct::LoadOp,
                  ZStruct::SubscriptOp,
                  arith::ConstantOp>(op))
      return success();

    return failure();
  });
}

mlir::LogicalResult ValidityTapsFuncOp::verifyRegions() {
  return verifyRegion(*this, getBody(), [&](auto op) -> LogicalResult {
    if (llvm::isa<Zll::PolyOp,
                  Zhlt::ReturnOp,
                  ZStruct::GetBufferOp,
                  ZStruct::BindLayoutOp,
                  ZStruct::LookupOp,
                  ZStruct::LoadOp,
                  ZStruct::SubscriptOp,
                  arith::ConstantOp>(op))
      return success();

    return failure();
  });
}

} // namespace zirgen::Zhlt