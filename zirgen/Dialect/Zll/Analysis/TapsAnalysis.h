// Copyright (c) 2024 RISC Zero, Inc.
//
// All rights reserved.

#pragma once

#include "mlir/IR/Operation.h"
#include "zirgen/Dialect/Zll/IR/IR.h"
#include <vector>

namespace zirgen::Zll {

// Analyzes a set of taps to gather the data we need for ZKP.  This is
// in a format agreeable to the verifier to get compiled to zkr.
//
// TODO: It would be nice to reuse this logic for e.g generating the
// tap data for risc0_zkp::risc0::taps::TapSet. instead of having a
// separate implementation.
class TapsAnalysis {
public:
  struct Reg {
    unsigned offset;
    unsigned combo;
    unsigned tapPos;
    std::vector<unsigned> backs;
  };

  struct Group {
    std::vector<Reg> regs;
  };

  struct Combo {
    unsigned combo;
    std::vector<unsigned> backs;
  };

  struct TapSet {
    std::vector<TapsAnalysis::Group> groups;
    std::vector<TapsAnalysis::Combo> combos;
    unsigned tapCount;
  };

  TapsAnalysis(mlir::MLIRContext* ctx, llvm::SmallVector<TapAttr> taps);

  const TapSet& getTapSet() { return tapSet; }
  llvm::ArrayRef<TapAttr> getTapAttrs() { return tapAttrs; }
  llvm::SmallVector<TapRegAttr> getTapRegAttrs();
  llvm::SmallVector<mlir::ArrayAttr> getTapCombosAttrs() { return tapComboAttrs; }

  unsigned getTapIndex(unsigned regGroupId, unsigned offset, unsigned back) {
    return getTapIndex(TapAttr::get(ctx, regGroupId, offset, back));
  }
  unsigned getTapIndex(TapAttr tapAttr) { return tapIndex.lookup(tapAttr); }
  unsigned getComboIndex(llvm::ArrayRef<unsigned> backs) {
    return backComboIndex.lookup(comboToAttr(backs));
  }
  mlir::ArrayAttr comboToAttr(llvm::ArrayRef<unsigned> backs);

private:
  mlir::MLIRContext* ctx;

  TapSet tapSet;
  llvm::DenseMap<TapAttr, /*index=*/unsigned> tapIndex;
  llvm::SmallVector<TapAttr> tapAttrs;
  llvm::SmallVector<mlir::ArrayAttr> tapComboAttrs;
  llvm::DenseMap<mlir::ArrayAttr, /*comboIndex=*/unsigned> backComboIndex;
};

using TapSet = TapsAnalysis::TapSet;

} // namespace zirgen::Zll