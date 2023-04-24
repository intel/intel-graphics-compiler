;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXPacketize -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXPacketize
; ------------------------------------------------
; This test checks that GenXPacketize pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
;
; Entry part

define void @test_packetize(i32 %a) !dbg !6 {
  %1 = insertelement <8 x i32> zeroinitializer, i32 %a, i32 2, !dbg !11
  call void @llvm.dbg.value(metadata <8 x i32> %1, metadata !9, metadata !DIExpression()), !dbg !11
  call void @test_vectorize(<8 x i32> %1), !dbg !12
  ret void, !dbg !13
}

; Modified part:
;
; Currently this value is not salvageble, test checks that it's not propageted wrongly
; Correct it if functionality is added.
;
; CHECK: end:
; CHECK-NOT: void @llvm.dbg.value(metadata {{.*}} [[VAL:%[A-z0-9]*]]
;

define internal void @test_vectorize(<8 x i32> %a) #0 !dbg !14 {
entry:
  %0 = extractelement <8 x i32> %a, i32 2, !dbg !28
  call void @llvm.dbg.value(metadata i32 %0, metadata !16, metadata !DIExpression()), !dbg !28
  %1 = call i32 @foo(i32 %0), !dbg !29
  call void @llvm.dbg.value(metadata i32 %1, metadata !18, metadata !DIExpression()), !dbg !29
  %2 = inttoptr i32 %1 to i32*, !dbg !30
  call void @llvm.dbg.declare(metadata i32* %2, metadata !19, metadata !DIExpression()), !dbg !30
  %3 = addrspacecast i32* %2 to i32 addrspace(1)*, !dbg !31
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %3, metadata !21, metadata !DIExpression()), !dbg !31
  %4 = getelementptr i32, i32 addrspace(1)* %3, i32 %0, !dbg !32
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %4, metadata !22, metadata !DIExpression()), !dbg !32
  %5 = load i32, i32 addrspace(1)* %4, align 4, !dbg !33
  call void @llvm.dbg.value(metadata i32 %5, metadata !23, metadata !DIExpression()), !dbg !33
  %6 = icmp sgt i32 %5, %0, !dbg !34
  call void @llvm.dbg.value(metadata i1 %6, metadata !24, metadata !DIExpression()), !dbg !34
  br i1 %6, label %end, label %bb, !dbg !35

bb:                                               ; preds = %entry
  %7 = select i1 %6, i32 %0, i32 %5, !dbg !36
  call void @llvm.dbg.value(metadata i32 %7, metadata !26, metadata !DIExpression()), !dbg !36
  br label %end, !dbg !37

end:                                              ; preds = %bb, %entry
  %8 = phi i32 [ %7, %bb ], [ %0, %entry ], !dbg !38
  call void @llvm.dbg.value(metadata i32 %8, metadata !27, metadata !DIExpression()), !dbg !38
  store i32 %8, i32 addrspace(1)* %4, align 4, !dbg !39
  ret void, !dbg !40
}

define internal i32 @foo(i32 %a) !dbg !41 {
  ret i32 %a, !dbg !42
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

attributes #0 = { "CMGenxSIMT"="8" }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "1", directory: "/")
!2 = !{}
!3 = !{i32 17}
!4 = !{i32 10}
!5 = !{i32 2, !"Debug Info Version", i32 3}
!6 = distinct !DISubprogram(name: "test_packetize", linkageName: "test_packetize", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !10)
!10 = !DIBasicType(name: "ty256", size: 256, encoding: DW_ATE_unsigned)
!11 = !DILocation(line: 1, column: 1, scope: !6)
!12 = !DILocation(line: 2, column: 1, scope: !6)
!13 = !DILocation(line: 3, column: 1, scope: !6)
!14 = distinct !DISubprogram(name: "test_vectorize", linkageName: "test_vectorize", scope: null, file: !1, line: 4, type: !7, scopeLine: 4, unit: !0, retainedNodes: !15)
!15 = !{!16, !18, !19, !21, !22, !23, !24, !26, !27}
!16 = !DILocalVariable(name: "2", scope: !14, file: !1, line: 4, type: !17)
!17 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!18 = !DILocalVariable(name: "3", scope: !14, file: !1, line: 5, type: !17)
!19 = !DILocalVariable(name: "4", scope: !14, file: !1, line: 6, type: !20)
!20 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!21 = !DILocalVariable(name: "5", scope: !14, file: !1, line: 7, type: !20)
!22 = !DILocalVariable(name: "6", scope: !14, file: !1, line: 8, type: !20)
!23 = !DILocalVariable(name: "7", scope: !14, file: !1, line: 9, type: !17)
!24 = !DILocalVariable(name: "8", scope: !14, file: !1, line: 10, type: !25)
!25 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!26 = !DILocalVariable(name: "9", scope: !14, file: !1, line: 12, type: !17)
!27 = !DILocalVariable(name: "10", scope: !14, file: !1, line: 14, type: !17)
!28 = !DILocation(line: 4, column: 1, scope: !14)
!29 = !DILocation(line: 5, column: 1, scope: !14)
!30 = !DILocation(line: 6, column: 1, scope: !14)
!31 = !DILocation(line: 7, column: 1, scope: !14)
!32 = !DILocation(line: 8, column: 1, scope: !14)
!33 = !DILocation(line: 9, column: 1, scope: !14)
!34 = !DILocation(line: 10, column: 1, scope: !14)
!35 = !DILocation(line: 11, column: 1, scope: !14)
!36 = !DILocation(line: 12, column: 1, scope: !14)
!37 = !DILocation(line: 13, column: 1, scope: !14)
!38 = !DILocation(line: 14, column: 1, scope: !14)
!39 = !DILocation(line: 15, column: 1, scope: !14)
!40 = !DILocation(line: 16, column: 1, scope: !14)
!41 = distinct !DISubprogram(name: "foo", linkageName: "foo", scope: null, file: !1, line: 17, type: !7, scopeLine: 17, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !2)
!42 = !DILocation(line: 17, column: 1, scope: !41)
