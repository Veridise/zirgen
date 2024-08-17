// Copyright (c) 2024 RISC Zero, Inc.
//
// All rights reserved.

#include "ffi.h"
#include "fp.h"
#include "fpext.h"

#include <cstdint>
#include <stdexcept>

using namespace risc0;

extern "C" const char* risc0_circuit_string_ptr(risc0_string* str) {
  return str->str.c_str();
}

extern "C" void risc0_circuit_string_free(risc0_string* str) {
  delete str;
}

struct BridgeContext {
  void* ctx;
  Callback* callback;
};

void bridgeCallback(void* ctx,
                    const char* name,
                    const char* extra,
                    const Fp* args_ptr,
                    size_t args_len,
                    Fp* outs_ptr,
                    size_t outs_len) {
  BridgeContext* bridgeCtx = reinterpret_cast<BridgeContext*>(ctx);
  if (!bridgeCtx->callback(bridgeCtx->ctx, name, extra, args_ptr, args_len, outs_ptr, outs_len)) {
    throw std::runtime_error("Host callback failure");
  }
}

extern "C" uint32_t risc0_circuit_fib_step_compute_accum(risc0_error* err,
                                                         void* ctx,
                                                         Callback callback,
                                                         size_t steps,
                                                         size_t cycle,
                                                         Fp** args_ptr,
                                                         size_t /*args_len*/) {
  return ffi_wrap<uint32_t>(err, 0, [&] {
    BridgeContext bridgeCtx{ctx, callback};
    return circuit::fib::step_compute_accum(&bridgeCtx, bridgeCallback, steps, cycle, args_ptr)
        .asRaw();
  });
}

extern "C" uint32_t risc0_circuit_fib_step_verify_accum(risc0_error* err,
                                                        void* ctx,
                                                        Callback callback,
                                                        size_t steps,
                                                        size_t cycle,
                                                        Fp** args_ptr,
                                                        size_t /*args_len*/) {
  return ffi_wrap<uint32_t>(err, 0, [&] {
    BridgeContext bridgeCtx{ctx, callback};
    return circuit::fib::step_verify_accum(&bridgeCtx, bridgeCallback, steps, cycle, args_ptr)
        .asRaw();
  });
}

extern "C" uint32_t risc0_circuit_fib_step_exec(risc0_error* err,
                                                void* ctx,
                                                Callback callback,
                                                size_t steps,
                                                size_t cycle,
                                                Fp** args_ptr,
                                                size_t /*args_len*/) {
  return ffi_wrap<uint32_t>(err, 0, [&] {
    BridgeContext bridgeCtx{ctx, callback};
    return circuit::fib::step_exec(&bridgeCtx, bridgeCallback, steps, cycle, args_ptr).asRaw();
  });
}

extern "C" uint32_t risc0_circuit_fib_step_verify_bytes(risc0_error* err,
                                                        void* ctx,
                                                        Callback callback,
                                                        size_t steps,
                                                        size_t cycle,
                                                        Fp** args_ptr,
                                                        size_t /*args_len*/) {
  return ffi_wrap<uint32_t>(err, 0, [&] {
    BridgeContext bridgeCtx{ctx, callback};
    return circuit::fib::step_verify_bytes(&bridgeCtx, bridgeCallback, steps, cycle, args_ptr)
        .asRaw();
  });
}

extern "C" uint32_t risc0_circuit_fib_step_verify_mem(risc0_error* err,
                                                      void* ctx,
                                                      Callback callback,
                                                      size_t steps,
                                                      size_t cycle,
                                                      Fp** args_ptr,
                                                      size_t /*args_len*/) {
  return ffi_wrap<uint32_t>(err, 0, [&] {
    BridgeContext bridgeCtx{ctx, callback};
    return circuit::fib::step_verify_mem(&bridgeCtx, bridgeCallback, steps, cycle, args_ptr)
        .asRaw();
  });
}

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif

extern "C" FpExt risc0_circuit_fib_poly_fp(size_t cycle, size_t steps, FpExt* poly_mix, Fp** args) {
  return circuit::fib::poly_fp(cycle, steps, poly_mix, args);
}