// Copyright (c) 2024 RISC Zero, Inc.
//
// All rights reserved.

#include "zirgen/circuit/rv32im/v1/platform/page_table.h"

#include "zirgen/circuit/rv32im/v1/platform/constants.h"
#include "zirgen/compiler/zkp/util.h"
#include "llvm/Support/Format.h"

namespace zirgen::rv32im_v1 {

PageTableInfo::PageTableInfo() : maxMem(kPageTableAddr * kWordSize) {
  size_t total = 0;
  size_t remain = maxMem;
  while (remain >= kPageSize) {
    size_t numPages = remain / kPageSize;
    remain = numPages * kDigestBytes;
    layers.push_back(remain);
    total += remain;
  }
  maxMem += total;
  numPages = maxMem / kPageSize;
  size_t totalWords = total / kWordSize;

  lastAddr = kPageTableAddr + totalWords;
  rootAddr = kPageTableAddr + roundUp(totalWords, kBlockSize);
  rootIndex = getPageIndex(rootAddr);
  rootPageAddr = getPageAddr(rootIndex);
  numRootEntries = (rootAddr - rootPageAddr) / kDigestWords;
  // llvm::errs() << "  numPages: " << numPages
  //              << ", lastAddr: " << llvm::format_hex(lastAddr * kWordSize, 10)
  //              << ", rootAddr: " << llvm::format_hex(rootAddr * kWordSize, 10)
  //              << ", rootIndex: " << llvm::format_hex(rootIndex, 10)
  //              << ", rootPageAddr: " << llvm::format_hex(rootPageAddr * kWordSize, 10)
  //              << ", numRootEntries: " << numRootEntries << "\n";
}

uint32_t getPageAddr(uint32_t pageIndex) {
  return pageIndex * kPageSize / kWordSize;
}

uint32_t getPageIndex(uint32_t addr) {
  return addr * kWordSize / kPageSize;
}

} // namespace zirgen::rv32im_v1