;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-early-out-patterns-pass -S -inputcs < %s | FileCheck %s
; ------------------------------------------------
; EarlyOutPatterns
; ------------------------------------------------
; This test checks that EarlyOutPatterns pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @test_earlyout{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK-DAG: [[VAL1_V:%[A-z0-9]*]] = uitofp i32 %a {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V:%[A-z0-9]*]] = uitofp i32 %b {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V:%[A-z0-9]*]] = uitofp i32 %c {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V:%[A-z0-9]*]] = fmul {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V:%[A-z0-9]*]] = fmul {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL6_V:%[A-z0-9]*]] = fmul {{.*}}, !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: [[VAL7_V:%[A-z0-9]*]] = fadd {{.*}}, !dbg [[VAL7_LOC:![0-9]*]]
; CHECK-DAG: [[VAL8_V:%[A-z0-9]*]] = fadd {{.*}}, !dbg [[VAL8_LOC:![0-9]*]]
; CHECK-DAG: [[VAL9_V:%[A-z0-9]*]] = fcmp {{.*}}, !dbg [[VAL9_LOC:![0-9]*]]
;
; CHECK: {{.*}}:
; CHECK: void @llvm.dbg.value(metadata float [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL6_V]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL7_V]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL8_V]], metadata [[VAL8_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL8_LOC]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL9_V]], metadata [[VAL9_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL9_LOC]]
;
; CHECK: [[VAL17_V:%[A-z0-9]*]] = fmul {{.*}}, !dbg [[VAL17_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL17_V]], metadata [[VAL17_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL17_LOC]]

; CHECK: {{.*}}:
; CHECK: void @llvm.dbg.value(metadata float [[VAL1_V]], metadata [[VAL1_MD]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL2_V]], metadata [[VAL2_MD]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL3_V]], metadata [[VAL3_MD]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL4_V]], metadata [[VAL4_MD]], metadata !DIExpression()), !dbg [[VAL4_LOC]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL5_V]], metadata [[VAL5_MD]], metadata !DIExpression()), !dbg [[VAL5_LOC]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL6_V]], metadata [[VAL6_MD]], metadata !DIExpression()), !dbg [[VAL6_LOC]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL7_V]], metadata [[VAL7_MD]], metadata !DIExpression()), !dbg [[VAL7_LOC]]
; CHECK: void @llvm.dbg.value(metadata float [[VAL8_V]], metadata [[VAL8_MD]], metadata !DIExpression()), !dbg [[VAL8_LOC]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL9_V]], metadata [[VAL9_MD]], metadata !DIExpression()), !dbg [[VAL9_LOC]]
;
; CHECK: [[VAL17_V:%[A-z0-9]*]] = fmul {{.*}}, !dbg [[VAL17_LOC]]
; CHECK: void @llvm.dbg.value(metadata float {{0.0.*}}, metadata [[VAL17_MD]], metadata !DIExpression()), !dbg [[VAL17_LOC]]

define spir_kernel void @test_earlyout(i32 %a, i32 %b, i32 %c, float* %d) !dbg !6 {
entry:
  %bt1 = uitofp i32 %a to float, !dbg !35
  call void @llvm.dbg.value(metadata float %bt1, metadata !9, metadata !DIExpression()), !dbg !35
  %bt2 = uitofp i32 %b to float, !dbg !36
  call void @llvm.dbg.value(metadata float %bt2, metadata !11, metadata !DIExpression()), !dbg !36
  %bt3 = uitofp i32 %c to float, !dbg !37
  call void @llvm.dbg.value(metadata float %bt3, metadata !12, metadata !DIExpression()), !dbg !37
  %fmul1 = fmul float %bt1, %bt1, !dbg !38
  call void @llvm.dbg.value(metadata float %fmul1, metadata !13, metadata !DIExpression()), !dbg !38
  %fmul2 = fmul float %bt2, %bt2, !dbg !39
  call void @llvm.dbg.value(metadata float %fmul2, metadata !14, metadata !DIExpression()), !dbg !39
  %fmul3 = fmul float %bt3, %bt3, !dbg !40
  call void @llvm.dbg.value(metadata float %fmul3, metadata !15, metadata !DIExpression()), !dbg !40
  %fadd1 = fadd float %fmul2, %fmul3, !dbg !41
  call void @llvm.dbg.value(metadata float %fadd1, metadata !16, metadata !DIExpression()), !dbg !41
  %0 = fadd float %fmul1, %fadd1, !dbg !42
  call void @llvm.dbg.value(metadata float %0, metadata !17, metadata !DIExpression()), !dbg !42
  %1 = fcmp fast ogt float %0, 0.000000e+00, !dbg !43
  call void @llvm.dbg.value(metadata i1 %1, metadata !18, metadata !DIExpression()), !dbg !43
  %2 = sext i1 %1 to i32, !dbg !44
  call void @llvm.dbg.value(metadata i32 %2, metadata !20, metadata !DIExpression()), !dbg !44
  %3 = and i32 %a, %2, !dbg !45
  call void @llvm.dbg.value(metadata i32 %3, metadata !21, metadata !DIExpression()), !dbg !45
  %4 = and i32 %b, %2, !dbg !46
  call void @llvm.dbg.value(metadata i32 %4, metadata !22, metadata !DIExpression()), !dbg !46
  %5 = and i32 %c, %2, !dbg !47
  call void @llvm.dbg.value(metadata i32 %5, metadata !23, metadata !DIExpression()), !dbg !47
  %6 = bitcast i32 %3 to float, !dbg !48
  call void @llvm.dbg.value(metadata float %6, metadata !24, metadata !DIExpression()), !dbg !48
  %7 = bitcast i32 %4 to float, !dbg !49
  call void @llvm.dbg.value(metadata float %7, metadata !25, metadata !DIExpression()), !dbg !49
  %8 = bitcast i32 %5 to float, !dbg !50
  call void @llvm.dbg.value(metadata float %8, metadata !26, metadata !DIExpression()), !dbg !50
  %9 = fmul fast float %bt3, %6, !dbg !51
  call void @llvm.dbg.value(metadata float %9, metadata !27, metadata !DIExpression()), !dbg !51
  %10 = fmul fast float %bt2, %7, !dbg !52
  call void @llvm.dbg.value(metadata float %10, metadata !28, metadata !DIExpression()), !dbg !52
  %11 = fmul fast float %bt1, %8, !dbg !53
  call void @llvm.dbg.value(metadata float %11, metadata !29, metadata !DIExpression()), !dbg !53
  %12 = call fast float @llvm.maxnum.f32(float %9, float 0.000000e+00), !dbg !54
  call void @llvm.dbg.value(metadata float %12, metadata !30, metadata !DIExpression()), !dbg !54
  %13 = call fast float @llvm.maxnum.f32(float %10, float 0.000000e+00), !dbg !55
  call void @llvm.dbg.value(metadata float %13, metadata !31, metadata !DIExpression()), !dbg !55
  %14 = call fast float @llvm.maxnum.f32(float %11, float 0.000000e+00), !dbg !56
  call void @llvm.dbg.value(metadata float %14, metadata !32, metadata !DIExpression()), !dbg !56
  %15 = fadd float %12, %13, !dbg !57
  call void @llvm.dbg.value(metadata float %15, metadata !33, metadata !DIExpression()), !dbg !57
  %16 = fadd float %15, %14, !dbg !58
  call void @llvm.dbg.value(metadata float %16, metadata !34, metadata !DIExpression()), !dbg !58
  store float %16, float* %d, !dbg !59
  ret void, !dbg !60
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "dotp.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_earlyout", linkageName: "test_earlyout", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL7_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[VAL7_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL8_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL8_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL9_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[VAL9_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL17_MD]] = !DILocalVariable(name: "17", scope: [[SCOPE]], file: [[FILE]], line: 17
; CHECK-DAG: [[VAL17_LOC]] = !DILocation(line: 17, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare float @llvm.maxnum.f32(float, float) #0

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "dotp.ll", directory: "/")
!2 = !{}
!3 = !{i32 26}
!4 = !{i32 24}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_earlyout", linkageName: "test_earlyout", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !12, !13, !14, !15, !16, !17, !18, !20, !21, !22, !23, !24, !25, !26, !27, !28, !29, !30, !31, !32, !33, !34}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !10)
!12 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!13 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !10)
!14 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !10)
!15 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !10)
!16 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !10)
!17 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !10)
!18 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 9, type: !19)
!19 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!20 = !DILocalVariable(name: "10", scope: !6, file: !1, line: 10, type: !10)
!21 = !DILocalVariable(name: "11", scope: !6, file: !1, line: 11, type: !10)
!22 = !DILocalVariable(name: "12", scope: !6, file: !1, line: 12, type: !10)
!23 = !DILocalVariable(name: "13", scope: !6, file: !1, line: 13, type: !10)
!24 = !DILocalVariable(name: "14", scope: !6, file: !1, line: 14, type: !10)
!25 = !DILocalVariable(name: "15", scope: !6, file: !1, line: 15, type: !10)
!26 = !DILocalVariable(name: "16", scope: !6, file: !1, line: 16, type: !10)
!27 = !DILocalVariable(name: "17", scope: !6, file: !1, line: 17, type: !10)
!28 = !DILocalVariable(name: "18", scope: !6, file: !1, line: 18, type: !10)
!29 = !DILocalVariable(name: "19", scope: !6, file: !1, line: 19, type: !10)
!30 = !DILocalVariable(name: "20", scope: !6, file: !1, line: 20, type: !10)
!31 = !DILocalVariable(name: "21", scope: !6, file: !1, line: 21, type: !10)
!32 = !DILocalVariable(name: "22", scope: !6, file: !1, line: 22, type: !10)
!33 = !DILocalVariable(name: "23", scope: !6, file: !1, line: 23, type: !10)
!34 = !DILocalVariable(name: "24", scope: !6, file: !1, line: 24, type: !10)
!35 = !DILocation(line: 1, column: 1, scope: !6)
!36 = !DILocation(line: 2, column: 1, scope: !6)
!37 = !DILocation(line: 3, column: 1, scope: !6)
!38 = !DILocation(line: 4, column: 1, scope: !6)
!39 = !DILocation(line: 5, column: 1, scope: !6)
!40 = !DILocation(line: 6, column: 1, scope: !6)
!41 = !DILocation(line: 7, column: 1, scope: !6)
!42 = !DILocation(line: 8, column: 1, scope: !6)
!43 = !DILocation(line: 9, column: 1, scope: !6)
!44 = !DILocation(line: 10, column: 1, scope: !6)
!45 = !DILocation(line: 11, column: 1, scope: !6)
!46 = !DILocation(line: 12, column: 1, scope: !6)
!47 = !DILocation(line: 13, column: 1, scope: !6)
!48 = !DILocation(line: 14, column: 1, scope: !6)
!49 = !DILocation(line: 15, column: 1, scope: !6)
!50 = !DILocation(line: 16, column: 1, scope: !6)
!51 = !DILocation(line: 17, column: 1, scope: !6)
!52 = !DILocation(line: 18, column: 1, scope: !6)
!53 = !DILocation(line: 19, column: 1, scope: !6)
!54 = !DILocation(line: 20, column: 1, scope: !6)
!55 = !DILocation(line: 21, column: 1, scope: !6)
!56 = !DILocation(line: 22, column: 1, scope: !6)
!57 = !DILocation(line: 23, column: 1, scope: !6)
!58 = !DILocation(line: 24, column: 1, scope: !6)
!59 = !DILocation(line: 25, column: 1, scope: !6)
!60 = !DILocation(line: 26, column: 1, scope: !6)
