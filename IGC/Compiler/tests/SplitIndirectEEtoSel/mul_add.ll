;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify -SplitIndirectEEtoSel -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; SplitIndirectEEtoSel
; ------------------------------------------------
;
; This test checks mul+add pattern for SplitIndirectEEtoSel pass
; ------------------------------------------------

; Debug-info related check
;
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS
; ------------------------------------------------
; Case1: Mul pattern matched
;        flags: none, nsw, nuw, both
; ------------------------------------------------

define void @test_mul(i32 %src1, <12 x float> %src2, float* %dst) {
none:
; CHECK-LABEL: @test_mul(
; CHECK:    [[TMP0:%.*]] = mul i32 [[SRC1:%.*]], 3
; CHECK:    [[TMP1:%.*]] = extractelement <12 x float> [[SRC2:%.*]], i32 [[TMP0]]
; CHECK:    store float [[TMP1]], float* [[DST:%.*]], align 4
;
  %0 = mul i32 %src1, 3
  %1 = extractelement <12 x float> %src2, i32 %0
  store float %1, float* %dst, align 4
  br label %nsw
nsw:
; CHECK:    [[TMP2:%.*]] = icmp eq i32 [[SRC1]], 0
; CHECK:    [[TMP3:%.*]] = extractelement <12 x float> [[SRC2]], i32 0
; CHECK:    [[TMP4:%.*]] = select i1 [[TMP2]], float [[TMP3]], float undef
; CHECK:    [[TMP5:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP6:%.*]] = extractelement <12 x float> [[SRC2]], i32 3
; CHECK:    [[TMP7:%.*]] = select i1 [[TMP5]], float [[TMP6]], float [[TMP4]]
; CHECK:    [[TMP8:%.*]] = icmp eq i32 [[SRC1]], 2
; CHECK:    [[TMP9:%.*]] = extractelement <12 x float> [[SRC2]], i32 6
; CHECK:    [[TMP10:%.*]] = select i1 [[TMP8]], float [[TMP9]], float [[TMP7]]
; CHECK:    [[TMP11:%.*]] = icmp eq i32 [[SRC1]], 3
; CHECK:    [[TMP12:%.*]] = extractelement <12 x float> [[SRC2]], i32 9
; CHECK:    [[TMP13:%.*]] = select i1 [[TMP11]], float [[TMP12]], float [[TMP10]]
; CHECK:    store float [[TMP13]], float* [[DST]], align 4
;
  %2 = mul nsw i32 %src1, 3
  %3 = extractelement <12 x float> %src2, i32 %2
  store float %3, float* %dst, align 4
  br label %nuw
nuw:
; CHECK:    [[TMP14:%.*]] = icmp eq i32 [[SRC1]], 0
; CHECK:    [[TMP15:%.*]] = extractelement <12 x float> [[SRC2]], i32 0
; CHECK:    [[TMP16:%.*]] = select i1 [[TMP14]], float [[TMP15]], float undef
; CHECK:    [[TMP17:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP18:%.*]] = extractelement <12 x float> [[SRC2]], i32 3
; CHECK:    [[TMP19:%.*]] = select i1 [[TMP17]], float [[TMP18]], float [[TMP16]]
; CHECK:    [[TMP20:%.*]] = icmp eq i32 [[SRC1]], 2
; CHECK:    [[TMP21:%.*]] = extractelement <12 x float> [[SRC2]], i32 6
; CHECK:    [[TMP22:%.*]] = select i1 [[TMP20]], float [[TMP21]], float [[TMP19]]
; CHECK:    [[TMP23:%.*]] = icmp eq i32 [[SRC1]], 3
; CHECK:    [[TMP24:%.*]] = extractelement <12 x float> [[SRC2]], i32 9
; CHECK:    [[TMP25:%.*]] = select i1 [[TMP23]], float [[TMP24]], float [[TMP22]]
; CHECK:    store float [[TMP25]], float* [[DST]], align 4
;
  %4 = mul nuw i32 %src1, 3
  %5 = extractelement <12 x float> %src2, i32 %4
  store float %5, float* %dst, align 4
  br label %both
both:
; CHECK:    [[TMP26:%.*]] = icmp eq i32 [[SRC1]], 0
; CHECK:    [[TMP27:%.*]] = extractelement <12 x float> [[SRC2]], i32 0
; CHECK:    [[TMP28:%.*]] = select i1 [[TMP26]], float [[TMP27]], float undef
; CHECK:    [[TMP29:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP30:%.*]] = extractelement <12 x float> [[SRC2]], i32 3
; CHECK:    [[TMP31:%.*]] = select i1 [[TMP29]], float [[TMP30]], float [[TMP28]]
; CHECK:    [[TMP32:%.*]] = icmp eq i32 [[SRC1]], 2
; CHECK:    [[TMP33:%.*]] = extractelement <12 x float> [[SRC2]], i32 6
; CHECK:    [[TMP34:%.*]] = select i1 [[TMP32]], float [[TMP33]], float [[TMP31]]
; CHECK:    [[TMP35:%.*]] = icmp eq i32 [[SRC1]], 3
; CHECK:    [[TMP36:%.*]] = extractelement <12 x float> [[SRC2]], i32 9
; CHECK:    [[TMP37:%.*]] = select i1 [[TMP35]], float [[TMP36]], float [[TMP34]]
; CHECK:    store float [[TMP37]], float* [[DST]], align 4
;
  %6 = mul nuw nsw i32 %src1, 3
  %7 = extractelement <12 x float> %src2, i32 %6
  store float %7, float* %dst, align 4
  ret void
}

; ------------------------------------------------
; Case2: Add + mul pattern matched
;        flags on add: none, nsw, nuw
;        flags on mul: nsw
; ------------------------------------------------

define void @test_mul_add(i32 %src1, <12 x float> %src2, float* %dst) {
none:
; CHECK-LABEL: @test_mul_add(
; CHECK:    [[TMP0:%.*]] = mul nsw i32 [[SRC1:%.*]], 3
; CHECK:    [[TMP1:%.*]] = add i32 [[TMP0]], 1
; CHECK:    [[TMP2:%.*]] = extractelement <12 x float> [[SRC2:%.*]], i32 [[TMP1]]
; CHECK:    store float [[TMP2]], float* [[DST:%.*]], align 4
;
  %0 = mul nsw i32 %src1, 3
  %1 = add i32 %0, 1
  %2 = extractelement <12 x float> %src2, i32 %1
  store float %2, float* %dst, align 4
  br label %nsw
nsw:
; CHECK:    [[TMP3:%.*]] = icmp eq i32 [[SRC1]], 0
; CHECK:    [[TMP4:%.*]] = extractelement <12 x float> [[SRC2]], i32 1
; CHECK:    [[TMP5:%.*]] = select i1 [[TMP3]], float [[TMP4]], float undef
; CHECK:    [[TMP6:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP7:%.*]] = extractelement <12 x float> [[SRC2]], i32 4
; CHECK:    [[TMP8:%.*]] = select i1 [[TMP6]], float [[TMP7]], float [[TMP5]]
; CHECK:    [[TMP9:%.*]] = icmp eq i32 [[SRC1]], 2
; CHECK:    [[TMP10:%.*]] = extractelement <12 x float> [[SRC2]], i32 7
; CHECK:    [[TMP11:%.*]] = select i1 [[TMP9]], float [[TMP10]], float [[TMP8]]
; CHECK:    [[TMP12:%.*]] = icmp eq i32 [[SRC1]], 3
; CHECK:    [[TMP13:%.*]] = extractelement <12 x float> [[SRC2]], i32 10
; CHECK:    [[TMP14:%.*]] = select i1 [[TMP12]], float [[TMP13]], float [[TMP11]]
; CHECK:    store float [[TMP14]], float* [[DST]], align 4
;
  %3 = mul nsw i32 %src1, 3
  %4 = add nsw i32 %3, 1
  %5 = extractelement <12 x float> %src2, i32 %4
  store float %5, float* %dst, align 4
  br label %nuw
nuw:
; CHECK:    [[TMP15:%.*]] = icmp eq i32 [[SRC1]], 0
; CHECK:    [[TMP16:%.*]] = extractelement <12 x float> [[SRC2]], i32 1
; CHECK:    [[TMP17:%.*]] = select i1 [[TMP15]], float [[TMP16]], float undef
; CHECK:    [[TMP18:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP19:%.*]] = extractelement <12 x float> [[SRC2]], i32 4
; CHECK:    [[TMP20:%.*]] = select i1 [[TMP18]], float [[TMP19]], float [[TMP17]]
; CHECK:    [[TMP21:%.*]] = icmp eq i32 [[SRC1]], 2
; CHECK:    [[TMP22:%.*]] = extractelement <12 x float> [[SRC2]], i32 7
; CHECK:    [[TMP23:%.*]] = select i1 [[TMP21]], float [[TMP22]], float [[TMP20]]
; CHECK:    [[TMP24:%.*]] = icmp eq i32 [[SRC1]], 3
; CHECK:    [[TMP25:%.*]] = extractelement <12 x float> [[SRC2]], i32 10
; CHECK:    [[TMP26:%.*]] = select i1 [[TMP24]], float [[TMP25]], float [[TMP23]]
; CHECK:    store float [[TMP26]], float* [[DST]], align 4
;
  %6 = mul nsw i32 %src1, 3
  %7 = add nuw i32 %6, 1
  %8 = extractelement <12 x float> %src2, i32 %7
  store float %8, float* %dst, align 4
  br label %both
both:
; CHECK:    [[TMP27:%.*]] = icmp eq i32 [[SRC1]], 0
; CHECK:    [[TMP28:%.*]] = extractelement <12 x float> [[SRC2]], i32 1
; CHECK:    [[TMP29:%.*]] = select i1 [[TMP27]], float [[TMP28]], float undef
; CHECK:    [[TMP30:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP31:%.*]] = extractelement <12 x float> [[SRC2]], i32 4
; CHECK:    [[TMP32:%.*]] = select i1 [[TMP30]], float [[TMP31]], float [[TMP29]]
; CHECK:    [[TMP33:%.*]] = icmp eq i32 [[SRC1]], 2
; CHECK:    [[TMP34:%.*]] = extractelement <12 x float> [[SRC2]], i32 7
; CHECK:    [[TMP35:%.*]] = select i1 [[TMP33]], float [[TMP34]], float [[TMP32]]
; CHECK:    [[TMP36:%.*]] = icmp eq i32 [[SRC1]], 3
; CHECK:    [[TMP37:%.*]] = extractelement <12 x float> [[SRC2]], i32 10
; CHECK:    [[TMP38:%.*]] = select i1 [[TMP36]], float [[TMP37]], float [[TMP35]]
; CHECK:    store float [[TMP38]], float* [[DST]], align 4
;
  %9 = mul nsw i32 %src1, 3
  %10 = add nuw nsw i32 %9, 1
  %11 = extractelement <12 x float> %src2, i32 %10
  store float %11, float* %dst, align 4
  ret void
}

; ------------------------------------------------
; Case3: Add + mul pattern matched
;        flags on add: nuw
;        flags on mul: none
; ------------------------------------------------

define void @test_mul_add_2(i32 %src1, <12 x float> %src2, float* %dst) {
entry:
; CHECK-LABEL: @test_mul_add_2(
; CHECK:    [[TMP0:%.*]] = mul i32 [[SRC1:%.*]], 3
; CHECK:    [[TMP1:%.*]] = add nuw i32 [[TMP0]], 1
; CHECK:    [[TMP2:%.*]] = extractelement <12 x float> [[SRC2:%.*]], i32 [[TMP1]]
; CHECK:    store float [[TMP2]], float* [[DST:%.*]], align 4
;
  %0 = mul i32 %src1, 3
  %1 = add nuw i32 %0, 1
  %2 = extractelement <12 x float> %src2, i32 %1
  store float %2, float* %dst, align 4
  ret void
}

; ------------------------------------------------
; Case4: mul pattern matched
;        flags on mul: none
;        but profitable
; ------------------------------------------------

define void @test_mul_profit(i32 %src1, <4 x float> %src2, float* %dst) {
entry:
; CHECK-LABEL: @test_mul_profit(
; CHECK:    [[TMP0:%.*]] = mul i32 [[SRC1:%.*]], 3
; CHECK:    [[TMP1:%.*]] = extractelement <4 x float> [[SRC2:%.*]], i32 [[TMP0]]
; CHECK:    [[TMP2:%.*]] = icmp eq i32 [[TMP0]], 0
; CHECK:    [[TMP3:%.*]] = extractelement <4 x float> [[SRC2]], i32 0
; CHECK:    [[TMP4:%.*]] = select i1 [[TMP2]], float [[TMP3]], float undef
; CHECK:    [[TMP5:%.*]] = icmp eq i32 [[TMP0]], 1
; CHECK:    [[TMP6:%.*]] = extractelement <4 x float> [[SRC2]], i32 1
; CHECK:    [[TMP7:%.*]] = select i1 [[TMP5]], float [[TMP6]], float [[TMP4]]
; CHECK:    [[TMP8:%.*]] = icmp eq i32 [[TMP0]], 2
; CHECK:    [[TMP9:%.*]] = extractelement <4 x float> [[SRC2]], i32 2
; CHECK:    [[TMP10:%.*]] = select i1 [[TMP8]], float [[TMP9]], float [[TMP7]]
; CHECK:    [[TMP11:%.*]] = icmp eq i32 [[TMP0]], 3
; CHECK:    [[TMP12:%.*]] = extractelement <4 x float> [[SRC2]], i32 3
; CHECK:    [[TMP13:%.*]] = select i1 [[TMP11]], float [[TMP12]], float [[TMP10]]
; CHECK:    store float [[TMP13]], float* [[DST:%.*]], align 4
;
  %0 = mul i32 %src1, 3
  %1 = extractelement <4 x float> %src2, i32 %0
  store float %1, float* %dst, align 4
  ret void
}
