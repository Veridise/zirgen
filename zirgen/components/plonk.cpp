// Copyright (c) 2024 RISC Zero, Inc.
//
// All rights reserved.

#include "mlir/Support/DebugStringHelper.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/include/llvm/ADT/StringExtras.h"

#include "zirgen/components/plonk.h"

namespace zirgen {

std::vector<uint64_t> PlonkExternHandler::doExtern(llvm::StringRef name,
                                                   llvm::StringRef extra,
                                                   llvm::ArrayRef<const Zll::InterpVal*> args,
                                                   size_t outCount) {
  if (name == "plonkWrite") {
    assert(outCount == 0);
    plonkRows[extra.str()].emplace_back(asFpArray(args));
    return {};
  }
  if (name == "plonkRead") {
    assert(!plonkRows[extra.str()].empty());
    std::vector<uint64_t> top = plonkRows[extra.str()].front();
    assert(top.size() == outCount);
    plonkRows[extra.str()].pop_front();
    return top;
  }

  if (name == "plonkWriteAccum") {
    assert(outCount == 0);
    plonkAccumRows[extra.str()].emplace_back(asFpArray(args));
    return {};
  }
  if (name == "plonkReadAccum") {
    assert(!plonkAccumRows[extra.str()].empty());
    std::vector<uint64_t> top = plonkAccumRows[extra.str()].front();
    assert(top.size() == outCount);
    plonkAccumRows[extra.str()].pop_front();
    return top;
  }
  return ExternHandler::doExtern(name, extra, args, outCount);
}

void PlonkExternHandler::sort(llvm::StringRef name) {
  FpMat& mat = plonkRows.at(name.str());
  std::sort(mat.begin(), mat.end());
}

void PlonkExternHandler::calcPrefixProducts(Zll::ExtensionField f) {
  for (auto& kv : plonkAccumRows) {
    FpMat& mat = kv.second;

    auto accum = f.One();

    std::vector<uint64_t> newRow;
    for (auto& row : mat) {
      assert((row.size() % f.degree) == 0);
      newRow.clear();
      for (size_t i = 0; i != row.size(); i += f.degree) {
        auto val = llvm::ArrayRef(row).slice(i, f.degree);
        accum = f.Mul(accum, val);
        newRow.insert(newRow.end(), accum.begin(), accum.end());
      }
      assert(newRow.size() == row.size());
      std::swap(newRow, row);
    }
  }
}

} // namespace zirgen