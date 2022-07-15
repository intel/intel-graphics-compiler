;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-synchronization-object-coalescing -S < %s | FileCheck %s
; ------------------------------------------------
; SynchronizationObjectCoalescing
; ------------------------------------------------
; This test checks that SynchronizationObjectCoalescing pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test1{{.*}} !dbg [[SCOPE1:![0-9]*]]
; CHECK: store float {{.*}}, !dbg [[STR1_LOC:![0-9]*]]
; CHECK: call void @llvm.genx.GenISA.memoryfence{{.*}}, !dbg [[FENCE1_LOC:![0-9]*]]
; CHECK: call void @llvm.genx.GenISA.threadgroupbarrier{{.*}}, !dbg [[BARRIER1_LOC:![0-9]*]]
; CHECK: [[LOAD1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[LOAD1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[LOAD1_V]], metadata [[LOAD1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD1_LOC]]

define void @test1(float addrspace(3)* %src1, float addrspace(3)* %src2) !dbg !6 {
  store float 5.000000e-01, float addrspace(3)* %src1, align 4, !dbg !11
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false), !dbg !12
  call void @llvm.genx.GenISA.threadgroupbarrier(), !dbg !13
  %1 = load float, float addrspace(3)* %src2, align 4, !dbg !14
  call void @llvm.dbg.value(metadata float %1, metadata !9, metadata !DIExpression()), !dbg !14
  ret void, !dbg !15
}

; CHECK: define void @test2{{.*}} !dbg [[SCOPE2:![0-9]*]]
; CHECK: [[LOAD2_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[LOAD2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[LOAD2_V]], metadata [[LOAD2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD2_LOC]]
; CHECK: call void @llvm.genx.GenISA.memoryfence{{.*}}, !dbg [[FENCE2_LOC:![0-9]*]]
; CHECK: call void @llvm.genx.GenISA.threadgroupbarrier{{.*}}, !dbg [[BARRIER2_LOC:![0-9]*]]
; CHECK: store float {{.*}}, !dbg [[STR2_LOC:![0-9]*]]

define void @test2(float addrspace(3)* %src1, float addrspace(3)* %src2) !dbg !16 {
  %1 = load float, float addrspace(3)* %src2, align 4, !dbg !19
  call void @llvm.dbg.value(metadata float %1, metadata !18, metadata !DIExpression()), !dbg !19
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false), !dbg !20
  call void @llvm.genx.GenISA.threadgroupbarrier(), !dbg !21
  store float 5.000000e-01, float addrspace(3)* %src1, align 4, !dbg !22
  ret void, !dbg !23
}

; CHECK: define void @test3{{.*}} !dbg [[SCOPE3:![0-9]*]]
; CHECK: store float {{.*}}, !dbg [[STR3_LOC:![0-9]*]]
; CHECK: call void @llvm.genx.GenISA.memoryfence{{.*}}, !dbg [[FENCE3_LOC:![0-9]*]]
; CHECK: call void @llvm.genx.GenISA.threadgroupbarrier{{.*}}, !dbg [[BARRIER3_LOC:![0-9]*]]
; CHECK: store float {{.*}}, !dbg [[STR4_LOC:![0-9]*]]

define void @test3(float addrspace(3)* %src1, float addrspace(3)* %src2) !dbg !24 {
  store float 5.000000e-01, float addrspace(3)* %src1, align 4, !dbg !25
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false), !dbg !26
  call void @llvm.genx.GenISA.threadgroupbarrier(), !dbg !27
  store float 5.000000e-01, float addrspace(3)* %src2, align 4, !dbg !28
  ret void, !dbg !29
}

; CHECK: define void @test4{{.*}} !dbg [[SCOPE4:![0-9]*]]
; CHECK: [[LOAD3_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[LOAD3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[LOAD3_V]], metadata [[LOAD3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD3_LOC]]
; CHECK: [[LOAD4_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[LOAD4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[LOAD4_V]], metadata [[LOAD4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD4_LOC]]

define void @test4(float addrspace(3)* %src1, float addrspace(3)* %src2) !dbg !30 {
  %1 = load float, float addrspace(3)* %src2, align 4, !dbg !34
  call void @llvm.dbg.value(metadata float %1, metadata !32, metadata !DIExpression()), !dbg !34
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false), !dbg !35
  call void @llvm.genx.GenISA.threadgroupbarrier(), !dbg !36
  %2 = load float, float addrspace(3)* %src1, align 4, !dbg !37
  call void @llvm.dbg.value(metadata float %2, metadata !33, metadata !DIExpression()), !dbg !37
  ret void, !dbg !38
}

; CHECK: define void @test5{{.*}} !dbg [[SCOPE5:![0-9]*]]
; CHECK: store float {{.*}}, !dbg [[STR5_LOC:![0-9]*]]

define void @test5(float addrspace(3)* %src1, float addrspace(3)* %src2) !dbg !39 {
  store float 5.000000e-01, float addrspace(3)* %src1, align 4, !dbg !40
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false), !dbg !41
  call void @llvm.genx.GenISA.threadgroupbarrier(), !dbg !42
  ret void, !dbg !43
}

; CHECK: define void @test6{{.*}} !dbg [[SCOPE6:![0-9]*]]
; CHECK: store float {{.*}}, !dbg [[STR6_LOC:![0-9]*]]

define void @test6(float addrspace(3)* %src1, float addrspace(3)* %src2) !dbg !44 {
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false), !dbg !45
  call void @llvm.genx.GenISA.threadgroupbarrier(), !dbg !46
  store float 5.000000e-01, float addrspace(3)* %src2, align 4, !dbg !47
  ret void, !dbg !48
}

define void @test7(float addrspace(3)* %src1, float addrspace(3)* %src2) !dbg !49 {
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false), !dbg !50
  call void @llvm.genx.GenISA.threadgroupbarrier(), !dbg !51
  ret void, !dbg !52
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "SynchronizationObjectCoalescing.ll", directory: "/")
; CHECK-DAG: [[SCOPE1]] = distinct !DISubprogram(name: "test1", linkageName: "test1", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[STR1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[FENCE1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[BARRIER1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE1]])
; CHECK-DAG: [[LOAD1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE1]], file: [[FILE]], line: 4
; CHECK-DAG: [[LOAD1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE1]])


; CHECK-DAG: [[SCOPE2]] = distinct !DISubprogram(name: "test2", linkageName: "test2", scope: null, file: [[FILE]], line: 6
; CHECK-DAG: [[LOAD2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE2]], file: [[FILE]], line: 6
; CHECK-DAG: [[LOAD2_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE2]])
; CHECK-DAG: [[FENCE2_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE2]])
; CHECK-DAG: [[BARRIER2_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE2]])
; CHECK-DAG: [[STR2_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE2]])

; CHECK-DAG: [[SCOPE3]] = distinct !DISubprogram(name: "test3", linkageName: "test3", scope: null, file: [[FILE]], line: 11
; CHECK-DAG: [[STR3_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE3]])
; CHECK-DAG: [[FENCE3_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE3]])
; CHECK-DAG: [[BARRIER3_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE3]])
; CHECK-DAG: [[STR4_LOC]] = !DILocation(line: 14, column: 1, scope: [[SCOPE3]])

; CHECK-DAG: [[SCOPE4]] = distinct !DISubprogram(name: "test4", linkageName: "test4", scope: null, file: [[FILE]], line: 16
; CHECK-DAG: [[LOAD3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE4]], file: [[FILE]], line: 16
; CHECK-DAG: [[LOAD3_LOC]] = !DILocation(line: 16, column: 1, scope: [[SCOPE4]])
; CHECK-DAG: [[LOAD4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE4]], file: [[FILE]], line: 19
; CHECK-DAG: [[LOAD4_LOC]] = !DILocation(line: 19, column: 1, scope: [[SCOPE4]])

; CHECK-DAG: [[SCOPE5]] = distinct !DISubprogram(name: "test5", linkageName: "test5", scope: null, file: [[FILE]], line: 21
; CHECK-DAG: [[STR5_LOC]] = !DILocation(line: 21, column: 1, scope: [[SCOPE5]])

; CHECK-DAG: [[SCOPE6]] = distinct !DISubprogram(name: "test6", linkageName: "test6", scope: null, file: [[FILE]], line: 25
; CHECK-DAG: [[STR6_LOC]] = !DILocation(line: 27, column: 1, scope: [[SCOPE6]])


declare void @llvm.genx.GenISA.memoryfence(i1, i1, i1, i1, i1, i1, i1)

declare void @llvm.genx.GenISA.threadgroupbarrier()

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "SynchronizationObjectCoalescing.ll", directory: "/")
!2 = !{}
!3 = !{i32 31}
!4 = !{i32 4}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test1", linkageName: "test1", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 4, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocation(line: 1, column: 1, scope: !6)
!12 = !DILocation(line: 2, column: 1, scope: !6)
!13 = !DILocation(line: 3, column: 1, scope: !6)
!14 = !DILocation(line: 4, column: 1, scope: !6)
!15 = !DILocation(line: 5, column: 1, scope: !6)
!16 = distinct !DISubprogram(name: "test2", linkageName: "test2", scope: null, file: !1, line: 6, type: !7, scopeLine: 6, unit: !0, retainedNodes: !17)
!17 = !{!18}
!18 = !DILocalVariable(name: "2", scope: !16, file: !1, line: 6, type: !10)
!19 = !DILocation(line: 6, column: 1, scope: !16)
!20 = !DILocation(line: 7, column: 1, scope: !16)
!21 = !DILocation(line: 8, column: 1, scope: !16)
!22 = !DILocation(line: 9, column: 1, scope: !16)
!23 = !DILocation(line: 10, column: 1, scope: !16)
!24 = distinct !DISubprogram(name: "test3", linkageName: "test3", scope: null, file: !1, line: 11, type: !7, scopeLine: 11, unit: !0, retainedNodes: !2)
!25 = !DILocation(line: 11, column: 1, scope: !24)
!26 = !DILocation(line: 12, column: 1, scope: !24)
!27 = !DILocation(line: 13, column: 1, scope: !24)
!28 = !DILocation(line: 14, column: 1, scope: !24)
!29 = !DILocation(line: 15, column: 1, scope: !24)
!30 = distinct !DISubprogram(name: "test4", linkageName: "test4", scope: null, file: !1, line: 16, type: !7, scopeLine: 16, unit: !0, retainedNodes: !31)
!31 = !{!32, !33}
!32 = !DILocalVariable(name: "3", scope: !30, file: !1, line: 16, type: !10)
!33 = !DILocalVariable(name: "4", scope: !30, file: !1, line: 19, type: !10)
!34 = !DILocation(line: 16, column: 1, scope: !30)
!35 = !DILocation(line: 17, column: 1, scope: !30)
!36 = !DILocation(line: 18, column: 1, scope: !30)
!37 = !DILocation(line: 19, column: 1, scope: !30)
!38 = !DILocation(line: 20, column: 1, scope: !30)
!39 = distinct !DISubprogram(name: "test5", linkageName: "test5", scope: null, file: !1, line: 21, type: !7, scopeLine: 21, unit: !0, retainedNodes: !2)
!40 = !DILocation(line: 21, column: 1, scope: !39)
!41 = !DILocation(line: 22, column: 1, scope: !39)
!42 = !DILocation(line: 23, column: 1, scope: !39)
!43 = !DILocation(line: 24, column: 1, scope: !39)
!44 = distinct !DISubprogram(name: "test6", linkageName: "test6", scope: null, file: !1, line: 25, type: !7, scopeLine: 25, unit: !0, retainedNodes: !2)
!45 = !DILocation(line: 25, column: 1, scope: !44)
!46 = !DILocation(line: 26, column: 1, scope: !44)
!47 = !DILocation(line: 27, column: 1, scope: !44)
!48 = !DILocation(line: 28, column: 1, scope: !44)
!49 = distinct !DISubprogram(name: "test7", linkageName: "test7", scope: null, file: !1, line: 29, type: !7, scopeLine: 29, unit: !0, retainedNodes: !2)
!50 = !DILocation(line: 29, column: 1, scope: !49)
!51 = !DILocation(line: 30, column: 1, scope: !49)
!52 = !DILocation(line: 31, column: 1, scope: !49)
