;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXPromoteArray -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32"

%aggregate = type { i32, { i32 }, <2 x i32>, [2 x [1 x i32]] }

define dllexport void @test() {
; CHECK-LABEL: @test(
; CHECK-NEXT:    [[TMP1:%.*]] = alloca <6 x i32>, align 32
; CHECK-NEXT:    [[TMP2:%.*]] = load <6 x i32>, <6 x i32>* [[TMP1]], align 32
; CHECK-NEXT:    [[TMP3:%.*]] = insertelement <6 x i32> [[TMP2]], i32 0, i32 0
; CHECK-NEXT:    store <6 x i32> [[TMP3]], <6 x i32>* [[TMP1]], align 32
; CHECK-NEXT:    [[TMP4:%.*]] = load <6 x i32>, <6 x i32>* [[TMP1]], align 32
; CHECK-NEXT:    [[TMP5:%.*]] = insertelement <6 x i32> [[TMP4]], i32 1, i32 1
; CHECK-NEXT:    store <6 x i32> [[TMP5]], <6 x i32>* [[TMP1]], align 32
; CHECK-NEXT:    [[TMP6:%.*]] = load <6 x i32>, <6 x i32>* [[TMP1]], align 32
; CHECK-NEXT:    [[TMP7:%.*]] = insertelement <6 x i32> [[TMP6]], i32 2, i32 2
; CHECK-NEXT:    [[TMP8:%.*]] = insertelement <6 x i32> [[TMP7]], i32 3, i32 3
; CHECK-NEXT:    store <6 x i32> [[TMP8]], <6 x i32>* [[TMP1]], align 32
; CHECK-NEXT:    [[TMP9:%.*]] = load <6 x i32>, <6 x i32>* [[TMP1]], align 32
; CHECK-NEXT:    [[TMP10:%.*]] = insertelement <6 x i32> [[TMP9]], i32 4, i32 4
; CHECK-NEXT:    store <6 x i32> [[TMP10]], <6 x i32>* [[TMP1]], align 32
; CHECK-NEXT:    [[TMP11:%.*]] = load <6 x i32>, <6 x i32>* [[TMP1]], align 32
; CHECK-NEXT:    [[TMP12:%.*]] = insertelement <6 x i32> [[TMP11]], i32 5, i32 5
; CHECK-NEXT:    store <6 x i32> [[TMP12]], <6 x i32>* [[TMP1]], align 32
; CHECK-NEXT:    [[TMP13:%.*]] = load <6 x i32>, <6 x i32>* [[TMP1]], align 32
; CHECK-NEXT:    [[TMP14:%.*]] = extractelement <6 x i32> [[TMP13]], i32 0
; CHECK-NEXT:    [[TMP15:%.*]] = insertvalue [[AGGREGATE:%.*]] undef, i32 [[TMP14]], 0
; CHECK-NEXT:    [[TMP16:%.*]] = load <6 x i32>, <6 x i32>* [[TMP1]], align 32
; CHECK-NEXT:    [[TMP17:%.*]] = extractelement <6 x i32> [[TMP16]], i32 1
; CHECK-NEXT:    [[TMP18:%.*]] = insertvalue { i32 } undef, i32 [[TMP17]], 0
; CHECK-NEXT:    [[TMP19:%.*]] = insertvalue [[AGGREGATE]] [[TMP15]], { i32 } [[TMP18]], 1
; CHECK-NEXT:    [[TMP20:%.*]] = load <6 x i32>, <6 x i32>* [[TMP1]], align 32
; CHECK-NEXT:    [[TMP21:%.*]] = extractelement <6 x i32> [[TMP20]], i32 2
; CHECK-NEXT:    [[TMP22:%.*]] = insertelement <2 x i32> undef, i32 [[TMP21]], i32 0
; CHECK-NEXT:    [[TMP23:%.*]] = extractelement <6 x i32> [[TMP20]], i32 3
; CHECK-NEXT:    [[TMP24:%.*]] = insertelement <2 x i32> [[TMP22]], i32 [[TMP23]], i32 1
; CHECK-NEXT:    [[TMP25:%.*]] = insertvalue [[AGGREGATE]] [[TMP19]], <2 x i32> [[TMP24]], 2
; CHECK-NEXT:    [[TMP26:%.*]] = load <6 x i32>, <6 x i32>* [[TMP1]], align 32
; CHECK-NEXT:    [[TMP27:%.*]] = extractelement <6 x i32> [[TMP26]], i32 4
; CHECK-NEXT:    [[TMP28:%.*]] = insertvalue [1 x i32] undef, i32 [[TMP27]], 0
; CHECK-NEXT:    [[TMP29:%.*]] = insertvalue [2 x [1 x i32]] undef, [1 x i32] [[TMP28]], 0
; CHECK-NEXT:    [[TMP30:%.*]] = load <6 x i32>, <6 x i32>* [[TMP1]], align 32
; CHECK-NEXT:    [[TMP31:%.*]] = extractelement <6 x i32> [[TMP30]], i32 5
; CHECK-NEXT:    [[TMP32:%.*]] = insertvalue [1 x i32] undef, i32 [[TMP31]], 0
; CHECK-NEXT:    [[TMP33:%.*]] = insertvalue [2 x [1 x i32]] [[TMP29]], [1 x i32] [[TMP32]], 1
; CHECK-NEXT:    [[VAL:%.*]] = insertvalue [[AGGREGATE]] [[TMP25]], [2 x [1 x i32]] [[TMP33]], 3
; CHECK-NEXT:    ret void
;
  %buf = alloca %aggregate, align 4
  store %aggregate { i32 0, { i32 } { i32 1 }, <2 x i32> <i32 2, i32 3>, [2 x [1 x i32]] [[1 x i32] [i32 4], [1 x i32] [i32 5]] }, %aggregate* %buf, align 4
  %val = load %aggregate, %aggregate* %buf, align 4
  ret void
}
