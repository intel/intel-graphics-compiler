;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; GenXCoalescing eliminates all LiveRanges for undef values and
; undef elements of Struct values, for example in insertvalue instruction
;
; Here we have %loadstruct value with defined 0th element and undef 1st element
; This test ensures that GenXCoalescing does not crash after it will remove
; LiveRange for the 1st element

; RUN: opt %use_old_pass_manager% -GenXModule -GenXNumberingWrapper -GenXLiveRangesWrapper -GenXCoalescingWrapper \
; RUN:  -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -vc-disable-coalescing -S \
; RUN:  < %s

; ModuleID = 'test.ll'
source_filename = "test.ll"
target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

%str = type { i32, i32 }

define spir_func i32 @test() #0 !FuncArgSize !0 !FuncRetSize !1 {
  %constant = call i32 @llvm.genx.constanti.i32(i32 0)
  %loadstruct = insertvalue %str undef, i32 %constant, 0
  %ret = tail call spir_func i32 @proc(%str %loadstruct)
  %reg1 = call <96 x i32> @llvm.genx.read.predef.reg.v96i32.v96i32(i32 9, <96 x i32> undef)
  %wrregioni = call <96 x i32> @llvm.genx.wrregioni.v96i32.i32.i16.i1(<96 x i32> %reg1, i32 %ret, i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
  %reg = call i32 @llvm.genx.write.predef.reg.i32.v96i32(i32 9, <96 x i32> %wrregioni)
  ret i32 %ret
}

; Function Attrs: noinline
define internal spir_func i32 @proc(%str %arg) #1 {
  %v = extractvalue %str %arg, 1
  ret i32 %v
}

; Function Attrs: nounwind readonly
declare !genx_intrinsic_id !2 <96 x i32> @llvm.genx.read.predef.reg.v96i32.v96i32(i32, <96 x i32>) #2

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !3 <96 x i32> @llvm.genx.wrregioni.v96i32.i32.i16.i1(<96 x i32>, i32, i32, i32, i32, i16, i32, i1) #3

; Function Attrs: nounwind writeonly
declare !genx_intrinsic_id !4 i32 @llvm.genx.write.predef.reg.i32.v96i32(i32, <96 x i32>) #4

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !5 i32 @llvm.genx.constanti.i32(i32) #3

; Function Attrs: nounwind readnone
declare i32 @llvm.ssa.copy.i32(i32 returned) #3

; Function Attrs: nounwind writeonly
declare !genx_intrinsic_id !11 void @llvm.genx.unmask.end(i32) #4

; Function Attrs: nounwind writeonly
declare !genx_intrinsic_id !12 i32 @llvm.genx.unmask.begin() #4

attributes #0 = { "CMStackCall" "target-cpu"="Gen9" "target-features"="+ocl_runtime" }
attributes #1 = { noinline "target-cpu"="Gen9" "target-features"="+ocl_runtime" }
attributes #2 = { nounwind readonly }
attributes #3 = { nounwind readnone }
attributes #4 = { nounwind writeonly }
attributes #5 = { nounwind }

!0 = !{i32 0}
!1 = !{i32 1}
!2 = !{i32 7744}
!3 = !{i32 7948}
!4 = !{i32 7943}
!5 = !{i32 7570}
!6 = !{i32 7770}
!7 = !{i32 7771}
!8 = !{i32 7774}
!9 = !{i32 7775}
!10 = !{i32 7773}
!11 = !{i32 7870}
!12 = !{i32 7869}
