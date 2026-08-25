;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt --regkey LoopSinkMinSave=0 --regkey ForceLoopSink=1 --regkey LoopSinkForceRollback=1 --regkey CodeLoopSinkingMinSize=10 --CheckInstrTypes -igc-update-instrtypes-on-run --basic-aa --igc-code-loop-sinking --verify -S %s | FileCheck %s --check-prefixes=CHECK,%if llvm-22-plus %{CHECK-RECORDS%} %else %{CHECK-INTRINSIC%}

; A latch debug value must survive the loop-sink rollback. Six loops: the exposing block
; order comes from a pointer-keyed DenseMap, so one loop hits it only half the time.

; CHECK-LABEL: @foo(
; CHECK: %cmp0 = icmp ult i32 %index0, %count
; CHECK-INTRINSIC-NEXT: call void @llvm.dbg.value(metadata i32 %index0
; CHECK-RECORDS-NEXT: #dbg_value(i32 %index0
; CHECK-NEXT: br i1 %cmp0
; CHECK: %cmp5 = icmp ult i32 %index5, %count
; CHECK-INTRINSIC-NEXT: call void @llvm.dbg.value(metadata i32 %index5
; CHECK-RECORDS-NEXT: #dbg_value(i32 %index5
; CHECK-NEXT: br i1 %cmp5

define spir_kernel void @foo(i32 %t, i32 %count) {
  br label %ph0

ph0:
  %a0 = add i32 %t, 1
  %b0 = add i32 %a0, 2
  br label %loop0

loop0:                                            ; preds = %loop0, %ph0
  %index0 = phi i32 [ 0, %ph0 ], [ %inc0, %loop0 ]
  %c0 = add i32 %a0, 3
  %d0 = add i32 %b0, %c0
  %inc0 = add i32 %index0, 1
  %cmp0 = icmp ult i32 %index0, %count
  call void @llvm.dbg.value(metadata i32 %index0, metadata !12, metadata !DIExpression()), !dbg !13
  br i1 %cmp0, label %loop0, label %ph1

ph1:
  %a1 = add i32 %t, 2
  %b1 = add i32 %a1, 2
  br label %loop1

loop1:                                            ; preds = %loop1, %ph1
  %index1 = phi i32 [ 0, %ph1 ], [ %inc1, %loop1 ]
  %c1 = add i32 %a1, 3
  %d1 = add i32 %b1, %c1
  %inc1 = add i32 %index1, 1
  %cmp1 = icmp ult i32 %index1, %count
  call void @llvm.dbg.value(metadata i32 %index1, metadata !12, metadata !DIExpression()), !dbg !13
  br i1 %cmp1, label %loop1, label %ph2

ph2:
  %a2 = add i32 %t, 3
  %b2 = add i32 %a2, 2
  br label %loop2

loop2:                                            ; preds = %loop2, %ph2
  %index2 = phi i32 [ 0, %ph2 ], [ %inc2, %loop2 ]
  %c2 = add i32 %a2, 3
  %d2 = add i32 %b2, %c2
  %inc2 = add i32 %index2, 1
  %cmp2 = icmp ult i32 %index2, %count
  call void @llvm.dbg.value(metadata i32 %index2, metadata !12, metadata !DIExpression()), !dbg !13
  br i1 %cmp2, label %loop2, label %ph3

ph3:
  %a3 = add i32 %t, 4
  %b3 = add i32 %a3, 2
  br label %loop3

loop3:                                            ; preds = %loop3, %ph3
  %index3 = phi i32 [ 0, %ph3 ], [ %inc3, %loop3 ]
  %c3 = add i32 %a3, 3
  %d3 = add i32 %b3, %c3
  %inc3 = add i32 %index3, 1
  %cmp3 = icmp ult i32 %index3, %count
  call void @llvm.dbg.value(metadata i32 %index3, metadata !12, metadata !DIExpression()), !dbg !13
  br i1 %cmp3, label %loop3, label %ph4

ph4:
  %a4 = add i32 %t, 5
  %b4 = add i32 %a4, 2
  br label %loop4

loop4:                                            ; preds = %loop4, %ph4
  %index4 = phi i32 [ 0, %ph4 ], [ %inc4, %loop4 ]
  %c4 = add i32 %a4, 3
  %d4 = add i32 %b4, %c4
  %inc4 = add i32 %index4, 1
  %cmp4 = icmp ult i32 %index4, %count
  call void @llvm.dbg.value(metadata i32 %index4, metadata !12, metadata !DIExpression()), !dbg !13
  br i1 %cmp4, label %loop4, label %ph5

ph5:
  %a5 = add i32 %t, 6
  %b5 = add i32 %a5, 2
  br label %loop5

loop5:                                            ; preds = %loop5, %ph5
  %index5 = phi i32 [ 0, %ph5 ], [ %inc5, %loop5 ]
  %c5 = add i32 %a5, 3
  %d5 = add i32 %b5, %c5
  %inc5 = add i32 %index5, 1
  %cmp5 = icmp ult i32 %index5, %count
  call void @llvm.dbg.value(metadata i32 %index5, metadata !12, metadata !DIExpression()), !dbg !13
  br i1 %cmp5, label %loop5, label %exit

exit:
  ret void
}


; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }

!llvm.module.flags = !{!3, !4, !5}
!llvm.dbg.cu = !{!0}

!0 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus_14, file: !1, producer: "clang", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "hi.cpp", directory: "/test/")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!7 = distinct !DISubprogram(name: "test", scope: !1, file: !1, line: 3, type: !8, isLocal: false, isDefinition: true, scopeLine: 3, flags: DIFlagPrototyped, isOptimized: true, unit: !0, retainedNodes: !11)
!8 = !DISubroutineType(types: !9)
!9 = !{!10, !10}
!10 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!11 = !{!12}
!12 = !DILocalVariable(name: "i", scope: !7, file: !1, line: 4, type: !10)
!13 = !DILocation(line: 4, column: 12, scope: !7)
