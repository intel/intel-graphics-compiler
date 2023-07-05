;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXPatternMatch -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=XeHPC -S < %s | FileCheck %s

; target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
; target triple = "genx64-unknown-unknown"

; Function Attrs: nounwind readonly
declare <16 x i32> @llvm.genx.lsc.load.bti.v16i32.v16i1.v16i32(<16 x i1> %0, i8 %1, i8 %2, i8 %3, i16 %4, i32 %5, i8 %6, i8 %7, i8 %8, i8 %9, <16 x i32> %10, i32 %11) #0

; Function Attrs: nounwind
declare void @llvm.genx.lsc.store.bti.i1.i32.v16i32(i1 %0, i8 %1, i8 %2, i8 %3, i16 %4, i32 %5, i8 %6, i8 %7, i8 %8, i8 %9, i32 %10, <16 x i32> %11, i32 %12) #1

; CHECK: state_grf
; Function Attrs: noinline nounwind
define dllexport spir_kernel void @state_grf(i32 %d_result_table, i64 %impl.arg.private.base) local_unnamed_addr #2 {
entry:
; COM: Not apply for this lsc.load:
;  CHECK: [[LSC_LOAD:%[A-z0-9.]*]] = {{.*}} @llvm.genx.lsc.load.bti.v16i32.v16i1.v16i32
  %0 = tail call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.v16i1.v16i32(<16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>, i8 0, i8 2, i8 2, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> <i32 15, i32 14, i32 13, i32 12, i32 11, i32 10, i32 9, i32 8, i32 7, i32 6, i32 5, i32 4, i32 3, i32 2, i32 1, i32 3>, i32 1)
  %1 = and <16 x i32> %0, <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  %2 = icmp ne <16 x i32> %1, zeroinitializer
  %constant = call <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32> <i32 0, i32 4, i32 8, i32 12, i32 16, i32 20, i32 24, i32 28>)
  %3 = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> %constant, i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
  %4 = add <8 x i32> %constant, <i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32>
  %5 = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> %3, <8 x i32> %4, i32 8, i32 8, i32 1, i16 32, i32 undef, i1 true)
; COM: Apply for lsc.load -> select pattern:
; CHECK: [[LSC_MERGE:%[A-z0-9.]*]] = {{.*}} @llvm.genx.lsc.load.merge.bti.v16i32.v16i1.v16i32
  %6 = tail call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.v16i1.v16i32(<16 x i1> %2, i8 0, i8 2, i8 2, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %5, i32 1)
; COM: select-instruction must be removed:
; CHECK-NOT: {{.*}} select [[LSC_MERGE]]
; COM: and store must use replaced merge-instruction:
; CHECK-DAG: tail call void @llvm.genx.lsc.store.bti{{.*}}[[LSC_MERGE]]
  %lowerpred = select <16 x i1> %2, <16 x i32> %6, <16 x i32> <i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  tail call void @llvm.genx.lsc.store.bti.i1.i32.v16i32(i1 true, i8 4, i8 3, i8 3, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 0, <16 x i32> %lowerpred, i32 1)
  ret void
}


; CHECK: state_grf2
; Function Attrs: noinline nounwind
define dllexport spir_kernel void @state_grf2(i32 %0, i64 %impl.arg.private.base) local_unnamed_addr #2 {
  %constant = call <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32> <i32 0, i32 4, i32 8, i32 12, i32 16, i32 20, i32 24, i32 28>)
  %2 = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> undef, <8 x i32> %constant, i32 8, i32 8, i32 1, i16 0, i32 undef, i1 true)
  %3 = add <8 x i32> %constant, <i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32, i32 32>
  %4 = call <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> %2, <8 x i32> %3, i32 8, i32 8, i32 1, i16 32, i32 undef, i1 true)
  ; CHECK: [[LSC_MERGE:%[A-z0-9.]*]] = {{.*}} @llvm.genx.lsc.load.merge.bti.v16i32.v16i1.v16i32
  %5 = tail call <16 x i32> @llvm.genx.lsc.load.bti.v16i32.v16i1.v16i32(<16 x i1> <i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false>, i8 0, i8 2, i8 2, i16 1, i32 0, i8 3, i8 1, i8 1, i8 0, <16 x i32> %4, i32 1)
  %6 = select <16 x i1> <i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false>, <16 x i32> %5, <16 x i32> <i32 undef, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1>
  tail call void @llvm.genx.lsc.store.bti.i1.i32.v16i32(i1 true, i8 4, i8 3, i8 3, i16 1, i32 0, i8 3, i8 6, i8 2, i8 0, i32 0, <16 x i32> %6, i32 1)
  ret void
}


; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !22 <8 x i32> @llvm.genx.constanti.v8i32(<8 x i32> %0) #3

; Function Attrs: nounwind readnone
declare !genx_intrinsic_id !21 <16 x i32> @llvm.genx.wrregioni.v16i32.v8i32.i16.i1(<16 x i32> %0, <8 x i32> %1, i32 %2, i32 %3, i32 %4, i16 %5, i32 %6, i1 %7) #3

attributes #0 = { nounwind readonly }
attributes #1 = { nounwind }
attributes #2 = { noinline nounwind "CMGenxMain" "VC.Stack.Amount"="0" }
attributes #3 = { nounwind readnone }

!genx.kernels = !{!4}
!genx.kernel.internal = !{!9}

!0 = !{i32 0, i32 0}
!2 = !{}
!4 = !{void (i32, i64)* @state_grf, !"state_grf", !5, i32 0, !6, !7, !8, i32 0}
!5 = !{i32 2, i32 96}
!6 = !{i32 72, i32 64}
!7 = !{i32 0}
!8 = !{!"buffer_t read_write"}
!9 = !{void (i32, i64)* @state_grf, !0, !10, !2, !11}
!10 = !{i32 0, i32 1}
!11 = !{i32 1, i32 255}
!21 = !{i32 7959}
!22 = !{i32 7571}
