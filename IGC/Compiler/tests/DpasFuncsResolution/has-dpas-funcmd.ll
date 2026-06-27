;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -platformCri --igc-arith-funcs-translation -igc-serialize-metadata -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; DpasFuncsResolution records per-function DPAS presence in FunctionMetaData
; ------------------------------------------------

; CHECK-DAG: call i32 @llvm.genx.GenISA.sub.group.dpas.i32
; CHECK-DAG: call i16 @llvm.genx.GenISA.sub.group.dpas.i16
; CHECK-DAG: call <8 x float> @llvm.genx.GenISA.sub.group.bdpas

; The serializer dedups the hasDPAS leaf, so the three DPAS functions all reference
; one "i1 true" node and the no-DPAS function the "i1 false" node. Bind both node IDs
; then check each FuncMDValue[N] references the expected one.

; CHECK-DAG: ![[TRUE:[0-9]+]] = !{!"hasDPAS", i1 true}
; CHECK-DAG: ![[FALSE:[0-9]+]] = !{!"hasDPAS", i1 false}
; CHECK-DAG: !{!"FuncMDMap[0]", ptr @test_idpas}
; CHECK-DAG: !{!"FuncMDValue[0]",{{.*}}![[TRUE]]{{[,}]}}
; CHECK-DAG: !{!"FuncMDMap[1]", ptr @test_fdpas}
; CHECK-DAG: !{!"FuncMDValue[1]",{{.*}}![[TRUE]]{{[,}]}}
; CHECK-DAG: !{!"FuncMDMap[2]", ptr @test_bdpas}
; CHECK-DAG: !{!"FuncMDValue[2]",{{.*}}![[TRUE]]{{[,}]}}
; CHECK-DAG: !{!"FuncMDMap[3]", ptr @test_no_dpas}
; CHECK-DAG: !{!"FuncMDValue[3]",{{.*}}![[FALSE]]{{[,}]}}


define spir_kernel void @test_idpas(i32 %acc, i16 %a, <8 x i32> %b, ptr %dst) {
  %r= call i32 @__builtin_IB_sub_group16_idpas_s8_s8_8_1(i32 %acc, i16 %a, <8 x i32> %b)
  store i32 %r, ptr %dst, align 4
  ret void
}

define spir_kernel void @test_fdpas(i16 %acc, i16 %a, <8 x i32> %b, ptr %dst) {
  %r = call i16 @__builtin_IB_sub_group16_fdpas_bf_bf_bf_bf_8_1(i16 %acc, i16 %a, <8 x i32> %b)
  store i16 %r, ptr %dst, align 2
  ret void
}

define spir_kernel void @test_bdpas(<8 x float> %acc, <8 x i16> %a, <8 x i32> %b, i8 %sa, i8 %sb, ptr %dst) {
  %r = call <8 x float> @__builtin_IB_sub_group16_bdpas_f_f_bf_bf_8_8(<8 x float> %acc, <8 x i16> %a, <8 x i32> %b, i8 %sa, i8 %sb)
  store <8 x float> %r, ptr %dst, align 4
  ret void
}

; A function with no DPAS keeps hasDPAS = false.
define spir_kernel void @test_no_dpas(i32 %a, ptr %dst) {
  store i32 %a, ptr %dst, align 4
  ret void
}

declare i32         @__builtin_IB_sub_group16_idpas_s8_s8_8_1(i32, i16, <8 x i32>)
declare i16         @__builtin_IB_sub_group16_fdpas_bf_bf_bf_bf_8_1(i16, i16, <8 x i32>)
declare <8 x float> @__builtin_IB_sub_group16_bdpas_f_f_bf_bf_8_8(<8 x float>, <8 x i16>, <8 x i32>, i8, i8)

!IGCMetadata = !{!0}
!igc.functions = !{!10, !11, !12, !13}

!0  = !{!"ModuleMD", !1}
!1  = !{!"FuncMD", !2, !3, !4, !5, !6, !7, !8, !9}
!2  = !{!"FuncMDMap[0]", ptr @test_idpas}
!3  = !{!"FuncMDValue[0]", !20}
!4  = !{!"FuncMDMap[1]", ptr @test_fdpas}
!5  = !{!"FuncMDValue[1]", !20}
!6  = !{!"FuncMDMap[2]", ptr @test_bdpas}
!7  = !{!"FuncMDValue[2]", !20}
!8  = !{!"FuncMDMap[3]", ptr @test_no_dpas}
!9  = !{!"FuncMDValue[3]", !20}
!20 = !{!"hasDPAS", i1 false}
!10 = !{ptr @test_idpas,  !30}
!11 = !{ptr @test_fdpas,  !30}
!12 = !{ptr @test_bdpas,  !30}
!13 = !{ptr @test_no_dpas, !30}
!30 = !{!31}
!31 = !{!"function_type", i32 0}
