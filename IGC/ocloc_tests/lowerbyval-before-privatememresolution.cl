/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// REQUIRES: regkeys, dg2-supported

// RUN: ocloc compile -file %s -options " -igc_opts 'ShaderDisplayAllPassesNames=1 EnableExplicitCopyForByVal=1'"  -device dg2 2>&1 | FileCheck %s

// Verify if LowerByValAttribute pass is run right before PrivateMemoryResolution pass.
// (ReplaceUnsupportedIntrinsics is run between them just to lower memcpy instructions inserted by LowerByValAttribute)

// If LowerByValAttribute was run after PrivateMemoryResolution, alloca instructions inserted by LowerByValAttribute
// wouldn't be resolved.
// If LowerByValAttribute was run earlier than right before PrivateMemoryResolution, there is a chance that
// optimization passes would remove explicit copy (alloca + memcpy) inserted by LowerByValAttribute.

// CHECK: LowerByValAttribute
// CHECK-NEXT: ReplaceUnsupportedIntrinsics
// CHECK-NEXT: PrivateMemoryResolution

__kernel void foo(int a, int b, __global int *res) { *res = a + b; }
