;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; RUN: igc_opt --typed-pointers -enable-debugify -regkey RPEDumpLevel=2 -regkey ForceRPE=2 --igc-registerestimator -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; RegisterEstimator
; ------------------------------------------------

; Check that pressure estimations are done
; CHECK: BB[0]:
; CHECK:   livein simd8|16|32=<32, 64, 128>  maxLive simd8|16|32=<34, 68, 136>
; CHECK:     a {16, 32, 64}, b {16, 32, 64},
; CHECK:   {32, 64, 128}   %1 = add <16 x i32> %a, %b
; CHECK:   {34, 68, 136}   %2 = alloca <16 x i32>, align 4
; CHECK:   {18, 36, 72}   store <16 x i32> %b, <16 x i32>* %2, align 64
; CHECK:   {34, 68, 136}   %3 = load <16 x i32>, <16 x i32>* %2, align 64
; CHECK:   {18, 36, 72}   %4 = mul <16 x i32> %3, %1
; CHECK:   {0, 0, 0}   store <16 x i32> %4, <16 x i32>* %2, align 64
; CHECK:   {0, 0, 0}   ret void

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_register(<16 x i32> %a, <16 x i32> %b) {
  %1 = add <16 x i32> %a, %b
  %2 = alloca <16 x i32>, align 4
  store <16 x i32> %b, <16 x i32>* %2, align 64
  %3 = load <16 x i32>, <16 x i32>* %2, align 64
  %4 = mul <16 x i32> %3, %1
  store <16 x i32> %4, <16 x i32>* %2, align 64
  ret void
}
