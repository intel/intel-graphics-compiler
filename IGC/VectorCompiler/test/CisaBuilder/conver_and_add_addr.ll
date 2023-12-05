;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: ;;;;;;;;;; RUNNERS ;;;;;;;;;;

; RUN: %opt %use_old_pass_manager% -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPC \
; RUN: -GenXModule -GenXLivenessWrapper -GenXCategoryWrapper -GenXLiveRangesWrapper -GenXCisaBuilderPass -GenXFinalizer \
; RUN: -disable-verify -finalizer-opts="-dumpcommonisa -isaasmToConsole" < %s | FileCheck %s

; COM: ;;;;;;;;;; CHECKERS ;;;;;;;;;;

; Gen9 VISA check
; CHECK: addr_add (M1, 1) [[A0:A[0-9]]](0)<1> &V{{[0-9]+}}[616] [[VBASE:V[0-9]+]](
; CHECK-NEXT: addr_add (M1, 1) [[A1:A[0-9]]](0)<1> &V{{[0-9]+}}[617] [[VBASE]](
; CHECK-NEXT: addr_add (M1, 1) [[A2:A[0-9]]](0)<1> [[A0]](0)<1> 0x264:uw

; COM: ;;;;;;;;;; KERNEL ;;;;;;;;;;

define dllexport void @test_kernel(<16 x i32>* %a, i16 %b, <16 x i32> %c) #0 {
entry:
  %ptrtoint = ptrtoint <16 x i32>* %a to i64
  %ugm = call <8 x i64> @llvm.vc.internal.lsc.load.ugm.v8i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 5, <2 x i8> zeroinitializer, i64 0, i64 %ptrtoint, i16 1, i32 0, <8 x i64> undef)
  %bitcast = bitcast <8 x i64> %ugm to <16 x i32>
  %addr = call i16 @llvm.genx.convert.addr.i16(i16 %b, i16 616)
  %addr1 = call i16 @llvm.genx.convert.addr.i16(i16 %b, i16 617)
  %addr2 = call i16 @llvm.genx.add.addr.i16.i16(i16 %addr, i16 612)
  %0 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %bitcast, i32 1, i32 1, i32 0, i16 %addr2, i32 0)
  %1 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32> %bitcast, i32 1, i32 1, i32 0, i16 %addr1, i32 0)
  %add = add <8 x i32> %0, %1
  %bitcast3 = bitcast <8 x i32> %add to <4 x i64>
  call void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v4i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 4, <2 x i8> zeroinitializer, i64 0, i64 %ptrtoint, i16 1, i32 0, <4 x i64> %bitcast3)
  ret void
}

declare <8 x i32> @llvm.genx.rdregioni.v8i32.v16i32.i16(<16 x i32>, i32, i32, i32, i16, i32) #1
declare i16 @llvm.genx.add.addr.i16.i16(i16, i16) #1
declare i16 @llvm.genx.convert.addr.i16(i16, i16) #1
declare <8 x i64> @llvm.vc.internal.lsc.load.ugm.v8i64.v1i1.v2i8.i64(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <8 x i64>) #2
declare void @llvm.vc.internal.lsc.store.ugm.v1i1.v2i8.i64.v4i64(<1 x i1>, i8, i8, i8, <2 x i8>, i64, i64, i16, i32, <4 x i64>) #3
declare i16 @llvm.genx.convert.i16(i16) #4

attributes #0 = { "CMGenxMain" "VC.Stack.Amount"="0" "target-cpu"="XeHPC" }
attributes #1 = { "target-cpu"="XeHPC" }
attributes #2 = { nounwind readonly }
attributes #3 = { nounwind writeonly }
attributes #4 = { nounwind readnone }

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}

!0 = !{void (<16 x i32>*, i16, <16 x i32>)* @test_kernel, !"test_kernel", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 0, i32 1, i32 2}
!2 = !{i32 32, i32 96, i32 128}
!3 = !{i32 0, i32 0, i32 0}
!4 = !{}
!5 = !{void (<16 x i32>*, i16, <16 x i32>)* @test_kernel, null, null, null, null}
