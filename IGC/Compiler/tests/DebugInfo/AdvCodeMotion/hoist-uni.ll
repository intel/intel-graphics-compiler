;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt  -adv-codemotion-cm=1 -igc-advcodemotion -S < %s | FileCheck %s
; ------------------------------------------------
; AdvCodeMotion
; ------------------------------------------------
; This test checks that AdvCodeMotion pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @test{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: entry:
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: [[VAL3_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL3_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL3_V]], metadata [[VAL3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL3_LOC]]
; CHECK: [[VAL4_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL4_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL4_V]], metadata [[VAL4_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL4_LOC]]
; CHECK: [[VAL5_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL5_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL5_V]], metadata [[VAL5_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL5_LOC]]
; CHECK: [[VAL6_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL6_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL6_V]], metadata [[VAL6_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL6_LOC]]
; CHECK: [[VAL7_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL7_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL7_V]], metadata [[VAL7_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL7_LOC]]
; CHECK: [[VAL8_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL8_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL8_V]], metadata [[VAL8_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL8_LOC]]
; CHECK: [[VAL9_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL9_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL9_V]], metadata [[VAL9_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL9_LOC]]

; CHECK: bb1:
; CHECK: [[VAL10_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL10_LOC:![0-9]*]]
; CHECK: [[VAL11_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL11_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL10_V]], metadata [[VAL10_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL10_LOC]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL11_V]], metadata [[VAL11_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL11_LOC]]
; CHECK: [[VAL12_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL12_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL12_V]], metadata [[VAL12_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL12_LOC]]
; CHECK: [[VAL13_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL13_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL13_V]], metadata [[VAL13_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL13_LOC]]
; CHECK: [[VAL14_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL14_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 [[VAL14_V]], metadata [[VAL14_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL14_LOC]]

; Modified part, check that uniform ir is moved to bb2

; CHECK: bb2:
; CHECK-DAG: [[AAA_V:%aaa]] = {{.*}}, !dbg [[AAA_LOC:![0-9]*]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[AAA_V]], metadata [[AAA_MD:![0-9]*]], metadata !DIExpression()), !dbg [[AAA_LOC]]
; CHECK-DAG: br {{.*}}, !dbg [[BR_LOC:![0-9]*]]


; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test(i32 addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, <3 x i32> %globalSize, <3 x i32> %enqueuedLocalSize, <3 x i32> %localSize, i8* %privateBase, i32 %bufferOffset) #0 !dbg !19 {
entry:
  %0 = extractelement <8 x i32> %r0, i32 1, !dbg !44
  call void @llvm.dbg.value(metadata i32 %0, metadata !22, metadata !DIExpression()), !dbg !44
  %1 = extractelement <3 x i32> %globalSize, i32 0, !dbg !45
  call void @llvm.dbg.value(metadata i32 %1, metadata !24, metadata !DIExpression()), !dbg !45
  %2 = extractelement <3 x i32> %localSize, i32 0, !dbg !46
  call void @llvm.dbg.value(metadata i32 %2, metadata !25, metadata !DIExpression()), !dbg !46
  %3 = extractelement <3 x i32> %enqueuedLocalSize, i32 0, !dbg !47
  call void @llvm.dbg.value(metadata i32 %3, metadata !26, metadata !DIExpression()), !dbg !47
  %4 = mul i32 %3, %0, !dbg !48
  call void @llvm.dbg.value(metadata i32 %4, metadata !27, metadata !DIExpression()), !dbg !48
  %5 = zext i16 %localIdX to i32, !dbg !49
  call void @llvm.dbg.value(metadata i32 %5, metadata !28, metadata !DIExpression()), !dbg !49
  %6 = add i32 %5, %4, !dbg !50
  call void @llvm.dbg.value(metadata i32 %6, metadata !29, metadata !DIExpression()), !dbg !50
  %7 = extractelement <8 x i32> %payloadHeader, i32 0, !dbg !51
  call void @llvm.dbg.value(metadata i32 %7, metadata !30, metadata !DIExpression()), !dbg !51
  %8 = add i32 %6, %7, !dbg !52
  call void @llvm.dbg.value(metadata i32 %8, metadata !31, metadata !DIExpression()), !dbg !52
  br label %bb3, !dbg !53

bb1:                                              ; preds = %bb3, %bb1
  %a = phi i32 [ %b, %bb3 ], [ %ai, %bb1 ], !dbg !54
  %lc = phi i32 [ %bi, %bb3 ], [ %lc, %bb1 ], !dbg !55
  call void @llvm.dbg.value(metadata i32 %a, metadata !32, metadata !DIExpression()), !dbg !54
  call void @llvm.dbg.value(metadata i32 %lc, metadata !33, metadata !DIExpression()), !dbg !55
  %ai = add i32 %a, 1, !dbg !56
  call void @llvm.dbg.value(metadata i32 %ai, metadata !34, metadata !DIExpression()), !dbg !56
  %ac = icmp ne i32 %8, %ai, !dbg !57
  call void @llvm.dbg.value(metadata i1 %ac, metadata !35, metadata !DIExpression()), !dbg !57
  %cc = icmp eq i32 %ai, %lc, !dbg !58
  call void @llvm.dbg.value(metadata i1 %cc, metadata !37, metadata !DIExpression()), !dbg !58
  br i1 %cc, label %bb2, label %bb1, !dbg !59

bb2:                                              ; preds = %bb1
  br i1 %ac, label %tbb, label %fbb, !dbg !60

bb3:                                              ; preds = %join, %entry
  %b = phi i32 [ -1, %entry ], [ %bi, %join ], !dbg !61
  %bl = phi i32 [ 0, %entry ], [ %bli, %join ], !dbg !62
  call void @llvm.dbg.value(metadata i32 %b, metadata !38, metadata !DIExpression()), !dbg !61
  call void @llvm.dbg.value(metadata i32 %bl, metadata !39, metadata !DIExpression()), !dbg !62
  %bi = add i32 %b, %2, !dbg !63
  call void @llvm.dbg.value(metadata i32 %bi, metadata !40, metadata !DIExpression()), !dbg !63
  %bli = add i32 %bl, %2, !dbg !64
  call void @llvm.dbg.value(metadata i32 %bli, metadata !41, metadata !DIExpression()), !dbg !64
  %bc = icmp ult i32 %bli, %1, !dbg !65
  call void @llvm.dbg.value(metadata i1 %bc, metadata !42, metadata !DIExpression()), !dbg !65
  br i1 %bc, label %bb1, label %end, !dbg !66

tbb:                                              ; preds = %bb2
  %aaa = add i32 %1, %2, !dbg !67
  call void @llvm.dbg.value(metadata i32 %aaa, metadata !43, metadata !DIExpression()), !dbg !67
  br label %join, !dbg !68

fbb:                                              ; preds = %bb2
  br label %join, !dbg !69

join:                                             ; preds = %fbb, %tbb
  br label %bb3, !dbg !70

end:                                              ; preds = %bb3
  store i32 %8, i32 addrspace(1)* %dst, align 4, !dbg !71
  ret void, !dbg !72
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "hoist-uni.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
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
; CHECK-DAG: [[VAL10_MD]] = !DILocalVariable(name: "10", scope: [[SCOPE]], file: [[FILE]], line: 11
; CHECK-DAG: [[VAL10_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL11_MD]] = !DILocalVariable(name: "11", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[VAL11_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL12_MD]] = !DILocalVariable(name: "12", scope: [[SCOPE]], file: [[FILE]], line: 13
; CHECK-DAG: [[VAL12_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL13_MD]] = !DILocalVariable(name: "13", scope: [[SCOPE]], file: [[FILE]], line: 14
; CHECK-DAG: [[VAL13_LOC]] = !DILocation(line: 14, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[VAL14_MD]] = !DILocalVariable(name: "14", scope: [[SCOPE]], file: [[FILE]], line: 15
; CHECK-DAG: [[VAL14_LOC]] = !DILocation(line: 15, column: 1, scope: [[SCOPE]])

; CHECK-DAG: [[AAA_MD]] = !DILocalVariable(name: "20", scope: [[SCOPE]], file: [[FILE]], line: 24
; CHECK-DAG: [[AAA_LOC]] = !DILocation(line: 24, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BR_LOC]] = !DILocation(line: 17, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_local_size(i32) local_unnamed_addr #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_global_size(i32) local_unnamed_addr #2

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { convergent noinline nounwind optnone }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { convergent nounwind readnone }

!llvm.module.flags = !{!0, !1, !2}
!igc.functions = !{!3}
!llvm.dbg.cu = !{!14}
!llvm.debugify = !{!17, !18}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = !{i32 1, !"wchar_size", i32 4}
!3 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, <3 x i32>, <3 x i32>, <3 x i32>, i8*, i32)* @test, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8, !9, !10, !11, !12}
!7 = !{i32 0}
!8 = !{i32 1}
!9 = !{i32 4}
!10 = !{i32 5}
!11 = !{i32 12}
!12 = !{i32 14, !13}
!13 = !{!"explicit_arg_num", i32 0}
!14 = distinct !DICompileUnit(language: DW_LANG_C, file: !15, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !16)
!15 = !DIFile(filename: "hoist-uni.ll", directory: "/")
!16 = !{}
!17 = !{i32 29}
!18 = !{i32 20}
!19 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !15, line: 1, type: !20, scopeLine: 1, unit: !14, retainedNodes: !21)
!20 = !DISubroutineType(types: !16)
!21 = !{!22, !24, !25, !26, !27, !28, !29, !30, !31, !32, !33, !34, !35, !37, !38, !39, !40, !41, !42, !43}
!22 = !DILocalVariable(name: "1", scope: !19, file: !15, line: 1, type: !23)
!23 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!24 = !DILocalVariable(name: "2", scope: !19, file: !15, line: 2, type: !23)
!25 = !DILocalVariable(name: "3", scope: !19, file: !15, line: 3, type: !23)
!26 = !DILocalVariable(name: "4", scope: !19, file: !15, line: 4, type: !23)
!27 = !DILocalVariable(name: "5", scope: !19, file: !15, line: 5, type: !23)
!28 = !DILocalVariable(name: "6", scope: !19, file: !15, line: 6, type: !23)
!29 = !DILocalVariable(name: "7", scope: !19, file: !15, line: 7, type: !23)
!30 = !DILocalVariable(name: "8", scope: !19, file: !15, line: 8, type: !23)
!31 = !DILocalVariable(name: "9", scope: !19, file: !15, line: 9, type: !23)
!32 = !DILocalVariable(name: "10", scope: !19, file: !15, line: 11, type: !23)
!33 = !DILocalVariable(name: "11", scope: !19, file: !15, line: 12, type: !23)
!34 = !DILocalVariable(name: "12", scope: !19, file: !15, line: 13, type: !23)
!35 = !DILocalVariable(name: "13", scope: !19, file: !15, line: 14, type: !36)
!36 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!37 = !DILocalVariable(name: "14", scope: !19, file: !15, line: 15, type: !36)
!38 = !DILocalVariable(name: "15", scope: !19, file: !15, line: 18, type: !23)
!39 = !DILocalVariable(name: "16", scope: !19, file: !15, line: 19, type: !23)
!40 = !DILocalVariable(name: "17", scope: !19, file: !15, line: 20, type: !23)
!41 = !DILocalVariable(name: "18", scope: !19, file: !15, line: 21, type: !23)
!42 = !DILocalVariable(name: "19", scope: !19, file: !15, line: 22, type: !36)
!43 = !DILocalVariable(name: "20", scope: !19, file: !15, line: 24, type: !23)
!44 = !DILocation(line: 1, column: 1, scope: !19)
!45 = !DILocation(line: 2, column: 1, scope: !19)
!46 = !DILocation(line: 3, column: 1, scope: !19)
!47 = !DILocation(line: 4, column: 1, scope: !19)
!48 = !DILocation(line: 5, column: 1, scope: !19)
!49 = !DILocation(line: 6, column: 1, scope: !19)
!50 = !DILocation(line: 7, column: 1, scope: !19)
!51 = !DILocation(line: 8, column: 1, scope: !19)
!52 = !DILocation(line: 9, column: 1, scope: !19)
!53 = !DILocation(line: 10, column: 1, scope: !19)
!54 = !DILocation(line: 11, column: 1, scope: !19)
!55 = !DILocation(line: 12, column: 1, scope: !19)
!56 = !DILocation(line: 13, column: 1, scope: !19)
!57 = !DILocation(line: 14, column: 1, scope: !19)
!58 = !DILocation(line: 15, column: 1, scope: !19)
!59 = !DILocation(line: 16, column: 1, scope: !19)
!60 = !DILocation(line: 17, column: 1, scope: !19)
!61 = !DILocation(line: 18, column: 1, scope: !19)
!62 = !DILocation(line: 19, column: 1, scope: !19)
!63 = !DILocation(line: 20, column: 1, scope: !19)
!64 = !DILocation(line: 21, column: 1, scope: !19)
!65 = !DILocation(line: 22, column: 1, scope: !19)
!66 = !DILocation(line: 23, column: 1, scope: !19)
!67 = !DILocation(line: 24, column: 1, scope: !19)
!68 = !DILocation(line: 25, column: 1, scope: !19)
!69 = !DILocation(line: 26, column: 1, scope: !19)
!70 = !DILocation(line: 27, column: 1, scope: !19)
!71 = !DILocation(line: 28, column: 1, scope: !19)
!72 = !DILocation(line: 29, column: 1, scope: !19)
