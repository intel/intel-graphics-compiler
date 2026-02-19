;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify -igc-custom-loop-opt -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; CustomLoopVersioning
; ------------------------------------------------

; Test checks that debug intrinsics don't affect CustomLoopVersioning pass operation
; Dbg intrinsics calls are added using debugify pass

define spir_kernel void @test_customloop(float addrspace(65549)* %a, float %b, float %c, float %d) {

; Entry bb is not modified
;
; CHECK: @test_customloop(
; CHECK:  entry:
; CHECK:    [[AA:%.*]] = inttoptr i32 42 to float addrspace(65549)*
; CHECK:    [[CC:%.*]] = call float @llvm.maxnum.f32(float %c, float 3.000000e+00)
; CHECK:    [[DD:%.*]] = call float @llvm.minnum.f32(float %d, float 2.000000e+00)
; CHECK:    br label %[[PRE_HEADER:[A-z0-9.]*]]
;
entry:
  %aa = inttoptr i32 42 to float addrspace(65549)*
  %cc = call float @llvm.maxnum.f32(float %c, float 3.000000e+00)
  %dd = call float @llvm.minnum.f32(float %d, float 2.000000e+00)
  br label %pre_header

; Additional condition is added to preheader bb
;
; CHECK:  [[PRE_HEADER]]:
; CHECK:    [[TMP0:%.*]] = load float, float addrspace(65549)* [[AA]], align 4
; CHECK:    [[TMP1:%.*]] = fmul float %b, [[TMP0]]
; CHECK:    [[TMP2:%.*]] = fcmp fast ogt float [[TMP0]], 1.000000e+00
; CHECK:    [[TMP3:%.*]] = fmul fast float [[CC]], [[TMP0]]
; CHECK:    [[TMP4:%.*]] = fcmp fast olt float [[TMP3]], [[DD]]
; CHECK:    [[TMP5:%.*]] = and i1 [[TMP2]], [[TMP4]]
; CHECK:    br i1 [[TMP5]], label %[[PRE_HEADER_SPLIT_SEG1:[A-z0-9.]*]], label %[[PRE_HEADER_SPLIT:[A-z0-9.]*]]
; CHECK:  [[PRE_HEADER_SPLIT_SEG1]]:
; CHECK:    br label %[[LOOP_BODY_SEG1:[A-z0-9.]*]]

pre_header:
  %0 = load float, float addrspace(65549)* %aa, align 4
  %1 = fmul float %b, %0
  br label %loop_body

; Loop body bb is split
;
; CHECK:  [[LOOP_BODY_SEG1]]:
; CHECK:    [[TMP6:%.*]] = phi float [ %b, %[[PRE_HEADER_SPLIT_SEG1]] ], [ [[TMP7:%.*]], %[[LOOP_BODY_SEG1]] ]
; CHECK:    [[TMP7]] = phi float [ [[TMP1]], %[[PRE_HEADER_SPLIT_SEG1]] ], [ [[TMP11:%.*]], %[[LOOP_BODY_SEG1]] ]
; CHECK:    [[TMP8:%.*]] = call float @llvm.maxnum.f32(float [[CC]], float [[TMP6]])
; CHECK:    [[TMP9:%.*]] = call float @llvm.minnum.f32(float [[DD]], float [[TMP7]])
; CHECK:    [[TMP10:%.*]] = load float, float addrspace(65549)* [[AA]], align 4
; CHECK:    [[TMP11]] = fmul float [[TMP7]], [[TMP10]]
; CHECK:    [[TMP12:%.*]] = fcmp ult float [[TMP7]], [[CC]]
; CHECK:    br i1 [[TMP12]], label %[[LOOP_BODY_SEG1]], label %[[PRE_HEADER_SPLIT_SEG2:[A-z0-9.]*]]
; CHECK:  [[PRE_HEADER_SPLIT_SEG2]]:
; CHECK:    br label %[[LOOP_BODY_SEG2:[A-z0-9.]*]]
; CHECK:  [[LOOP_BODY_SEG2]]:
; CHECK:    [[TMP13:%.*]] = phi float [ [[TMP7]], %[[PRE_HEADER_SPLIT_SEG2]] ], [ [[TMP14:%.*]], %[[LOOP_BODY_SEG2]] ]
; CHECK:    [[TMP14]] = phi float [ [[TMP11]], %[[PRE_HEADER_SPLIT_SEG2]] ], [ [[TMP18:%.*]], %[[LOOP_BODY_SEG2]] ]
; CHECK:    [[TMP15:%.*]] = call float @llvm.maxnum.f32(float [[CC]], float [[TMP13]])
; CHECK:    [[TMP16:%.*]] = call float @llvm.minnum.f32(float [[DD]], float [[TMP14]])
; CHECK:    [[TMP17:%.*]] = load float, float addrspace(65549)* [[AA]], align 4
; CHECK:    [[TMP18]] = fmul float [[TMP14]], [[TMP17]]
; CHECK:    [[TMP19:%.*]] = fdiv fast float [[DD]], [[TMP0]]
; CHECK:    [[TMP20:%.*]] = fcmp ult float [[TMP14]], [[TMP19]]
; CHECK:    br i1 [[TMP20]], label %[[LOOP_BODY_SEG2]], label %[[PRE_HEADER_SPLIT_SEG3:[A-z0-9.]*]]
; CHECK:  [[PRE_HEADER_SPLIT_SEG3]]:
; CHECK:    br label %[[LOOP_BODY_SEG3:[A-z0-9.]*]]
; CHECK:  [[LOOP_BODY_SEG3]]:
; CHECK:    [[TMP21:%.*]] = call float @llvm.maxnum.f32(float [[CC]], float [[TMP14]])
; CHECK:    [[TMP22:%.*]] = call float @llvm.minnum.f32(float [[DD]], float [[TMP18]])
; CHECK:    [[TMP23:%.*]] = load float, float addrspace(65549)* [[AA]], align 4
; CHECK:    [[TMP24:%.*]] = fmul float [[TMP18]], [[TMP23]]
; CHECK:    [[TMP25:%.*]] = fcmp ult float [[TMP18]], [[DD]]
; CHECK:    br label %[[END:[A-z0-9.]*]]
; CHECK:  [[PRE_HEADER_SPLIT]]:
; CHECK:    br label %[[LOOP_BODY:[A-z0-9.]*]]

; Unmodified loop body:
;
; CHECK:  [[LOOP_BODY]]:
; CHECK:    [[TMP26:%.*]] = phi float [ %b, %[[PRE_HEADER_SPLIT]] ], [ [[TMP27:%.*]], %[[LOOP_BODY]] ]
; CHECK:    [[TMP27]] = phi float [ [[TMP1]], %[[PRE_HEADER_SPLIT]] ], [ [[TMP31:%.*]], %[[LOOP_BODY]] ]
; CHECK:    [[TMP28:%.*]] = call float @llvm.maxnum.f32(float [[CC]], float [[TMP26]])
; CHECK:    [[TMP29:%.*]] = call float @llvm.minnum.f32(float [[DD]], float [[TMP27]])
; CHECK:    [[TMP30:%.*]] = load float, float addrspace(65549)* [[AA]], align 4
; CHECK:    [[TMP31]] = fmul float [[TMP27]], [[TMP30]]
; CHECK:    [[TMP32:%.*]] = fcmp ult float [[TMP27]], [[DD]]
; CHECK:    br i1 [[TMP32]], label %[[LOOP_BODY]], label %[[END]]

loop_body:
  %2 = phi float [ %b, %pre_header ], [ %3, %loop_body ]
  %3 = phi float [ %1, %pre_header ], [ %7, %loop_body ]
  %4 = call float @llvm.maxnum.f32(float %cc, float %2)
  %5 = call float @llvm.minnum.f32(float %dd, float %3)
  %6 = load float, float addrspace(65549)* %aa, align 4
  %7 = fmul float %3, %6
  %8 = fcmp ult float %3, %dd
  br i1 %8, label %loop_body, label %end

; End bb with phi due to split
;
; CHECK:  [[END]]:
; CHECK:    [[TMP33:%.*]] = phi float [ [[TMP18]], %[[LOOP_BODY_SEG3]] ], [ [[TMP27]], %[[LOOP_BODY]] ]
; CHECK:    store float [[TMP33]], float addrspace(65549)* %a, align 4
; CHECK:    ret void
end:
  store float %3, float addrspace(65549)* %a, align 4
  ret void
}


declare float @llvm.maxnum.f32(float, float) #0
declare float @llvm.minnum.f32(float, float) #0

!igc.functions = !{!0}

!0 = !{void (float addrspace(65549)*, float, float, float)* @test_customloop, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
