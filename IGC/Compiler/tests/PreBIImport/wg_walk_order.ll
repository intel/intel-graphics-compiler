;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -platformpvc -igc-Pre-BIImport-Analysis -serialize-igc-metadata -S < %s | FileCheck %s
; ------------------------------------------------
; ------------------------------------------------
; PreBIImportAnalysis
; ------------------------------------------------
; This test checks that PreBIImportAnalysis sets workGroupWalkOrder meatData.
; ------------------------------------------------

define void @test_wg_walk_order() {
  %1 = call spir_func i32 @_Z40__spirv_BuiltInSubgroupLocalInvocationIdv() #0
  ret void
}

; CHECK-DAG: [[FMD:![0-9]*]] = !{!"FuncMD", [[FMDMAP:![0-9]*]], [[FMDVAL:![0-9]*]], [[T0:![0-9]*]], [[T1:![0-9]*]]}
; CHECK-DAG: [[FMDMAP]] = !{!"FuncMDMap[0]", void ()* @test_wg_walk_order}
; CHECK-DAG: [[FMDVAL]] = !{!"FuncMDValue[0]", [[T2:![0-9]*]], [[WGWO:![0-9]*]], [[T3:.*]]}
; CHECK-DAG: [[WGWO]] = !{!"workGroupWalkOrder", [[DIM0:![0-9]*]], [[DIM1:![0-9]*]], [[DIM2:![0-9]*]]}
; CHECK-DAG: [[DIM0]] = !{!"dim0", i32 0}
; CHECK-DAG: [[DIM1]] = !{!"dim1", i32 1}
; CHECK-DAG: [[DIM2]] = !{!"dim2", i32 2}

declare spir_func i32 @_Z40__spirv_BuiltInSubgroupLocalInvocationIdv() #0

attributes #0 = { nounwind readnone willreturn }

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD", !4}
!1 = !{void ()* @test_wg_walk_order, !2}
!2 = !{!3}
!3 = !{!"function_type"}
!4 = !{!"FuncMD", !5, !6}
!5 = distinct !{!"FuncMDMap[0]", void ()* @test_wg_walk_order}
!6 = !{!"FuncMDValue[0]", !7}
!7 = !{!"workGroupWalkOrder", !8, !9, !10}
!8 = !{!"dim0", i32 0}
!9 = !{!"dim1", i32 0}
!10 = !{!"dim2", i32 0}
