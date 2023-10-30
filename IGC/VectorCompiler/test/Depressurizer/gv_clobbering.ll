;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; This test checks that GenXDepressurizerWrapper will not move superbales in a way
; that can result in potential global volatile value clobbering.
; Test case reduced from: frc_iteration6_4x8_outlined_global

; RUN: %opt %use_old_pass_manager% -GenXModule -GenXLiveRangesWrapper -GenXDepressurizerWrapper \
; RUN:  -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s
target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

@OffsetX = external global <24 x i32> #0

declare <8 x i32> @llvm.genx.rdregioni.v8i32.v24i32.i16(<24 x i32>, i32, i32, i32, i16, i32)
declare <24 x i32> @llvm.genx.wrregioni.v24i32.v8i32.i16.i1(<24 x i32>, <8 x i32>, i32, i32, i32, i16, i32, i1)
declare <24 x i32> @llvm.genx.wrregioni.v24i32.v24i32.i16.i1(<24 x i32>, <24 x i32>, i32, i32, i32, i16, i32, i1)

define internal spir_func void @_Z18simple_MV_groupingy15cm_surfaceindexii15cm_surfaceindex() {
  %load343 = load volatile <24 x i32>, <24 x i32>* @OffsetX, align 128
  %.gload = load volatile <24 x i32>, <24 x i32>* @OffsetX, align 128
  %.split16158 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v24i32.i16(<24 x i32> %load343, i32 0, i32 0, i32 0, i16 0, i32 0)
  %.split16159 = icmp sgt <8 x i32> %.split16158, zeroinitializer
  %.wrr.gstore = call <24 x i32> @llvm.genx.wrregioni.v24i32.v24i32.i16.i1(<24 x i32> %.gload, <24 x i32> zeroinitializer, i32 0, i32 24, i32 1, i16 0, i32 undef, i1 true)
  store volatile <24 x i32> %.wrr.gstore, <24 x i32>* @OffsetX, align 128
; CHECK:  store volatile <24 x i32> %.wrr.gstore, <24 x i32>* @OffsetX, align 128
; CHECK-NOT:  %.split16158 = call <8 x i32> @llvm.genx.rdregioni.v8i32.v24i32.i16(<24 x i32> %load343, i32 0, i32 0, i32 0, i16 0, i32 0)
; CHECK-NOT:  %.split16159 = icmp sgt <8 x i32> %.split16158, zeroinitializer
  %.split16154 = select <8 x i1> %.split16159, <8 x i32> zeroinitializer, <8 x i32> zeroinitializer
  %.wrr.gstore10.join16 = call <24 x i32> @llvm.genx.wrregioni.v24i32.v8i32.i16.i1(<24 x i32> zeroinitializer, <8 x i32> %.split16154, i32 0, i32 0, i32 0, i16 0, i32 0, i1 false)
  ret void
}

define spir_kernel void @bmc() #1 {
  call spir_func void @_Z18simple_MV_groupingy15cm_surfaceindexii15cm_surfaceindex()
  ret void
}


attributes #0 = { "genx_volatile" }
attributes #1 = { "CMGenxMain"  }
