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

define void @test(i32 %src1, <12 x float> %src2, float* %dst) {
; CHECK-LABEL: @test(
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
; CHECK:    [[TMP13:%.*]] = icmp eq i32 [[SRC1]], 0
; CHECK:    [[TMP14:%.*]] = extractelement <12 x float> [[SRC2]], i32 1
; CHECK:    [[TMP15:%.*]] = select i1 [[TMP13]], float [[TMP14]], float undef
; CHECK:    [[TMP16:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP17:%.*]] = extractelement <12 x float> [[SRC2]], i32 4
; CHECK:    [[TMP18:%.*]] = select i1 [[TMP16]], float [[TMP17]], float [[TMP15]]
; CHECK:    [[TMP19:%.*]] = icmp eq i32 [[SRC1]], 2
; CHECK:    [[TMP20:%.*]] = extractelement <12 x float> [[SRC2]], i32 7
; CHECK:    [[TMP21:%.*]] = select i1 [[TMP19]], float [[TMP20]], float [[TMP18]]
; CHECK:    [[TMP22:%.*]] = icmp eq i32 [[SRC1]], 3
; CHECK:    [[TMP23:%.*]] = extractelement <12 x float> [[SRC2]], i32 10
; CHECK:    [[TMP24:%.*]] = select i1 [[TMP22]], float [[TMP23]], float [[TMP21]]
; CHECK:    store float [[TMP24]], float* [[DST]], align 4
; CHECK:    [[TMP25:%.*]] = icmp eq i32 [[SRC1]], 0
; CHECK:    [[TMP26:%.*]] = extractelement <12 x float> [[SRC2]], i32 2
; CHECK:    [[TMP27:%.*]] = select i1 [[TMP25]], float [[TMP26]], float undef
; CHECK:    [[TMP28:%.*]] = icmp eq i32 [[SRC1]], 1
; CHECK:    [[TMP29:%.*]] = extractelement <12 x float> [[SRC2]], i32 5
; CHECK:    [[TMP30:%.*]] = select i1 [[TMP28]], float [[TMP29]], float [[TMP27]]
; CHECK:    [[TMP31:%.*]] = icmp eq i32 [[SRC1]], 2
; CHECK:    [[TMP32:%.*]] = extractelement <12 x float> [[SRC2]], i32 8
; CHECK:    [[TMP33:%.*]] = select i1 [[TMP31]], float [[TMP32]], float [[TMP30]]
; CHECK:    [[TMP34:%.*]] = icmp eq i32 [[SRC1]], 3
; CHECK:    [[TMP35:%.*]] = extractelement <12 x float> [[SRC2]], i32 11
; CHECK:    [[TMP36:%.*]] = select i1 [[TMP34]], float [[TMP35]], float [[TMP33]]
; CHECK:    store float [[TMP36]], float* [[DST]], align 4
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
