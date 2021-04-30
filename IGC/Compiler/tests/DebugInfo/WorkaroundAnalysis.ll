;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; This test marked as XFAIL until the dependancy of WorkaroundAnalysis pass on CodeGencontext will be removed
; XFAIL: *
; RUN: opt -igc-workaround -enable-fmax-fmin-plus-zero -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that WorkaroundAnalysis pass handles line debug info.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a64:64:64-f80:128:128-n8:16:32:64"
target triple = "igil_32_GEN8"

define void @test(float addrspace(1)* %dst, float %x, float %y) #0 {
entry:
  %res = call float @llvm.genx.GenISA.min.f32(float %x, float %y), !dbg !1
  store float %res, float addrspace(1)* %dst, align 4, !dbg !2
  ret void

; CHECK: [[x:%[a-zA-Z0-9_]+]] = fadd float %x, 0.000000e+00, !dbg !1
; CHECK: [[y:%[a-zA-Z0-9_]+]] = fadd float %y, 0.000000e+00, !dbg !1
; CHECK: [[res:%[a-zA-Z0-9_]+]] = call float @llvm.genx.GenISA.min.f32(float [[x]], float [[y]]), !dbg !1
}

declare float @llvm.genx.GenISA.min.f32(float, float) #1

attributes #0 = { alwaysinline nounwind }
attributes #1 = { nounwind readnone }

;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2}


!0 = metadata !{}
!1 = metadata !{i32 1, i32 0, metadata !0, null}
!2 = metadata !{i32 3, i32 0, metadata !0, null}
