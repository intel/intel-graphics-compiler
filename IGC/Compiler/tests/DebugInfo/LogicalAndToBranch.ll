;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -logicalAndToBranch -S < %s | FileCheck %s
; ------------------------------------------------
; LogicalAndToBranch
; ------------------------------------------------
; This test checks that LogicalAndToBranch pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define void @test_logicaland{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:

; CHECK: [[VAL0_V:%[A-z0-9]*]] = add i32 %a, %a
; CHECK-DAG: [[VAL1_V:%[A-z0-9]*]] = icmp ne i32 %a, %b, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL6_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: [[VAL7_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL7_LOC:![0-9]*]]
; CHECK-DAG: [[VAL8_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL8_LOC:![0-9]*]]
; CHECK-DAG: [[VAL9_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL9_LOC:![0-9]*]]
; CHECK-DAG: [[VAL10_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL10_LOC:![0-9]*]]
; CHECK-DAG: [[VAL11_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL11_LOC:![0-9]*]]
; CHECK-DAG: [[VAL12_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL12_LOC:![0-9]*]]
; CHECK-DAG: [[VAL13_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL13_LOC:![0-9]*]]
; CHECK-DAG: [[VAL14_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL14_LOC:![0-9]*]]
; CHECK-DAG: [[VAL15_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL15_LOC:![0-9]*]]
; CHECK-DAG: [[VAL16_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL16_LOC:![0-9]*]]
; CHECK-DAG: [[VAL17_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL17_LOC:![0-9]*]]
; CHECK-DAG: [[VAL18_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL18_LOC:![0-9]*]]
; CHECK-DAG: [[VAL19_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL19_LOC:![0-9]*]]
; CHECK-DAG: [[VAL20_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL20_LOC:![0-9]*]]
; CHECK-DAG: [[VAL21_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL21_LOC:![0-9]*]]
; CHECK-DAG: [[VAL22_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL22_LOC:![0-9]*]]
; CHECK-DAG: [[VAL23_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL23_LOC:![0-9]*]]
; CHECK-DAG: [[VAL24_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL24_LOC:![0-9]*]]
; CHECK-DAG: [[VAL25_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL25_LOC:![0-9]*]]
; CHECK-DAG: [[VAL26_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL26_LOC:![0-9]*]]
; CHECK-DAG: [[VAL27_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL27_LOC:![0-9]*]]
; CHECK-DAG: [[VAL28_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL28_LOC:![0-9]*]]
; CHECK-DAG: [[VAL29_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL29_LOC:![0-9]*]]
; CHECK-DAG: [[VAL30_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL30_LOC:![0-9]*]]
; CHECK-DAG: [[VAL31_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL31_LOC:![0-9]*]]
; CHECK-DAG: [[VAL32_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL32_LOC:![0-9]*]]
; CHECK-DAG: [[VAL33_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL33_LOC:![0-9]*]]
; CHECK-DAG: [[VAL34_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL34_LOC:![0-9]*]]
; CHECK-DAG: [[VAL35_V:%[A-z0-9]*]] = add{{.*}}, !dbg [[VAL35_LOC:![0-9]*]]
;
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL6_V]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL7_V]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL8_V]], metadata [[VAL8_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL8_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL9_V]], metadata [[VAL9_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL9_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL10_V]], metadata [[VAL10_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL10_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL11_V]], metadata [[VAL11_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL11_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL12_V]], metadata [[VAL12_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL12_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL13_V]], metadata [[VAL13_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL13_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL14_V]], metadata [[VAL14_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL14_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL15_V]], metadata [[VAL15_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL15_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL16_V]], metadata [[VAL16_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL16_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL17_V]], metadata [[VAL17_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL17_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL18_V]], metadata [[VAL18_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL18_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL19_V]], metadata [[VAL19_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL19_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL20_V]], metadata [[VAL20_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL20_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL21_V]], metadata [[VAL21_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL21_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL22_V]], metadata [[VAL22_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL22_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL23_V]], metadata [[VAL23_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL23_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL24_V]], metadata [[VAL24_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL24_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL25_V]], metadata [[VAL25_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL25_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL26_V]], metadata [[VAL26_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL26_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL27_V]], metadata [[VAL27_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL27_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL28_V]], metadata [[VAL28_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL28_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL29_V]], metadata [[VAL29_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL29_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL30_V]], metadata [[VAL30_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL30_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL31_V]], metadata [[VAL31_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL31_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL32_V]], metadata [[VAL32_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL32_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL33_V]], metadata [[VAL33_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL33_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL34_V]], metadata [[VAL34_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL34_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL35_V]], metadata [[VAL35_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL35_LOC]]

; CHECK: [[VAL36_V:%[A-z0-9]*]] = icmp sgt {{.*}}, !dbg [[VAL36_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL36_V]], metadata [[VAL36_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL36_LOC]]
; CHECK: [[VAL37_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL37_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL37_V]], metadata [[VAL37_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL37_LOC]]
; CHECK: [[VAL38_V:%[A-z0-9]*]] = {{.*}}, !dbg [[VAL38_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL38_V]], metadata [[VAL38_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL38_LOC]]


define void @test_logicaland(i32 %a, i32 %b, i32* %c) !dbg !6 {
entry:
  %0 = add i32 %a, %a
  %cmp1 = icmp ne i32 %a, %b, !dbg !50
  call void @llvm.dbg.value(metadata i1 %cmp1, metadata !9, metadata !DIExpression()), !dbg !50
  %1 = add i32 %0, %b, !dbg !51
  call void @llvm.dbg.value(metadata i32 %1, metadata !11, metadata !DIExpression()), !dbg !51
  %2 = add i32 %1, %b, !dbg !52
  call void @llvm.dbg.value(metadata i32 %2, metadata !13, metadata !DIExpression()), !dbg !52
  %3 = add i32 %2, %a, !dbg !53
  call void @llvm.dbg.value(metadata i32 %3, metadata !14, metadata !DIExpression()), !dbg !53
  %4 = add i32 %3, %b, !dbg !54
  call void @llvm.dbg.value(metadata i32 %4, metadata !15, metadata !DIExpression()), !dbg !54
  %5 = add i32 %4, %a, !dbg !55
  call void @llvm.dbg.value(metadata i32 %5, metadata !16, metadata !DIExpression()), !dbg !55
  %6 = add i32 %5, %b, !dbg !56
  call void @llvm.dbg.value(metadata i32 %6, metadata !17, metadata !DIExpression()), !dbg !56
  %7 = add i32 %6, %a, !dbg !57
  call void @llvm.dbg.value(metadata i32 %7, metadata !18, metadata !DIExpression()), !dbg !57
  %8 = add i32 %7, %b, !dbg !58
  call void @llvm.dbg.value(metadata i32 %8, metadata !19, metadata !DIExpression()), !dbg !58
  %9 = add i32 %8, %a, !dbg !59
  call void @llvm.dbg.value(metadata i32 %9, metadata !20, metadata !DIExpression()), !dbg !59
  %10 = add i32 %9, %b, !dbg !60
  call void @llvm.dbg.value(metadata i32 %10, metadata !21, metadata !DIExpression()), !dbg !60
  %11 = add i32 %10, %a, !dbg !61
  call void @llvm.dbg.value(metadata i32 %11, metadata !22, metadata !DIExpression()), !dbg !61
  %12 = add i32 %11, %b, !dbg !62
  call void @llvm.dbg.value(metadata i32 %12, metadata !23, metadata !DIExpression()), !dbg !62
  %13 = add i32 %12, %a, !dbg !63
  call void @llvm.dbg.value(metadata i32 %13, metadata !24, metadata !DIExpression()), !dbg !63
  %14 = add i32 %13, %b, !dbg !64
  call void @llvm.dbg.value(metadata i32 %14, metadata !25, metadata !DIExpression()), !dbg !64
  %15 = add i32 %14, %a, !dbg !65
  call void @llvm.dbg.value(metadata i32 %15, metadata !26, metadata !DIExpression()), !dbg !65
  %16 = add i32 %15, %b, !dbg !66
  call void @llvm.dbg.value(metadata i32 %16, metadata !27, metadata !DIExpression()), !dbg !66
  %17 = add i32 %16, %a, !dbg !67
  call void @llvm.dbg.value(metadata i32 %17, metadata !28, metadata !DIExpression()), !dbg !67
  %18 = add i32 %17, %b, !dbg !68
  call void @llvm.dbg.value(metadata i32 %18, metadata !29, metadata !DIExpression()), !dbg !68
  %19 = add i32 %18, %a, !dbg !69
  call void @llvm.dbg.value(metadata i32 %19, metadata !30, metadata !DIExpression()), !dbg !69
  %20 = add i32 %19, %b, !dbg !70
  call void @llvm.dbg.value(metadata i32 %20, metadata !31, metadata !DIExpression()), !dbg !70
  %21 = add i32 %20, %a, !dbg !71
  call void @llvm.dbg.value(metadata i32 %21, metadata !32, metadata !DIExpression()), !dbg !71
  %22 = add i32 %21, %b, !dbg !72
  call void @llvm.dbg.value(metadata i32 %22, metadata !33, metadata !DIExpression()), !dbg !72
  %23 = add i32 %22, %a, !dbg !73
  call void @llvm.dbg.value(metadata i32 %23, metadata !34, metadata !DIExpression()), !dbg !73
  %24 = add i32 %23, %b, !dbg !74
  call void @llvm.dbg.value(metadata i32 %24, metadata !35, metadata !DIExpression()), !dbg !74
  %25 = add i32 %24, %a, !dbg !75
  call void @llvm.dbg.value(metadata i32 %25, metadata !36, metadata !DIExpression()), !dbg !75
  %26 = add i32 %25, %b, !dbg !76
  call void @llvm.dbg.value(metadata i32 %26, metadata !37, metadata !DIExpression()), !dbg !76
  %27 = add i32 %26, %a, !dbg !77
  call void @llvm.dbg.value(metadata i32 %27, metadata !38, metadata !DIExpression()), !dbg !77
  %28 = add i32 %27, %b, !dbg !78
  call void @llvm.dbg.value(metadata i32 %28, metadata !39, metadata !DIExpression()), !dbg !78
  %29 = add i32 %28, %a, !dbg !79
  call void @llvm.dbg.value(metadata i32 %29, metadata !40, metadata !DIExpression()), !dbg !79
  %30 = add i32 %29, %b, !dbg !80
  call void @llvm.dbg.value(metadata i32 %30, metadata !41, metadata !DIExpression()), !dbg !80
  %31 = add i32 %30, %a, !dbg !81
  call void @llvm.dbg.value(metadata i32 %31, metadata !42, metadata !DIExpression()), !dbg !81
  %32 = add i32 %31, %b, !dbg !82
  call void @llvm.dbg.value(metadata i32 %32, metadata !43, metadata !DIExpression()), !dbg !82
  %33 = add i32 %32, %a, !dbg !83
  call void @llvm.dbg.value(metadata i32 %33, metadata !44, metadata !DIExpression()), !dbg !83
  %34 = add i32 %33, %b, !dbg !84
  call void @llvm.dbg.value(metadata i32 %34, metadata !45, metadata !DIExpression()), !dbg !84
  %cmp2 = icmp sgt i32 %33, %b, !dbg !85
  call void @llvm.dbg.value(metadata i1 %cmp2, metadata !46, metadata !DIExpression()), !dbg !85
  %and = and i1 %cmp1, %cmp2, !dbg !86
  call void @llvm.dbg.value(metadata i1 %and, metadata !47, metadata !DIExpression()), !dbg !86
  %result = select i1 %and, i32 13, i32 %34, !dbg !87
  call void @llvm.dbg.value(metadata i32 %result, metadata !48, metadata !DIExpression()), !dbg !87
  store i32 %result, i32* %c, !dbg !88
  %35 = add i32 %33, 13, !dbg !89
  call void @llvm.dbg.value(metadata i32 %35, metadata !49, metadata !DIExpression()), !dbg !89
  store i32 %35, i32* %c, !dbg !90
  ret void, !dbg !91
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "LogicalAndToBranch.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_logicaland", linkageName: "test_logicaland", scope: null, file: [[FILE]], line: 1
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
; CHECK-DAG: [[VAL10_MD]] = !DILocalVariable(name: "10", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[VAL10_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL11_MD]] = !DILocalVariable(name: "11", scope: [[SCOPE]], file: [[FILE]], line: 11
; CHECK-DAG: [[VAL11_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL12_MD]] = !DILocalVariable(name: "12", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[VAL12_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL13_MD]] = !DILocalVariable(name: "13", scope: [[SCOPE]], file: [[FILE]], line: 13
; CHECK-DAG: [[VAL13_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL14_MD]] = !DILocalVariable(name: "14", scope: [[SCOPE]], file: [[FILE]], line: 14
; CHECK-DAG: [[VAL14_LOC]] = !DILocation(line: 14, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL15_MD]] = !DILocalVariable(name: "15", scope: [[SCOPE]], file: [[FILE]], line: 15
; CHECK-DAG: [[VAL15_LOC]] = !DILocation(line: 15, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL16_MD]] = !DILocalVariable(name: "16", scope: [[SCOPE]], file: [[FILE]], line: 16
; CHECK-DAG: [[VAL16_LOC]] = !DILocation(line: 16, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL17_MD]] = !DILocalVariable(name: "17", scope: [[SCOPE]], file: [[FILE]], line: 17
; CHECK-DAG: [[VAL17_LOC]] = !DILocation(line: 17, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL18_MD]] = !DILocalVariable(name: "18", scope: [[SCOPE]], file: [[FILE]], line: 18
; CHECK-DAG: [[VAL18_LOC]] = !DILocation(line: 18, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL19_MD]] = !DILocalVariable(name: "19", scope: [[SCOPE]], file: [[FILE]], line: 19
; CHECK-DAG: [[VAL19_LOC]] = !DILocation(line: 19, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL20_MD]] = !DILocalVariable(name: "20", scope: [[SCOPE]], file: [[FILE]], line: 20
; CHECK-DAG: [[VAL20_LOC]] = !DILocation(line: 20, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL21_MD]] = !DILocalVariable(name: "21", scope: [[SCOPE]], file: [[FILE]], line: 21
; CHECK-DAG: [[VAL21_LOC]] = !DILocation(line: 21, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL22_MD]] = !DILocalVariable(name: "22", scope: [[SCOPE]], file: [[FILE]], line: 22
; CHECK-DAG: [[VAL22_LOC]] = !DILocation(line: 22, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL23_MD]] = !DILocalVariable(name: "23", scope: [[SCOPE]], file: [[FILE]], line: 23
; CHECK-DAG: [[VAL23_LOC]] = !DILocation(line: 23, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL24_MD]] = !DILocalVariable(name: "24", scope: [[SCOPE]], file: [[FILE]], line: 24
; CHECK-DAG: [[VAL24_LOC]] = !DILocation(line: 24, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL25_MD]] = !DILocalVariable(name: "25", scope: [[SCOPE]], file: [[FILE]], line: 25
; CHECK-DAG: [[VAL25_LOC]] = !DILocation(line: 25, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL26_MD]] = !DILocalVariable(name: "26", scope: [[SCOPE]], file: [[FILE]], line: 26
; CHECK-DAG: [[VAL26_LOC]] = !DILocation(line: 26, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL27_MD]] = !DILocalVariable(name: "27", scope: [[SCOPE]], file: [[FILE]], line: 27
; CHECK-DAG: [[VAL27_LOC]] = !DILocation(line: 27, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL28_MD]] = !DILocalVariable(name: "28", scope: [[SCOPE]], file: [[FILE]], line: 28
; CHECK-DAG: [[VAL28_LOC]] = !DILocation(line: 28, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL29_MD]] = !DILocalVariable(name: "29", scope: [[SCOPE]], file: [[FILE]], line: 29
; CHECK-DAG: [[VAL29_LOC]] = !DILocation(line: 29, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL30_MD]] = !DILocalVariable(name: "30", scope: [[SCOPE]], file: [[FILE]], line: 30
; CHECK-DAG: [[VAL30_LOC]] = !DILocation(line: 30, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL31_MD]] = !DILocalVariable(name: "31", scope: [[SCOPE]], file: [[FILE]], line: 31
; CHECK-DAG: [[VAL31_LOC]] = !DILocation(line: 31, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL32_MD]] = !DILocalVariable(name: "32", scope: [[SCOPE]], file: [[FILE]], line: 32
; CHECK-DAG: [[VAL32_LOC]] = !DILocation(line: 32, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL33_MD]] = !DILocalVariable(name: "33", scope: [[SCOPE]], file: [[FILE]], line: 33
; CHECK-DAG: [[VAL33_LOC]] = !DILocation(line: 33, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL34_MD]] = !DILocalVariable(name: "34", scope: [[SCOPE]], file: [[FILE]], line: 34
; CHECK-DAG: [[VAL34_LOC]] = !DILocation(line: 34, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL35_MD]] = !DILocalVariable(name: "35", scope: [[SCOPE]], file: [[FILE]], line: 35
; CHECK-DAG: [[VAL35_LOC]] = !DILocation(line: 35, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL36_MD]] = !DILocalVariable(name: "36", scope: [[SCOPE]], file: [[FILE]], line: 36
; CHECK-DAG: [[VAL36_LOC]] = !DILocation(line: 36, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL37_MD]] = !DILocalVariable(name: "37", scope: [[SCOPE]], file: [[FILE]], line: 37
; CHECK-DAG: [[VAL37_LOC]] = !DILocation(line: 37, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL38_MD]] = !DILocalVariable(name: "38", scope: [[SCOPE]], file: [[FILE]], line: 38
; CHECK-DAG: [[VAL38_LOC]] = !DILocation(line: 38, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "LogicalAndToBranch.ll", directory: "/")
!2 = !{}
!3 = !{i32 42}
!4 = !{i32 39}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_logicaland", linkageName: "test_logicaland", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !15, !16, !17, !18, !19, !20, !21, !22, !23, !24, !25, !26, !27, !28, !29, !30, !31, !32, !33, !34, !35, !36, !37, !38, !39, !40, !41, !42, !43, !44, !45, !46, !47, !48, !49}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !12)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !12)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !12)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !12)
!17 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !12)
!18 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !12)
!19 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 9, type: !12)
!20 = !DILocalVariable(name: "10", scope: !6, file: !1, line: 10, type: !12)
!21 = !DILocalVariable(name: "11", scope: !6, file: !1, line: 11, type: !12)
!22 = !DILocalVariable(name: "12", scope: !6, file: !1, line: 12, type: !12)
!23 = !DILocalVariable(name: "13", scope: !6, file: !1, line: 13, type: !12)
!24 = !DILocalVariable(name: "14", scope: !6, file: !1, line: 14, type: !12)
!25 = !DILocalVariable(name: "15", scope: !6, file: !1, line: 15, type: !12)
!26 = !DILocalVariable(name: "16", scope: !6, file: !1, line: 16, type: !12)
!27 = !DILocalVariable(name: "17", scope: !6, file: !1, line: 17, type: !12)
!28 = !DILocalVariable(name: "18", scope: !6, file: !1, line: 18, type: !12)
!29 = !DILocalVariable(name: "19", scope: !6, file: !1, line: 19, type: !12)
!30 = !DILocalVariable(name: "20", scope: !6, file: !1, line: 20, type: !12)
!31 = !DILocalVariable(name: "21", scope: !6, file: !1, line: 21, type: !12)
!32 = !DILocalVariable(name: "22", scope: !6, file: !1, line: 22, type: !12)
!33 = !DILocalVariable(name: "23", scope: !6, file: !1, line: 23, type: !12)
!34 = !DILocalVariable(name: "24", scope: !6, file: !1, line: 24, type: !12)
!35 = !DILocalVariable(name: "25", scope: !6, file: !1, line: 25, type: !12)
!36 = !DILocalVariable(name: "26", scope: !6, file: !1, line: 26, type: !12)
!37 = !DILocalVariable(name: "27", scope: !6, file: !1, line: 27, type: !12)
!38 = !DILocalVariable(name: "28", scope: !6, file: !1, line: 28, type: !12)
!39 = !DILocalVariable(name: "29", scope: !6, file: !1, line: 29, type: !12)
!40 = !DILocalVariable(name: "30", scope: !6, file: !1, line: 30, type: !12)
!41 = !DILocalVariable(name: "31", scope: !6, file: !1, line: 31, type: !12)
!42 = !DILocalVariable(name: "32", scope: !6, file: !1, line: 32, type: !12)
!43 = !DILocalVariable(name: "33", scope: !6, file: !1, line: 33, type: !12)
!44 = !DILocalVariable(name: "34", scope: !6, file: !1, line: 34, type: !12)
!45 = !DILocalVariable(name: "35", scope: !6, file: !1, line: 35, type: !12)
!46 = !DILocalVariable(name: "36", scope: !6, file: !1, line: 36, type: !10)
!47 = !DILocalVariable(name: "37", scope: !6, file: !1, line: 37, type: !10)
!48 = !DILocalVariable(name: "38", scope: !6, file: !1, line: 38, type: !12)
!49 = !DILocalVariable(name: "39", scope: !6, file: !1, line: 40, type: !12)
!50 = !DILocation(line: 1, column: 1, scope: !6)
!51 = !DILocation(line: 2, column: 1, scope: !6)
!52 = !DILocation(line: 3, column: 1, scope: !6)
!53 = !DILocation(line: 4, column: 1, scope: !6)
!54 = !DILocation(line: 5, column: 1, scope: !6)
!55 = !DILocation(line: 6, column: 1, scope: !6)
!56 = !DILocation(line: 7, column: 1, scope: !6)
!57 = !DILocation(line: 8, column: 1, scope: !6)
!58 = !DILocation(line: 9, column: 1, scope: !6)
!59 = !DILocation(line: 10, column: 1, scope: !6)
!60 = !DILocation(line: 11, column: 1, scope: !6)
!61 = !DILocation(line: 12, column: 1, scope: !6)
!62 = !DILocation(line: 13, column: 1, scope: !6)
!63 = !DILocation(line: 14, column: 1, scope: !6)
!64 = !DILocation(line: 15, column: 1, scope: !6)
!65 = !DILocation(line: 16, column: 1, scope: !6)
!66 = !DILocation(line: 17, column: 1, scope: !6)
!67 = !DILocation(line: 18, column: 1, scope: !6)
!68 = !DILocation(line: 19, column: 1, scope: !6)
!69 = !DILocation(line: 20, column: 1, scope: !6)
!70 = !DILocation(line: 21, column: 1, scope: !6)
!71 = !DILocation(line: 22, column: 1, scope: !6)
!72 = !DILocation(line: 23, column: 1, scope: !6)
!73 = !DILocation(line: 24, column: 1, scope: !6)
!74 = !DILocation(line: 25, column: 1, scope: !6)
!75 = !DILocation(line: 26, column: 1, scope: !6)
!76 = !DILocation(line: 27, column: 1, scope: !6)
!77 = !DILocation(line: 28, column: 1, scope: !6)
!78 = !DILocation(line: 29, column: 1, scope: !6)
!79 = !DILocation(line: 30, column: 1, scope: !6)
!80 = !DILocation(line: 31, column: 1, scope: !6)
!81 = !DILocation(line: 32, column: 1, scope: !6)
!82 = !DILocation(line: 33, column: 1, scope: !6)
!83 = !DILocation(line: 34, column: 1, scope: !6)
!84 = !DILocation(line: 35, column: 1, scope: !6)
!85 = !DILocation(line: 36, column: 1, scope: !6)
!86 = !DILocation(line: 37, column: 1, scope: !6)
!87 = !DILocation(line: 38, column: 1, scope: !6)
!88 = !DILocation(line: 39, column: 1, scope: !6)
!89 = !DILocation(line: 40, column: 1, scope: !6)
!90 = !DILocation(line: 41, column: 1, scope: !6)
!91 = !DILocation(line: 42, column: 1, scope: !6)
