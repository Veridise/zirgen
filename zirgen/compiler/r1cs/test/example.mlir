// RUN: zirgen-r1cs --emit=mlir %s.r1cs 2>&1 | FileCheck %s

// CHECK: %0 = r1cs.def 0, true -> !r1cs.wire
// CHECK: %1 = r1cs.def 3, true -> !r1cs.wire
// CHECK: %2 = r1cs.def 10, false -> !r1cs.wire
// CHECK: %3 = r1cs.def 11, false -> !r1cs.wire
// CHECK: %4 = r1cs.def 12, false -> !r1cs.wire
// CHECK: %5 = r1cs.def 15, false -> !r1cs.wire
// CHECK: %6 = r1cs.def 324, false -> !r1cs.wire
// CHECK: %7 = r1cs.mul %5 * 3 : i256 -> !r1cs.factor {prime = 21888242871839275222246405745257275088548364400416034343698204186575808495617 : i256}
// CHECK: %8 = r1cs.mul %6 * 8 : i256 -> !r1cs.factor {prime = 21888242871839275222246405745257275088548364400416034343698204186575808495617 : i256}
// CHECK: %9 = r1cs.sum %7 + %8 -> !r1cs.factor
// CHECK: %10 = r1cs.mul %0 * 2 : i256 -> !r1cs.factor {prime = 21888242871839275222246405745257275088548364400416034343698204186575808495617 : i256}
// CHECK: %11 = r1cs.mul %2 * 20 : i256 -> !r1cs.factor {prime = 21888242871839275222246405745257275088548364400416034343698204186575808495617 : i256}
// CHECK: %12 = r1cs.sum %10 + %11 -> !r1cs.factor
// CHECK: %13 = r1cs.mul %3 * 12 : i256 -> !r1cs.factor {prime = 21888242871839275222246405745257275088548364400416034343698204186575808495617 : i256}
// CHECK: %14 = r1cs.sum %12 + %13 -> !r1cs.factor
// CHECK: %15 = r1cs.mul %0 * 5 : i256 -> !r1cs.factor {prime = 21888242871839275222246405745257275088548364400416034343698204186575808495617 : i256}
// CHECK: %16 = r1cs.mul %2 * 7 : i256 -> !r1cs.factor {prime = 21888242871839275222246405745257275088548364400416034343698204186575808495617 : i256}
// CHECK: %17 = r1cs.sum %15 + %16 -> !r1cs.factor
// CHECK: r1cs.constrain %9, %14, %17
// CHECK: %18 = r1cs.mul %1 * 4 : i256 -> !r1cs.factor {prime = 21888242871839275222246405745257275088548364400416034343698204186575808495617 : i256}
// CHECK: %19 = r1cs.mul %4 * 8 : i256 -> !r1cs.factor {prime = 21888242871839275222246405745257275088548364400416034343698204186575808495617 : i256}
// CHECK: %20 = r1cs.sum %18 + %19 -> !r1cs.factor
// CHECK: %21 = r1cs.mul %5 * 3 : i256 -> !r1cs.factor {prime = 21888242871839275222246405745257275088548364400416034343698204186575808495617 : i256}
// CHECK: %22 = r1cs.sum %20 + %21 -> !r1cs.factor
// CHECK: %23 = r1cs.mul %3 * 44 : i256 -> !r1cs.factor {prime = 21888242871839275222246405745257275088548364400416034343698204186575808495617 : i256}
// CHECK: %24 = r1cs.mul %6 * 6 : i256 -> !r1cs.factor {prime = 21888242871839275222246405745257275088548364400416034343698204186575808495617 : i256}
// CHECK: %25 = r1cs.sum %23 + %24 -> !r1cs.factor
// CHECK: r1cs.constrain %22, %25
// CHECK: %26 = r1cs.mul %6 * 4 : i256 -> !r1cs.factor {prime = 21888242871839275222246405745257275088548364400416034343698204186575808495617 : i256}
// CHECK: %27 = r1cs.mul %0 * 6 : i256 -> !r1cs.factor {prime = 21888242871839275222246405745257275088548364400416034343698204186575808495617 : i256}
// CHECK: %28 = r1cs.mul %2 * 11 : i256 -> !r1cs.factor {prime = 21888242871839275222246405745257275088548364400416034343698204186575808495617 : i256}
// CHECK: %29 = r1cs.sum %27 + %28 -> !r1cs.factor
// CHECK: %30 = r1cs.mul %3 * 5 : i256 -> !r1cs.factor {prime = 21888242871839275222246405745257275088548364400416034343698204186575808495617 : i256}
// CHECK: %31 = r1cs.sum %29 + %30 -> !r1cs.factor
// CHECK: %32 = r1cs.mul %6 * 600 : i256 -> !r1cs.factor {prime = 21888242871839275222246405745257275088548364400416034343698204186575808495617 : i256}
// CHECK: r1cs.constrain %26, %31, %32

