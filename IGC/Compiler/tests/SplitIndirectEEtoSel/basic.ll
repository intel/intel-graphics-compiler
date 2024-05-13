;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -enable-debugify -SplitIndirectEEtoSel -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; SplitIndirectEEtoSel
; ------------------------------------------------
; Debug-info related check
;
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS
define void @test_nuw(i32 %src1, <12 x float> %src2, float* %dst) {
; CHECK-LABEL: @test_nuw(
; CHECK:    [[TMP1:%.*]] = icmp eq i32 [[SRC1:%.*]], 0
; CHECK:    [[TMP2:%.*]] = extractelement <12 x float> [[SRC2:%.*]], i32 0
; CHECK:    [[TMP3:%.*]] = select i1 [[TMP1]], float [[TMP2]], float undef
; CHECK:    [[TMP4:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP5:%.*]] = extractelement <12 x float> [[SRC2]], i32 3
; CHECK:    [[TMP6:%.*]] = select i1 [[TMP4]], float [[TMP5]], float [[TMP3]]
; CHECK:    [[TMP7:%.*]] = icmp eq i32 [[SRC1]], 2
; CHECK:    [[TMP8:%.*]] = extractelement <12 x float> [[SRC2]], i32 6
; CHECK:    [[TMP9:%.*]] = select i1 [[TMP7]], float [[TMP8]], float [[TMP6]]
; CHECK:    [[TMP10:%.*]] = icmp eq i32 [[SRC1]], 3
; CHECK:    [[TMP11:%.*]] = extractelement <12 x float> [[SRC2]], i32 9
; CHECK:    [[TMP12:%.*]] = select i1 [[TMP10]], float [[TMP11]], float [[TMP9]]
; CHECK:    store float [[TMP12]], float* [[DST:%.*]], align 4
; CHECK:    [[TMP13:%.*]] = add i32 [[TMP18:%.*]], 1
; CHECK:    [[TMP14:%.*]] = extractelement <12 x float> [[SRC2]], i32 [[TMP13]]
; CHECK:    store float [[TMP14]], float* [[DST]], align 4
; CHECK:    [[TMP15:%.*]] = add i32 [[TMP18]], 2
; CHECK:    [[TMP17:%.*]] = extractelement <12 x float> [[SRC2]], i32 [[TMP15]]
; CHECK:    store float [[TMP17]], float* [[DST]], align 4
; CHECK:    ret void
;
  %1 = mul nuw i32 %src1, 3
  %2 = extractelement <12 x float> %src2, i32 %1
  store float %2, float* %dst, align 4
  %3 = add i32 %1, 1
  %4 = extractelement <12 x float> %src2, i32 %3
  store float %4, float* %dst, align 4
  %5 = add i32 %1, 2
  %6 = extractelement <12 x float> %src2, i32 %5
  store float %6, float* %dst, align 4
  ret void
}

define void @test_nsw(i32 %src1, <12 x float> %src2, float* %dst) {
; CHECK-LABEL: @test_nsw(
; CHECK:    [[TMP1:%.*]] = icmp eq i32 [[SRC1:%.*]], 0
; CHECK:    [[TMP2:%.*]] = extractelement <12 x float> [[SRC2:%.*]], i32 0
; CHECK:    [[TMP3:%.*]] = select i1 [[TMP1]], float [[TMP2]], float undef
; CHECK:    [[TMP4:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP5:%.*]] = extractelement <12 x float> [[SRC2]], i32 3
; CHECK:    [[TMP6:%.*]] = select i1 [[TMP4]], float [[TMP5]], float [[TMP3]]
; CHECK:    [[TMP7:%.*]] = icmp eq i32 [[SRC1]], 2
; CHECK:    [[TMP8:%.*]] = extractelement <12 x float> [[SRC2]], i32 6
; CHECK:    [[TMP9:%.*]] = select i1 [[TMP7]], float [[TMP8]], float [[TMP6]]
; CHECK:    [[TMP10:%.*]] = icmp eq i32 [[SRC1]], 3
; CHECK:    [[TMP11:%.*]] = extractelement <12 x float> [[SRC2]], i32 9
; CHECK:    [[TMP12:%.*]] = select i1 [[TMP10]], float [[TMP11]], float [[TMP9]]
; CHECK:    store float [[TMP12]], float* [[DST:%.*]], align 4
; CHECK:    [[TMP13:%.*]] = add i32 [[TMP18:%.*]], 1
; CHECK:    [[TMP14:%.*]] = extractelement <12 x float> [[SRC2]], i32 [[TMP13]]
; CHECK:    store float [[TMP14]], float* [[DST]], align 4
; CHECK:    [[TMP15:%.*]] = add i32 [[TMP18]], 2
; CHECK:    [[TMP17:%.*]] = extractelement <12 x float> [[SRC2]], i32 [[TMP15]]
; CHECK:    store float [[TMP17]], float* [[DST]], align 4
; CHECK:    ret void
;
  %1 = mul nsw i32 %src1, 3
  %2 = extractelement <12 x float> %src2, i32 %1
  store float %2, float* %dst, align 4
  %3 = add i32 %1, 1
  %4 = extractelement <12 x float> %src2, i32 %3
  store float %4, float* %dst, align 4
  %5 = add i32 %1, 2
  %6 = extractelement <12 x float> %src2, i32 %5
  store float %6, float* %dst, align 4
  ret void
}

define void @test(i32 %src1, <12 x float> %src2, float* %dst) {
; CHECK-LABEL: @test(
; CHECK:    [[TMP1:%.*]] = mul i32 [[SRC1:%.*]], 3
; CHECK:    [[TMP2:%.*]] = extractelement <12 x float> [[SRC2:%.*]], i32 [[TMP1]]
; CHECK:    store float [[TMP2]], float* [[DST:%.*]], align 4
; CHECK:    [[TMP3:%.*]] = add i32 [[TMP1]], 1
; CHECK:    [[TMP4:%.*]] = extractelement <12 x float> [[SRC2]], i32 [[TMP3]]
; CHECK:    store float [[TMP4]], float* [[DST]], align 4
; CHECK:    [[TMP5:%.*]] = add i32 [[TMP1]], 2
; CHECK:    [[TMP6:%.*]] = extractelement <12 x float> [[SRC2]], i32 [[TMP5]]
; CHECK:    store float [[TMP6]], float* [[DST]], align 4
; CHECK:    ret void
;
  %1 = mul i32 %src1, 3
  %2 = extractelement <12 x float> %src2, i32 %1
  store float %2, float* %dst, align 4
  %3 = add i32 %1, 1
  %4 = extractelement <12 x float> %src2, i32 %3
  store float %4, float* %dst, align 4
  %5 = add i32 %1, 2
  %6 = extractelement <12 x float> %src2, i32 %5
  store float %6, float* %dst, align 4
  ret void
}

define void @test_no_match(<4 x i32> %vec) {
; CHECK-LABEL: @test_no_match(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    br label %body
; CHECK:       body:
; CHECK-NEXT:    [[TMP0:%.*]] = phi i32 [ 0, [[ENTRY:%.*]] ], [ [[TMP3:%.*]], [[LATCH:%.*]] ]
; CHECK:         [[TMP1:%.*]] = extractelement <4 x i32> [[VEC:%.*]], i32 [[TMP0]]
; CHECK:         [[TMP2:%.*]] = icmp ne i32 [[TMP1]], 0
; CHECK:         br label [[LATCH]]
; CHECK:       latch:
; CHECK-NEXT:    [[TMP3]] = add nuw nsw i32 [[TMP0]], 4
; CHECK:         [[TMP4:%.*]] = icmp ult i32 [[TMP0]], 12
; CHECK:         br i1 [[TMP4]], label %body, label [[EXIT:%.*]]
; CHECK:       exit:
; CHECK-NEXT:    ret void
;
entry:
  br label %body

body:
  %0 = phi i32 [ 0, %entry ], [ %3, %latch ]
  %1 = extractelement <4 x i32> %vec, i32 %0
  %2 = icmp ne i32 %1, 0
  br label %latch

latch:
  %3 = add nuw nsw i32 %0, 4
  %4 = icmp ult i32 %0, 12
  br i1 %4, label %body, label %exit

exit:
  ret void
}
