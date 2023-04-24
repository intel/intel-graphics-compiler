;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

;
; RUN: opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXArgIndirectionWrapper -march=genx64 -mtriple=spir64-unkonwn-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXArgIndirection
; ------------------------------------------------

source_filename = "GenXArgIndirection.ll"
target datalayout = "e-p:64:64-i64:64-n8:16:32:64"

; Function Attrs: nounwind
declare <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32>, i32, i32, i32, i16, i32) #0

; Function Attrs: nounwind
declare <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32>, <16 x i32>, i32, i32, i32, i16, i32, i1) #0

; Function Attrs: nounwind readonly
declare <16 x i32> @llvm.genx.gather.masked.scaled2.v16i32.v16i32.v16i1(i32, i16, i32, i32, <16 x i32>, <16 x i1>) #1

; CHECK-LABEL: @IP_AlphaBlendRound
; CHECK-SAME: (<64 x i32> %[[ARG:[^ )]+]]) #2 !dbg
; Function Attrs: noinline norecurse nounwind readnone
define internal spir_func <64 x i32> @IP_AlphaBlendRound(<64 x i32> %arg) #2 !dbg !5 {
    %.split48437.join48 = call <64 x i32> @llvm.genx.wrregioni.v64i32.v16i32.i16.i1(<64 x i32> %arg, <16 x i32> <i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255, i32 255>, i32 0, i32 16, i32 1, i16 192, i32 undef, i1 true)
    ret <64 x i32> %.split48437.join48
}

; CHECK-LABEL: @Compute_2D_STD_UV
; CHECK-SAME: (<64 x i32> %[[ARG:[^ )]+]]) local_unnamed_addr #3 !dbg
; Function Attrs: noinline nounwind
define dllexport spir_kernel void @Compute_2D_STD_UV(<64 x i32> %arg) local_unnamed_addr #3 !dbg !38 {
    %call1154 = call spir_func <64 x i32> @IP_AlphaBlendRound(<64 x i32> %arg), !dbg !98
    %retval.postcopy = call <16 x i32> @llvm.genx.rdregioni.v16i32.v64i32.i16(<64 x i32> %call1154, i32 0, i32 16, i32 1, i16 0, i32 undef)
    ret void
}

attributes #0 = { nounwind }
attributes #1 = { nounwind readonly }
attributes #2 = { noinline norecurse nounwind readnone }
attributes #3 = { noinline nounwind "CMGenxMain" "VC.Stack.Amount"="0" "oclrt"="1" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "basic.ll", directory: "/")
!2 = !{}
!3 = !{i32 1, !"wchar_size", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = distinct !DISubprogram(name: "IP_AlphaBlendRound", linkageName: "IP_AlphaBlendRound", scope: null, file: !1, line: 1, type: !6, scopeLine: 1, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition | DISPFlagOptimized, unit: !0)
!6 = !DISubroutineType(types: !2)
!38 = distinct !DISubprogram(name: "Compute_2D_STD_UV", linkageName: "Compute_2D_STD_UV", scope: null, file: !1, line: 30, type: !6, scopeLine: 30, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0)
!98 = !DILocation(line: 78, column: 1, scope: !38)
