;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers --ocl --platformPtl --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=1,EnableAggressiveSOAPromotion=1,EnableSOAFallbackToOldAlgorithm=0 \
; RUN:   -S %s | FileCheck %s --check-prefix=NEW
; RUN: igc_opt --opaque-pointers --ocl --platformPtl --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=0 \
; RUN:   -S %s | FileCheck %s --check-prefix=LEG
;
; Check that a SoA transposed access doesn't overestimate alignment.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%S4 = type { i32, float, i32, float }
%S8 = type { i64, i64 }

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0.i32(ptr, i64, i1, i32)

; 4-byte partition: an i32 field declared align 16 is only 4-aligned after the transpose.
;
; NEW-LABEL: @new_overaligned(
; NEW-NOT:   alloca
; NEW:       store i32 7, ptr %{{.*}}, align 4
; NEW:       load i32, ptr %{{.*}}, align 4
; NEW:       ret void
define spir_kernel void @new_overaligned(ptr addrspace(1) %out, i32 %i, <8 x i32> %r0, <8 x i32> %payloadHeader,
                                         ptr %privateBase) {
entry:
  %a = alloca [4 x %S4], align 16
  %p = getelementptr [4 x %S4], ptr %a, i32 0, i32 %i, i32 0
  store i32 7, ptr %p, align 16
  %v = load i32, ptr %p, align 16
  store i32 %v, ptr addrspace(1) %out, align 4
  ret void
}

; An alignment already below the lane size is kept as is.
;
; NEW-LABEL: @new_underaligned_kept(
; NEW-NOT:   alloca
; NEW:       store float 1.000000e+00, ptr %{{.*}}, align 2
; NEW:       load float, ptr %{{.*}}, align 2
; NEW:       ret void
define spir_kernel void @new_underaligned_kept(ptr addrspace(1) %out, i32 %i, <8 x i32> %r0,
                                               <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %a = alloca [4 x %S4], align 16
  %p = getelementptr [4 x %S4], ptr %a, i32 0, i32 %i, i32 1
  store float 1.0, ptr %p, align 2
  %v = load float, ptr %p, align 2
  store float %v, ptr addrspace(1) %out, align 4
  ret void
}

; 8-byte partition (i64 fields): the cap is the partition, not a fixed 4 bytes.
;
; NEW-LABEL: @new_overaligned_partition8(
; NEW-NOT:   alloca
; NEW:       store i64 7, ptr %{{.*}}, align 8
; NEW:       load i64, ptr %{{.*}}, align 8
; NEW:       ret void
define spir_kernel void @new_overaligned_partition8(ptr addrspace(1) %out, i32 %i, <8 x i32> %r0,
                                                    <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %a = alloca [2 x %S8], align 16
  %p = getelementptr [2 x %S8], ptr %a, i32 0, i32 %i, i32 0
  store i64 7, ptr %p, align 16
  %v = load i64, ptr %p, align 16
  store i64 %v, ptr addrspace(1) %out, align 8
  ret void
}

; The alignment operand of a single-chunk predicated load is capped the same way.
;
; NEW-LABEL: @new_predicated_overaligned(
; NEW-NOT:   alloca
; NEW:       call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0.i32(ptr %{{.*}}, i64 4, i1 %pred, i32 %merge)
; NEW:       ret void
define spir_kernel void @new_predicated_overaligned(ptr addrspace(1) %out, i32 %i, i1 %pred, i32 %merge,
                                                    <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %a = alloca [4 x %S4], align 16
  %p = getelementptr [4 x %S4], ptr %a, i32 0, i32 %i, i32 0
  store i32 7, ptr %p, align 4
  %v = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0.i32(ptr %p, i64 16, i1 %pred, i32 %merge)
  store i32 %v, ptr addrspace(1) %out, align 4
  ret void
}

; Legacy transpose with a scalar base element: the lane size is sizeof(float).
;
; LEG-LABEL: @legacy_scalar_base(
; LEG-NOT:   alloca
; LEG:       store float 1.000000e+00, ptr %{{.*}}, align 4
; LEG:       load float, ptr %{{.*}}, align 4
; LEG:       ret void
define spir_kernel void @legacy_scalar_base(ptr addrspace(1) %out, i32 %i, <8 x i32> %r0,
                                            <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %a = alloca [8 x float], align 16
  %p = getelementptr [8 x float], ptr %a, i32 0, i32 %i
  store float 1.0, ptr %p, align 16
  %v = load float, ptr %p, align 16
  store float %v, ptr addrspace(1) %out, align 4
  ret void
}

; Legacy transpose with a vector base element: the lane size is sizeof(<4 x float>).
;
; LEG-LABEL: @legacy_vector_base(
; LEG-NOT:   alloca
; LEG:       store <4 x float> zeroinitializer, ptr %{{.*}}, align 16
; LEG:       load <4 x float>, ptr %{{.*}}, align 16
; LEG:       ret void
define spir_kernel void @legacy_vector_base(ptr addrspace(1) %out, i32 %i, <8 x i32> %r0,
                                            <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %a = alloca [2 x <4 x float>], align 32
  %p = getelementptr [2 x <4 x float>], ptr %a, i32 0, i32 %i
  store <4 x float> zeroinitializer, ptr %p, align 32
  %v = load <4 x float>, ptr %p, align 32
  store <4 x float> %v, ptr addrspace(1) %out, align 16
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!20, !21, !22, !23, !24, !25}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5, !6, !7, !8, !9, !10, !11, !12, !13, !14, !15}
!4 = !{!"FuncMDMap[0]", ptr @new_overaligned}
!5 = !{!"FuncMDValue[0]", !2}
!6 = !{!"FuncMDMap[1]", ptr @new_underaligned_kept}
!7 = !{!"FuncMDValue[1]", !2}
!8 = !{!"FuncMDMap[2]", ptr @new_overaligned_partition8}
!9 = !{!"FuncMDValue[2]", !2}
!10 = !{!"FuncMDMap[3]", ptr @new_predicated_overaligned}
!11 = !{!"FuncMDValue[3]", !2}
!12 = !{!"FuncMDMap[4]", ptr @legacy_scalar_base}
!13 = !{!"FuncMDValue[4]", !2}
!14 = !{!"FuncMDMap[5]", ptr @legacy_vector_base}
!15 = !{!"FuncMDValue[5]", !2}
!20 = !{ptr @new_overaligned, !408}
!21 = !{ptr @new_underaligned_kept, !408}
!22 = !{ptr @new_overaligned_partition8, !408}
!23 = !{ptr @new_predicated_overaligned, !408}
!24 = !{ptr @legacy_scalar_base, !408}
!25 = !{ptr @legacy_vector_base, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!417 = !{i32 13}
