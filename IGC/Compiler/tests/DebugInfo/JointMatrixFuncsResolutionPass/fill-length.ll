;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-joint-matrix-resolution -S < %s | FileCheck %s
; ------------------------------------------------
; JointMatrixFuncsResolutionPass
; ------------------------------------------------
; This test checks that JointMatrixFuncsResolutionPass pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata {{.*}}, metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: call void {{.*}}, !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 32, metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: store {{.*}}, !dbg [[STORE2_LOC:![0-9]*]]

%intel.joint_matrix_packedA_8x4_i8_ = type opaque

define void @test(i32 %a, i8* %dst, i32* %dst2) !dbg !6 {
  %1 = call spir_func %intel.joint_matrix_packedA_8x4_i8_* @__builtin_spirv_OpCompositeConstructJointMatrixINTEL(i32 %a), !dbg !13
  call void @llvm.dbg.value(metadata %intel.joint_matrix_packedA_8x4_i8_* %1, metadata !9, metadata !DIExpression()), !dbg !13
  call spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL(i8* %dst, %intel.joint_matrix_packedA_8x4_i8_* %1, i32 8, i32 0), !dbg !14
  %2 = call spir_func i32 @__builtin_spirv_OpJointMatrixWorkItemLengthINTEL(%intel.joint_matrix_packedA_8x4_i8_* %1), !dbg !15
  call void @llvm.dbg.value(metadata i32 %2, metadata !11, metadata !DIExpression()), !dbg !15
  store i32 %2, i32* %dst2, !dbg !16
  ret void, !dbg !17
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "fill-length.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])

declare spir_func %intel.joint_matrix_packedA_8x4_i8_* @__builtin_spirv_OpCompositeConstructJointMatrixINTEL(i32)

declare spir_func i32 @__builtin_spirv_OpJointMatrixWorkItemLengthINTEL(%intel.joint_matrix_packedA_8x4_i8_*)

declare spir_func void @__builtin_spirv_OpJointMatrixStoreINTEL(i8*, %intel.joint_matrix_packedA_8x4_i8_*, i32, i32)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "fill-length.ll", directory: "/")
!2 = !{}
!3 = !{i32 5}
!4 = !{i32 2}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 3, type: !12)
!12 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!13 = !DILocation(line: 1, column: 1, scope: !6)
!14 = !DILocation(line: 2, column: 1, scope: !6)
!15 = !DILocation(line: 3, column: 1, scope: !6)
!16 = !DILocation(line: 4, column: 1, scope: !6)
!17 = !DILocation(line: 5, column: 1, scope: !6)
