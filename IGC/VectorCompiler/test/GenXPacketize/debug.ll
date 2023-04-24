;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Tests that GenXPacketize pass completes
;
; RUN: opt %use_old_pass_manager% -GenXPacketize -march=genx64 -mtriple=spir64-unkonwn-unknown -mcpu=Gen9 -S < %s | FileCheck %s
;
; ------------------------------------------------
; GenXPacketize
; ------------------------------------------------
;
; CHECK: void @test_packetize{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL1_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[VAL1_LOC:![0-9]*]]
; CHECK: call void @llvm.dbg.declare({{.*}} [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: call void @llvm.dbg.addr({{.*}} [[VAL1_V]], metadata [[VAL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL1_LOC]]
; CHECK: [[VAL2_V:%[A-z0-9.]*]] = call <8 x float> @llvm.sqrt.v8f32(<8 x float> [[VAL1_V]])


define void @test_packetize(<8 x float>* %a) #0 !dbg !6 {
entry:
  %0 = load <8 x float>, <8 x float>* %a, !dbg !10
  call void @llvm.dbg.declare(metadata <8 x float> %0, metadata !9,
                             metadata !DIExpression()), !dbg !10
  call void @llvm.dbg.addr(metadata <8 x float> %0, metadata !9,
                             metadata !DIExpression()), !dbg !10

  %1 = call <8 x float> @sqrtf(<8 x float> %0)
  call void @llvm.dbg.value(metadata <8 x float> %1, metadata !9,
                             metadata !DIExpression()), !dbg !10

  call void @llvm.dbg.label(metadata !13), !dbg !10
  store <8 x float> %1, <8 x float>* %a
  ret void
}

define void @test_packetize_no_debug(<8 x float>* %a) #0 {
entry:
  %0 = load <8 x float>, <8 x float>* %a
  %1 = call <8 x float> @sqrtf(<8 x float> %0)
  store <8 x float> %1, <8 x float>* %a
  ret void
}

declare <8 x float> @sqrtf(<8 x float>)
declare void @llvm.dbg.declare(metadata, metadata, metadata)
declare void @llvm.dbg.addr(metadata, metadata, metadata)
declare void @llvm.dbg.value(metadata, metadata, metadata)
declare void @llvm.dbg.label(metadata)

attributes #0 = { "CMGenxSIMT"="8" }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.debugify = !{!3, !4}
!llvm.module.flags = !{!5}


!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "GenXPacketize.ll", directory: "/")
!2 = !{}
!3 = !{i32 14}
!4 = !{i32 9}
!5 = !{i32 2, !"Debug Info Version", i32 3}

!6 = distinct !DISubprogram(name: "test_packetize", linkageName: "test_packetize", scope: null, file: !1, line: 1, type: !7, scopeLine: 1, unit: !0, retainedNodes: !8)
!7 = !DISubroutineType(types: !2)
!8 = !{!9}
!9 = !DILocalVariable(name: "1", scope: !6, file: !1, line: 1, type: !11)
!10 = !DILocation(line: 1, column: 1, scope: !6)
; - !11 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
; !11 = !DICompositeType(tag: DW_TAG_array_type, baseType: !12, size: 512, flags: DIFlagVector, elements: !8)
!11 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !12, size: 64)
!12 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)

!13 = !DILabel(scope: !6, name: "top", file: !1, line: 4)

; !12 = !{!13}
; !13 = !DISubrange(count: 16)
; !82 = !DICompositeType(tag: DW_TAG_array_type, baseType: !83, size: 512, flags: DIFlagVector, elements: !12)
; !83 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
