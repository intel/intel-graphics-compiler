;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
; RUN: igc_opt -opaque-pointers -S --regkey=ForceOCLSIMDWidth=16 --igc-code-loop-sinking -igc-serialize-metadata < %s 2>&1 | FileCheck %s

; CHECK: !{!"maxRegUniformPressure", i32 32
; CHECK: !{!"maxRegNonUniformPressure", i32 320
; CHECK: !{!"bestGuessSimd", i32 16

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @foo(float %0, ptr addrspace(1) %1, i32 %2, ptr addrspace(1) %3, ptr addrspace(1) %4, ptr addrspace(1) %5, ptr addrspace(1) %6, ptr addrspace(1) %7, ptr addrspace(1) %8, <8 x i32> %r0, <3 x i32> %globalOffset, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr addrspace(2) %constBase, i32 %bufferOffset, i32 %bufferOffset1, i32 %bufferOffset3, i32 %bufferOffset4, i32 %bufferOffset5) {
  %10 = lshr i32 0, 0
  %11 = add i32 0, 0
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 0, i32 %11, i32 0, i32 0, i32 %10, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %12 = lshr i32 0, 0
  %Block2D_AddrPayload = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 0, i32 0, i32 0, i32 0, i32 %12, i32 0, i32 0, i32 0, i32 0)
  %Block2D_AddrPayload928 = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0)
  %Block2D_AddrPayload937 = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0)
  br label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge, %9
  %13 = phi i32 [ 0, %9 ], [ 0, %._crit_edge ]
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr null, i32 0, i32 %13, i1 false)
  %Block2D_ReadAddrPayload = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload897 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload898 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload899 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload900 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload901 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload902 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload903 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload904 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload905 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload906 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload907 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload908 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload909 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload910 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload911 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload912 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload913 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload914 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload915 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload916 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload917 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload918 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload919 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload920 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload921 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload922 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload923 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload924 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload925 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload926 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %Block2D_ReadAddrPayload927 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr null, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i1 false, i1 false, i32 0)
  %14 = insertelement <8 x i16> zeroinitializer, i16 0, i32 0
  %15 = extractelement <16 x i16> zeroinitializer, i32 0
  %16 = insertelement <8 x i16> %14, i16 %15, i32 0
  %17 = extractelement <16 x i16> zeroinitializer, i32 0
  %18 = insertelement <8 x i16> zeroinitializer, i16 %17, i32 0
  %19 = extractelement <16 x i16> zeroinitializer, i32 0
  %20 = insertelement <8 x i16> %18, i16 %19, i32 0
  %21 = extractelement <16 x i16> zeroinitializer, i32 0
  %22 = insertelement <8 x i16> %20, i16 %21, i32 0
  %23 = extractelement <16 x i16> zeroinitializer, i32 0
  %24 = insertelement <8 x i16> %22, i16 %23, i32 0
  %25 = extractelement <16 x i16> zeroinitializer, i32 0
  %26 = insertelement <8 x i16> %24, i16 %25, i32 0
  %27 = extractelement <16 x i16> zeroinitializer, i32 0
  %28 = insertelement <8 x i16> %26, i16 %27, i32 0
  %29 = extractelement <16 x i16> zeroinitializer, i32 0
  %30 = insertelement <8 x i16> %28, i16 %29, i32 0
  %31 = extractelement <16 x i16> zeroinitializer, i32 0
  %32 = insertelement <8 x i16> %30, i16 %31, i32 0
  %33 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %34 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %35 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %36 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %37 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %33, <8 x i16> zeroinitializer, <8 x i32> %Block2D_ReadAddrPayload898, i32 0, i32 0, i32 0, i32 0, i1 false)
  %38 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %34, <8 x i16> zeroinitializer, <8 x i32> %Block2D_ReadAddrPayload899, i32 0, i32 0, i32 0, i32 0, i1 false)
  %39 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %35, <8 x i16> zeroinitializer, <8 x i32> %Block2D_ReadAddrPayload914, i32 0, i32 0, i32 0, i32 0, i1 false)
  %40 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %36, <8 x i16> zeroinitializer, <8 x i32> %Block2D_ReadAddrPayload915, i32 0, i32 0, i32 0, i32 0, i1 false)
  %41 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %37, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %42 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %38, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %43 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %39, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %44 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %40, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %45 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %41, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %46 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %42, <8 x i16> zeroinitializer, <8 x i32> %Block2D_ReadAddrPayload903, i32 0, i32 0, i32 0, i32 0, i1 false)
  %47 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %43, <8 x i16> zeroinitializer, <8 x i32> %Block2D_ReadAddrPayload918, i32 0, i32 0, i32 0, i32 0, i1 false)
  %48 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %44, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %49 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %45, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %50 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %46, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %51 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %47, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %52 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %48, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %53 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %49, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %54 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %50, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %55 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %51, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %56 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %52, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %57 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %53, <8 x i16> %16, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %58 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %54, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %59 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %55, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %60 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %56, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %61 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %57, <8 x i16> %32, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %62 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %58, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %vectorized_intrinsic = fmul <8 x float> %61, %62
  %63 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %59, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %vectorized_intrinsic345 = fmul <8 x float> %vectorized_intrinsic, %63
  %64 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %60, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %vectorized_intrinsic354 = fmul <8 x float> %vectorized_intrinsic345, %64
  %vectorized_joint_waveall = call <8 x float> @llvm.genx.GenISA.WaveAll.v8f32(<8 x float> %vectorized_intrinsic354, i8 0, i1 false, i32 0)
  br label %._crit_edge
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

declare void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)

declare <8 x float> @llvm.genx.GenISA.WaveAll.v8f32(<8 x float>, i8, i1, i32)

declare ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32)

declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr, i32, i32, i1)

declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32)

!igc.functions = !{!0}

!0 = !{ptr @foo, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
