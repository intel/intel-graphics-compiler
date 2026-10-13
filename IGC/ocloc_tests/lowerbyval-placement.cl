/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// ShaderDisplayAllPassesNames is unavailable in Release builds.
// UNSUPPORTED: release
// REQUIRES: regkeys, dg2-supported

// RUN: ocloc compile -file %s -options " -igc_opts 'ShaderDisplayAllPassesNames=1 EnableExplicitCopyForByVal=1'"  -device dg2 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-O2
// RUN: ocloc compile -file %s -options "-cl-opt-disable -igc_opts 'ShaderDisplayAllPassesNames=1 EnableExplicitCopyForByVal=1'"  -device dg2 2>&1 | FileCheck %s --check-prefixes=CHECK

// LowerByValAttribute has to run after OptimizeIR: passes there, e.g. MemCpyOpt, rely on byval implying
// a hidden copy and may forward through it. It has to run before memcpy lowering, so that byval temporaries
// made by frontends are still recognizable, and before PrivateMemoryResolution, so that the allocas it
// inserts are resolved. It must run only once, as a second run would copy its own copies again. MemCpyOpt
// must not run again before PrivateMemoryResolution, as it could forward through the copies it inserts.

// CHECK-O2: {{LegacyWrappedMemCpyOpt|MemCpy Optimization}}
// CHECK: LowerByValAttribute
// CHECK-NEXT: ReplaceUnsupportedIntrinsics
// CHECK-NOT: {{LowerByValAttribute|MemCpyOpt|MemCpy Optimization}}
// CHECK: MergeAllocasOCL
// CHECK-NOT: {{LowerByValAttribute|MemCpyOpt|MemCpy Optimization}}
// CHECK: PrivateMemoryResolution
// CHECK-NOT: LowerByValAttribute

__kernel void foo(int a, int b, __global int *res) { *res = a + b; }
