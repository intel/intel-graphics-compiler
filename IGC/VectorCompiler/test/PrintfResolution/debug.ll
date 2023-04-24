;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXPrintfResolution -vc-printf-bif-path=%VC_PRITF_OCL_BIF% -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXPrintfResolution
; ------------------------------------------------
; This test checks that GenXPrintfResolution pass follows
; 'How to Update Debug Info' llvm guideline.

; CHECK: void @resolution{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i8 addrspace(2)* [[INT_V:%[A-z0-9.]*]], metadata [[INT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[INT_LOC:![0-9]*]]
; CHECK-DAG: [[INT_V]] = {{.*}}, !dbg [[INT_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL1_V:%[A-z0-9.]*]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC:![0-9]*]]
; CHECK-DAG: [[VAL1_V]] = {{.*}}, !dbg [[VAL1_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i8 addrspace(2)* [[FLOAT_V:%[A-z0-9.]*]], metadata [[FLOAT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FLOAT_LOC:![0-9]*]]
; CHECK-DAG: [[FLOAT_V]] = {{.*}}, !dbg [[FLOAT_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL2_V:%[A-z0-9.]*]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC:![0-9]*]]
; CHECK-DAG: [[VAL2_V]] = {{.*}}, !dbg [[VAL2_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i8 addrspace(2)* [[CHAR_V:%[A-z0-9.]*]], metadata [[CHAR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CHAR_LOC:![0-9]*]]
; CHECK-DAG: [[CHAR_V]] = {{.*}}, !dbg [[CHAR_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL3_V:%[A-z0-9.]*]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC:![0-9]*]]
; CHECK-DAG: [[VAL3_V]] = {{.*}}, !dbg [[VAL3_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i8 addrspace(2)* [[SHORT_V:%[A-z0-9.]*]], metadata [[SHORT_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SHORT_LOC:![0-9]*]]
; CHECK-DAG: [[SHORT_V]] = {{.*}}, !dbg [[SHORT_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL4_V:%[A-z0-9.]*]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC:![0-9]*]]
; CHECK-DAG: [[VAL4_V]] = {{.*}}, !dbg [[VAL4_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i8 addrspace(2)* [[LONG_V:%[A-z0-9.]*]], metadata [[LONG_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LONG_LOC:![0-9]*]]
; CHECK-DAG: [[LONG_V]] = {{.*}}, !dbg [[LONG_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL5_V:%[A-z0-9.]*]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC:![0-9]*]]
; CHECK-DAG: [[VAL5_V]] = {{.*}}, !dbg [[VAL5_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i8 addrspace(2)* [[PTR_V:%[A-z0-9.]*]], metadata [[PTR_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PTR_LOC:![0-9]*]]
; CHECK-DAG: [[PTR_V]] = {{.*}}, !dbg [[PTR_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[VAL6_V:%[A-z0-9.]*]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC:![0-9]*]]
; CHECK-DAG: [[VAL6_V]] = {{.*}}, !dbg [[VAL6_LOC]]

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"

@int.str = internal unnamed_addr addrspace(2) constant [3 x i8] c"%d\00", align 1
@float.str = internal unnamed_addr addrspace(2) constant [3 x i8] c"%f\00", align 1
@char.str = internal unnamed_addr addrspace(2) constant [5 x i8] c"%hhd\00", align 1
@short.str = internal unnamed_addr addrspace(2) constant [4 x i8] c"%hd\00", align 1
@long.str = internal unnamed_addr addrspace(2) constant [4 x i8] c"%ld\00", align 1
@ptr.str = internal unnamed_addr addrspace(2) constant [3 x i8] c"%p\00", align 1

declare spir_func i32 @printf(i8 addrspace(2)*, ...)

define dllexport spir_kernel void @resolution(i32 %s1, float %s2, i8 %s3, i16 %s4, i64 %s5, i32* %s6) !dbg !6 {
  %int.str.ptr = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @int.str, i64 0, i64 0, !dbg !23
  call void @llvm.dbg.value(metadata i8 addrspace(2)* %int.str.ptr, metadata !9, metadata !DIExpression()), !dbg !23
  %1 = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %int.str.ptr, i32 %s1), !dbg !24
  call void @llvm.dbg.value(metadata i32 %1, metadata !11, metadata !DIExpression()), !dbg !24
  %float.str.ptr = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @float.str, i64 0, i64 0, !dbg !25
  call void @llvm.dbg.value(metadata i8 addrspace(2)* %float.str.ptr, metadata !13, metadata !DIExpression()), !dbg !25
  %2 = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %float.str.ptr, float %s2), !dbg !26
  call void @llvm.dbg.value(metadata i32 %2, metadata !14, metadata !DIExpression()), !dbg !26
  %char.str.ptr = getelementptr inbounds [5 x i8], [5 x i8] addrspace(2)* @char.str, i64 0, i64 0, !dbg !27
  call void @llvm.dbg.value(metadata i8 addrspace(2)* %char.str.ptr, metadata !15, metadata !DIExpression()), !dbg !27
  %3 = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %char.str.ptr, i8 %s3), !dbg !28
  call void @llvm.dbg.value(metadata i32 %3, metadata !16, metadata !DIExpression()), !dbg !28
  %short.str.ptr = getelementptr inbounds [4 x i8], [4 x i8] addrspace(2)* @short.str, i64 0, i64 0, !dbg !29
  call void @llvm.dbg.value(metadata i8 addrspace(2)* %short.str.ptr, metadata !17, metadata !DIExpression()), !dbg !29
  %4 = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %short.str.ptr, i16 %s4), !dbg !30
  call void @llvm.dbg.value(metadata i32 %4, metadata !18, metadata !DIExpression()), !dbg !30
  %long.str.ptr = getelementptr inbounds [4 x i8], [4 x i8] addrspace(2)* @long.str, i64 0, i64 0, !dbg !31
  call void @llvm.dbg.value(metadata i8 addrspace(2)* %long.str.ptr, metadata !19, metadata !DIExpression()), !dbg !31
  %5 = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %long.str.ptr, i64 %s5), !dbg !32
  call void @llvm.dbg.value(metadata i32 %5, metadata !20, metadata !DIExpression()), !dbg !32
  %ptr.str.ptr = getelementptr inbounds [3 x i8], [3 x i8] addrspace(2)* @ptr.str, i64 0, i64 0, !dbg !33
  call void @llvm.dbg.value(metadata i8 addrspace(2)* %ptr.str.ptr, metadata !21, metadata !DIExpression()), !dbg !33
  %6 = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %ptr.str.ptr, i32* %s6), !dbg !34
  call void @llvm.dbg.value(metadata i32 %6, metadata !22, metadata !DIExpression()), !dbg !34
  ret void, !dbg !35
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "GenXPrintfResolution.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "resolution", linkageName: "resolution", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[INT_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[INT_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL1_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL1_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])

; CHECK-DAG: [[FLOAT_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[FLOAT_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])

; CHECK-DAG: [[CHAR_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[CHAR_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL3_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[VAL3_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])

; CHECK-DAG: [[SHORT_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[SHORT_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL4_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[VAL4_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])

; CHECK-DAG: [[LONG_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[LONG_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL5_MD]] = !DILocalVariable(name: "10", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[VAL5_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])

; CHECK-DAG: [[PTR_MD]] = !DILocalVariable(name: "11", scope: [[SCOPE]], file: [[FILE]], line: 11
; CHECK-DAG: [[PTR_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL6_MD]] = !DILocalVariable(name: "12", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[VAL6_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "GenXPrintfResolution.ll", directory: "/")
!2 = !{}
!3 = !{i32 13}
!4 = !{i32 12}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "resolution", linkageName: "resolution", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9, !11, !13, !14, !15, !16, !17, !18, !19, !20, !21, !22}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!11 = !DILocalVariable(name: "2", scope: !6, file: !1, line: 2, type: !12)
!12 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!13 = !DILocalVariable(name: "3", scope: !6, file: !1, line: 3, type: !10)
!14 = !DILocalVariable(name: "4", scope: !6, file: !1, line: 4, type: !12)
!15 = !DILocalVariable(name: "5", scope: !6, file: !1, line: 5, type: !10)
!16 = !DILocalVariable(name: "6", scope: !6, file: !1, line: 6, type: !12)
!17 = !DILocalVariable(name: "7", scope: !6, file: !1, line: 7, type: !10)
!18 = !DILocalVariable(name: "8", scope: !6, file: !1, line: 8, type: !12)
!19 = !DILocalVariable(name: "9", scope: !6, file: !1, line: 9, type: !10)
!20 = !DILocalVariable(name: "10", scope: !6, file: !1, line: 10, type: !12)
!21 = !DILocalVariable(name: "11", scope: !6, file: !1, line: 11, type: !10)
!22 = !DILocalVariable(name: "12", scope: !6, file: !1, line: 12, type: !12)
!23 = !DILocation(line: 1, column: 1, scope: !6)
!24 = !DILocation(line: 2, column: 1, scope: !6)
!25 = !DILocation(line: 3, column: 1, scope: !6)
!26 = !DILocation(line: 4, column: 1, scope: !6)
!27 = !DILocation(line: 5, column: 1, scope: !6)
!28 = !DILocation(line: 6, column: 1, scope: !6)
!29 = !DILocation(line: 7, column: 1, scope: !6)
!30 = !DILocation(line: 8, column: 1, scope: !6)
!31 = !DILocation(line: 9, column: 1, scope: !6)
!32 = !DILocation(line: 10, column: 1, scope: !6)
!33 = !DILocation(line: 11, column: 1, scope: !6)
!34 = !DILocation(line: 12, column: 1, scope: !6)
!35 = !DILocation(line: 13, column: 1, scope: !6)
