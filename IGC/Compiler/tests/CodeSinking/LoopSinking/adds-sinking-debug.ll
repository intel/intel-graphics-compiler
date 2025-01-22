;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt --regkey LoopSinkMinSave=0 --regkey ForceLoopSink=1 --regkey CodeLoopSinkingMinSize=10 --CheckInstrTypes -igc-update-instrtypes-on-run --basic-aa --igc-code-loop-sinking --verify -S %s | FileCheck %s --check-prefix=CHECK-SINK
; RUN: igc_opt --regkey LoopSinkMinSave=0 --regkey ForceLoopSink=1 --regkey LoopSinkForceRollback=1 --regkey CodeLoopSinkingMinSize=10 --CheckInstrTypes -igc-update-instrtypes-on-run --basic-aa --igc-code-loop-sinking --verify -S %s | FileCheck %s --check-prefix=CHECK-ROLLBACK

; CHECK-LABEL: @foo(

; For sink we check there is only one debug value in the loop

; CHECK-SINK: ph:
; CHECK-SINK-NOT: call void @llvm.dbg.value
; CHECK-SINK: loop:
; CHECK-SINK:     call void @llvm.dbg.value
; CHECK-SINK-NOT: call void @llvm.dbg.value


; For rollback check the debug value is not duplicated
; Also --verify pass checks that the IR is correct

; CHECK-ROLLBACK:     call void @llvm.dbg.value
; CHECK-ROLLBACK-NOT: call void @llvm.dbg.value


define spir_kernel void @foo(i32 %t, i32 %count) {
  br label %ph

ph:
  %1 = add i32 %t, 1
  %2 = add i32 %1, 2
  br label %loop

loop:                                             ; preds = %loop, %ph
  %index = phi i32 [ 0, %ph ], [ %inc, %loop ]
  call void @llvm.dbg.value(metadata i32 %1, metadata !12, metadata !DIExpression(DW_OP_constu, 4, DW_OP_swap, DW_OP_xderef)), !dbg !13
  %3 = add i32 %1, 3
  %4 = add i32 %2, %3
  %inc = add i32 %index, 1
  %cmptmp = icmp ult i32 %index, %count
  br i1 %cmptmp, label %loop, label %afterloop

afterloop:                                        ; preds = %loop
  ret void
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

declare i32 @llvm.genx.GenISA.WavePrefix.i32(i32, i8, i1, i1, i32)

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }

!llvm.module.flags = !{!3, !4, !5}
!llvm.dbg.cu = !{!0}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "hi.cpp", directory: "/test/")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{!"clang"}
!7 = distinct !DISubprogram(name: "test", scope: !1, file: !1, line: 3, type: !8, isLocal: false, isDefinition: true, scopeLine: 3, flags: DIFlagPrototyped, isOptimized: true, unit: !0, retainedNodes: !11)
!8 = !DISubroutineType(types: !9)
!9 = !{!10, !10}
!10 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!11 = !{!12}
!12 = !DILocalVariable(name: "t", arg: 1, scope: !7, file: !1, line: 3, type: !10)
!13 = !DILocation(line: 3, column: 14, scope: !7)
!14 = !DILocation(line: 4, column: 12, scope: !7)
!15 = distinct !DISubprogram(name: "_start", scope: !1, file: !1, line: 7, type: !16, isLocal: false, isDefinition: true, scopeLine: 7, isOptimized: true, unit: !0, retainedNodes: !2)
!16 = !DISubroutineType(types: !17)
!17 = !{!10}
!18 = !DILocalVariable(name: "a", arg: 1, scope: !7, file: !1, line: 3, type: !10)
