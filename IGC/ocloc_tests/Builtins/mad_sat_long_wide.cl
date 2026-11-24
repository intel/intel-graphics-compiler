/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
// UNSUPPORTED: system-windows
// REQUIRES: cri-supported

// RUN: ocloc compile -file %s -device cri -options "-igc_opts 'PrintToConsole=1 PrintAfter=PreCompiledFuncImport'" 2>&1 | FileCheck %s

// CHECK-LABEL: define spir_kernel void @k(
// CHECK: call i64 @llvm.genx.GenISA.WideSMulHi(i64 %a, i64 %b)
// CHECK: [[LO:%.*]] = mul i64
// CHECK: [[SUM:%.*]] = add i64 [[LO]], %c
// CHECK: icmp ult i64 [[SUM]], [[LO]]
// CHECK: [[SIGN:%.*]] = ashr i64 [[SUM]], 63
// CHECK: [[FITS:%.*]] = icmp eq i64 {{%.*}}, [[SIGN]]
// CHECK: [[SAT:%.*]] = select i1 {{%.*}}, i64 -9223372036854775808, i64 9223372036854775807
// CHECK: select i1 [[FITS]], i64 [[SUM]], i64 [[SAT]]
// CHECK-NOT: br
// CHECK: ret void
kernel void k(global long *r, long a, long b, long c) { *r = mad_sat(a, b, c); }
