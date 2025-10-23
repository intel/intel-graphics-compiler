;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt -S  --igc-vectorizer -dce  --regkey=VectorizerAllowADD=1 --regkey=VectorizerAllowMUL=1 --regkey=VectorizerLog=1 --regkey=VectorizerLogToErr=1 < %s 2>&1 | FileCheck %s

; CHECK: %vectorized_phi = phi <8 x i32>
; CHECK: %vectorized_binary = add <8 x i32>
; CHECK: %{{vectorized_binary.*}} = add <8 x i32>
; CHECK: %{{vectorized_binary.*}} = add <8 x i32>
; CHECK: %{{vectorized_binary.*}} = add <8 x i32>
; CHECK: %{{vectorized_binary.*}} = add <8 x i32>
; CHECK: %{{vectorized_binary.*}} = add <8 x i32>

; Function Attrs: convergent nounwind
define spir_kernel void @matmul_kernel_with_block_pointers() #0 {
cond-add-join:
  %0 = or i32 0, 0
  %1 = and i32 0, 224
  %2 = or i32 %1, 0
  %3 = shl nuw nsw i32 0, 5
  %4 = and i32 %3, 96
  %.reass155.reass = or i32 0, %0
  %5 = add i32 %0, %4
  br label %._crit_edge

._crit_edge:                                      ; preds = %._crit_edge, %cond-add-join
  %.pn2427 = phi i32 [ 0, %cond-add-join ], [ %119, %._crit_edge ]
  %6 = phi i32 [ 0, %cond-add-join ], [ %129, %._crit_edge ]
  %7 = phi i32 [ 0, %cond-add-join ], [ %130, %._crit_edge ]
  %8 = phi i32 [ 0, %cond-add-join ], [ %131, %._crit_edge ]
  %9 = phi i32 [ 0, %cond-add-join ], [ %132, %._crit_edge ]
  %10 = phi i32 [ 0, %cond-add-join ], [ %133, %._crit_edge ]
  %11 = phi i32 [ 0, %cond-add-join ], [ %134, %._crit_edge ]
  %12 = phi i32 [ 0, %cond-add-join ], [ %135, %._crit_edge ]
  %13 = phi i32 [ 0, %cond-add-join ], [ %136, %._crit_edge ]
  %14 = phi i32 [ 0, %cond-add-join ], [ %165, %._crit_edge ]
  %15 = phi i32 [ 0, %cond-add-join ], [ %166, %._crit_edge ]
  %16 = phi i32 [ 0, %cond-add-join ], [ %167, %._crit_edge ]
  %17 = phi i32 [ 0, %cond-add-join ], [ %168, %._crit_edge ]
  %18 = phi i32 [ 0, %cond-add-join ], [ %169, %._crit_edge ]
  %19 = phi i32 [ 0, %cond-add-join ], [ %170, %._crit_edge ]
  %20 = phi i32 [ 0, %cond-add-join ], [ %171, %._crit_edge ]
  %21 = phi i32 [ 0, %cond-add-join ], [ %172, %._crit_edge ]
  %22 = phi i32 [ 0, %cond-add-join ], [ %138, %._crit_edge ]
  %23 = phi i32 [ 0, %cond-add-join ], [ %139, %._crit_edge ]
  %24 = phi i32 [ 0, %cond-add-join ], [ %140, %._crit_edge ]
  %25 = phi i32 [ 0, %cond-add-join ], [ %141, %._crit_edge ]
  %26 = phi i32 [ 0, %cond-add-join ], [ %142, %._crit_edge ]
  %27 = phi i32 [ 0, %cond-add-join ], [ %143, %._crit_edge ]
  %28 = phi i32 [ 0, %cond-add-join ], [ %144, %._crit_edge ]
  %29 = phi i32 [ 0, %cond-add-join ], [ %145, %._crit_edge ]
  %30 = phi i32 [ 0, %cond-add-join ], [ %174, %._crit_edge ]
  %31 = phi i32 [ 0, %cond-add-join ], [ %175, %._crit_edge ]
  %32 = phi i32 [ 0, %cond-add-join ], [ %176, %._crit_edge ]
  %33 = phi i32 [ 0, %cond-add-join ], [ %177, %._crit_edge ]
  %34 = phi i32 [ 0, %cond-add-join ], [ %178, %._crit_edge ]
  %35 = phi i32 [ 0, %cond-add-join ], [ %179, %._crit_edge ]
  %36 = phi i32 [ 0, %cond-add-join ], [ %180, %._crit_edge ]
  %37 = phi i32 [ 0, %cond-add-join ], [ %181, %._crit_edge ]
  %38 = phi i32 [ 0, %cond-add-join ], [ %147, %._crit_edge ]
  %39 = phi i32 [ 0, %cond-add-join ], [ %148, %._crit_edge ]
  %40 = phi i32 [ 0, %cond-add-join ], [ %149, %._crit_edge ]
  %41 = phi i32 [ 0, %cond-add-join ], [ %150, %._crit_edge ]
  %42 = phi i32 [ 0, %cond-add-join ], [ %151, %._crit_edge ]
  %43 = phi i32 [ 0, %cond-add-join ], [ %152, %._crit_edge ]
  %44 = phi i32 [ 0, %cond-add-join ], [ %153, %._crit_edge ]
  %45 = phi i32 [ 0, %cond-add-join ], [ %154, %._crit_edge ]
  %46 = phi i32 [ 0, %cond-add-join ], [ %183, %._crit_edge ]
  %47 = phi i32 [ 0, %cond-add-join ], [ %184, %._crit_edge ]
  %48 = phi i32 [ 0, %cond-add-join ], [ %185, %._crit_edge ]
  %49 = phi i32 [ 0, %cond-add-join ], [ %186, %._crit_edge ]
  %50 = phi i32 [ 0, %cond-add-join ], [ %187, %._crit_edge ]
  %51 = phi i32 [ 0, %cond-add-join ], [ %188, %._crit_edge ]
  %52 = phi i32 [ 0, %cond-add-join ], [ %189, %._crit_edge ]
  %53 = phi i32 [ 0, %cond-add-join ], [ %190, %._crit_edge ]
  %54 = phi i32 [ 0, %cond-add-join ], [ %156, %._crit_edge ]
  %55 = phi i32 [ 0, %cond-add-join ], [ %157, %._crit_edge ]
  %56 = phi i32 [ 0, %cond-add-join ], [ %158, %._crit_edge ]
  %57 = phi i32 [ 0, %cond-add-join ], [ %159, %._crit_edge ]
  %58 = phi i32 [ 0, %cond-add-join ], [ %160, %._crit_edge ]
  %59 = phi i32 [ 0, %cond-add-join ], [ %161, %._crit_edge ]
  %60 = phi i32 [ 0, %cond-add-join ], [ %162, %._crit_edge ]
  %61 = phi i32 [ 0, %cond-add-join ], [ %163, %._crit_edge ]
  %62 = phi i32 [ 0, %cond-add-join ], [ %192, %._crit_edge ]
  %63 = phi i32 [ 0, %cond-add-join ], [ %193, %._crit_edge ]
  %64 = phi i32 [ 0, %cond-add-join ], [ %194, %._crit_edge ]
  %65 = phi i32 [ 0, %cond-add-join ], [ %195, %._crit_edge ]
  %66 = phi i32 [ 0, %cond-add-join ], [ %196, %._crit_edge ]
  %67 = phi i32 [ 0, %cond-add-join ], [ %197, %._crit_edge ]
  %68 = phi i32 [ 0, %cond-add-join ], [ %198, %._crit_edge ]
  %69 = phi i32 [ 0, %cond-add-join ], [ %199, %._crit_edge ]
  %70 = or i32 %.pn2427, 32
  %.reass = add nuw nsw i32 %.pn2427, 0
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 0, i32 0, i32 16383, i32 8191, i32 %.reass, i32 0, i32 8, i32 32, i32 8, i32 1, i1 false, i1 false, i32 4)
  %71 = or i32 0, %70
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 0, i32 0, i32 8191, i32 4095, i32 %.reass155.reass, i32 %71, i32 8, i32 64, i32 2, i32 1, i1 false, i1 false, i32 4)
  %72 = or i32 %.pn2427, 0
  %Block2D_AddrPayload = call i32* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64 0, i32 0, i32 16383, i32 8191, i32 0, i32 0, i32 32, i32 32, i32 1)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload, i32 5, i32 %72, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload, i32 6, i32 %2, i1 false)
  %Block2D_ReadAddrPayload = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i32(i32* %Block2D_AddrPayload, i32 0, i32 0, i32 8, i32 32, i32 32, i32 1, i1 false, i1 false, i32 0)
  %73 = shufflevector <32 x i16> %Block2D_ReadAddrPayload, <32 x i16> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %74 = shufflevector <32 x i16> %Block2D_ReadAddrPayload, <32 x i16> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %75 = shufflevector <32 x i16> %Block2D_ReadAddrPayload, <32 x i16> undef, <8 x i32> <i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23>
  %76 = shufflevector <32 x i16> %Block2D_ReadAddrPayload, <32 x i16> undef, <8 x i32> <i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31>
  %Block2D_AddrPayload205 = call i32* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64 0, i32 0, i32 8191, i32 4095, i32 0, i32 0, i32 16, i32 32, i32 2)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload205, i32 5, i32 %5, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload205, i32 6, i32 %.pn2427, i1 false)
  %Block2D_ReadAddrPayload206 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0i32(i32* %Block2D_AddrPayload205, i32 0, i32 0, i32 8, i32 16, i32 32, i32 2, i1 false, i1 true, i32 0)
  %77 = shufflevector <16 x i32> %Block2D_ReadAddrPayload206, <16 x i32> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %78 = shufflevector <16 x i32> %Block2D_ReadAddrPayload206, <16 x i32> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %79 = insertelement <8 x i32> undef, i32 %6, i64 0
  %80 = insertelement <8 x i32> %79, i32 %7, i64 1
  %81 = insertelement <8 x i32> %80, i32 %8, i64 2
  %82 = insertelement <8 x i32> %81, i32 %9, i64 3
  %83 = insertelement <8 x i32> %82, i32 %10, i64 4
  %84 = insertelement <8 x i32> %83, i32 %11, i64 5
  %85 = insertelement <8 x i32> %84, i32 %12, i64 6
  %86 = insertelement <8 x i32> %85, i32 %13, i64 7
  %87 = insertelement <8 x i32> undef, i32 %14, i64 0
  %88 = insertelement <8 x i32> %87, i32 %15, i64 1
  %89 = insertelement <8 x i32> %88, i32 %16, i64 2
  %90 = insertelement <8 x i32> %89, i32 %17, i64 3
  %91 = insertelement <8 x i32> %90, i32 %18, i64 4
  %92 = insertelement <8 x i32> %91, i32 %19, i64 5
  %93 = insertelement <8 x i32> %92, i32 %20, i64 6
  %94 = insertelement <8 x i32> %93, i32 %21, i64 7
  %95 = insertelement <8 x i32> undef, i32 %22, i64 0
  %96 = insertelement <8 x i32> %95, i32 %23, i64 1
  %97 = insertelement <8 x i32> %96, i32 %24, i64 2
  %98 = insertelement <8 x i32> %97, i32 %25, i64 3
  %99 = insertelement <8 x i32> %98, i32 %26, i64 4
  %100 = insertelement <8 x i32> %99, i32 %27, i64 5
  %101 = insertelement <8 x i32> %100, i32 %28, i64 6
  %102 = insertelement <8 x i32> %101, i32 %29, i64 7
  %103 = insertelement <8 x i32> undef, i32 %30, i64 0
  %104 = insertelement <8 x i32> %103, i32 %31, i64 1
  %105 = insertelement <8 x i32> %104, i32 %32, i64 2
  %106 = insertelement <8 x i32> %105, i32 %33, i64 3
  %107 = insertelement <8 x i32> %106, i32 %34, i64 4
  %108 = insertelement <8 x i32> %107, i32 %35, i64 5
  %109 = insertelement <8 x i32> %108, i32 %36, i64 6
  %110 = insertelement <8 x i32> %109, i32 %37, i64 7
  %111 = insertelement <8 x i32> undef, i32 %38, i64 0
  %112 = insertelement <8 x i32> %111, i32 %39, i64 1
  %113 = insertelement <8 x i32> %112, i32 %40, i64 2
  %114 = insertelement <8 x i32> %113, i32 %41, i64 3
  %115 = insertelement <8 x i32> %114, i32 %42, i64 4
  %116 = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> %110, <8 x i16> %74, <8 x i32> %78, i32 4, i32 4, i32 8, i32 8, i1 false)
  %117 = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> zeroinitializer, <8 x i16> %75, <8 x i32> %78, i32 4, i32 4, i32 8, i32 8, i1 false)
  %118 = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> zeroinitializer, <8 x i16> %76, <8 x i32> %78, i32 4, i32 4, i32 8, i32 8, i1 false)
  %119 = add nuw nsw i32 %.pn2427, 64
  %.reass.1 = add nuw nsw i32 %70, 0
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 0, i32 0, i32 16383, i32 8191, i32 %.reass.1, i32 0, i32 8, i32 32, i32 8, i32 1, i1 false, i1 false, i32 4)
  %120 = or i32 0, %119
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64 0, i32 0, i32 8191, i32 4095, i32 %.reass155.reass, i32 %120, i32 8, i32 64, i32 2, i32 1, i1 false, i1 false, i32 4)
  %121 = add nuw nsw i32 %70, 0
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload, i32 5, i32 %121, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload, i32 6, i32 %2, i1 false)
  %Block2D_ReadAddrPayload208 = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i32(i32* %Block2D_AddrPayload, i32 0, i32 0, i32 8, i32 32, i32 32, i32 1, i1 false, i1 false, i32 0)
  %122 = shufflevector <32 x i16> %Block2D_ReadAddrPayload208, <32 x i16> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %123 = shufflevector <32 x i16> %Block2D_ReadAddrPayload208, <32 x i16> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %124 = shufflevector <32 x i16> %Block2D_ReadAddrPayload208, <32 x i16> undef, <8 x i32> <i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23>
  %125 = shufflevector <32 x i16> %Block2D_ReadAddrPayload208, <32 x i16> undef, <8 x i32> <i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31>
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload205, i32 5, i32 %5, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32* %Block2D_AddrPayload205, i32 6, i32 %70, i1 false)
  %Block2D_ReadAddrPayload210 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0i32(i32* %Block2D_AddrPayload205, i32 0, i32 0, i32 8, i32 16, i32 32, i32 2, i1 false, i1 true, i32 0)
  %126 = shufflevector <16 x i32> %Block2D_ReadAddrPayload210, <16 x i32> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %127 = shufflevector <16 x i32> %Block2D_ReadAddrPayload210, <16 x i32> undef, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %128 = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> zeroinitializer, <8 x i16> %122, <8 x i32> %126, i32 4, i32 4, i32 8, i32 8, i1 false)
  %129 = extractelement <8 x i32> %128, i64 0
  %130 = extractelement <8 x i32> %128, i64 1
  %131 = extractelement <8 x i32> %128, i64 2
  %132 = extractelement <8 x i32> %128, i64 3
  %133 = extractelement <8 x i32> %128, i64 4
  %134 = extractelement <8 x i32> %128, i64 5
  %135 = extractelement <8 x i32> %128, i64 6
  %136 = extractelement <8 x i32> %128, i64 7
  %137 = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> zeroinitializer, <8 x i16> %123, <8 x i32> %126, i32 4, i32 4, i32 8, i32 8, i1 false)
  %138 = extractelement <8 x i32> %137, i64 0
  %139 = extractelement <8 x i32> %137, i64 1
  %140 = extractelement <8 x i32> %137, i64 2
  %141 = extractelement <8 x i32> %137, i64 3
  %142 = extractelement <8 x i32> %137, i64 4
  %143 = extractelement <8 x i32> %137, i64 5
  %144 = extractelement <8 x i32> %137, i64 6
  %145 = extractelement <8 x i32> %137, i64 7
  %146 = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> zeroinitializer, <8 x i16> %124, <8 x i32> %126, i32 4, i32 4, i32 8, i32 8, i1 false)
  %147 = extractelement <8 x i32> %146, i64 0
  %148 = extractelement <8 x i32> %146, i64 1
  %149 = extractelement <8 x i32> %146, i64 2
  %150 = extractelement <8 x i32> %146, i64 3
  %151 = extractelement <8 x i32> %146, i64 4
  %152 = extractelement <8 x i32> %146, i64 5
  %153 = extractelement <8 x i32> %146, i64 6
  %154 = extractelement <8 x i32> %146, i64 7
  %155 = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> zeroinitializer, <8 x i16> %125, <8 x i32> %126, i32 4, i32 4, i32 8, i32 8, i1 false)
  %156 = extractelement <8 x i32> %155, i64 0
  %157 = extractelement <8 x i32> %155, i64 1
  %158 = extractelement <8 x i32> %155, i64 2
  %159 = extractelement <8 x i32> %155, i64 3
  %160 = extractelement <8 x i32> %155, i64 4
  %161 = extractelement <8 x i32> %155, i64 5
  %162 = extractelement <8 x i32> %155, i64 6
  %163 = extractelement <8 x i32> %155, i64 7
  %164 = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> zeroinitializer, <8 x i16> %122, <8 x i32> %127, i32 4, i32 4, i32 8, i32 8, i1 false)
  %165 = extractelement <8 x i32> %164, i64 0
  %166 = extractelement <8 x i32> %164, i64 1
  %167 = extractelement <8 x i32> %164, i64 2
  %168 = extractelement <8 x i32> %164, i64 3
  %169 = extractelement <8 x i32> %164, i64 4
  %170 = extractelement <8 x i32> %164, i64 5
  %171 = extractelement <8 x i32> %164, i64 6
  %172 = extractelement <8 x i32> %164, i64 7
  %173 = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> %116, <8 x i16> %123, <8 x i32> %127, i32 4, i32 4, i32 8, i32 8, i1 false)
  %174 = extractelement <8 x i32> %173, i64 0
  %175 = extractelement <8 x i32> %173, i64 1
  %176 = extractelement <8 x i32> %173, i64 2
  %177 = extractelement <8 x i32> %173, i64 3
  %178 = extractelement <8 x i32> %173, i64 4
  %179 = extractelement <8 x i32> %173, i64 5
  %180 = extractelement <8 x i32> %173, i64 6
  %181 = extractelement <8 x i32> %173, i64 7
  %182 = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> %117, <8 x i16> %124, <8 x i32> %127, i32 4, i32 4, i32 8, i32 8, i1 false)
  %183 = extractelement <8 x i32> %182, i64 0
  %184 = extractelement <8 x i32> %182, i64 1
  %185 = extractelement <8 x i32> %182, i64 2
  %186 = extractelement <8 x i32> %182, i64 3
  %187 = extractelement <8 x i32> %182, i64 4
  %188 = extractelement <8 x i32> %182, i64 5
  %189 = extractelement <8 x i32> %182, i64 6
  %190 = extractelement <8 x i32> %182, i64 7
  %191 = call <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32> %118, <8 x i16> %125, <8 x i32> %127, i32 4, i32 4, i32 8, i32 8, i1 false)
  %192 = extractelement <8 x i32> %191, i64 0
  %193 = extractelement <8 x i32> %191, i64 1
  %194 = extractelement <8 x i32> %191, i64 2
  %195 = extractelement <8 x i32> %191, i64 3
  %196 = extractelement <8 x i32> %191, i64 4
  %197 = extractelement <8 x i32> %191, i64 5
  %198 = extractelement <8 x i32> %191, i64 6
  %199 = extractelement <8 x i32> %191, i64 7
  %200 = icmp ult i32 %70, 8160
  br i1 %200, label %._crit_edge, label %201

201:                                              ; preds = %._crit_edge
  %202 = and i64 0, -64
  %203 = trunc i64 0 to i32
  %204 = and i32 %203, 63
  %205 = lshr i32 %204, 2
  %206 = or i32 %205, %4
  %207 = or i32 %206, 0
  %208 = add nuw nsw i32 %204, 16383
  %209 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %202, i32 %208, i32 16383, i32 16383, i32 %207, i32 %2, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %210 = extractelement <8 x i32> %209, i64 0
  %211 = extractelement <8 x i32> %209, i64 1
  %212 = extractelement <8 x i32> %209, i64 2
  %213 = extractelement <8 x i32> %209, i64 3
  %214 = extractelement <8 x i32> %209, i64 4
  %215 = extractelement <8 x i32> %209, i64 5
  %216 = extractelement <8 x i32> %209, i64 6
  %217 = extractelement <8 x i32> %209, i64 7
  %218 = or i32 %207, 16
  %219 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %202, i32 %208, i32 16383, i32 16383, i32 %218, i32 %2, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %220 = extractelement <8 x i32> %219, i64 0
  %221 = extractelement <8 x i32> %219, i64 1
  %222 = extractelement <8 x i32> %219, i64 2
  %223 = extractelement <8 x i32> %219, i64 3
  %224 = extractelement <8 x i32> %219, i64 4
  %225 = extractelement <8 x i32> %219, i64 5
  %226 = extractelement <8 x i32> %219, i64 6
  %227 = extractelement <8 x i32> %219, i64 7
  %228 = or i32 %2, 8
  %229 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %202, i32 %208, i32 16383, i32 16383, i32 %207, i32 %228, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %230 = extractelement <8 x i32> %229, i64 0
  %231 = extractelement <8 x i32> %229, i64 1
  %232 = extractelement <8 x i32> %229, i64 2
  %233 = extractelement <8 x i32> %229, i64 3
  %234 = extractelement <8 x i32> %229, i64 4
  %235 = extractelement <8 x i32> %229, i64 5
  %236 = extractelement <8 x i32> %229, i64 6
  %237 = extractelement <8 x i32> %229, i64 7
  %238 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %202, i32 %208, i32 16383, i32 16383, i32 %218, i32 %228, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %239 = extractelement <8 x i32> %238, i64 0
  %240 = extractelement <8 x i32> %238, i64 1
  %241 = extractelement <8 x i32> %238, i64 2
  %242 = extractelement <8 x i32> %238, i64 3
  %243 = extractelement <8 x i32> %238, i64 4
  %244 = extractelement <8 x i32> %238, i64 5
  %245 = extractelement <8 x i32> %238, i64 6
  %246 = extractelement <8 x i32> %238, i64 7
  %247 = or i32 %2, 16
  %248 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %202, i32 %208, i32 16383, i32 16383, i32 %207, i32 %247, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %249 = extractelement <8 x i32> %248, i64 0
  %250 = extractelement <8 x i32> %248, i64 1
  %251 = extractelement <8 x i32> %248, i64 2
  %252 = extractelement <8 x i32> %248, i64 3
  %253 = extractelement <8 x i32> %248, i64 4
  %254 = extractelement <8 x i32> %248, i64 5
  %255 = extractelement <8 x i32> %248, i64 6
  %256 = extractelement <8 x i32> %248, i64 7
  %257 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %202, i32 %208, i32 16383, i32 16383, i32 %218, i32 %247, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %258 = extractelement <8 x i32> %257, i64 0
  %259 = extractelement <8 x i32> %257, i64 1
  %260 = extractelement <8 x i32> %257, i64 2
  %261 = extractelement <8 x i32> %257, i64 3
  %262 = extractelement <8 x i32> %257, i64 4
  %263 = extractelement <8 x i32> %257, i64 5
  %264 = extractelement <8 x i32> %257, i64 6
  %265 = extractelement <8 x i32> %257, i64 7
  %266 = or i32 %2, 24
  %267 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %202, i32 %208, i32 16383, i32 16383, i32 %207, i32 %266, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %268 = extractelement <8 x i32> %267, i64 0
  %269 = extractelement <8 x i32> %267, i64 1
  %270 = extractelement <8 x i32> %267, i64 2
  %271 = extractelement <8 x i32> %267, i64 3
  %272 = extractelement <8 x i32> %267, i64 4
  %273 = extractelement <8 x i32> %267, i64 5
  %274 = extractelement <8 x i32> %267, i64 6
  %275 = extractelement <8 x i32> %267, i64 7
  %276 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %202, i32 %208, i32 16383, i32 16383, i32 %218, i32 %266, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %277 = extractelement <8 x i32> %276, i64 0
  %278 = extractelement <8 x i32> %276, i64 1
  %279 = extractelement <8 x i32> %276, i64 2
  %280 = extractelement <8 x i32> %276, i64 3
  %281 = extractelement <8 x i32> %276, i64 4
  %282 = extractelement <8 x i32> %276, i64 5
  %283 = extractelement <8 x i32> %276, i64 6
  %284 = extractelement <8 x i32> %276, i64 7
  %285 = add i32 %210, %129
  %286 = add i32 %211, %130
  %287 = add i32 %212, %131
  %288 = add i32 %213, %132
  %289 = add i32 %214, %133
  %290 = add i32 %215, %134
  %291 = add i32 %216, %135
  %292 = add i32 %217, %136
  %293 = add i32 %220, %165
  %294 = add i32 %221, %166
  %295 = add i32 %222, %167
  %296 = add i32 %223, %168
  %297 = add i32 %224, %169
  %298 = add i32 %225, %170
  %299 = add i32 %226, %171
  %300 = add i32 %227, %172
  %301 = add i32 %230, %138
  %302 = add i32 %231, %139
  %303 = add i32 %232, %140
  %304 = add i32 %233, %141
  %305 = add i32 %234, %142
  %306 = add i32 %235, %143
  %307 = add i32 %236, %144
  %308 = add i32 %237, %145
  %309 = add i32 %239, %174
  %310 = add i32 %240, %175
  %311 = add i32 %241, %176
  %312 = add i32 %242, %177
  %313 = add i32 %243, %178
  %314 = add i32 %244, %179
  %315 = add i32 %245, %180
  %316 = add i32 %246, %181
  %317 = add i32 %249, %147
  %318 = add i32 %250, %148
  %319 = add i32 %251, %149
  %320 = add i32 %252, %150
  %321 = add i32 %253, %151
  %322 = add i32 %254, %152
  %323 = add i32 %255, %153
  %324 = add i32 %256, %154
  %325 = add i32 %258, %183
  %326 = add i32 %259, %184
  %327 = add i32 %260, %185
  %328 = add i32 %261, %186
  %329 = add i32 %262, %187
  %330 = add i32 %263, %188
  %331 = add i32 %264, %189
  %332 = add i32 %265, %190
  %333 = add i32 %268, %156
  %334 = add i32 %269, %157
  %335 = add i32 %270, %158
  %336 = add i32 %271, %159
  %337 = add i32 %272, %160
  %338 = add i32 %273, %161
  %339 = add i32 %274, %162
  %340 = add i32 %275, %163
  %341 = add i32 %277, %192
  %342 = add i32 %278, %193
  %343 = add i32 %279, %194
  %344 = add i32 %280, %195
  %345 = add i32 %281, %196
  %346 = add i32 %282, %197
  %347 = add i32 %283, %198
  %348 = add i32 %284, %199
  %349 = insertelement <8 x i32> undef, i32 %285, i64 0
  %350 = insertelement <8 x i32> %349, i32 %286, i64 1
  %351 = insertelement <8 x i32> %350, i32 %287, i64 2
  %352 = insertelement <8 x i32> %351, i32 %288, i64 3
  %353 = insertelement <8 x i32> %352, i32 %289, i64 4
  %354 = insertelement <8 x i32> %353, i32 %290, i64 5
  %355 = insertelement <8 x i32> %354, i32 %291, i64 6
  %356 = insertelement <8 x i32> %355, i32 %292, i64 7
  %357 = and i64 0, -64
  %358 = trunc i64 0 to i32
  %359 = and i32 %358, 63
  %360 = lshr i32 %359, 2
  %361 = or i32 %360, %4
  %362 = or i32 %361, 0
  %363 = add nuw nsw i32 %359, 16383
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 %357, i32 %363, i32 16383, i32 16383, i32 %362, i32 %2, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %356)
  %364 = insertelement <8 x i32> undef, i32 %293, i64 0
  %365 = insertelement <8 x i32> %364, i32 %294, i64 1
  %366 = insertelement <8 x i32> %365, i32 %295, i64 2
  %367 = insertelement <8 x i32> %366, i32 %296, i64 3
  %368 = insertelement <8 x i32> %367, i32 %297, i64 4
  %369 = insertelement <8 x i32> %368, i32 %298, i64 5
  %370 = insertelement <8 x i32> %369, i32 %299, i64 6
  %371 = insertelement <8 x i32> %370, i32 %300, i64 7
  %372 = or i32 %362, 16
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 %357, i32 %363, i32 16383, i32 16383, i32 %372, i32 %2, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %371)
  %373 = insertelement <8 x i32> undef, i32 %301, i64 0
  %374 = insertelement <8 x i32> %373, i32 %302, i64 1
  %375 = insertelement <8 x i32> %374, i32 %303, i64 2
  %376 = insertelement <8 x i32> %375, i32 %304, i64 3
  %377 = insertelement <8 x i32> %376, i32 %305, i64 4
  %378 = insertelement <8 x i32> %377, i32 %306, i64 5
  %379 = insertelement <8 x i32> %378, i32 %307, i64 6
  %380 = insertelement <8 x i32> %379, i32 %308, i64 7
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 %357, i32 %363, i32 16383, i32 16383, i32 %362, i32 %228, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %380)
  %381 = insertelement <8 x i32> undef, i32 %309, i64 0
  %382 = insertelement <8 x i32> %381, i32 %310, i64 1
  %383 = insertelement <8 x i32> %382, i32 %311, i64 2
  %384 = insertelement <8 x i32> %383, i32 %312, i64 3
  %385 = insertelement <8 x i32> %384, i32 %313, i64 4
  %386 = insertelement <8 x i32> %385, i32 %314, i64 5
  %387 = insertelement <8 x i32> %386, i32 %315, i64 6
  %388 = insertelement <8 x i32> %387, i32 %316, i64 7
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 %357, i32 %363, i32 16383, i32 16383, i32 %372, i32 %228, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %388)
  %389 = insertelement <8 x i32> undef, i32 %317, i64 0
  %390 = insertelement <8 x i32> %389, i32 %318, i64 1
  %391 = insertelement <8 x i32> %390, i32 %319, i64 2
  %392 = insertelement <8 x i32> %391, i32 %320, i64 3
  %393 = insertelement <8 x i32> %392, i32 %321, i64 4
  %394 = insertelement <8 x i32> %393, i32 %322, i64 5
  %395 = insertelement <8 x i32> %394, i32 %323, i64 6
  %396 = insertelement <8 x i32> %395, i32 %324, i64 7
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 %357, i32 %363, i32 16383, i32 16383, i32 %362, i32 %247, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %396)
  %397 = insertelement <8 x i32> undef, i32 %325, i64 0
  %398 = insertelement <8 x i32> %397, i32 %326, i64 1
  %399 = insertelement <8 x i32> %398, i32 %327, i64 2
  %400 = insertelement <8 x i32> %399, i32 %328, i64 3
  %401 = insertelement <8 x i32> %400, i32 %329, i64 4
  %402 = insertelement <8 x i32> %401, i32 %330, i64 5
  %403 = insertelement <8 x i32> %402, i32 %331, i64 6
  %404 = insertelement <8 x i32> %403, i32 %332, i64 7
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 %357, i32 %363, i32 16383, i32 16383, i32 %372, i32 %247, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %404)
  %405 = insertelement <8 x i32> undef, i32 %333, i64 0
  %406 = insertelement <8 x i32> %405, i32 %334, i64 1
  %407 = insertelement <8 x i32> %406, i32 %335, i64 2
  %408 = insertelement <8 x i32> %407, i32 %336, i64 3
  %409 = insertelement <8 x i32> %408, i32 %337, i64 4
  %410 = insertelement <8 x i32> %409, i32 %338, i64 5
  %411 = insertelement <8 x i32> %410, i32 %339, i64 6
  %412 = insertelement <8 x i32> %411, i32 %340, i64 7
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 %357, i32 %363, i32 16383, i32 16383, i32 %362, i32 %266, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %412)
  %413 = insertelement <8 x i32> undef, i32 %341, i64 0
  %414 = insertelement <8 x i32> %413, i32 %342, i64 1
  %415 = insertelement <8 x i32> %414, i32 %343, i64 2
  %416 = insertelement <8 x i32> %415, i32 %344, i64 3
  %417 = insertelement <8 x i32> %416, i32 %345, i64 4
  %418 = insertelement <8 x i32> %417, i32 %346, i64 5
  %419 = insertelement <8 x i32> %418, i32 %347, i64 6
  %420 = insertelement <8 x i32> %419, i32 %348, i64 7
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 %357, i32 %363, i32 16383, i32 16383, i32 %372, i32 %266, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %420)
  ret void
}

; Function Attrs: convergent nounwind readnone willreturn
declare <8 x i32> @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32(<8 x i32>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #1

; Function Attrs: nounwind
declare void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32) #2

; Function Attrs: nounwind
declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32) #2

; Function Attrs: nounwind
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>) #2

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare i32 @llvm.smin.i32(i32, i32) #3

; Function Attrs: nounwind readnone speculatable willreturn
declare i32* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64, i32, i32, i32, i32, i32, i32, i32, i32) #4

; Function Attrs: argmemonly nounwind speculatable willreturn writeonly
declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32*, i32, i32, i1) #5

; Function Attrs: nounwind willreturn
declare <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i32(i32*, i32, i32, i32, i32, i32, i32, i1, i1, i32) #6

; Function Attrs: nounwind willreturn
declare <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0i32(i32*, i32, i32, i32, i32, i32, i32, i1, i1, i32) #6

; uselistorder directives
uselistorder i32 0, { 22, 23, 24, 25, 26, 27, 28, 29, 20, 30, 31, 32, 33, 34, 35, 36, 37, 19, 38, 39, 40, 41, 42, 43, 13, 4, 8, 15, 11, 1, 44, 45, 46, 47, 48, 3, 49, 50, 51, 52, 53, 10, 12, 2, 7, 14, 9, 0, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 6, 17, 21, 16, 5, 18 }
uselistorder i64 0, { 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 2, 5, 34, 35, 36, 37, 38, 1, 4, 0, 3 }
uselistorder <8 x i32> (<8 x i32>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)* @llvm.genx.GenISA.sub.group.dpas.v8i32.v8i32.v8i16.v8i32, { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 }
uselistorder void (i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)* @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid, { 3, 2, 1, 0 }
uselistorder <8 x i32> (i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)* @llvm.genx.GenISA.LSC2DBlockRead.v8i32, { 7, 6, 5, 4, 3, 2, 1, 0 }
uselistorder void (i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)* @llvm.genx.GenISA.LSC2DBlockWrite.v8i32, { 7, 6, 5, 4, 3, 2, 1, 0 }
uselistorder i32* (i64, i32, i32, i32, i32, i32, i32, i32, i32)* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32, { 1, 0 }
uselistorder void (i32*, i32, i32, i1)* @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32, { 7, 6, 5, 4, 3, 2, 1, 0 }
uselistorder <32 x i16> (i32*, i32, i32, i32, i32, i32, i32, i1, i1, i32)* @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i32, { 1, 0 }
uselistorder <16 x i32> (i32*, i32, i32, i32, i32, i32, i32, i1, i1, i32)* @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0i32, { 1, 0 }

attributes #0 = { convergent nounwind "less-precise-fpmad"="true" }
attributes #1 = { convergent nounwind readnone willreturn }
attributes #2 = { nounwind }
attributes #3 = { nocallback nofree nosync nounwind readnone speculatable willreturn }
attributes #4 = { nounwind readnone speculatable willreturn }
attributes #5 = { argmemonly nounwind speculatable willreturn writeonly }
attributes #6 = { nounwind willreturn }

!igc.functions = !{!0}

!0 = distinct !{void (i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, <8 x i32>, <3 x i32>, i8*, i32, i32, i32, i32, i32)* bitcast (void ()* @matmul_kernel_with_block_pointers to void (i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, i8 addrspace(1)*, <8 x i32>, <3 x i32>, i8*, i32, i32, i32, i32, i32)*), !1}
!1 = distinct !{!2}
!2 = distinct !{!"sub_group_size", i32 16}
