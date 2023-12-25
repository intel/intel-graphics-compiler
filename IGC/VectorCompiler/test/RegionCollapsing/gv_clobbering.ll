;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -GenXRegionCollapsing -march=genx64 -mtriple=spir64-unknown-unknown  -mcpu=Gen9 -S < %s | FileCheck %s
; ------------------------------------------------
; GenXRegionCollapsing
; ------------------------------------------------
; This test checks that GenXRegionCollapsing pass doesn't collapse rdregion/wrregion if this will
; result in potential clobbering of a global volatile value access.

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

declare <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.i1(<4 x i32>, <1 x i32>, i32, i32, i32, i16, i32, i1)
declare <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32>, <4 x i32>, i32, i32, i32, i16, i32, i1)
declare <4 x i32> @llvm.genx.rdregioni.v4i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32)
declare <1 x i32> @llvm.genx.rdregioni.v1i32.v4i32.i16(<4 x i32>, i32, i32, i32, i16, i32)
declare void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32>, <4 x i32>*)
declare <4 x i32> @llvm.genx.vload.v4i32.p0v4i32(<4 x i32>*)
declare <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32>, <1 x i32>, i32, i32, i32, i16, i32, i1)
declare <8 x i32> @llvm.genx.wrregioni.v8i32.v4i32.i16.i1(<8 x i32>, <4 x i32>, i32, i32, i32, i16, i32, i1)
declare <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32>, i32, i32, i32, i16, i32)

@g1 = external global <4 x i32> #0

; NB: checking GenXRegionCollapsing::runOnBasicBlock(...)
; CHECK: define spir_kernel void @chkWrr2DoesntUseVload1()
define spir_kernel void @chkWrr2DoesntUseVload1() #1 {
; CHECK:   %vload1 = call <4 x i32> @llvm.genx.vload.v4i32.p0v4i32(<4 x i32>* @g1)
  %vload1 = call <4 x i32> @llvm.genx.vload.v4i32.p0v4i32(<4 x i32>* @g1)
  %rdr1 = call <1 x i32> @llvm.genx.rdregioni.v1i32.v4i32.i16(<4 x i32> %vload1, i32 0, i32 1, i32 0, i16 0, i32 0)
  %wrr1 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.i1(<4 x i32> undef, <1 x i32> %rdr1, i32 0, i32 1, i32 0, i16 0, i32 0, i1 true)
  call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> zeroinitializer, <4 x i32>* @g1)
; CHECK-NOT:   %wrr2 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.i1(<4 x i32> %vload1, <1 x i32> zeroinitializer, i32 0, i32 3, i32 0, i16 4, i32 0,
  %wrr2 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.i1(<4 x i32> %wrr1, <1 x i32> zeroinitializer, i32 0, i32 3, i32 0, i16 4, i32 0, i1 true)
  call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> %wrr2, <4 x i32>* @g1)
; CHECK: call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> %wrr2, <4 x i32>* @g1)
  ret void
}

; CHECK: define spir_kernel void @chkRdr1AndWrr1Collapse()
define spir_kernel void @chkRdr1AndWrr1Collapse() #1 {
  %vload1 = call <4 x i32> @llvm.genx.vload.v4i32.p0v4i32(<4 x i32>* @g1)
  %rdr1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v4i32.i16(<4 x i32> %vload1, i32 0, i32 4, i32 0, i16 0, i32 0)
  %wrr1 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.i1(<4 x i32> %rdr1, <1 x i32> undef, i32 0, i32 1, i32 0, i16 0, i32 0, i1 true)
; CHECK: call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> zeroinitializer, <4 x i32>* @g1)
  call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> zeroinitializer, <4 x i32>* @g1)
; CHECK-NEXT: %wrr2 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32> %rdr1, <4 x i32> %rdr1, i32 0, i32 4, i32 0, i16 0, i32 0, i1 true)
  %wrr2 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32> %rdr1, <4 x i32> %wrr1, i32 0, i32 4, i32 0, i16 0, i32 0, i1 true)
  call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> %wrr2, <4 x i32>* @g1)
  ret void
}

; NB: checking GenXRegionCollapsing::runOnBasicBlock(...)
; CHECK: define spir_kernel void @chkWrr1AndWrr2Collapse()
define spir_kernel void @chkWrr1AndWrr2Collapse() #1 {
  %vload1 = call <4 x i32> @llvm.genx.vload.v4i32.p0v4i32(<4 x i32>* @g1)
  %rdr1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v4i32.i16(<4 x i32> %vload1, i32 0, i32 4, i32 0, i16 0, i32 0)
  %wrr1 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.i1(<4 x i32> %rdr1, <1 x i32> undef, i32 0, i32 1, i32 0, i16 0, i32 0, i1 true)
; CHECK-NOT: %wrr1 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.i1(<4 x i32> %rdr1, <1 x i32> undef, i32 0, i32 1, i32 0, i16 0, i32 0, i1 true)
; CHECK: %wrr2 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32> %vload1, <4 x i32> %rdr1, i32 0, i32 4, i32 0, i16 0, i32 0, i1 true)
  %wrr2 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32> %vload1, <4 x i32> %wrr1, i32 0, i32 4, i32 0, i16 0, i32 0, i1 true)
  call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> zeroinitializer, <4 x i32>* @g1)
; CHECK-NOT:   %wrr3 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.i1(<4 x i32> %vload1, <1 x i32> zeroinitializer, i32 0, i32 3, i32 0, i16 4, i32 0,
  %wrr3 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.i1(<4 x i32> %wrr2, <1 x i32> zeroinitializer, i32 0, i32 3, i32 0, i16 4, i32 0, i1 true)
  call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> %wrr3, <4 x i32>* @g1)
  ret void
}

; NB: checking GenXRegionCollapsing::processWrRegionSplat(...)
; CHECK: define spir_kernel void @chkWrr2NotCollapsed()
define spir_kernel void @chkWrr2NotCollapsed() #1 {
  %vload1 = call <4 x i32> @llvm.genx.vload.v4i32.p0v4i32(<4 x i32>* @g1)
  %wrr1 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32> undef, <4 x i32> %vload1, i32 0, i32 1, i32 0, i16 0, i32 0, i1 true)
  call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> zeroinitializer, <4 x i32>* @g1)
; CHECK:  call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> zeroinitializer, <4 x i32>* @g1)
; CHECK-NOT:  %wrr1.regioncollapsed = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32> undef, <4 x i32> %vload1, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
; CHECK-NEXT:  %wrr2 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32> undef, <4 x i32> %wrr1, i32 0, i32 1, i32 0, i16 0, i32 0, i1 true)
  %wrr2 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32> undef, <4 x i32> %wrr1, i32 0, i32 1, i32 0, i16 0, i32 0, i1 true)
  call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> %wrr2, <4 x i32>* @g1)
  ret void
}

; NB: checking GenXRegionCollapsing::processWrRegionElim(...)
; CHECK: define spir_kernel void @chkWrr2NotCollapsed2()
define spir_kernel void @chkWrr2NotCollapsed2() #1 {
; CHECK: %vload1 = call <4 x i32> @llvm.genx.vload.v4i32.p0v4i32(<4 x i32>* @g1)
  %vload1 = call <4 x i32> @llvm.genx.vload.v4i32.p0v4i32(<4 x i32>* @g1)
  %wrr1 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32> %vload1, <4 x i32> undef, i32 0, i32 1, i32 1, i16 0, i32 0, i1 true)
  call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> zeroinitializer, <4 x i32>* @g1)
  %vload2 = call <4 x i32> @llvm.genx.vload.v4i32.p0v4i32(<4 x i32>* @g1)
  %wrr2 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32> %wrr1, <4 x i32> %vload2, i32 0, i32 1, i32 1, i16 0, i32 0, i1 true)
; CHECK-NOT:  %wrr2.regioncollapsed = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32> %vload1, <4 x i32> %vload2, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
  call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> %wrr2, <4 x i32>* @g1)
; CHECK: call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> %wrr2, <4 x i32>* @g1)
  ret void
}

; NB: checking GenXRegionCollapsing::runOnBasicBlock(...)
; CHECK: define spir_kernel void @chkRdr0AndRdr1_Wrr0AndWrr1_Collapse()
define spir_kernel void @chkRdr0AndRdr1_Wrr0AndWrr1_Collapse() #1 {
  %vload1 = call <4 x i32> @llvm.genx.vload.v4i32.p0v4i32(<4 x i32>* @g1)
; CHECK-NEXT:  %vload1 = call <4 x i32> @llvm.genx.vload.v4i32.p0v4i32(<4 x i32>* @g1)
; CHECK:  %rdr0 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v4i32.i16(<4 x i32> %vload1, i32 0, i32 4, i32 1, i16 0, i32 0)
  %rdr0 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v4i32.i16(<4 x i32> %vload1 , i32 0, i32 4, i32 1, i16 0, i32 0)
  %rdr1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v4i32.i16(<4 x i32> %rdr0 , i32 0, i32 4, i32 1, i16 0, i32 0)
  call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> zeroinitializer, <4 x i32>* @g1)
  %wrr0 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.i1(<4 x i32> %rdr1, <1 x i32> <i32 42>, i32 0, i32 1, i32 0, i16 0, i32 0, i1 true)
  %wrr1 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32> %rdr0 , <4 x i32> %wrr0, i32 0, i32 4, i32 1, i16 0, i32 0, i1 true)
; CHECK: %wrr0 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.i1(<4 x i32> %rdr0, <1 x i32> <i32 42>, i32 0, i32 1, i32 0, i16 0, i32 0, i1 true)
; CHECK-NEXT:  call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> %wrr0, <4 x i32>* @g1)
  call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> %wrr0, <4 x i32>* @g1)
  ret void
}

; NB: checking GenXRegionCollapsing::processWrRegion(...)
; NB: %0 function param is here because it restricts other phases of
; this optimization from simplifying IR so that processWrRegion() had a job to do.
; Rephrazing the comment in function documentation, this checks for the following
; case:
;
;------------------------------------
; B = rdregion(A, OuterR)
; L = vload(G)
; C = wrregion(B, L, InnerR)
; vstore(G)
; D = wrregion(A, C, OuterR)
;------------------------------------
;
; not being converted into
;
;------------------------------------
; L = vload(G)
; vstore(G)
; D = wrregion(A, L, CombinedR)
;------------------------------------

; CHECK: define spir_kernel void @chkProcessWrRegion(<1 x i32> %0)
define spir_kernel void @chkProcessWrRegion(<1 x i32> %0) #1 {
; CHECK-NEXT: %wrr0 = call <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32> zeroinitializer, <1 x i32> %0, i32 0, i32 1, i32 0, i16 0, i32 0, i1 true)
; CHECK-NOT: %vload1 = call <4 x i32> @llvm.genx.vload.v4i32.p0v4i32(<4 x i32>* @g1)
; CHECK-NOT: call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> zeroinitializer, <4 x i32>* @g1
; CHECK-NOT: %wrr1.regioncollapsed = call <8 x i32> @llvm.genx.wrregioni.v8i32.v4i32.i16.i1(<8 x i32> %wrr0, <4 x i32> %vload1, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
; CHECK-NOT: store <8 x i32> %wrr1.regioncollapsed, <8 x i32>* null, align 1024
; CHECK: %rdr1 =
; CHECK: %vload1 =
; CHECK: %wrr1 =
; CHECK: call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> zeroinitializer, <4 x i32>* @g1)
; CHECK: %wrr2 = call <8 x i32> @llvm.genx.wrregioni.v8i32.v4i32.i16.i1(<8 x i32> %wrr0, <4 x i32> %wrr1, i32 0, i32 4, i32 1, i16 0, i32 0, i1 true)
  %wrr0 = call <8 x i32> @llvm.genx.wrregioni.v8i32.v1i32.i16.i1(<8 x i32> zeroinitializer, <1 x i32> %0, i32 0, i32 1, i32 0, i16 0, i32 0, i1 true)
  %rdr1 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v8i32.i16(<8 x i32> %wrr0, i32 0, i32 4, i32 1, i16 0, i32 0)
  %vload1 = call <4 x i32> @llvm.genx.vload.v4i32.p0v4i32(<4 x i32>* @g1)
  %wrr1 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32> %rdr1, <4 x i32> %vload1, i32 0, i32 1, i32 0, i16 0, i32 0, i1 true)
  call void @llvm.genx.vstore.v4i32.p0v4i32(<4 x i32> zeroinitializer, <4 x i32>* @g1)
  %wrr2 = call <8 x i32> @llvm.genx.wrregioni.v8i32.v4i32.i16.i1(<8 x i32> %wrr0, <4 x i32> %wrr1, i32 0, i32 4, i32 1, i16 0, i32 0, i1 true)
  store <8 x i32> %wrr2, <8 x i32>* null, align 1024
  ret void
}

declare void @llvm.genx.svm.block.st.i64.v4i32(i64, <4 x i32>)
declare <4 x i32> @llvm.genx.svm.block.ld.v4i32.i64(i64)
declare <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32>, i32, i32, i32, i16, i32)

; NB: checking GenXRegionCollapsing::processRdRegion(...)
; CHECK: define spir_kernel void @chkRdRegion(i64 %inSvmPtr, i64 %outSvmPtr)
define spir_kernel void @chkRdRegion(i64 %inSvmPtr, i64 %outSvmPtr) #1 {
; This also checks that load volatile is treated same as genx.vload
  %vload1 = load volatile <4 x i32>, <4 x i32>* @g1, align 16
  %svm.load = tail call <4 x i32> @llvm.genx.svm.block.ld.v4i32.i64(i64 %inSvmPtr)
  %wrr1 = call <4 x i32> @llvm.genx.wrregioni.v4i32.v4i32.i16.i1(<4 x i32> %vload1, <4 x i32> %svm.load, i32 0, i32 4, i32 1, i16 0, i32 undef, i1 true)
; This also checks that store volatile is treated same as genx.vstore
  store volatile <4 x i32> %wrr1, <4 x i32>* @g1, align 16
  %vload2 = load volatile <4 x i32>, <4 x i32>* @g1, align 16
  %rdr1 = tail call <1 x i32> @llvm.genx.rdregioni.v1i32.v4i32.i16(<4 x i32> %vload2, i32 0, i32 1, i32 0, i16 0, i32 undef)
; CHECK:  %vload2 = load volatile <4 x i32>, <4 x i32>* @g1, align 16
; CHECK-NEXT:  %rdr1 = tail call <1 x i32> @llvm.genx.rdregioni.v1i32.v4i32.i16(<4 x i32> %vload2, i32 0, i32 1, i32 0, i16 0, i32 undef)
  %wrr2 = tail call <4 x i32> @llvm.genx.wrregioni.v4i32.v1i32.i16.i1(<4 x i32> %vload2, <1 x i32> <i32 100500>, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
  store volatile <4 x i32> %wrr2, <4 x i32>* @g1, align 16
; %rdr1 must not get collapsed with %rdr2 since there is the potentially clobbering store to related global value preceding %rdr2.
  %rdr2 = call <4 x i32> @llvm.genx.rdregioni.v4i32.v1i32.i16(<1 x i32> %rdr1, i32 0, i32 4, i32 0, i16 0, i32 undef)
  tail call void @llvm.genx.svm.block.st.i64.v4i32(i64 %outSvmPtr, <4 x i32> %rdr2)
  ret void
}

; NB: checking GenXRegionCollapsing::processRdRegion(...)
@g2 = external global <3584 x float> #0

declare <16 x i32> @llvm.genx.wrregioni.v16i32.v1i32.i16.i1(<16 x i32>, <1 x i32>, i32, i32, i32, i16, i32, i1)
declare void @llvm.genx.raw.send2.noresult.v16i1.v32i16(i8, i8, <16 x i1>, i8, i8, i32, i32, <32 x i16>)
declare i32 @llvm.genx.rdregioni.i32.v16i32.i16(<16 x i32>, i32, i32, i32, i16, i32)
declare <16 x float> @llvm.genx.rdregionf.v16f32.v3584f32.i16(<3584 x float>, i32, i32, i32, i16, i32)

; CHECK: define spir_kernel void @reducedFromSGEMM_32x64()
define spir_kernel void @reducedFromSGEMM_32x64() #1 {
; This also checks that load volatile is treated same as genx.vload
  %vload1 = load volatile <3584 x float>, <3584 x float>* @g2, align 16384
  %rdr1 = tail call <16 x float> @llvm.genx.rdregionf.v16f32.v3584f32.i16(<3584 x float> %vload1, i32 0, i32 0, i32 0, i16 0, i32 0)
  %bc1 = bitcast <16 x float> %rdr1 to <16 x i32>
;------------------------------
; The above sequence transforms to the below during RegionCollapsing execution:
;------------------------------
;  %vload1 = load volatile <3584 x float>, <3584 x float>* @g2, align 16384
;  %bc1 = bitcast <3584 x float> %vload1 to <3584 x i32>
;  %rdr1 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v3584i32.i16(<3584 x i32> %bc1, i32 0, i32 1, i32 0, i16 0, i32 undef)
;------------------------------
; And later can get collapsed after the store to related global volatile value:
;------------------------------
;  store volatile <3584 x float> zeroinitializer, <3584 x float>* @g2, align 16384
;  %rdr2.regioncollapsed = call i32 @llvm.genx.rdregioni.i32.v3584i32.i16(<3584 x i32> %bc1, i32 0, i32 1, i32 0, i16 0, i32 undef)
;------------------------------
; Further we check that the %rdr1 doesn't get collapsed with %rdr2 after the store to @g2.
;------------------------------
; CHECK: %vload1 = load volatile <3584 x float>, <3584 x float>* @g2, align 16384
; CHECK: %bc1 = bitcast <3584 x float> %vload1 to <3584 x i32>
; CHECK: %rdr1 = call <16 x i32> @llvm.genx.rdregioni.v16i32.v3584i32.i16(<3584 x i32> %bc1, i32 0, i32 1, i32 0, i16 0, i32 undef)
; This also checks that store volatile is treated same as genx.vstore
  store volatile <3584 x float> zeroinitializer, <3584 x float>* @g2, align 16384
; CHECK: store volatile <3584 x float> zeroinitializer, <3584 x float>* @g2, align 16384
; CHECK-NOT: %rdr2.regioncollapsed =
; CHECK: %rdr2 =
  %rdr2 = tail call i32 @llvm.genx.rdregioni.i32.v16i32.i16(<16 x i32> %bc1, i32 0, i32 0, i32 0, i16 0, i32 0)
  %sev.cast.189115 = bitcast i32 %rdr2 to <1 x i32>
  %wrr1 = tail call <16 x i32> @llvm.genx.wrregioni.v16i32.v1i32.i16.i1(<16 x i32> zeroinitializer, <1 x i32> %sev.cast.189115, i32 0, i32 0, i32 0, i16 0, i32 0, i1 false)
  %bc2 = bitcast <16 x i32> %wrr1 to <32 x i16>
  tail call void @llvm.genx.raw.send2.noresult.v16i1.v32i16(i8 0, i8 0, <16 x i1> zeroinitializer, i8 0, i8 0, i32 0, i32 0, <32 x i16> %bc2)
  ret void
}


attributes #0 = { "genx_volatile" }
attributes #1 = { "CMGenxMain" }
