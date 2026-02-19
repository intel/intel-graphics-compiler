;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-constant-coalescing | FileCheck %s

; The current version of ConstantCoalescing pass doesn't have an ability to optimize
; ldrawvector instructions that read global pointer from memory. This test has been
; implemented to confirm that the pass is able to detect that case, leave the module
; unchanged and bail out without any crash.
; TODO: Implement support for ldrawvector instructions that read global pointer from memory

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define <5 x i32 addrspace(1)*> @f0(i32 %src) {
entry:
  %0 = inttoptr i32 %src to <2 x i32 addrspace(1)*> addrspace(2490368)*
  %1 = call <2 x i32 addrspace(1)*> @llvm.genx.GenISA.ldrawvector.indexed.v2p1i32.p2490368v2p1i32(<2 x i32 addrspace(1)*> addrspace(2490368)* %0, i32 %src, i32 8, i1 false)
  %2 = add i32 %src, 32
  %3 = call <2 x i32 addrspace(1)*> @llvm.genx.GenISA.ldrawvector.indexed.v2p1i32.p2490368v2p1i32(<2 x i32 addrspace(1)*> addrspace(2490368)* %0, i32 %2, i32 8, i1 false)
  %4 = extractelement <2 x i32 addrspace(1)*> %1, i32 0
  %5 = extractelement <2 x i32 addrspace(1)*> %1, i32 1
  %6 = extractelement <2 x i32 addrspace(1)*> %3, i32 0
  %7 = extractelement <2 x i32 addrspace(1)*> %3, i32 1
  %8 = insertelement <5 x i32 addrspace(1)*> undef, i32 addrspace(1)* %4, i32 0
  %9 = insertelement <5 x i32 addrspace(1)*> %8, i32 addrspace(1)* %5, i32 1
  %10 = insertelement <5 x i32 addrspace(1)*> %9, i32 addrspace(1)* %6, i32 2
  %11 = insertelement <5 x i32 addrspace(1)*> %10, i32 addrspace(1)* %7, i32 3
  %12 = inttoptr i32 %src to i32 addrspace(1)* addrspace(2490368)*
  %13 = add i32 %2, 32
  %14 = call i32 addrspace(1)* @llvm.genx.GenISA.ldrawvector.indexed.p1i32.p2490368p1i32(i32 addrspace(1)* addrspace(2490368)* %12, i32 %13, i32 8, i1 false)
  %15 = insertelement <5 x i32 addrspace(1)*> %10, i32 addrspace(1)* %14, i32 5
  ret <5 x i32 addrspace(1)*> %15
}

; CHECK-LABEL: <5 x i32 addrspace(1)*> @f0(i32 %src)
; CHECK: call <2 x i32 addrspace(1)*> @llvm.genx.GenISA.ldrawvector.indexed.v2p1i32.p2490368v2p1i32(<2 x i32 addrspace(1)*> addrspace(2490368)* %0, i32 %src, i32 8, i1 false)
; CHECK: call <2 x i32 addrspace(1)*> @llvm.genx.GenISA.ldrawvector.indexed.v2p1i32.p2490368v2p1i32(<2 x i32 addrspace(1)*> addrspace(2490368)* %0, i32 %2, i32 8, i1 false)
; CHECK: call i32 addrspace(1)* @llvm.genx.GenISA.ldrawvector.indexed.p1i32.p2490368p1i32(i32 addrspace(1)* addrspace(2490368)* %12, i32 %13, i32 8, i1 false)

declare <2 x i32 addrspace(1)*> @llvm.genx.GenISA.ldrawvector.indexed.v2p1i32.p2490368v2p1i32(<2 x i32 addrspace(1)*> addrspace(2490368)*, i32, i32, i1)
declare i32 addrspace(1)* @llvm.genx.GenISA.ldrawvector.indexed.p1i32.p2490368p1i32(i32 addrspace(1)* addrspace(2490368)*, i32, i32, i1)

!igc.functions = !{!0}

!0 = !{<5 x i32 addrspace(1)*> (i32)* @f0, !1}

!1 = !{!2}
!2 = !{!"function_type", i32 0}
