;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify --igc-3d-to-2darray -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; Image3dToImage2darray
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_i3dto2da(i8* %a) {
; CHECK-LABEL: @test_i3dto2da(
; CHECK:    [[TMP1:%.*]] = call <16 x i32> @llvm.genx.GenISA.ldptr.16i32(i32 1, i32 2, i32 3, i32 4, i8* [[A:%.*]], i8* [[A]], i32 0, i32 1, i32 2)
; CHECK:    call void @use.v16i32(<16 x i32> [[TMP1]])
; CHECK:    ret void
;
  %1 = call <16 x i32> @llvm.genx.GenISA.ldptr.16i32(i32 1, i32 2, i32 3, i32 4, i8* %a, i8* %a, i32 0, i32 1, i32 2)
  call void @use.v16i32(<16 x i32> %1)
  ret void
}

define void @test_i3dto2da_f32(float* %a, i32* %b) {
; CHECK-LABEL: @test_i3dto2da_f32(
; CHECK:    [[TMP1:%.*]] = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.f32.i32(float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.500000e+00, float 5.000000e+00, float* [[A:%.*]], i32* [[B:%.*]], i32 1, i32 2, i32 3)
; CHECK:    call void @use.v4f32(<4 x float> [[TMP1]])
; CHECK:    ret void
;
  %1 = call <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.f32.i32(float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.500000e+00, float 5.000000e+00, float* %a, i32* %b, i32 1, i32 2, i32 3)
  call void @use.v4f32(<4 x float> %1)
  ret void
}

; CHECK-DAG: !{void (i8*)* @test_i3dto2da, [[FI_MD:![0-9]*]]}
; CHECK-DAG: [[FT:![0-9]*]] = !{!"function_type", i32 0}
; CHECK-DAG: [[IMP_ARG:![0-9]*]] = !{!"implicit_arg_desc"}
; CHECK-DAG: [[EXP_ARG:![0-9]*]] = !{!"explicit_arg_num", i32 0}
; CHECK-DAG: [[FI_IMG_FLOAT:![0-9]*]] = !{!"img_access_float_coords", i1 false}
; CHECK-DAG: [[FI_IMG_INT:![0-9]*]] = !{!"img_access_int_coords", i1 true}
; CHECK-DAG: [[FI_MD]] = !{[[FT]], [[FI_IMG_MD:![0-9]*]], [[IMP_ARG]]}
; CHECK-DAG: [[FI_IMG_MD]] = !{[[FI_IMG_A:![0-9]*]]}
; CHECK-DAG: [[FI_IMG_A]] = !{null, [[EXP_ARG]], [[FI_IMG_FLOAT]], [[FI_IMG_INT]]}


; CHECK-DAG: !{void (float*, i32*)* @test_i3dto2da_f32, [[FF_MD:![0-9]*]]}
; CHECK-DAG: [[FF_IMG_FLOAT:![0-9]*]] = !{!"img_access_float_coords", i1 true}
; CHECK-DAG: [[FF_IMG_INT:![0-9]*]] = !{!"img_access_int_coords", i1 false}
; CHECK-DAG: [[FF_MD]] = !{[[FT]], [[FF_IMG_MD:![0-9]*]], [[IMP_ARG]]}
; CHECK-DAG: [[FF_IMG_MD]] = !{[[FF_IMG_A:![0-9]*]]}
; CHECK-DAG: [[FF_IMG_A]] = !{null, [[EXP_ARG]], [[FF_IMG_FLOAT]], [[FF_IMG_INT]]}


declare void @use.v16i32(<16 x i32>)

declare void @use.v4f32(<4 x float>)

declare <16 x i32> @llvm.genx.GenISA.ldptr.16i32.const(i32, i32, i32, i32, i8, i8, i32, i32, i32)

declare <16 x i32> @llvm.genx.GenISA.ldptr.16i32(i32, i32, i32, i32, i8*, i8*, i32, i32, i32)

declare <4 x float> @llvm.genx.GenISA.sampleLptr.v4f32.f32.f32.i32(float, float, float, float, float, float*, i32*, i32, i32, i32)

!igc.functions = !{!0, !4}
!IGCMetadata = !{!5}

!0 = !{void (i8*)* @test_i3dto2da, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
!4 = !{void (float*, i32*)* @test_i3dto2da_f32, !1}
!5 = !{!"ModuleMD", !6}
!6 = !{!"FuncMD", !7, !8, !29, !30}
!7 = !{!"FuncMDMap[0]", void (i8*)* @test_i3dto2da}
!8 = !{!"FuncMDValue[0]", !9}
!9 = !{!"resAllocMD", !10, !11}
!10 = !{!"samplersNumType", i32 0}
!11 = !{!"argAllocMDList", !12, !16, !18, !22, !23, !24, !25, !26, !27, !28}
!12 = !{!"argAllocMDListVec[0]", !13, !14, !15}
!13 = !{!"type", i32 2}
!14 = !{!"extensionType", i32 0}
!15 = !{!"indexType", i32 42}
!16 = !{!"argAllocMDListVec[1]", !17, !14, !15}
!17 = !{!"type", i32 1}
!18 = !{!"argAllocMDListVec[2]", !19, !20, !21}
!19 = !{!"type", i32 4}
!20 = !{!"extensionType", i32 -1}
!21 = !{!"indexType", i32 5}
!22 = !{!"argAllocMDListVec[3]", !19, !20, !21}
!23 = !{!"argAllocMDListVec[4]", !19, !20, !21}
!24 = !{!"argAllocMDListVec[5]", !19, !20, !21}
!25 = !{!"argAllocMDListVec[6]", !19, !20, !21}
!26 = !{!"argAllocMDListVec[7]", !19, !20, !21}
!27 = !{!"argAllocMDListVec[8]", !19, !20, !21}
!28 = !{!"argAllocMDListVec[9]", !17, !20, !15}
!29 = !{!"FuncMDMap[1]", void (float*, i32*)* @test_i3dto2da_f32}
!30 = !{!"FuncMDValue[1]", !9}
