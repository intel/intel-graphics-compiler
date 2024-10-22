;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt --regkey CodeSinkingMinSize=10 --CheckInstrTypes -igc-update-instrtypes-on-run -igc-code-sinking -S %s | FileCheck %s

define spir_kernel void @foo() {
  br label %label_1

label_1:
  %1 = zext i16 0 to i32
  call void @llvm.dbg.value(metadata i32 %1, metadata !12, metadata !DIExpression(DW_OP_constu, 4, DW_OP_swap, DW_OP_xderef)), !dbg !13
  %2 = call i32 @llvm.genx.GenISA.WavePrefix.i32(i32 %1, i8 0, i1 false, i1 false, i32 0)
  br label %label_2

.exit:
  br label %._crit_edge

label_3:
  br label %._crit_edge

; CHECK:       ._crit_edge:
; CHECK-NEXT:         = phi
; CHECK-NEXT:         = phi
; CHECK-NEXT:         call void @llvm.dbg.value(
; CHECK-NEXT:         ret void
; CHECK-NOT:         = phi
._crit_edge:
  %.sroa.2 = phi i32 [ 0, %label_3 ], [ 0, %.exit ]
  %.sroa.0 = phi i32 [ 0, %label_3 ], [ 0, %.exit ]
  ret void

label_2:
  call void @llvm.dbg.value(metadata i32 %.sroa.2, metadata !18, metadata !DIExpression(DW_OP_constu, 4, DW_OP_swap, DW_OP_xderef)), !dbg !14
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
