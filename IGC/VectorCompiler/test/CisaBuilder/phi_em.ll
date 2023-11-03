;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: ;;;;;;;;;; RUNNERS ;;;;;;;;;;

; RUN: %opt %use_old_pass_manager% -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPC \
; RUN: -GenXModule -GenXNumberingWrapper -GenXLiveRangesWrapper -GenXCoalescingWrapper -GenXCisaBuilderPass -GenXFinalizer \
; RUN: -disable-verify -finalizer-opts="-dumpcommonisa -isaasmToConsole" < %s | FileCheck %s

; COM: ;;;;;;;;;; CHECKERS ;;;;;;;;;;

; Gen9 VISA check
; CHECK-DAG: .decl [[V1:V[0-9]+]] v_type=G type=d num_elts=1 align=dword
; CHECK-DAG: .decl [[V2:V[0-9]+]] v_type=G type=ud num_elts=1 alias=<[[V1]], 0>
; CHECK-DAG: .implicit_LOCAL_ID [[V1]] offset=24 size=4

; CHECK: setp (M1_NM, 32) [[P1:P[0-9]]] [[V2]]
; CHECK: cmp.eq (M1_NM, 32) [[P2:P[0-9]]] 0x1:ub 0x1:ub
; CHECK: and (M1_NM, 32) P{{[0-9]}} [[P2]] [[P1]]

; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;

define dllexport spir_kernel void @test(i32 %arg) #0 {
entry:
  %val = bitcast i32 %arg to <32 x i1>
  %constant = call <32 x i1> @llvm.genx.constantpred.v32i1(<32 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true>)
  %constant1 = call <32 x i1> @llvm.genx.constantpred.v32i1(<32 x i1> zeroinitializer)
  br label %loop

loop:
  %EM = phi <32 x i1> [ %constant, %entry ], [ %newEM, %loop ], !pred.index !4
  %RM = phi <32 x i1> [ %constant1, %entry ], [ %newRM, %loop ], !pred.index !5
  %getEM = call <32 x i1> @llvm.genx.simdcf.get.em.v32i1(<32 x i1> %EM)
  %goto_cond = and <32 x i1> %getEM, %val, !pred.index !6
  %goto_struct = tail call { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1> %EM, <32 x i1> %RM, <32 x i1> %goto_cond)
  %newRM = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct, 1
  %newEM = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct, 0
  %branchCond = extractvalue { <32 x i1>, <32 x i1>, i1 } %goto_struct, 2
  br i1 %branchCond, label %exit, label %loop

exit:
  %join_struct = tail call { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1> %newEM, <32 x i1> %newRM)
  ret void
}

declare !genx_intrinsic_id !7 { <32 x i1>, <32 x i1>, i1 } @llvm.genx.simdcf.goto.v32i1.v32i1(<32 x i1>, <32 x i1>, <32 x i1>) #1
declare !genx_intrinsic_id !8 { <32 x i1>, i1 } @llvm.genx.simdcf.join.v32i1.v32i1(<32 x i1>, <32 x i1>) #2
declare !genx_intrinsic_id !9 <32 x i1> @llvm.genx.simdcf.get.em.v32i1(<32 x i1>) #3
declare !genx_intrinsic_id !10 <32 x i1> @llvm.genx.constantpred.v32i1(<32 x i1>) #1

attributes #0 = { "CMGenxMain" "VC.Stack.Amount"="0" "target-cpu"="XeHPC" }
attributes #1 = { nounwind readnone }
attributes #2 = { nounwind }
attributes #3 = { nounwind writeonly }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!3}

!0 = !{void (i32)* @test, !"test", !1, i32 0, !1, !2, !2, i32 0}
!1 = !{i32 24}
!2 = !{}
!3 = !{void (i32)* @test, null, null, null, null}
!4 = !{i64 1}
!5 = !{i64 2}
!6 = !{i64 3}
!7 = !{i32 11000}
!8 = !{i32 11001}
!9 = !{i32 10999}
!10 = !{i32 10786}