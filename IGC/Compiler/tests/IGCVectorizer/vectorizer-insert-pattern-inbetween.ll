;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt -S  --igc-vectorizer -dce --regkey=VectorizerAllowSelect=1 --regkey=VectorizerAllowCMP=1 --regkey=VectorizerAllowMAXNUM=1 --regkey=VectorizerAllowWAVEALL=1 --regkey=VectorizerDepWindowMultiplier=4 < %s 2>&1 | FileCheck %s

; CHECK: [[fmul0:%.*]] = fmul float {{.*}}, 1.250000e-01
; CHECK: [[fmul1:%.*]] = fmul float {{.*}}, 1.250000e-01
; CHECK: [[fmul2:%.*]] = fmul float {{.*}}, 1.250000e-01

; CHECK:   [[insrt_0:%.*]] = insertelement <8 x float> undef
; CHECK:   [[insrt_1:%.*]] = insertelement <8 x float> [[insrt_0]], float [[fmul0]], i32 1
; CHECK:   [[insrt_2:%.*]] = insertelement <8 x float> [[insrt_1]]
; CHECK:   [[insrt_3:%.*]] = insertelement <8 x float> [[insrt_2]], float [[fmul1]], i32 3
; CHECK:   [[insrt_4:%.*]] = insertelement <8 x float> [[insrt_3]]
; CHECK:   [[insrt_5:%.*]] = insertelement <8 x float> [[insrt_4]]
; CHECK:   [[insrt_6:%.*]] = insertelement <8 x float> [[insrt_5]], float [[fmul2]], i32 6
; CHECK:   [[insrt_7:%.*]] = insertelement <8 x float> [[insrt_6]]
; CHECK:   %vectorized_binary = fmul <8 x float> [[insrt_7]], <float 1.250000e-01, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000>
; CHECK:   [[extrct_0:%.*]] = extractelement <8 x float> %vectorized_binary, i32 0
; CHECK:   [[extrct_1:%.*]] = extractelement <8 x float> %vectorized_binary, i32 1
; CHECK:   [[extrct_2:%.*]] = extractelement <8 x float> %vectorized_binary, i32 2
; CHECK:   [[extrct_3:%.*]] = extractelement <8 x float> %vectorized_binary, i32 3
; CHECK:   [[extrct_4:%.*]] = extractelement <8 x float> %vectorized_binary, i32 4
; CHECK:   [[extrct_5:%.*]] = extractelement <8 x float> %vectorized_binary, i32 5
; CHECK:   [[extrct_6:%.*]] = extractelement <8 x float> %vectorized_binary, i32 6
; CHECK:   [[extrct_7:%.*]] = extractelement <8 x float> %vectorized_binary, i32 7
; CHECK:   %19 = icmp slt i32 %2, 1
; CHECK:   %20 = icmp slt i32 1, %.pn1482
; CHECK:   %21 = icmp slt i32 %3, %.pn1482
; CHECK:   %22 = icmp slt i32 %4, 1
; CHECK:   %23 = icmp slt i32 %5, 1
; CHECK:   %24 = icmp slt i32 %6, %.pn1482
; CHECK:   %25 = select i1 %19, float 0xFFF0000000000000, float [[extrct_0]]
; CHECK:   %26 = select i1 %20, float 0xFFF0000000000000, float [[extrct_1]]
; CHECK:   %27 = select i1 %21, float 0xFFF0000000000000, float [[extrct_2]]
; CHECK:   %28 = select i1 %20, float 0xFFF0000000000000, float [[extrct_3]]
; CHECK:   %29 = select i1 %22, float 0xFFF0000000000000, float [[extrct_4]]
; CHECK:   %30 = select i1 %23, float 0xFFF0000000000000, float [[extrct_5]]
; CHECK:   %31 = select i1 %24, float 0xFFF0000000000000, float [[extrct_6]]
; CHECK:   %32 = select i1 %20, float 0xFFF0000000000000, float [[extrct_7]]


; ModuleID = 'reduced.ll'
source_filename = "initial.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @quux(i16 %localIdX) #0 {
cond-add-join548:
  %0 = zext i16 %localIdX to i32
  %1 = and i32 %0, 15
  %2 = and i32 %0, 24
  %3 = or i32 %2, 2
  %4 = or i32 %2, 4
  %5 = or i32 %2, 5
  %6 = or i32 %2, 6
  br label %._crit_edge333

._crit_edge333:                                   ; preds = %._crit_edge333, %cond-add-join548
  %.pn1482 = phi i32 [ %1, %cond-add-join548 ], [ 0, %._crit_edge333 ]
  %7 = phi float [ 0.000000e+00, %cond-add-join548 ], [ %57, %._crit_edge333 ]
  %8 = phi float [ 0.000000e+00, %cond-add-join548 ], [ %58, %._crit_edge333 ]
  %9 = phi float [ 0.000000e+00, %cond-add-join548 ], [ %59, %._crit_edge333 ]
  %10 = phi float [ 0.000000e+00, %cond-add-join548 ], [ %60, %._crit_edge333 ]
  %11 = phi float [ 0.000000e+00, %cond-add-join548 ], [ %61, %._crit_edge333 ]
  %12 = phi float [ 0.000000e+00, %cond-add-join548 ], [ %62, %._crit_edge333 ]
  %13 = phi float [ 0.000000e+00, %cond-add-join548 ], [ %63, %._crit_edge333 ]
  %14 = phi float [ 0.000000e+00, %cond-add-join548 ], [ %64, %._crit_edge333 ]
  %15 = phi float [ 0.000000e+00, %cond-add-join548 ], [ %124, %._crit_edge333 ]
  %16 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %17 = extractelement <8 x float> %16, i64 0
  %18 = extractelement <8 x float> %16, i64 1
  %19 = extractelement <8 x float> %16, i64 2
  %20 = extractelement <8 x float> %16, i64 3
  %21 = extractelement <8 x float> %16, i64 4
  %22 = extractelement <8 x float> %16, i64 6
  %23 = extractelement <8 x float> %16, i64 7
  %24 = fmul float %17, 1.250000e-01
  %25 = fmul float %18, 1.250000e-01
  %26 = fmul float %20, 1.250000e-01
  %27 = fmul float %22, 1.250000e-01
  %28 = icmp slt i32 %2, 1
  %29 = icmp slt i32 1, %.pn1482
  %30 = icmp slt i32 %3, %.pn1482
  %31 = icmp slt i32 %4, 1
  %32 = icmp slt i32 %5, 1
  %33 = icmp slt i32 %6, %.pn1482
  %34 = select i1 %28, float 0xFFF0000000000000, float %24
  %35 = fmul float %25, 0x3FF7154760000000
  %36 = select i1 %29, float 0xFFF0000000000000, float %35
  %37 = fmul float %19, 0x3FF7154760000000
  %38 = select i1 %30, float 0xFFF0000000000000, float %37
  %39 = fmul float %26, 0x3FF7154760000000
  %40 = select i1 %29, float 0xFFF0000000000000, float %39
  %41 = fmul float %21, 0x3FF7154760000000
  %42 = select i1 %31, float 0xFFF0000000000000, float %41
  %43 = fmul float %18, 0x3FF7154760000000
  %44 = select i1 %32, float 0xFFF0000000000000, float %43
  %45 = fmul float %27, 0x3FF7154760000000
  %46 = select i1 %33, float 0xFFF0000000000000, float %45
  %47 = fmul float %23, 0x3FF7154760000000
  %48 = select i1 %29, float 0xFFF0000000000000, float %47
  %49 = call float @llvm.genx.GenISA.WaveAll.f32(float %34, i8 12, i1 true, i32 0)
  %50 = call float @llvm.genx.GenISA.WaveAll.f32(float %36, i8 12, i1 true, i32 0)
  %51 = call float @llvm.genx.GenISA.WaveAll.f32(float %38, i8 12, i1 true, i32 0)
  %52 = call float @llvm.genx.GenISA.WaveAll.f32(float %40, i8 12, i1 true, i32 0)
  %53 = call float @llvm.genx.GenISA.WaveAll.f32(float %42, i8 12, i1 true, i32 0)
  %54 = call float @llvm.genx.GenISA.WaveAll.f32(float %44, i8 12, i1 true, i32 0)
  %55 = call float @llvm.genx.GenISA.WaveAll.f32(float %46, i8 12, i1 true, i32 0)
  %56 = call float @llvm.genx.GenISA.WaveAll.f32(float %48, i8 12, i1 true, i32 0)
  %57 = call float @llvm.maxnum.f32(float %7, float %49)
  %58 = call float @llvm.maxnum.f32(float %8, float %50)
  %59 = call float @llvm.maxnum.f32(float %9, float %51)
  %60 = call float @llvm.maxnum.f32(float %10, float %52)
  %61 = call float @llvm.maxnum.f32(float %11, float %53)
  %62 = call float @llvm.maxnum.f32(float %12, float %54)
  %63 = call float @llvm.maxnum.f32(float %13, float %55)
  %64 = call float @llvm.maxnum.f32(float %14, float %56)
  %65 = fcmp oeq float %57, 0.000000e+00
  %66 = fcmp oeq float %58, 0.000000e+00
  %67 = fcmp oeq float %59, 0.000000e+00
  %68 = fcmp oeq float %60, 0.000000e+00
  %69 = fcmp oeq float %61, 0.000000e+00
  %70 = fcmp oeq float %62, 0.000000e+00
  %71 = fcmp oeq float %63, 0.000000e+00
  %72 = fcmp oeq float %64, 0.000000e+00
  %73 = select i1 %65, float 0.000000e+00, float %57
  %74 = select i1 %66, float 0.000000e+00, float %58
  %75 = select i1 %67, float 0.000000e+00, float %59
  %76 = select i1 %68, float 0.000000e+00, float %60
  %77 = select i1 %69, float 0.000000e+00, float %61
  %78 = select i1 %70, float 0.000000e+00, float %62
  %79 = select i1 %71, float 0.000000e+00, float %63
  %80 = select i1 %72, float 0.000000e+00, float %64
  %81 = fsub float %34, %73
  %82 = fsub float %36, %74
  %83 = fsub float %38, %75
  %84 = fsub float %40, %76
  %85 = fsub float %42, %77
  %86 = fsub float %44, %78
  %87 = fsub float %46, %79
  %88 = fsub float %48, %80
  %89 = call float @llvm.exp2.f32(float %81)
  %90 = call float @llvm.exp2.f32(float %82)
  %91 = call float @llvm.exp2.f32(float %83)
  %92 = call float @llvm.exp2.f32(float %84)
  %93 = call float @llvm.exp2.f32(float %85)
  %94 = call float @llvm.exp2.f32(float %86)
  %95 = call float @llvm.exp2.f32(float %87)
  %96 = call float @llvm.exp2.f32(float %88)
  %97 = fmul float %15, 0.000000e+00
  %98 = fptrunc float %89 to half
  %99 = fptrunc float %90 to half
  %100 = fptrunc float %91 to half
  %101 = fptrunc float %92 to half
  %102 = fptrunc float %93 to half
  %103 = fptrunc float %94 to half
  %104 = fptrunc float %95 to half
  %105 = fptrunc float %96 to half
  %106 = insertelement <8 x float> zeroinitializer, float %97, i64 0
  %107 = insertelement <8 x float> %106, float 0.000000e+00, i64 0
  %108 = insertelement <8 x float> %107, float 0.000000e+00, i64 0
  %109 = insertelement <8 x float> %108, float 0.000000e+00, i64 0
  %110 = insertelement <8 x float> %109, float 0.000000e+00, i64 0
  %111 = insertelement <8 x float> %110, float 0.000000e+00, i64 0
  %112 = insertelement <8 x float> %111, float 0.000000e+00, i64 0
  %113 = insertelement <8 x float> %112, float 0.000000e+00, i64 0
  %114 = insertelement <8 x half> zeroinitializer, half %98, i64 0
  %115 = insertelement <8 x half> %114, half %99, i64 1
  %116 = insertelement <8 x half> %115, half %100, i64 2
  %117 = insertelement <8 x half> %116, half %101, i64 3
  %118 = insertelement <8 x half> %117, half %102, i64 4
  %119 = insertelement <8 x half> %118, half %103, i64 5
  %120 = insertelement <8 x half> %119, half %104, i64 6
  %121 = insertelement <8 x half> %120, half %105, i64 7
  %122 = bitcast <8 x half> %121 to <8 x i16>
  %123 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %113, <8 x i16> %122, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %124 = extractelement <8 x float> %123, i64 0
  br label %._crit_edge333
}

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.fma.f32(float, float, float) #1

; Function Attrs: convergent inaccessiblememonly nounwind
declare float @llvm.genx.GenISA.WaveAll.f32(float, i8, i1, i32) #2

; Function Attrs: convergent nounwind readnone willreturn
declare i8 @llvm.genx.GenISA.WaveShuffleIndex.i8(i8, i32, i32) #3

; Function Attrs: convergent nounwind readnone willreturn
declare i32 @llvm.genx.GenISA.WaveShuffleIndex.i32(i32, i32, i32) #3

; Function Attrs: convergent nounwind readnone willreturn
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #3

; Function Attrs: nounwind
declare <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32) #4

; Function Attrs: nounwind
declare void @llvm.genx.GenISA.LSC2DBlockPrefetch.isVoid(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32) #4

; Function Attrs: nounwind
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i16>) #4

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.maxnum.f32(float, float) #1

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.exp2.f32(float) #1

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare i32 @llvm.umin.i32(i32, i32) #1

; Function Attrs: nounwind readnone speculatable willreturn
declare i32* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i32(i64, i32, i32, i32, i32, i32, i32, i32, i32) #5

; Function Attrs: argmemonly nounwind speculatable willreturn writeonly
declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i32.i32(i32*, i32, i32, i1) #6

; Function Attrs: nounwind willreturn
declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0i32(i32*, i32, i32, i32, i32, i32, i32, i1, i1, i32) #7

; Function Attrs: nounwind willreturn
declare <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0i32(i32*, i32, i32, i32, i32, i32, i32, i1, i1, i32) #7

; Function Attrs: nounwind readnone willreturn
declare i32 @llvm.genx.GenISA.imulH.i32(i32, i32) #8

attributes #0 = { convergent nounwind }
attributes #1 = { nocallback nofree nosync nounwind readnone speculatable willreturn }
attributes #2 = { convergent inaccessiblememonly nounwind }
attributes #3 = { convergent nounwind readnone willreturn }
attributes #4 = { nounwind }
attributes #5 = { nounwind readnone speculatable willreturn }
attributes #6 = { argmemonly nounwind speculatable willreturn writeonly }
attributes #7 = { nounwind willreturn }
attributes #8 = { nounwind readnone willreturn }

!igc.functions = !{!0}

!0 = distinct !{void (i16)* @quux, !1}
!1 = distinct !{!2, !29}
!2 = distinct !{!"function_type", i32 0}
!29 = distinct !{!"sub_group_size", i32 16}
