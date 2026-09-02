;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; checkStruct's per-field vetoes for aggressive SoA promotion.  A struct is only
; accepted when every field starts on a partition boundary, every nested struct
; passes the same check recursively, and every array field spans a whole number
; of partitions.  Each of the three kernels below violates exactly one of those
; rules and must stay AoS; the last kernel is the positive control that shows
; the flags and the access pattern are otherwise promotable.
;
; EnablePrivMemNewSOATranspose=2 is used so that the "same-sized scalar members
; only" restriction of level 1 does not reject the structs before the per-field
; checks are reached.
;
; RUN: igc_opt --opaque-pointers --ocl --platformPtl --igc-private-mem-resolution \
; RUN:   --regkey EnablePrivMemNewSOATranspose=2,EnableAggressiveSOAPromotion=1 -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; sizeof = 12 (a multiple of the 4-byte partition), but field 2 sits at byte
; offset 5.
%Misaligned = type <{ float, i8, i8, i8, i8, float }>

; sizeof(%OddInner) = 5, which is neither a multiple of nor a divisor of the
; 4-byte partition, so the recursive checkStruct on the nested field fails.
%OddInner = type <{ float, i8 }>
%OuterWithBadInner = type { float, %OddInner }

; The [3 x i8] field starts on a partition boundary (offset 4) but is 3 bytes
; long, so it does not span whole partitions.
%OddArrayField = type { float, [3 x i8] }

; Positive control: every field is partition-aligned and partition-sized.
%Good = type { float, [4 x i8], float }

; CHECK-LABEL: define spir_kernel void @test_misaligned_field_offset(
; CHECK: mul i32 %{{.*}}, 192
; CHECK: getelementptr inbounds [16 x %Misaligned]
define spir_kernel void @test_misaligned_field_offset(ptr addrspace(1) nocapture %d, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %arr = alloca [16 x %Misaligned], align 4
  %idx = zext i32 %ix to i64
  %g = getelementptr inbounds [16 x %Misaligned], ptr %arr, i64 0, i64 %idx, i32 0
  store float 1.000000e+00, ptr %g, align 4
  %v = load float, ptr %g, align 4
  store float %v, ptr addrspace(1) %d, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @test_nested_struct_field_rejected(
; CHECK: mul i32 %{{.*}}, 192
; CHECK: getelementptr inbounds [16 x %OuterWithBadInner]
define spir_kernel void @test_nested_struct_field_rejected(ptr addrspace(1) nocapture %d, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %arr = alloca [16 x %OuterWithBadInner], align 4
  %idx = zext i32 %ix to i64
  %g = getelementptr inbounds [16 x %OuterWithBadInner], ptr %arr, i64 0, i64 %idx, i32 0
  store float 1.000000e+00, ptr %g, align 4
  %v = load float, ptr %g, align 4
  store float %v, ptr addrspace(1) %d, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @test_array_field_size_not_partition_multiple(
; CHECK: mul i32 %{{.*}}, 128
; CHECK: getelementptr inbounds [16 x %OddArrayField]
define spir_kernel void @test_array_field_size_not_partition_multiple(ptr addrspace(1) nocapture %d, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %arr = alloca [16 x %OddArrayField], align 4
  %idx = zext i32 %ix to i64
  %g = getelementptr inbounds [16 x %OddArrayField], ptr %arr, i64 0, i64 %idx, i32 0
  store float 1.000000e+00, ptr %g, align 4
  %v = load float, ptr %g, align 4
  store float %v, ptr addrspace(1) %d, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @test_partition_aligned_fields(
; CHECK: [[LANE:%.*]] = zext i16 %{{.*}} to i32
; CHECK: [[SIMD:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK: mul i32 [[LANE]], 4
; CHECK: mul i32 [[SIMD]], %
; CHECK-NOT: getelementptr inbounds [16 x %Good]
; CHECK: ret void
define spir_kernel void @test_partition_aligned_fields(ptr addrspace(1) nocapture %d, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) {
entry:
  %arr = alloca [16 x %Good], align 4
  %idx = zext i32 %ix to i64
  %g = getelementptr inbounds [16 x %Good], ptr %arr, i64 0, i64 %idx, i32 0
  store float 1.000000e+00, ptr %g, align 4
  %v = load float, ptr %g, align 4
  store float %v, ptr addrspace(1) %d, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!6, !7, !8, !9}

!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 true}
!3 = !{!"FuncMD", !4, !5, !10, !11, !12, !13, !14, !15}
!4 = !{!"FuncMDMap[0]", ptr @test_misaligned_field_offset}
!5 = !{!"FuncMDValue[0]", !2}
!10 = !{!"FuncMDMap[1]", ptr @test_nested_struct_field_rejected}
!11 = !{!"FuncMDValue[1]", !2}
!12 = !{!"FuncMDMap[2]", ptr @test_array_field_size_not_partition_multiple}
!13 = !{!"FuncMDValue[2]", !2}
!14 = !{!"FuncMDMap[3]", ptr @test_partition_aligned_fields}
!15 = !{!"FuncMDValue[3]", !2}
!6 = !{ptr @test_misaligned_field_offset, !408}
!7 = !{ptr @test_nested_struct_field_rejected, !408}
!8 = !{ptr @test_array_field_size_not_partition_multiple, !408}
!9 = !{ptr @test_partition_aligned_fields, !408}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!417 = !{i32 13}
