;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -CMLowerVLoadVStore -march=genx64 -mtriple=spir64-unkonwn-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; CMLowerVLoadVStore
; ------------------------------------------------
; This test checks that CMLowerVLoadVStore pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; Check that lowered instructions have their location and values preserved
;
; CHECK: void @test_vloadvstore{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x i32>* [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata <4 x i32> [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: store <4 x i32> {{.*}}, !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: [[VAL4_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: [[VAL4_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: [[VAL5_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: [[VAL5_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL5_LOC]]
; CHECK: store <4 x i32> {{.*}}, !dbg [[STORE2_LOC:![0-9]*]]

@global_vec = internal global <4 x i32> undef, align 8 #0

define void @test_vloadvstore(<4 x i32>* %a, <4 x i32> addrspace(4)* %b) !dbg !6 {
entry:
  %0 = alloca <4 x i32>, align 8, !dbg !20
  call void @llvm.dbg.value(metadata <4 x i32>* %0, metadata !9, metadata !DIExpression()), !dbg !20
  %1 = load <4 x i32>, <4 x i32>* %a, !dbg !21
  call void @llvm.dbg.value(metadata <4 x i32> %1, metadata !11, metadata !DIExpression()), !dbg !21
  %2 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v4i32.i16(<4 x i32> %1, i32 1, i32 1, i32 0, i16 0, i32 0), !dbg !22
  call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> %2, <4 x i32>* %0), !dbg !23
  %3 = call <4 x i32> @llvm.genx.vload.v4i32.p0v4i32(<4 x i32>* %0), !dbg !24
  %4 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.v4i32.i16(<4 x i32> %1, <4 x i32> %3, i32 1, i32 1, i32 0, i16 0, i32 0), !dbg !25
  store <4 x i32> %4, <4 x i32>* %a, !dbg !26
  ret void, !dbg !27
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "CMLowerVLoadVStore_debug.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_vloadvstore", linkageName: "test_vloadvstore", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])

declare <4 x i32> @llvm.genx.vload.v4i32.p0v4i32(<4 x i32>*)
declare void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32>, <4 x i32>*)
declare <4 x i32> @llvm.genx.rdregioni.v4i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32)
declare <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.v4i32.i16(<4 x i32>, <4 x i32>, i32, i32, i32, i16, i32)
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { "VCByteOffset"="0" "VCGlobalVariable" "VCVolatile" "genx_byte_offset"="0" "genx_volatile" }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "CMLowerVLoadVStore_debug.ll", directory: "/")
!2 = !{}
!4 = !{i32 9}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_vloadvstore", linkageName: "test_vloadvstore", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!20 = !DILocation(line: 1, column: 1, scope: !6)
!21 = !DILocation(line: 2, column: 1, scope: !6)
!22 = !DILocation(line: 3, column: 1, scope: !6)
!23 = !DILocation(line: 4, column: 1, scope: !6)
!24 = !DILocation(line: 5, column: 1, scope: !6)
!25 = !DILocation(line: 6, column: 1, scope: !6)
!26 = !DILocation(line: 7, column: 1, scope: !6)
!27 = !DILocation(line: 8, column: 1, scope: !6)

