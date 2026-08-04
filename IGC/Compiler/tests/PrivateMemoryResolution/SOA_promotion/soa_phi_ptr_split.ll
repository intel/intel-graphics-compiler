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
; RUN: igc_opt --typed-pointers --ocl --platformPtl --igc-split-phis-of-alloca-pointers --igc-private-mem-resolution \
; RUN:   --regkey EnablePHIOfAllocaPtrSplit=1 -S %s | FileCheck %s
;
; Tests SoA promotion of the workload pattern `if (is_stack_slot) stack[idx] else svm_data[i]`
; diamond, whose pointer-merge PHI otherwise blocks SoA promotion.
;
; Expected transform:
;   bb_a:  %pa = ... (stack-derived)
;   bb_b:  %pb = ... (extern-derived)
;   join:  %p = phi i8 as(4)* [ %pa, bb_a ], [ %pb, bb_b ]
;          %v = load float, ptr %p
; becomes:
;   bb_a:  %va = load float, ptr (cast %pa)
;   bb_b:  %vb = load float, ptr (cast %pb)
;   join:  %v  = phi float [ %va, bb_a ], [ %vb, bb_b ]
;
; The new load on the stack predecessor targets a plain `float*` (private AS),
; which the subsequent SOALayoutChecker accepts and PMR transposes to SoA.

; CHECK-LABEL: @test_load_phi(
; CHECK:       call i32 @llvm.genx.GenISA.simdSize()
; CHECK:       load float, float*
; CHECK:       load float, float addrspace(4)*
; CHECK:       phi float
; CHECK-NOT:   phi i8 addrspace(4)*

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_load_phi(float addrspace(1)* nocapture writeonly %d, i8 addrspace(4)* %ext, i1 %c, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* nocapture readnone %privateBase) {
entry:
  %pb = alloca [33 x float], align 4
  %idx = zext i32 %ix to i64
  br i1 %c, label %bb_stack, label %bb_extern

bb_stack:
  %g = getelementptr inbounds [33 x float], [33 x float]* %pb, i64 0, i64 %idx
  %g_i8 = bitcast float* %g to i8*
  %g_as4 = addrspacecast i8* %g_i8 to i8 addrspace(4)*
  br label %join

bb_extern:
  %ext_off = getelementptr inbounds i8, i8 addrspace(4)* %ext, i64 4
  br label %join

join:
  %p = phi i8 addrspace(4)* [ %g_as4, %bb_stack ], [ %ext_off, %bb_extern ]
  %p_f = bitcast i8 addrspace(4)* %p to float addrspace(4)*
  %v = load float, float addrspace(4)* %p_f, align 4
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %d, i64 %idx
  store float %v, float addrspace(1)* %arrayidx, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!6}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", void (float addrspace(1)*, i8 addrspace(4)*, i1, i32, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*)* @test_load_phi}
!5 = !{!"FuncMDValue[0]", !2}
!6 = !{void (float addrspace(1)*, i8 addrspace(4)*, i1, i32, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*)* @test_load_phi, !408}
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
