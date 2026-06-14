;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; RUN: igc_opt --typed-pointers -regkey EnableSelectOfAllocaPtrSplit=1,EnablePrivMemNewSOAForScalarArrays=1 --ocl --platformPtl --igc-private-mem-resolution -S %s | FileCheck %s
;
; Test: SELECT-of-pointer with one alloca-derived operand is split before
; SOALayoutChecker so the underlying [33 x float] alloca becomes SoA.
;
; Models the `cond ? extern_field : stack[index]` idiom that
; otherwise blocks SoA promotion.
;
; Expected transform for LOAD-via-SELECT:
;   %sel = select i1 %c, ptr %ext, ptr %stack
;   %v   = load float, ptr %sel
; becomes:
;   %va  = load float, ptr %ext
;   %vb  = load float, ptr %stack
;   %v   = select i1 %c, %va, %vb
;
; Expected transform for STORE-via-SELECT:
;   %sel = select i1 %c, ptr %ext, ptr %stack
;   store float %x, ptr %sel
; becomes:
;   if (%c) store float %x, ptr %ext  else  store float %x, ptr %stack

; CHECK-LABEL: @test_load_select(
;;
;; Pre-pass: SELECT bypassed -- one load on the extern arm and one load on the
;; SoA-transposed stack arm, with a value-level SELECT merging them.
;;
; CHECK:       call i32 @llvm.genx.GenISA.simdSize()
; CHECK:       load float, float addrspace(4)*
; CHECK:       load float, float*
; CHECK:       select i1 %c, float
;;
; CHECK-LABEL: @test_store_select(
;;
;; STORE arm: SplitBlockAndInsertIfThenElse produces a conditional branch
;; (br i1 %c) with one store in each successor (stack store goes to SoA float*).
;;
; CHECK:       br i1 %c
; CHECK:       store float %x, float addrspace(4)*
; CHECK:       store float %x, float*

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_load_select(float addrspace(1)* nocapture writeonly %d, i8 addrspace(4)* %ext, i1 %c, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* nocapture readnone %privateBase) {
entry:
  %pb = alloca [33 x float], align 4
  %idx = zext i32 %ix to i64
  %g = getelementptr inbounds [33 x float], [33 x float]* %pb, i64 0, i64 %idx
  %g_i8 = bitcast float* %g to i8*
  %g_as4 = addrspacecast i8* %g_i8 to i8 addrspace(4)*
  %sel = select i1 %c, i8 addrspace(4)* %ext, i8 addrspace(4)* %g_as4
  %p_f = bitcast i8 addrspace(4)* %sel to float addrspace(4)*
  %v = load float, float addrspace(4)* %p_f, align 4
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %d, i64 %idx
  store float %v, float addrspace(1)* %arrayidx, align 4
  ret void
}

define spir_kernel void @test_store_select(float addrspace(1)* nocapture writeonly %d, i8 addrspace(4)* %ext, i1 %c, i32 %ix, float %x, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* nocapture readnone %privateBase) {
entry:
  %pb = alloca [33 x float], align 4
  %idx = zext i32 %ix to i64
  %g = getelementptr inbounds [33 x float], [33 x float]* %pb, i64 0, i64 %idx
  %g_i8 = bitcast float* %g to i8*
  %g_as4 = addrspacecast i8* %g_i8 to i8 addrspace(4)*
  %sel = select i1 %c, i8 addrspace(4)* %ext, i8 addrspace(4)* %g_as4
  %p_f = bitcast i8 addrspace(4)* %sel to float addrspace(4)*
  store float %x, float addrspace(4)* %p_f, align 4
  %ldaddr = getelementptr inbounds [33 x float], [33 x float]* %pb, i64 0, i64 %idx
  %v = load float, float* %ldaddr, align 4
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %d, i64 %idx
  store float %v, float addrspace(1)* %arrayidx, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!6, !7}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[1]", void (float addrspace(1)*, i8 addrspace(4)*, i1, i32, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*)* @test_load_select}
!5 = !{!"FuncMDValue[1]", !2}
!6 = !{void (float addrspace(1)*, i8 addrspace(4)*, i1, i32, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*)* @test_load_select, !408}
!7 = !{void (float addrspace(1)*, i8 addrspace(4)*, i1, i32, float, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*)* @test_store_select, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !413, !414, !415, !416, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!413 = !{i32 7}
!414 = !{i32 8}
!415 = !{i32 9}
!416 = !{i32 10}
!417 = !{i32 13}
