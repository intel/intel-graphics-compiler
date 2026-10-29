;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -enable-debugify -igc-agg-arg -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -enable-debugify -igc-agg-arg -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED %}
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
; CHECK-TYPED:    [[TMP0:%[A-z0-9]*]] = bitcast %struct._st_foo* [[SRC_ALLOCA]] to i8*
; CHECK-TYPED:    [[TMP1:%[A-z0-9]*]] = getelementptr i8, i8* [[TMP0]], i32 0
; CHECK-OPAQUE:    [[TMP1:%[A-z0-9]*]] = getelementptr i8, ptr [[SRC_ALLOCA]], i32 0
; CHECK-TYPED:    [[TMP2:%[A-z0-9]*]] = bitcast i8* [[TMP1]] to i32*
; CHECK-TYPED:    store i32 [[CONST_REG_DWORD:%[A-z0-9]*]], i32* [[TMP2]]
; CHECK-OPAQUE:    store i32 [[CONST_REG_DWORD:%[A-z0-9]*]], ptr [[TMP1]]
; CHECK-TYPED:    [[TMP3:%[A-z0-9]*]] = getelementptr i8, i8* [[TMP0]], i32 16
; CHECK-OPAQUE:    [[TMP3:%[A-z0-9]*]] = getelementptr i8, ptr [[SRC_ALLOCA]], i32 16
; CHECK-TYPED:    [[TMP4:%[A-z0-9]*]] = bitcast i8* [[TMP3]] to float*
; CHECK-TYPED:    store float [[CONST_REG_FP32:%[A-z0-9]*]], float* [[TMP4]]
; CHECK-OPAQUE:    store float [[CONST_REG_FP32:%[A-z0-9]*]], ptr [[TMP3]]
; CHECK-TYPED:    [[TMP5:%[A-z0-9]*]] = getelementptr i8, i8* [[TMP0]], i32 20
; CHECK-OPAQUE:    [[TMP5:%[A-z0-9]*]] = getelementptr i8, ptr [[SRC_ALLOCA]], i32 20
; CHECK-TYPED:    [[TMP6:%[A-z0-9]*]] = bitcast i8* [[TMP5]] to float*
; CHECK-TYPED:    store float [[CONST_REG_FP321:%[A-z0-9]*]], float* [[TMP6]]
; CHECK-OPAQUE:    store float [[CONST_REG_FP321:%[A-z0-9]*]], ptr [[TMP5]]
; CHECK-TYPED:    [[TMP7:%[A-z0-9]*]] = getelementptr i8, i8* [[TMP0]], i32 24
; CHECK-OPAQUE:    [[TMP7:%[A-z0-9]*]] = getelementptr i8, ptr [[SRC_ALLOCA]], i32 24
; CHECK-TYPED:    [[TMP8:%[A-z0-9]*]] = bitcast i8* [[TMP7]] to float*
; CHECK-TYPED:    store float [[CONST_REG_FP322:%[A-z0-9]*]], float* [[TMP8]]
; CHECK-OPAQUE:    store float [[CONST_REG_FP322:%[A-z0-9]*]], ptr [[TMP7]]
; CHECK-TYPED:    [[TMP9:%[A-z0-9]*]] = getelementptr i8, i8* [[TMP0]], i32 28
; CHECK-OPAQUE:    [[TMP9:%[A-z0-9]*]] = getelementptr i8, ptr [[SRC_ALLOCA]], i32 28
; CHECK-TYPED:    [[TMP10:%[A-z0-9]*]] = bitcast i8* [[TMP9]] to float*
; CHECK-TYPED:    store float [[CONST_REG_FP323:%[A-z0-9]*]], float* [[TMP10]]
; CHECK-OPAQUE:    store float [[CONST_REG_FP323:%[A-z0-9]*]], ptr [[TMP9]]
; CHECK-TYPED:    [[TMP11:%[A-z0-9]*]] = getelementptr i8, i8* [[TMP0]], i32 32
; CHECK-OPAQUE:    [[TMP11:%[A-z0-9]*]] = getelementptr i8, ptr [[SRC_ALLOCA]], i32 32
; CHECK-TYPED:    [[TMP12:%[A-z0-9]*]] = bitcast i8* [[TMP11]] to i64*
; CHECK-TYPED:    store i64 [[CONST_REG_QWORD:%[A-z0-9]*]], i64* [[TMP12]]
; CHECK-OPAQUE:    store i64 [[CONST_REG_QWORD:%[A-z0-9]*]], ptr [[TMP11]]
; CHECK-TYPED:    [[TMP13:%[A-z0-9]*]] = getelementptr i8, i8* [[TMP0]], i32 40
; CHECK-OPAQUE:    [[TMP13:%[A-z0-9]*]] = getelementptr i8, ptr [[SRC_ALLOCA]], i32 40
; CHECK-TYPED:    [[TMP14:%[A-z0-9]*]] = bitcast i8* [[TMP13]] to i64*
; CHECK-TYPED:    store i64 [[CONST_REG_QWORD4:%[A-z0-9]*]], i64* [[TMP14]]
; CHECK-OPAQUE:    store i64 [[CONST_REG_QWORD4:%[A-z0-9]*]], ptr [[TMP13]]
; CHECK:    [[AA:%[A-z0-9]*]] = alloca i32, align 4
; CHECK:    [[BB:%[A-z0-9]*]] = alloca i32, align 4
; CHECK:    [[CC:%[A-z0-9]*]] = alloca i32, align 4
; CHECK-TYPED:    [[A:%[A-z0-9]*]] = getelementptr inbounds [[STRUCT__ST_FOO]], %struct._st_foo* [[SRC_ALLOCA]], i32 0, i32 0
; CHECK-OPAQUE:    [[A:%[A-z0-9]*]] = getelementptr inbounds [[STRUCT__ST_FOO]], ptr [[SRC_ALLOCA]], i32 0, i32 0
; CHECK-TYPED:    [[TMP15:%[A-z0-9]*]] = load i32, i32* [[A]], align 16
; CHECK-OPAQUE:    [[TMP15:%[A-z0-9]*]] = load i32, ptr [[A]], align 16
; CHECK-TYPED:    store i32 [[TMP15]], i32* [[AA]], align 4
; CHECK-OPAQUE:    store i32 [[TMP15]], ptr [[AA]], align 4
; CHECK-TYPED:    [[B:%[A-z0-9]*]] = getelementptr inbounds [[STRUCT__ST_FOO]], %struct._st_foo* [[SRC_ALLOCA]], i32 0, i32 1
; CHECK-OPAQUE:    [[B:%[A-z0-9]*]] = getelementptr inbounds [[STRUCT__ST_FOO]], ptr [[SRC_ALLOCA]], i32 0, i32 1
; CHECK-TYPED:    [[TMP16:%[A-z0-9]*]] = load <4 x float>, <4 x float>* [[B]], align 16
; CHECK-OPAQUE:    [[TMP16:%[A-z0-9]*]] = load <4 x float>, ptr [[B]], align 16
; CHECK:    [[TMP17:%[A-z0-9]*]] = extractelement <4 x float> [[TMP16]], i32 1
; CHECK:    [[CONV_I:%[A-z0-9.]*]] = fptosi float [[TMP17]] to i32
; CHECK-TYPED:    store i32 [[CONV_I]], i32* [[BB]], align 4
; CHECK-OPAQUE:    store i32 [[CONV_I]], ptr [[BB]], align 4
; CHECK-TYPED:    [[C:%[A-z0-9]*]] = getelementptr inbounds [[STRUCT__ST_FOO]], %struct._st_foo* [[SRC_ALLOCA]], i32 0, i32 2
; CHECK-OPAQUE:    [[C:%[A-z0-9]*]] = getelementptr inbounds [[STRUCT__ST_FOO]], ptr [[SRC_ALLOCA]], i32 0, i32 2
; CHECK-TYPED:    [[ARRAYIDX:%[A-z0-9]*]] = getelementptr inbounds [2 x i64], [2 x i64]* [[C]], i64 0, i64 1
; CHECK-OPAQUE:    [[ARRAYIDX:%[A-z0-9]*]] = getelementptr inbounds [2 x i64], ptr [[C]], i64 0, i64 1
; CHECK-TYPED:    [[TMP18:%[A-z0-9]*]] = load i64, i64* [[ARRAYIDX]], align 8
; CHECK-OPAQUE:    [[TMP18:%[A-z0-9]*]] = load i64, ptr [[ARRAYIDX]], align 8
; CHECK:    [[CONV_I1:%[A-z0-9.]*]] = trunc i64 [[TMP18]] to i32
; CHECK-TYPED:    store i32 [[CONV_I1]], i32* [[CC]], align 4
; CHECK-OPAQUE:    store i32 [[CONV_I1]], ptr [[CC]], align 4
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
!4 = !{!5}
!5 = !{!"function_type", i32 0}
!25 = !{!"argId", i32 0}
!26 = !{!"implicitArgInfoListVec[0]", !25}
!27 = !{!"argId", i32 1}
!28 = !{!"implicitArgInfoListVec[1]", !27}
!29 = !{!"argId", i32 13}
!30 = !{!"implicitArgInfoListVec[2]", !29}
!31 = !{!"argId", i32 18}
!32 = !{!"explicitArgNum", i32 0}
!33 = !{!"structArgOffset", i32 0}
!34 = !{!"implicitArgInfoListVec[3]", !31, !32, !33}
!35 = !{!"argId", i32 16}
!36 = !{!"explicitArgNum", i32 0}
!37 = !{!"structArgOffset", i32 16}
!38 = !{!"implicitArgInfoListVec[4]", !35, !36, !37}
!39 = !{!"argId", i32 16}
!40 = !{!"explicitArgNum", i32 0}
!41 = !{!"structArgOffset", i32 20}
!42 = !{!"implicitArgInfoListVec[5]", !39, !40, !41}
!43 = !{!"argId", i32 16}
!44 = !{!"explicitArgNum", i32 0}
!45 = !{!"structArgOffset", i32 24}
!46 = !{!"implicitArgInfoListVec[6]", !43, !44, !45}
!47 = !{!"argId", i32 16}
!48 = !{!"explicitArgNum", i32 0}
!49 = !{!"structArgOffset", i32 28}
!50 = !{!"implicitArgInfoListVec[7]", !47, !48, !49}
!51 = !{!"argId", i32 17}
!52 = !{!"explicitArgNum", i32 0}
!53 = !{!"structArgOffset", i32 32}
!54 = !{!"implicitArgInfoListVec[8]", !51, !52, !53}
!55 = !{!"argId", i32 17}
!56 = !{!"explicitArgNum", i32 0}
!57 = !{!"structArgOffset", i32 40}
!58 = !{!"implicitArgInfoListVec[9]", !55, !56, !57}
!59 = !{!"implicitArgInfoList", !26, !28, !30, !34, !38, !42, !46, !50, !54, !58}
!60 = !{!"FuncMDMap[0]", void (%struct._st_foo*, <8 x i32>, <8 x i32>, i8*, i32, float, float, float, float, i64, i64)* @bar}
!61 = !{!"FuncMDValue[0]", !59}
!62 = !{!"FuncMD", !60, !61}
!63 = !{!"ModuleMD", !62}
!IGCMetadata = !{!63}
