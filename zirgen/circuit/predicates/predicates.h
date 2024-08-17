// Copyright (c) 2024 RISC Zero, Inc.
//
// All rights reserved.

#pragma once

#include "zirgen/compiler/edsl/edsl.h"

namespace zirgen::predicates {

constexpr size_t kDigestHalfs = 16;

struct U32Reg {
  constexpr static size_t size = 4;
  // Default constructor
  U32Reg() = default;
  // Construct via reading from a stream
  U32Reg(llvm::ArrayRef<Val>& stream);
  // Write to an output
  void write(std::vector<Val>& stream);

  Val flat();

  std::array<Val, 4> val;
};

struct SystemState {
  constexpr static size_t size = kDigestHalfs + U32Reg::size;
  // Default constructor
  SystemState() = default;
  // Construct via reading from a stream
  SystemState(llvm::ArrayRef<Val>& stream, bool longDigest = false);
  // Write to an output
  void write(std::vector<Val>& stream);
  // Digest into a single value
  DigestVal digest();

  U32Reg pc;
  DigestVal memory;
};

struct ReceiptClaim {
  constexpr static size_t size = 2 * kDigestHalfs + 2 * SystemState::size + 2;
  // Default constructor
  ReceiptClaim() = default;
  // Construct via reading from a stream
  ReceiptClaim(llvm::ArrayRef<Val>& stream, bool longDigest = false);
  // Write to an output
  void write(std::vector<Val>& stream);
  // Digest into a single value
  DigestVal digest();

  DigestVal input;
  SystemState pre;
  SystemState post;
  Val sysExit;
  Val userExit;
  DigestVal output;
};

struct Output {
  // Default constructor
  Output() = default;
  // Digest into a single value
  DigestVal digest();

  /// Digest of the journal committed to by the guest execution.
  DigestVal journal;

  /// Digest of an ordered list of Assumption digests corresponding to the
  /// calls to `env::verify` and `env::verify_integrity`.
  DigestVal assumptions;
};

struct Assumption {
  constexpr static size_t size = 2 * kDigestHalfs;
  // Default constructor
  Assumption() = default;
  // Construct via reading from a stream
  Assumption(llvm::ArrayRef<Val>& stream, bool longDigest = false);
  // Write to an output
  void write(std::vector<Val>& stream);
  // Digest into a single value
  DigestVal digest();

  DigestVal claim;
  DigestVal controlRoot;
};

// ReciptClaim lift(size_t po2, ReadIopVal seal);

ReceiptClaim join(ReceiptClaim in1, ReceiptClaim in2);
ReceiptClaim identity(ReceiptClaim in);
ReceiptClaim resolve(ReceiptClaim cond, Assumption assum, DigestVal tail, DigestVal journal);

} // namespace zirgen::predicates