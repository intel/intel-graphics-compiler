;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify -igc-agg-arg -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; ResolveAggregateArguments
; ------------------------------------------------

; Debug-info related check
;
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

%struct._st_foo = type { i32, <4 x float>, [2 x i64] }

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @bar(%struct._st_foo* byval(%struct._st_foo) %src, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %const_reg_dword, float %const_reg_fp32, float %const_reg_fp321, float %const_reg_fp322, float %const_reg_fp323, i64 %const_reg_qword, i64 %const_reg_qword4) #0 {
; CHECK-LABEL: @bar(
; CHECK-NEXT:  entry:
; CHECK:    [[SRC_ALLOCA:%.*]] = alloca [[STRUCT__ST_FOO:%[A-z0-9_.]*]]
; CHECK:    [[TMP0:%[A-z0-9]*]] = bitcast %struct._st_foo* [[SRC_ALLOCA]] to i8*
; CHECK:    [[TMP1:%[A-z0-9]*]] = getelementptr i8, i8* [[TMP0]], i32 0
; CHECK:    [[TMP2:%[A-z0-9]*]] = bitcast i8* [[TMP1]] to i32*
; CHECK:    store i32 [[CONST_REG_DWORD:%[A-z0-9]*]], i32* [[TMP2]]
; CHECK:    [[TMP3:%[A-z0-9]*]] = getelementptr i8, i8* [[TMP0]], i32 16
; CHECK:    [[TMP4:%[A-z0-9]*]] = bitcast i8* [[TMP3]] to float*
; CHECK:    store float [[CONST_REG_FP32:%[A-z0-9]*]], float* [[TMP4]]
; CHECK:    [[TMP5:%[A-z0-9]*]] = getelementptr i8, i8* [[TMP0]], i32 20
; CHECK:    [[TMP6:%[A-z0-9]*]] = bitcast i8* [[TMP5]] to float*
; CHECK:    store float [[CONST_REG_FP321:%[A-z0-9]*]], float* [[TMP6]]
; CHECK:    [[TMP7:%[A-z0-9]*]] = getelementptr i8, i8* [[TMP0]], i32 24
; CHECK:    [[TMP8:%[A-z0-9]*]] = bitcast i8* [[TMP7]] to float*
; CHECK:    store float [[CONST_REG_FP322:%[A-z0-9]*]], float* [[TMP8]]
; CHECK:    [[TMP9:%[A-z0-9]*]] = getelementptr i8, i8* [[TMP0]], i32 28
; CHECK:    [[TMP10:%[A-z0-9]*]] = bitcast i8* [[TMP9]] to float*
; CHECK:    store float [[CONST_REG_FP323:%[A-z0-9]*]], float* [[TMP10]]
; CHECK:    [[TMP11:%[A-z0-9]*]] = getelementptr i8, i8* [[TMP0]], i32 32
; CHECK:    [[TMP12:%[A-z0-9]*]] = bitcast i8* [[TMP11]] to i64*
; CHECK:    store i64 [[CONST_REG_QWORD:%[A-z0-9]*]], i64* [[TMP12]]
; CHECK:    [[TMP13:%[A-z0-9]*]] = getelementptr i8, i8* [[TMP0]], i32 40
; CHECK:    [[TMP14:%[A-z0-9]*]] = bitcast i8* [[TMP13]] to i64*
; CHECK:    store i64 [[CONST_REG_QWORD4:%[A-z0-9]*]], i64* [[TMP14]]
; CHECK:    [[AA:%[A-z0-9]*]] = alloca i32, align 4
; CHECK:    [[BB:%[A-z0-9]*]] = alloca i32, align 4
; CHECK:    [[CC:%[A-z0-9]*]] = alloca i32, align 4
; CHECK:    [[A:%[A-z0-9]*]] = getelementptr inbounds [[STRUCT__ST_FOO]], %struct._st_foo* [[SRC_ALLOCA]], i32 0, i32 0
; CHECK:    [[TMP15:%[A-z0-9]*]] = load i32, i32* [[A]], align 16
; CHECK:    store i32 [[TMP15]], i32* [[AA]], align 4
; CHECK:    [[B:%[A-z0-9]*]] = getelementptr inbounds [[STRUCT__ST_FOO]], %struct._st_foo* [[SRC_ALLOCA]], i32 0, i32 1
; CHECK:    [[TMP16:%[A-z0-9]*]] = load <4 x float>, <4 x float>* [[B]], align 16
; CHECK:    [[TMP17:%[A-z0-9]*]] = extractelement <4 x float> [[TMP16]], i32 1
; CHECK:    [[CONV_I:%[A-z0-9.]*]] = fptosi float [[TMP17]] to i32
; CHECK:    store i32 [[CONV_I]], i32* [[BB]], align 4
; CHECK:    [[C:%[A-z0-9]*]] = getelementptr inbounds [[STRUCT__ST_FOO]], %struct._st_foo* [[SRC_ALLOCA]], i32 0, i32 2
; CHECK:    [[ARRAYIDX:%[A-z0-9]*]] = getelementptr inbounds [2 x i64], [2 x i64]* [[C]], i64 0, i64 1
; CHECK:    [[TMP18:%[A-z0-9]*]] = load i64, i64* [[ARRAYIDX]], align 8
; CHECK:    [[CONV_I1:%[A-z0-9.]*]] = trunc i64 [[TMP18]] to i32
; CHECK:    store i32 [[CONV_I1]], i32* [[CC]], align 4
; CHECK:    ret void
;
entry:
  %aa = alloca i32, align 4
  %bb = alloca i32, align 4
  %cc = alloca i32, align 4
  %a = getelementptr inbounds %struct._st_foo, %struct._st_foo* %src, i32 0, i32 0
  %0 = load i32, i32* %a, align 16
  store i32 %0, i32* %aa, align 4
  %b = getelementptr inbounds %struct._st_foo, %struct._st_foo* %src, i32 0, i32 1
  %1 = load <4 x float>, <4 x float>* %b, align 16
  %2 = extractelement <4 x float> %1, i32 1
  %conv.i = fptosi float %2 to i32
  store i32 %conv.i, i32* %bb, align 4
  %c = getelementptr inbounds %struct._st_foo, %struct._st_foo* %src, i32 0, i32 2
  %arrayidx = getelementptr inbounds [2 x i64], [2 x i64]* %c, i64 0, i64 1
  %3 = load i64, i64* %arrayidx, align 8
  %conv.i1 = trunc i64 %3 to i32
  store i32 %conv.i1, i32* %cc, align 4
  ret void
}

attributes #0 = { convergent noinline nounwind optnone }
attributes #2 = { nounwind }

!igc.functions = !{!3}

!3 = !{void (%struct._st_foo*, <8 x i32>, <8 x i32>, i8*, i32, float, float, float, float, i64, i64)* @bar, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8, !9, !10, !13, !15, !17, !19, !21, !23}
!7 = !{i32 0}
!8 = !{i32 1}
!9 = !{i32 13}
!10 = !{i32 18, !11, !12}
!11 = !{!"explicit_arg_num", i32 0}
!12 = !{!"struct_arg_offset", i32 0}
!13 = !{i32 16, !11, !14}
!14 = !{!"struct_arg_offset", i32 16}
!15 = !{i32 16, !11, !16}
!16 = !{!"struct_arg_offset", i32 20}
!17 = !{i32 16, !11, !18}
!18 = !{!"struct_arg_offset", i32 24}
!19 = !{i32 16, !11, !20}
!20 = !{!"struct_arg_offset", i32 28}
!21 = !{i32 17, !11, !22}
!22 = !{!"struct_arg_offset", i32 32}
!23 = !{i32 17, !11, !24}
!24 = !{!"struct_arg_offset", i32 40}
