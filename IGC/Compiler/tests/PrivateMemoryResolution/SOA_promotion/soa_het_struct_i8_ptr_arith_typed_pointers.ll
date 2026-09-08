;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; Typed-pointer byte-address arithmetic on a heterogeneous-struct alloca:
; struct* -> i8*, a constant byte GEP, then i8* -> struct* again.
;
; RUN: igc_opt --typed-pointers --ocl --platformPtl --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=1,EnableAggressiveSOAPromotion=1 -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%S = type { float, i32 }

; The alloca is SOA-promoted: the byte offset 16 reaches element 2 of the array,
; so its two fields land in the partitions at per-lane byte offsets 16 and 20.
; The variable-index load computes its chunk from the byte offset instead
; (element stride 8, field 1 at +4, chunk number via lshr 2).
;
; CHECK-LABEL: @test_i8_ptr_arith(
; CHECK:       [[SIMDSIZE:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK:       mul i32 [[SIMDSIZE]], 16
; CHECK:       store float 1.000000e+00, float* %{{.*}}, align 4
; CHECK:       mul i32 [[SIMDSIZE]], 20
; CHECK:       store i32 7, i32* %{{.*}}, align 4
; CHECK:       [[OFF:%.*]] = mul nsw i32 %{{.*}}, 8
; CHECK:       [[OFF4:%.*]] = add nsw i32 [[OFF]], 4
; CHECK:       lshr i32 [[OFF4]], 2
; CHECK:       load i32, i32* %{{.*}}, align 4
; CHECK-NOT:   bitcast
define spir_kernel void @test_i8_ptr_arith(i32 addrspace(1)* nocapture %d, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase) {
entry:
  %arr = alloca [64 x %S], align 8
  %arr8 = bitcast [64 x %S]* %arr to i8*
  %off = getelementptr inbounds i8, i8* %arr8, i64 16
  %sp = bitcast i8* %off to %S*
  %f0 = getelementptr inbounds %S, %S* %sp, i64 0, i32 0
  store float 1.000000e+00, float* %f0, align 4
  %f1 = getelementptr inbounds %S, %S* %sp, i64 0, i32 1
  store i32 7, i32* %f1, align 4
  %idx = zext i32 %ix to i64
  %g = getelementptr inbounds [64 x %S], [64 x %S]* %arr, i64 0, i64 %idx, i32 1
  %v = load i32, i32* %g, align 4
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %idx
  store i32 %v, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!6}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, i32, <8 x i32>, <8 x i32>, i8*)* @test_i8_ptr_arith}
!5 = !{!"FuncMDValue[0]", !2}
!6 = !{void (i32 addrspace(1)*, i32, <8 x i32>, <8 x i32>, i8*)* @test_i8_ptr_arith, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!417 = !{i32 13}
