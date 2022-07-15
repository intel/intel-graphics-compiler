;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-lower-implicit-arg-intrinsic -S < %s | FileCheck %s
; ------------------------------------------------
; LowerImplicitArgIntrinsics
; ------------------------------------------------
; This test checks that LowerImplicitArgIntrinsics pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_func void @test{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: store
; CHECK-DAG: void @llvm.dbg.value(metadata i16 [[LIDX_V:%[A-z0-9]*]], metadata [[LIDX_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LIDX_LOC:![0-9]*]]
; CHECK-DAG: [[LIDX_V]] =  {{.*}}, !dbg [[LIDX_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[EXT1_V:%[A-z0-9]*]], metadata [[EXT1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EXT1_LOC:![0-9]*]]
; CHECK-DAG: [[EXT1_V]] =  {{.*}}, !dbg [[EXT1_LOC]]
; CHECK: store
; CHECK-DAG: void @llvm.dbg.value(metadata i16 [[LIDY_V:%[A-z0-9]*]], metadata [[LIDY_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LIDY_LOC:![0-9]*]]
; CHECK-DAG: [[LIDY_V]] =  {{.*}}, !dbg [[LIDY_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[EXT2_V:%[A-z0-9]*]], metadata [[EXT2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EXT2_LOC:![0-9]*]]
; CHECK-DAG: [[EXT2_V]] =  {{.*}}, !dbg [[EXT2_LOC]]
; CHECK: store
; CHECK-DAG: void @llvm.dbg.value(metadata i16 [[LIDZ_V:%[A-z0-9]*]], metadata [[LIDZ_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LIDZ_LOC:![0-9]*]]
; CHECK-DAG: [[LIDZ_V]] =  {{.*}}, !dbg [[LIDZ_LOC]]
; CHECK-DAG: void @llvm.dbg.value(metadata i32 [[EXT3_V:%[A-z0-9]*]], metadata [[EXT3_MD:![0-9]*]], metadata !DIExpression()), !dbg [[EXT3_LOC:![0-9]*]]
; CHECK-DAG: [[EXT3_V]] =  {{.*}}, !dbg [[EXT3_LOC]]

; Function Attrs: convergent noinline nounwind optnone
define spir_func void @test(i32 addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset) #0 !dbg !20 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8, !dbg !43
  call void @llvm.dbg.value(metadata i32 addrspace(1)** %dst.addr, metadata !23, metadata !DIExpression()), !dbg !43
  %x = alloca i32, align 4, !dbg !44
  call void @llvm.dbg.value(metadata i32* %x, metadata !25, metadata !DIExpression()), !dbg !44
  %y = alloca i32, align 4, !dbg !45
  call void @llvm.dbg.value(metadata i32* %y, metadata !26, metadata !DIExpression()), !dbg !45
  %z = alloca i32, align 4, !dbg !46
  call void @llvm.dbg.value(metadata i32* %z, metadata !27, metadata !DIExpression()), !dbg !46
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8, !dbg !47
  %call.i = call i16 @llvm.genx.GenISA.getLocalID.X(), !dbg !48
  call void @llvm.dbg.value(metadata i16 %call.i, metadata !28, metadata !DIExpression()), !dbg !48
  %ext1 = zext i16 %call.i to i32, !dbg !49
  call void @llvm.dbg.value(metadata i32 %ext1, metadata !30, metadata !DIExpression()), !dbg !49
  store i32 %ext1, i32* %x, align 4, !dbg !50
  %call1.i = call i16 @llvm.genx.GenISA.getLocalID.Y(), !dbg !51
  call void @llvm.dbg.value(metadata i16 %call1.i, metadata !32, metadata !DIExpression()), !dbg !51
  %ext2 = zext i16 %call1.i to i32, !dbg !52
  call void @llvm.dbg.value(metadata i32 %ext2, metadata !33, metadata !DIExpression()), !dbg !52
  store i32 %ext2, i32* %y, align 4, !dbg !53
  %call2.i = call i16 @llvm.genx.GenISA.getLocalID.Z(), !dbg !54
  call void @llvm.dbg.value(metadata i16 %call2.i, metadata !34, metadata !DIExpression()), !dbg !54
  %ext3 = zext i16 %call2.i to i32, !dbg !55
  call void @llvm.dbg.value(metadata i32 %ext3, metadata !35, metadata !DIExpression()), !dbg !55
  store i32 %ext3, i32* %z, align 4, !dbg !56
  %0 = load i32, i32* %x, align 4, !dbg !57
  call void @llvm.dbg.value(metadata i32 %0, metadata !36, metadata !DIExpression()), !dbg !57
  %1 = load i32, i32* %y, align 4, !dbg !58
  call void @llvm.dbg.value(metadata i32 %1, metadata !37, metadata !DIExpression()), !dbg !58
  %add = add nsw i32 %0, %1, !dbg !59
  call void @llvm.dbg.value(metadata i32 %add, metadata !38, metadata !DIExpression()), !dbg !59
  %2 = load i32, i32* %z, align 4, !dbg !60
  call void @llvm.dbg.value(metadata i32 %2, metadata !39, metadata !DIExpression()), !dbg !60
  %add5 = add nsw i32 %add, %2, !dbg !61
  call void @llvm.dbg.value(metadata i32 %add5, metadata !40, metadata !DIExpression()), !dbg !61
  %3 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !62
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %3, metadata !41, metadata !DIExpression()), !dbg !62
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %3, i64 0, !dbg !63
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %arrayidx, metadata !42, metadata !DIExpression()), !dbg !63
  store i32 %add5, i32 addrspace(1)* %arrayidx, align 4, !dbg !64
  ret void, !dbg !65
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "local-id.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[LIDX_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[LIDX_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[EXT1_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[EXT1_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LIDY_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[LIDY_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[EXT2_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[EXT2_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LIDZ_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 12
; CHECK-DAG: [[LIDZ_LOC]] = !DILocation(line: 12, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[EXT3_MD]] = !DILocalVariable(name: "10", scope: [[SCOPE]], file: [[FILE]], line: 13
; CHECK-DAG: [[EXT3_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE]])


declare i16 @llvm.genx.GenISA.getLocalID.X()

declare i16 @llvm.genx.GenISA.getLocalID.Y()

declare i16 @llvm.genx.GenISA.getLocalID.Z()

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { convergent noinline nounwind optnone "visaStackCall" }
attributes #1 = { nounwind readnone speculatable }

!llvm.module.flags = !{!0, !1, !2}
!igc.functions = !{!3}
!llvm.dbg.cu = !{!15}
!llvm.debugify = !{!18, !19}

!0 = !{i32 2, !"Dwarf Version", i32 4}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = !{i32 1, !"wchar_size", i32 4}
!3 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i8*, i32)* @test, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 2}
!6 = !{!"implicit_arg_desc", !7, !8, !9, !10, !11, !12, !13}
!7 = !{i32 0}
!8 = !{i32 1}
!9 = !{i32 7}
!10 = !{i32 8}
!11 = !{i32 9}
!12 = !{i32 12}
!13 = !{i32 14, !14}
!14 = !{!"explicit_arg_num", i32 0}
!15 = distinct !DICompileUnit(language: DW_LANG_C, file: !16, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !17)
!16 = !DIFile(filename: "local-id.ll", directory: "/")
!17 = !{}
!18 = !{i32 23}
!19 = !{i32 17}
!20 = distinct !DISubprogram(name: "test", linkageName: "test", scope: null, file: !16, line: 1, type: !21, scopeLine: 1, unit: !15, retainedNodes: !22)
!21 = !DISubroutineType(types: !17)
!22 = !{!23, !25, !26, !27, !28, !30, !32, !33, !34, !35, !36, !37, !38, !39, !40, !41, !42}
!23 = !DILocalVariable(name: "1", scope: !20, file: !16, line: 1, type: !24)
!24 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!25 = !DILocalVariable(name: "2", scope: !20, file: !16, line: 2, type: !24)
!26 = !DILocalVariable(name: "3", scope: !20, file: !16, line: 3, type: !24)
!27 = !DILocalVariable(name: "4", scope: !20, file: !16, line: 4, type: !24)
!28 = !DILocalVariable(name: "5", scope: !20, file: !16, line: 6, type: !29)
!29 = !DIBasicType(name: "ty16", size: 16, encoding: DW_ATE_unsigned)
!30 = !DILocalVariable(name: "6", scope: !20, file: !16, line: 7, type: !31)
!31 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!32 = !DILocalVariable(name: "7", scope: !20, file: !16, line: 9, type: !29)
!33 = !DILocalVariable(name: "8", scope: !20, file: !16, line: 10, type: !31)
!34 = !DILocalVariable(name: "9", scope: !20, file: !16, line: 12, type: !29)
!35 = !DILocalVariable(name: "10", scope: !20, file: !16, line: 13, type: !31)
!36 = !DILocalVariable(name: "11", scope: !20, file: !16, line: 15, type: !31)
!37 = !DILocalVariable(name: "12", scope: !20, file: !16, line: 16, type: !31)
!38 = !DILocalVariable(name: "13", scope: !20, file: !16, line: 17, type: !31)
!39 = !DILocalVariable(name: "14", scope: !20, file: !16, line: 18, type: !31)
!40 = !DILocalVariable(name: "15", scope: !20, file: !16, line: 19, type: !31)
!41 = !DILocalVariable(name: "16", scope: !20, file: !16, line: 20, type: !24)
!42 = !DILocalVariable(name: "17", scope: !20, file: !16, line: 21, type: !24)
!43 = !DILocation(line: 1, column: 1, scope: !20)
!44 = !DILocation(line: 2, column: 1, scope: !20)
!45 = !DILocation(line: 3, column: 1, scope: !20)
!46 = !DILocation(line: 4, column: 1, scope: !20)
!47 = !DILocation(line: 5, column: 1, scope: !20)
!48 = !DILocation(line: 6, column: 1, scope: !20)
!49 = !DILocation(line: 7, column: 1, scope: !20)
!50 = !DILocation(line: 8, column: 1, scope: !20)
!51 = !DILocation(line: 9, column: 1, scope: !20)
!52 = !DILocation(line: 10, column: 1, scope: !20)
!53 = !DILocation(line: 11, column: 1, scope: !20)
!54 = !DILocation(line: 12, column: 1, scope: !20)
!55 = !DILocation(line: 13, column: 1, scope: !20)
!56 = !DILocation(line: 14, column: 1, scope: !20)
!57 = !DILocation(line: 15, column: 1, scope: !20)
!58 = !DILocation(line: 16, column: 1, scope: !20)
!59 = !DILocation(line: 17, column: 1, scope: !20)
!60 = !DILocation(line: 18, column: 1, scope: !20)
!61 = !DILocation(line: 19, column: 1, scope: !20)
!62 = !DILocation(line: 20, column: 1, scope: !20)
!63 = !DILocation(line: 21, column: 1, scope: !20)
!64 = !DILocation(line: 22, column: 1, scope: !20)
!65 = !DILocation(line: 23, column: 1, scope: !20)
