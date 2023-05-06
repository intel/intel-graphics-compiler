;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXGVClobberCheckerWrapper -check-gv-clobbering=true -check-gv-clobbering-collect-store-related-call-sites=true -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Gen9 -S < %s | FileCheck %s
; RUN: opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXGVClobberCheckerWrapper -check-gv-clobbering=true -check-gv-clobbering-collect-store-related-call-sites=false -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; This test checks global volatile clobbering checker/fixup introduced late in pipeline to catch over-optimizations of global volatile access.
; This is an auxiliary utility used to help in detecting and fixing erroneous over-optimizations cases.
; The checker/fixup is only available under the -check-gv-clobbering=true option.
;
target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

@_ZL8g_global = internal global <4 x i32> zeroinitializer, align 16 #0

define dllexport spir_kernel void @TestGVClobberingFixupStoreInCall(i8 addrspace(1)* nocapture readonly %_arg_buf_gpu, i8 addrspace(1)* nocapture %_arg_res_gpu) local_unnamed_addr #1 {
entry:
  %ptrtoint = ptrtoint i8 addrspace(1)* %_arg_buf_gpu to i64
  %.splatinsert5 = bitcast i64 %ptrtoint to <1 x i64>
  %call.i.i.i.i.i.esimd5 = load volatile <4 x i32>, <4 x i32>* @_ZL8g_global
  %gather = call <1 x i32> @llvm.genx.svm.gather.v1i32.v1i1.v1i64(<1 x i1> <i1 true>, i32 0, <1 x i64> %.splatinsert5, <1 x i32> undef)
  %call4.i.i.i.i.esimd = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.v1i1(<4 x i32> %call.i.i.i.i.i.esimd5, <1 x i32> %gather, i32 0, i32 1, i32 1, i16 0, i32 0, <1 x i1> <i1 true>)
  store volatile <4 x i32> %call4.i.i.i.i.esimd, <4 x i32>* @_ZL8g_global
  %call.i.i.i8.i.esimd6 = load volatile <4 x i32>, <4 x i32>* @_ZL8g_global
; CHECK:  %call.i.i.i8.i.esimd6 = load volatile <4 x i32>, <4 x i32>* @_ZL8g_global
; CHECK-NEXT:  %vecext.i.i1.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %call.i.i.i8.i.esimd6, i32 0, i32 1, i32 1, i16 0, i32 undef)
; COM: if -check-gv-clobbering-collect-store-related-call-sites=true is supplied
; COM: store interference is precisely detected here down the call chain.
; COM: if -check-gv-clobbering-collect-store-related-call-sites=false is supplied or omitted
; COM: store interference is speculated because of call to a user function.
  tail call spir_func void @UserFunctionRewriteGV1()
; CHECK-NEXT:  tail call spir_func void @UserFunctionRewriteGV1()
; CHECK-NOT:  %vecext.i.i1.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %call.i.i.i8.i.esimd6, i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT:  %cmp.i.i = icmp eq i32 %vecext.i.i1.regioncollapsed, 55
  %vecext.i.i1.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %call.i.i.i8.i.esimd6, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %cmp.i.i = icmp eq i32 %vecext.i.i1.regioncollapsed, 55
  %conv.i.i4 = select i1 %cmp.i.i, i32 1, i32 0
  %bitcast = bitcast i32 %conv.i.i4 to <1 x i32>
  %ptrtoint6 = ptrtoint i8 addrspace(1)* %_arg_res_gpu to i64
  %.splatinsert13 = bitcast i64 %ptrtoint6 to <1 x i64>
  call void @llvm.genx.svm.scatter.v1i1.v1i64.v1i32(<1 x i1> <i1 true>, i32 0, <1 x i64> %.splatinsert13, <1 x i32> %bitcast)
  ret void
}

define dllexport spir_kernel void @TestGVClobberingFixupLocalStore(i8 addrspace(1)* nocapture readonly %_arg_buf_gpu, i8 addrspace(1)* nocapture %_arg_res_gpu, i64 %impl.arg.private.base) local_unnamed_addr #1 {
entry:
  %ptrtoint = ptrtoint i8 addrspace(1)* %_arg_buf_gpu to i64
  %.splatinsert5 = bitcast i64 %ptrtoint to <1 x i64>
  %call.i.i.i.i.i.esimd5 = load volatile <4 x i32>, <4 x i32>* @_ZL8g_global
  %gather = call <1 x i32> @llvm.genx.svm.gather.v1i32.v1i1.v1i64(<1 x i1> <i1 true>, i32 0, <1 x i64> %.splatinsert5, <1 x i32> undef)
  %call4.i.i.i.i.esimd = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.v1i1(<4 x i32> %call.i.i.i.i.i.esimd5, <1 x i32> %gather, i32 0, i32 1, i32 1, i16 0, i32 0, <1 x i1> <i1 true>)
  store volatile <4 x i32> %call4.i.i.i.i.esimd, <4 x i32>* @_ZL8g_global
  %call.i.i.i8.i.esimd6 = load volatile <4 x i32>, <4 x i32>* @_ZL8g_global
; CHECK:  %call.i.i.i8.i.esimd6 = load volatile <4 x i32>, <4 x i32>* @_ZL8g_global
; CHECK-NEXT:  %vecext.i.i1.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %call.i.i.i8.i.esimd6, i32 0, i32 1, i32 1, i16 0, i32 undef)
; COM: store interference is directly detected in this function.
  %call4.i.i.i.i.i.i.esimd = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.v1i1(<4 x i32> %call.i.i.i8.i.esimd6, <1 x i32> <i32 42>, i32 0, i32 1, i32 1, i16 0, i32 0, <1 x i1> <i1 true>)
; CHECK-NEXT:  %call4.i.i.i.i.i.i.esimd = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.v1i1(<4 x i32> %call.i.i.i8.i.esimd6, <1 x i32> <i32 42>, i32 0, i32 1, i32 1, i16 0, i32 0, <1 x i1> <i1 true>)
  store volatile <4 x i32> %call4.i.i.i.i.i.i.esimd, <4 x i32>* @_ZL8g_global
; CHECK-NEXT: store volatile <4 x i32> %call4.i.i.i.i.i.i.esimd, <4 x i32>* @_ZL8g_global
  %vecext.i.i1.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %call.i.i.i8.i.esimd6, i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NOT:  %vecext.i.i1.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %call.i.i.i8.i.esimd6, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %cmp.i.i = icmp eq i32 %vecext.i.i1.regioncollapsed, 55
; CHECK-NEXT:  %cmp.i.i = icmp eq i32 %vecext.i.i1.regioncollapsed, 55
  %conv.i.i4 = select i1 %cmp.i.i, i32 1, i32 0
  %bitcast = bitcast i32 %conv.i.i4 to <1 x i32>
  %ptrtoint6 = ptrtoint i8 addrspace(1)* %_arg_res_gpu to i64
  %.splatinsert13 = bitcast i64 %ptrtoint6 to <1 x i64>
  call void @llvm.genx.svm.scatter.v1i1.v1i64.v1i32(<1 x i1> <i1 true>, i32 0, <1 x i64> %.splatinsert13, <1 x i32> %bitcast)
  ret void
}

define dllexport spir_kernel void @TestGVClobberingFixupLoopLocalStore(i8 addrspace(1)* nocapture readonly %_arg_input_gpu, i8 addrspace(1)* nocapture %_arg_res_gpu, i64 %impl.arg.private.base) local_unnamed_addr #1 {
entry:
  %ptrtoint = ptrtoint i8 addrspace(1)* %_arg_input_gpu to i64
  %.splatinsert7 = bitcast i64 %ptrtoint to <1 x i64>
  %call.i.i.i.i.i.esimd7 = load volatile <4 x i32>, <4 x i32>* @_ZL8g_global
  %gather9 = call <1 x i32> @llvm.genx.svm.gather.v1i32.v1i1.v1i64(<1 x i1> <i1 true>, i32 0, <1 x i64> %.splatinsert7, <1 x i32> undef)
  %call4.i.i.i.i.esimd = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.v1i1(<4 x i32> %call.i.i.i.i.i.esimd7, <1 x i32> %gather9, i32 0, i32 1, i32 1, i16 0, i32 0, <1 x i1> <i1 true>)
  store volatile <4 x i32> %call4.i.i.i.i.esimd, <4 x i32>* @_ZL8g_global
  %.iv32cast = bitcast i64 %ptrtoint to <2 x i32>
  %.LoSplit = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> %.iv32cast, i32 0, i32 1, i32 2, i16 0, i32 undef)
  %int_emu.add64.lo.aggregate. = call { <1 x i32>, <1 x i32> } @llvm.genx.addc.v1i32.v1i32(<1 x i32> %.LoSplit, <1 x i32> <i32 4>)
  %int_emu.add64.lo.add. = extractvalue { <1 x i32>, <1 x i32> } %int_emu.add64.lo.aggregate., 1
  %int_emu.add64.lo.carry. = extractvalue { <1 x i32>, <1 x i32> } %int_emu.add64.lo.aggregate., 0
  %int_emu.add.partial_join = call <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32> undef, <1 x i32> %int_emu.add64.lo.add., i32 0, i32 1, i32 2, i16 0, i32 undef, i1 true)
  %.HiSplit = call <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32> %.iv32cast, i32 0, i32 1, i32 2, i16 4, i32 undef)
  %add_hi.part = add <1 x i32> %int_emu.add64.lo.carry., %.HiSplit
  %int_emu.add.joined = call <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32> %int_emu.add.partial_join, <1 x i32> %add_hi.part, i32 0, i32 1, i32 2, i16 4, i32 undef, i1 true)
  %int_emu.add. = bitcast <2 x i32> %int_emu.add.joined to <1 x i64>
  %gather = call <1 x i32> @llvm.genx.svm.gather.v1i32.v1i1.v1i64(<1 x i1> <i1 true>, i32 0, <1 x i64> %int_emu.add., <1 x i32> undef)
  %bitcast = bitcast <1 x i32> %gather to i32
  %call.i.i.i17.i.esimd8 = load volatile <4 x i32>, <4 x i32>* @_ZL8g_global
; CHECK:  %call.i.i.i17.i.esimd8 = load volatile <4 x i32>, <4 x i32>* @_ZL8g_global
; CHECK-NEXT:  %vecext.i.i3.regioncollapsed = call <1 x i32> @llvm.genx.rdregioni.v1i32.v4i32.i16(<4 x i32> %call.i.i.i17.i.esimd8, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %cmp.i1 = icmp sgt i32 %bitcast, 0
; CHECK-NEXT:  %cmp.i1 = icmp sgt i32 %bitcast, 0
  br i1 %cmp.i1, label %entry.while.body.i_crit_edge, label %entry.TestGVClobberingFixupLoopLocalStore.exit_crit_edge

entry.TestGVClobberingFixupLoopLocalStore.exit_crit_edge: ; preds = %entry
  br label %TestGVClobberingFixupLoopLocalStore.exit

entry.while.body.i_crit_edge:                     ; preds = %entry
  %ptrtoint8 = ptrtoint i8 addrspace(1)* %_arg_res_gpu to i64
  %.splatinsert35 = bitcast i64 %ptrtoint8 to <1 x i64>
  br label %while.body.i

while.body.i:                                     ; preds = %while.body.i.while.body.i_crit_edge, %entry.while.body.i_crit_edge
  %p2.0.i2 = phi i32 [ %dec.i, %while.body.i.while.body.i_crit_edge ], [ %bitcast, %entry.while.body.i_crit_edge ]
  %dec.i = add nsw i32 %p2.0.i2, -1
; CHECK: %dec.i = add nsw i32 %p2.0.i2, -1
  %vecext.i.i3.regioncollapsed = call <1 x i32> @llvm.genx.rdregioni.v1i32.v4i32.i16(<4 x i32> %call.i.i.i17.i.esimd8, i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NOT:  %vecext.i.i3.regioncollapsed = call <1 x i32> @llvm.genx.rdregioni.v1i32.v4i32.i16(<4 x i32> %call.i.i.i17.i.esimd8, i32 0, i32 1, i32 1, i16 0, i32 undef)
  call void @llvm.genx.svm.scatter.v1i1.v1i64.v1i32(<1 x i1> <i1 true>, i32 0, <1 x i64> %.splatinsert35, <1 x i32> %vecext.i.i3.regioncollapsed)
; CHECK-NEXT: call void @llvm.genx.svm.scatter.v1i1.v1i64.v1i32(<1 x i1> <i1 true>, i32 0, <1 x i64> %.splatinsert35, <1 x i32> %vecext.i.i3.regioncollapsed)
  %call.i.i.i.i.i.i.i.esimd9 = load volatile <4 x i32>, <4 x i32>* @_ZL8g_global
  %call4.i.i.i.i.i.i.esimd = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.v1i1(<4 x i32> %call.i.i.i.i.i.i.i.esimd9, <1 x i32> <i32 42>, i32 0, i32 1, i32 1, i16 0, i32 0, <1 x i1> <i1 true>)
  store volatile <4 x i32> %call4.i.i.i.i.i.i.esimd, <4 x i32>* @_ZL8g_global
  %cmp.i = icmp sgt i32 %p2.0.i2, 1
  br i1 %cmp.i, label %while.body.i.while.body.i_crit_edge, label %while.body.i.TestGVClobberingFixupLoopLocalStore.exit_crit_edge

while.body.i.TestGVClobberingFixupLoopLocalStore.exit_crit_edge: ; preds = %while.body.i
  br label %TestGVClobberingFixupLoopLocalStore.exit

while.body.i.while.body.i_crit_edge:              ; preds = %while.body.i
  br label %while.body.i

TestGVClobberingFixupLoopLocalStore.exit: ; preds = %while.body.i.TestGVClobberingFixupLoopLocalStore.exit_crit_edge, %entry.TestGVClobberingFixupLoopLocalStore.exit_crit_edge
  ret void
}

define internal spir_func void @UserFunctionRewriteGV1() unnamed_addr {
entry:
  tail call spir_func void @UserFunctionRewriteGV2()
  ret void
}

define internal spir_func void @UserFunctionRewriteGV2() unnamed_addr {
entry:
  %call.i.i.i.i.esimd3 = load volatile <4 x i32>, <4 x i32>* @_ZL8g_global
  %call4.i.i.i.esimd = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.v1i1(<4 x i32> %call.i.i.i.i.esimd3, <1 x i32> <i32 42>, i32 0, i32 1, i32 1, i16 0, i32 0, <1 x i1> <i1 true>)
  store volatile <4 x i32> %call4.i.i.i.esimd, <4 x i32>* @_ZL8g_global
  ret void
}

declare i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32)
declare { <1 x i32>, <1 x i32> } @llvm.genx.addc.v1i32.v1i32(<1 x i32>, <1 x i32>)
declare void @llvm.genx.svm.scatter.v1i1.v1i64.v1i32(<1 x i1>, i32, <1 x i64>, <1 x i32>)
declare <1 x i32> @llvm.genx.rdregioni.v1i32.v2i32.i16(<2 x i32>, i32, i32, i32, i16, i32)
declare <1 x i32> @llvm.genx.rdregioni.v1i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32)
declare <1 x i32> @llvm.genx.svm.gather.v1i32.v1i1.v1i64(<1 x i1>, i32, <1 x i64>, <1 x i32>)
declare <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32>, <1 x i32>, i32, i32, i32, i16, i32, i1)
declare <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.v1i1(<4 x i32>, <1 x i32>, i32, i32, i32, i16, i32, <1 x i1>)

attributes #0 = { "VCByteOffset"="128" "VCGlobalVariable" "VCVolatile" "genx_byte_offset"="128" "genx_volatile" }
attributes #1 = { nounwind "CMGenxMain" "VC.Stack.Amount"="0" "oclrt"="1" }

!genx.kernels = !{!1, !2, !3}
!genx.kernel.internal = !{!11, !12, !13}

!1 = !{void (i8 addrspace(1)*, i8 addrspace(1)*)* @TestGVClobberingFixupStoreInCall, !"TestGVClobberingFixupStoreInCall", !{i32 0, i32 0, i32 96}, i32 0, !{i32 72, i32 80, i32 64}, !{i32 0, i32 0}, !{!"svmptr_t", !"svmptr_t"}, i32 0}
!2 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i64)* @TestGVClobberingFixupLocalStore, !"TestGVClobberingFixupLocalStore", !{i32 0, i32 0, i32 96}, i32 0, !{i32 72, i32 80, i32 64}, !{i32 0, i32 0}, !{!"svmptr_t", !"svmptr_t"}, i32 0}
!3 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i64)* @TestGVClobberingFixupLoopLocalStore, !"TestGVClobberingFixupLoopLocalStore", !{i32 0, i32 0, i32 96}, i32 0, !{i32 72, i32 80, i32 64}, !{i32 0, i32 0}, !{!"svmptr_t", !"svmptr_t"}, i32 0}
!11 = !{void (i8 addrspace(1)*, i8 addrspace(1)*)* @TestGVClobberingFixupStoreInCall, !{i32 0, i32 0, i32 0}, !{i32 0, i32 1, i32 2}, !{},  !{i32 255, i32 255, i32 255}}
!12 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i64)* @TestGVClobberingFixupLocalStore, !{i32 0, i32 0, i32 0}, !{i32 0, i32 1, i32 2}, !{}, !{i32 255, i32 255, i32 255}}
!13 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, i64)* @TestGVClobberingFixupLoopLocalStore, !{i32 0, i32 0, i32 0}, !{i32 0, i32 1, i32 2}, !{}, !{i32 255, i32 255, i32 255}}
