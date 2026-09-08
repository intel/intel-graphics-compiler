;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus
; RUN: igc_opt -S --opaque-pointers --igc-vectorizer -dce --platformbmg < %s 2>&1 | FileCheck %s

; The 4-wide seed in %loop packs the loop carried %or0..%or3 into a <4 x i32>
; insert chain. The 2-wide seed in %exit then forms a narrower slice over the
; first two of them, so that wide vector must not be reused as the incoming
; value of a <2 x i32> PHI.

define spir_kernel void @test_narrow_slice_over_wide_vector(ptr addrspace(3) %dst, i1 %cond) {
; CHECK-LABEL: @test_narrow_slice_over_wide_vector(
; CHECK:       loop:
; CHECK:         [[NARROW:%.*]] = insertelement <2 x i32> {{.*}}, i32 %or1, i32 1
; CHECK:       exit:
; CHECK:         phi <2 x i32> [ zeroinitializer, %entry ], [ [[NARROW]], %loop ]
entry:
  br i1 %cond, label %loop, label %exit

loop:                                             ; preds = %loop, %entry
  %phi0 = phi i32 [ 0, %entry ], [ %or0, %loop ]
  %phi1 = phi i32 [ 0, %entry ], [ %or1, %loop ]
  %phi2 = phi i32 [ 0, %entry ], [ %or2, %loop ]
  %phi3 = phi i32 [ 0, %entry ], [ %or3, %loop ]
  %acc0 = insertelement <4 x i32> zeroinitializer, i32 %phi0, i64 0
  %acc1 = insertelement <4 x i32> %acc0, i32 %phi1, i64 1
  %acc2 = insertelement <4 x i32> %acc1, i32 %phi2, i64 2
  %acc3 = insertelement <4 x i32> %acc2, i32 %phi3, i64 3
  %dpas = call <4 x i32> @llvm.genx.GenISA.sub.group.dpas.v4i32.v4i32.v8i16.v8i32(<4 x i32> %acc3, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %ext0 = extractelement <4 x i32> %dpas, i64 0
  %ext1 = extractelement <4 x i32> %dpas, i64 1
  %ext2 = extractelement <4 x i32> %dpas, i64 2
  %ext3 = extractelement <4 x i32> %dpas, i64 3
  %or0 = or i32 %ext0, 1
  %or1 = or i32 %ext1, 1
  %or2 = or i32 %ext2, 1
  %or3 = or i32 %ext3, 1
  br i1 %cond, label %loop, label %exit

exit:                                             ; preds = %loop, %entry
  %lcssa0 = phi i32 [ 0, %entry ], [ %or0, %loop ]
  %lcssa1 = phi i32 [ 0, %entry ], [ %or1, %loop ]
  %store0 = insertelement <2 x i32> zeroinitializer, i32 %lcssa0, i64 0
  %store1 = insertelement <2 x i32> %store0, i32 %lcssa1, i64 1
  store <2 x i32> %store1, ptr addrspace(3) %dst, align 8
  ret void
}

declare <4 x i32> @llvm.genx.GenISA.sub.group.dpas.v4i32.v4i32.v8i16.v8i32(<4 x i32>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

!igc.functions = !{!0}
!0 = !{void (ptr addrspace(3), i1)* @test_narrow_slice_over_wide_vector, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"requiredSubGroupSize", i32 16}
!5 = !{!"FuncMDValue[0]", !4}
!6 = !{!"FuncMDMap[0]", void (ptr addrspace(3), i1)* @test_narrow_slice_over_wide_vector}
!7 = !{!"FuncMD", !6, !5}
!8 = !{!"ModuleMD", !7}
!IGCMetadata = !{!8}
