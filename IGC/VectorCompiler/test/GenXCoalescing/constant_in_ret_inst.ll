;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; GenXLiveRanges creates a dummy unified return value, not attached to any BB
; It assigns this unified ret category same as original ret val category
; Check that it does not fail in case if original ret val is constant
;

; RUN: opt %use_old_pass_manager% -GenXModule -GenXNumberingWrapper -GenXLiveRangesWrapper -GenXCoalescingWrapper \
; RUN:  -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -vc-disable-coalescing -S \
; RUN:  < %s

; ModuleID = 'test_2.ll'
source_filename = "test_2.ll"
target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

; Function Attrs: noinline norecurse nounwind
define spir_func i32 @_Z4testv() local_unnamed_addr #0 !FuncArgSize !0 !FuncRetSize !1 {
  %reg1 = call <96 x i32> @llvm.genx.read.predef.reg.v96i32.v96i32(i32 9, <96 x i32> undef)
  %wrregioni = call <96 x i32> @llvm.genx.wrregioni.v96i32.i32.i16.i1(<96 x i32> %reg1, i32 0, i32 0, i32 1, i32 1, i16 0, i32 undef, i1 true)
  %reg = call i32 @llvm.genx.write.predef.reg.i32.v96i32(i32 9, <96 x i32> %wrregioni)
  ret i32 0
}

; Function Attrs: nounwind readonly
declare !genx_intrinsic_id !2 <96 x i32> @llvm.genx.read.predef.reg.v96i32.v96i32(i32, <96 x i32>) #1

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !3 <96 x i32> @llvm.genx.wrregioni.v96i32.i32.i16.i1(<96 x i32>, i32, i32, i32, i32, i16, i32, i1) #2

; Function Attrs: nounwind writeonly
declare !genx_intrinsic_id !4 i32 @llvm.genx.write.predef.reg.i32.v96i32(i32, <96 x i32>) #3

; Function Attrs: nounwind readnone
declare i32 @llvm.ssa.copy.i32(i32 returned) #2

; Function Attrs: nounwind writeonly
declare !genx_intrinsic_id !10 void @llvm.genx.unmask.end(i32) #3

; Function Attrs: nounwind writeonly
declare !genx_intrinsic_id !11 i32 @llvm.genx.unmask.begin() #3

attributes #0 = { noinline norecurse nounwind "CMStackCall" "target-cpu"="Gen9" "target-features"="+ocl_runtime" }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind readnone }
attributes #3 = { nounwind writeonly }
attributes #4 = { nounwind }

!0 = !{i32 0}
!1 = !{i32 1}
!2 = !{i32 7744}
!3 = !{i32 7948}
!4 = !{i32 7943}
!5 = !{i32 7770}
!6 = !{i32 7771}
!7 = !{i32 7774}
!8 = !{i32 7775}
!9 = !{i32 7773}
!10 = !{i32 7870}
!11 = !{i32 7869}
