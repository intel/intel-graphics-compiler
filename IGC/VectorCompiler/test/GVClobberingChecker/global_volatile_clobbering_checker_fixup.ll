;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXGVClobberChecker -check-gv-clobbering=true -check-gv-clobbering-collect-store-related-call-sites=true -check-gv-clobbering-try-fixup=true -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Gen9 -S < %s | FileCheck %s
; RUN: opt %use_old_pass_manager% -GenXGVClobberChecker -check-gv-clobbering=true -check-gv-clobbering-collect-store-related-call-sites=false -check-gv-clobbering-try-fixup=true -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; This test checks global volatile clobbering checker/fixup introduced late in pipeline to catch over-optimizations of global volatile access.
; This is an auxiliary utility used to help in detecting and fixing erroneous over-optimizations cases.
; The checker/fixup is only available under the -check-gv-clobbering=true option.
;
target datalayout = "e-p:64:64-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

@_ZL8g_global = external global <4 x i32> #0

define spir_kernel void @TestGVClobberingFixupStoreInCall() {
entry:
  %call.i.i.i8.i.esimd6 = load volatile <4 x i32>, <4 x i32>* @_ZL8g_global, align 16
  tail call spir_func void @UserFunctionRewriteGV1()
  %vecext.i.i1.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %call.i.i.i8.i.esimd6, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %cmp.i.i = icmp eq i32 %vecext.i.i1.regioncollapsed, 55
  ret void
}
; CHECK:  %call.i.i.i8.i.esimd6 = load volatile <4 x i32>, <4 x i32>* @_ZL8g_global
; CHECK-NEXT:  %vecext.i.i1.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %call.i.i.i8.i.esimd6, i32 0, i32 1, i32 1, i16 0, i32 undef)
; COM: if -check-gv-clobbering-collect-store-related-call-sites=true is supplied
; COM: store interference is precisely detected here down the call chain.
; COM: if -check-gv-clobbering-collect-store-related-call-sites=false is supplied or omitted
; COM: store interference is speculated because of call to a user function.
; CHECK-NEXT:  tail call spir_func void @UserFunctionRewriteGV1()
; CHECK-NOT:  %vecext.i.i1.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %call.i.i.i8.i.esimd6, i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT:  %cmp.i.i = icmp eq i32 %vecext.i.i1.regioncollapsed, 55

define spir_kernel void @TestGVClobberingFixupLocalStore() {
entry:
  %call.i.i.i8.i.esimd6 = load volatile <4 x i32>, <4 x i32>* @_ZL8g_global, align 16
  %call4.i.i.i.i.i.i.esimd = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.v1i1(<4 x i32> %call.i.i.i8.i.esimd6, <1 x i32> <i32 42>, i32 0, i32 1, i32 1, i16 0, i32 0, <1 x i1> <i1 true>)
  store volatile <4 x i32> %call4.i.i.i.i.i.i.esimd, <4 x i32>* @_ZL8g_global, align 16
  %vecext.i.i1.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %call.i.i.i8.i.esimd6, i32 0, i32 1, i32 1, i16 0, i32 undef)
  %cmp.i.i = icmp eq i32 %vecext.i.i1.regioncollapsed, 55
  ret void
}
; CHECK:  %call.i.i.i8.i.esimd6 = load volatile <4 x i32>, <4 x i32>* @_ZL8g_global
; CHECK-NEXT:  %vecext.i.i1.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %call.i.i.i8.i.esimd6, i32 0, i32 1, i32 1, i16 0, i32 undef)
; COM: store interference is directly detected in this function.
; CHECK-NEXT:  %call4.i.i.i.i.i.i.esimd = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.v1i1(<4 x i32> %call.i.i.i8.i.esimd6, <1 x i32> <i32 42>, i32 0, i32 1, i32 1, i16 0, i32 0, <1 x i1> <i1 true>)
; CHECK-NEXT: store volatile <4 x i32> %call4.i.i.i.i.i.i.esimd, <4 x i32>* @_ZL8g_global
; CHECK-NOT:  %vecext.i.i1.regioncollapsed = tail call i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32> %call.i.i.i8.i.esimd6, i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT:  %cmp.i.i = icmp eq i32 %vecext.i.i1.regioncollapsed, 55

define spir_kernel void @TestGVClobberingFixupLoopLocalStore(i32 %bitcast, i32 %p2.0.i2, <1 x i64> %.splatinsert35) {
entry:
  %call.i.i.i17.i.esimd8 = load volatile <4 x i32>, <4 x i32>* @_ZL8g_global, align 16
  %cmp.i1 = icmp sgt i32 %bitcast, 0
  br label %while.body.i

while.body.i:                                     ; preds = %while.body.i.while.body.i_crit_edge, %entry
  %dec.i = add nsw i32 %p2.0.i2, -1
  %vecext.i.i3.regioncollapsed = call <1 x i32> @llvm.genx.rdregioni.v1i32.v4i32.i16(<4 x i32> %call.i.i.i17.i.esimd8, i32 0, i32 1, i32 1, i16 0, i32 undef)
  call void @llvm.genx.svm.scatter.v1i1.v1i64.v1i32(<1 x i1> <i1 true>, i32 0, <1 x i64> %.splatinsert35, <1 x i32> %vecext.i.i3.regioncollapsed)
  store volatile <4 x i32> zeroinitializer, <4 x i32>* @_ZL8g_global, align 16
  br label %while.body.i.while.body.i_crit_edge

while.body.i.while.body.i_crit_edge:              ; preds = %while.body.i
  br label %while.body.i
}
; CHECK:  %call.i.i.i17.i.esimd8 = load volatile <4 x i32>, <4 x i32>* @_ZL8g_global
; CHECK-NEXT:  %vecext.i.i3.regioncollapsed = call <1 x i32> @llvm.genx.rdregioni.v1i32.v4i32.i16(<4 x i32> %call.i.i.i17.i.esimd8, i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT:  %cmp.i1 = icmp sgt i32 %bitcast, 0
; CHECK: %dec.i = add nsw i32 %p2.0.i2, -1
; CHECK-NOT:  %vecext.i.i3.regioncollapsed = call <1 x i32> @llvm.genx.rdregioni.v1i32.v4i32.i16(<4 x i32> %call.i.i.i17.i.esimd8, i32 0, i32 1, i32 1, i16 0, i32 undef)
; CHECK-NEXT: call void @llvm.genx.svm.scatter.v1i1.v1i64.v1i32(<1 x i1> <i1 true>, i32 0, <1 x i64> %.splatinsert35, <1 x i32> %vecext.i.i3.regioncollapsed)

define internal spir_func void @UserFunctionRewriteGV1() {
entry:
  tail call spir_func void @UserFunctionRewriteGV2()
  ret void
}

define internal spir_func void @UserFunctionRewriteGV2() {
entry:
  store volatile <4 x i32> zeroinitializer, <4 x i32>* @_ZL8g_global, align 16
  ret void
}

declare i32 @llvm.genx.rdregioni.i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32)
declare void @llvm.genx.svm.scatter.v1i1.v1i64.v1i32(<1 x i1>, i32, <1 x i64>, <1 x i32>)
declare <1 x i32> @llvm.genx.rdregioni.v1i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32)
declare <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.v1i1(<4 x i32>, <1 x i32>, i32, i32, i32, i16, i32, <1 x i1>)

attributes #0 = { "genx_volatile" }
