// Copyright (c) 2024 RISC Zero, Inc.
//
// All rights reserved.

#include "zirgen/circuit/verify/merkle.h"

namespace zirgen::verify {

MerkleTreeParams::MerkleTreeParams(size_t rowSize,
                                   size_t colSize,
                                   size_t queries,
                                   bool useExtension)
    : rowSize(rowSize)
    , colSize(colSize)
    , queries(queries)
    , layers(log2Ceil(rowSize))
    , useExtension(useExtension) {
  assert(1U << layers == rowSize);
  topLayer = 0;
  for (size_t i = 1; i < layers; i++) {
    if ((1U << i) > queries) {
      break;
    }
    topLayer = i;
  }
  topSize = 1 << topLayer;
}

MerkleTreeVerifier::MerkleTreeVerifier(
    ReadIopVal& iop, size_t rowSize, size_t colSize, size_t queries, bool useExtension)
    : MerkleTreeParams(rowSize, colSize, queries, useExtension), top(topSize) {
  auto topRec = iop.readDigests(topSize);
  top.insert(top.end(), topRec.begin(), topRec.end());
  for (size_t i = topSize; i-- > 1;) {
    top[i] = fold(top[i * 2], top[i * 2 + 1]);
  }
  iop.commit(top[1]);
}

DigestVal MerkleTreeVerifier::getRoot() const {
  return top[1];
}

std::vector<Val> MerkleTreeVerifier::verify(ReadIopVal& iop, Val idx) const {
  std::vector<Val> out;
  if (useExtension) {
    out = iop.readExtVals(colSize);
  } else {
    out = iop.readBaseVals(colSize);
  }
  DigestVal cur = hash(out);
  idx = idx + rowSize;
  for (size_t i = 0; i < layers - topLayer; i++) {
    Val lowBit = idx & 1;
    DigestVal other = iop.readDigests(1)[0];
    idx = idx - lowBit;
    idx = idx / 2;
    auto lhs = select(lowBit, {cur, other});
    auto rhs = select(lowBit, {other, cur});
    cur = fold(lhs, rhs);
  }
  auto topDigest = select(idx - topSize, llvm::ArrayRef(top).slice(topSize, topSize));
  assert_eq(cur, topDigest);
  return out;
}

DigestVal calculateMerkleProofRoot(DigestVal member, Val idx, std::vector<DigestVal> proof) {
  DigestVal cur = member;
  for (DigestVal other : proof) {
    Val lowBit = idx & 1;
    idx = idx - lowBit;
    idx = idx / 2;
    auto lhs = select(lowBit, {cur, other});
    auto rhs = select(lowBit, {other, cur});
    cur = fold(lhs, rhs);
  }
  eqz(idx);
  return cur;
}

void verifyMerkleGroupMember(DigestVal member,
                             Val idx,
                             std::vector<DigestVal> proof,
                             DigestVal expectedRoot) {
  auto root = calculateMerkleProofRoot(member, idx, proof);
  assert_eq(root, expectedRoot);
}

} // namespace zirgen::verify