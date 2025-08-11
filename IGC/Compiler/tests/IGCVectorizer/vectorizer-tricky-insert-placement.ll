;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt -S  --igc-vectorizer -dce --regkey=VectorizerAllowSelect=1 --regkey=VectorizerAllowCMP=1 --regkey=VectorizerAllowMAXNUM=1 --regkey=VectorizerAllowWAVEALL=1 < %s 2>&1 | FileCheck %s

; CHECK-LABEL: ._crit_edge333:
; CHECK: [[VEC_BIN:%.*]] = fmul <8 x float> {{.*}}, <float 1.250000e-01, float 1.250000e-01, float 1.250000e-01, float 1.250000e-01, float 1.250000e-01, float 1.250000e-01, float 1.250000e-01, float 1.250000e-01>
; CHECK: [[VEC_BIN_2:%.*]] = fmul <8 x float> [[VEC_BIN]], <float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000, float 0x3FF7154760000000>
; CHECK: [[EXT_1_0:%.*]] = extractelement <8 x float> [[VEC_BIN_2]], i32 0
; CHECK: [[EXT_1_1:%.*]] = extractelement <8 x float> [[VEC_BIN_2]], i32 1
; CHECK: [[EXT_1_2:%.*]] = extractelement <8 x float> [[VEC_BIN_2]], i32 2
; CHECK: [[EXT_1_3:%.*]] = extractelement <8 x float> [[VEC_BIN_2]], i32 3
; CHECK: [[EXT_1_4:%.*]] = extractelement <8 x float> [[VEC_BIN_2]], i32 4
; CHECK: [[EXT_1_5:%.*]] = extractelement <8 x float> [[VEC_BIN_2]], i32 5
; CHECK: [[EXT_1_6:%.*]] = extractelement <8 x float> [[VEC_BIN_2]], i32 6
; CHECK: [[EXT_1_7:%.*]] = extractelement <8 x float> [[VEC_BIN_2]], i32 7

; CHECK: [[CMP_1_0:%.*]] = icmp slt i32 {{%.*}}, [[COMP:%.*]]
; CHECK: [[CMP_1_1:%.*]] = icmp slt i32 {{%.*}}, [[COMP]]
; CHECK: [[CMP_1_2:%.*]] = icmp slt i32 {{%.*}}, [[COMP]]
; CHECK: [[CMP_1_3:%.*]] = icmp slt i32 {{%.*}}, [[COMP]]
; CHECK: [[CMP_1_4:%.*]] = icmp slt i32 {{%.*}}, [[COMP]]
; CHECK: [[CMP_1_5:%.*]] = icmp slt i32 {{%.*}}, [[COMP]]
; CHECK: [[CMP_1_6:%.*]] = icmp slt i32 {{%.*}}, [[COMP]]
; CHECK: [[CMP_1_7:%.*]] = icmp slt i32 {{%.*}}, [[COMP]]

; CHECK: [[SEL_2_0:%.*]] = select i1 [[CMP_1_0]], float 0xFFF0000000000000, float [[EXT_1_0]]
; CHECK: [[SEL_2_1:%.*]] = select i1 [[CMP_1_1]], float 0xFFF0000000000000, float [[EXT_1_1]]
; CHECK: [[SEL_2_2:%.*]] = select i1 [[CMP_1_2]], float 0xFFF0000000000000, float [[EXT_1_2]]
; CHECK: [[SEL_2_3:%.*]] = select i1 [[CMP_1_3]], float 0xFFF0000000000000, float [[EXT_1_3]]
; CHECK: [[SEL_2_4:%.*]] = select i1 [[CMP_1_4]], float 0xFFF0000000000000, float [[EXT_1_4]]
; CHECK: [[SEL_2_5:%.*]] = select i1 [[CMP_1_5]], float 0xFFF0000000000000, float [[EXT_1_5]]
; CHECK: [[SEL_2_6:%.*]] = select i1 [[CMP_1_6]], float 0xFFF0000000000000, float [[EXT_1_6]]
; CHECK: [[SEL_2_7:%.*]] = select i1 [[CMP_1_7]], float 0xFFF0000000000000, float [[EXT_1_7]]

; CHECK-DAG: [[INSRT_2_0:%.*]] = insertelement <8 x float> undef,         float [[SEL_2_0]], i32 0
; CHECK-DAG: [[INSRT_2_1:%.*]] = insertelement <8 x float> [[INSRT_2_0]], float [[SEL_2_1]], i32 1
; CHECK-DAG: [[INSRT_2_2:%.*]] = insertelement <8 x float> [[INSRT_2_1]], float [[SEL_2_2]], i32 2
; CHECK-DAG: [[INSRT_2_3:%.*]] = insertelement <8 x float> [[INSRT_2_2]], float [[SEL_2_3]], i32 3
; CHECK-DAG: [[INSRT_2_4:%.*]] = insertelement <8 x float> [[INSRT_2_3]], float [[SEL_2_4]], i32 4
; CHECK-DAG: [[INSRT_2_5:%.*]] = insertelement <8 x float> [[INSRT_2_4]], float [[SEL_2_5]], i32 5
; CHECK-DAG: [[INSRT_2_6:%.*]] = insertelement <8 x float> [[INSRT_2_5]], float [[SEL_2_6]], i32 6
; CHECK-DAG: [[INSRT_2_7:%.*]] = insertelement <8 x float> [[INSRT_2_6]], float [[SEL_2_7]], i32 7

; CHECK: [[WAVE_1_0:%.*]] = call float @llvm.genx.GenISA.WaveAll.f32(float [[SEL_2_0]], i8 12, i32 0)
; CHECK: [[WAVE_1_1:%.*]] = call float @llvm.genx.GenISA.WaveAll.f32(float [[SEL_2_1]], i8 12, i32 0)
; CHECK: [[WAVE_1_2:%.*]] = call float @llvm.genx.GenISA.WaveAll.f32(float [[SEL_2_2]], i8 12, i32 0)
; CHECK: [[WAVE_1_3:%.*]] = call float @llvm.genx.GenISA.WaveAll.f32(float [[SEL_2_3]], i8 12, i32 0)
; CHECK: [[WAVE_1_4:%.*]] = call float @llvm.genx.GenISA.WaveAll.f32(float [[SEL_2_4]], i8 12, i32 0)
; CHECK: [[WAVE_1_5:%.*]] = call float @llvm.genx.GenISA.WaveAll.f32(float [[SEL_2_5]], i8 12, i32 0)
; CHECK: [[WAVE_1_6:%.*]] = call float @llvm.genx.GenISA.WaveAll.f32(float [[SEL_2_6]], i8 12, i32 0)
; CHECK: [[WAVE_1_7:%.*]] = call float @llvm.genx.GenISA.WaveAll.f32(float [[SEL_2_7]], i8 12, i32 0)

; CHECK-DAG: [[INSRT_1_0:%.*]] = insertelement <8 x float> undef,      float [[WAVE_1_0]], i32 0
; CHECK-DAG: [[INSRT_1_1:%.*]] = insertelement <8 x float> [[INSRT_1_0]], float [[WAVE_1_1]], i32 1
; CHECK-DAG: [[INSRT_1_2:%.*]] = insertelement <8 x float> [[INSRT_1_1]], float [[WAVE_1_2]], i32 2
; CHECK-DAG: [[INSRT_1_3:%.*]] = insertelement <8 x float> [[INSRT_1_2]], float [[WAVE_1_3]], i32 3
; CHECK-DAG: [[INSRT_1_4:%.*]] = insertelement <8 x float> [[INSRT_1_3]], float [[WAVE_1_4]], i32 4
; CHECK-DAG: [[INSRT_1_5:%.*]] = insertelement <8 x float> [[INSRT_1_4]], float [[WAVE_1_5]], i32 5
; CHECK-DAG: [[INSRT_1_6:%.*]] = insertelement <8 x float> [[INSRT_1_5]], float [[WAVE_1_6]], i32 6
; CHECK-DAG: [[INSRT_1_7:%.*]] = insertelement <8 x float> [[INSRT_1_6]], float [[WAVE_1_7]], i32 7

; CHECK: call <8 x float> @llvm.maxnum.v8f32(<8 x float> %vectorized_phi, <8 x float> [[INSRT_1_7]])
; CHECK: {{%.*}} = fsub <8 x float> [[INSRT_2_7]], {{%.*}}


; ModuleID = 'reduced.ll'
source_filename = "initial.ll"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @triton_tem_fused_0(i16 %localIdX) #0 {
cond-add-join548:
  %0 = zext i16 %localIdX to i32
  %1 = and i32 %0, 1
  %2 = lshr i32 %0, 0
  %3 = and i32 %2, 24
  %4 = or i32 %3, 1
  %5 = or i32 %3, 2
  %6 = or i32 %3, 3
  %7 = or i32 %3, 4
  %8 = or i32 %3, 5
  %9 = or i32 %3, 6
  %10 = or i32 %3, 7
  br label %.lr.ph

.lr.ph:                                           ; preds = %cond-add-join548
  br label %._crit_edge333

._crit_edge333:                                   ; preds = %._crit_edge333, %.lr.ph
  %.pn1482 = phi i32 [ %1, %.lr.ph ], [ 0, %._crit_edge333 ]
  %11 = phi float [ 0.000000e+00, %.lr.ph ], [ %69, %._crit_edge333 ]
  %12 = phi float [ 0.000000e+00, %.lr.ph ], [ %70, %._crit_edge333 ]
  %13 = phi float [ 0.000000e+00, %.lr.ph ], [ %71, %._crit_edge333 ]
  %14 = phi float [ 0.000000e+00, %.lr.ph ], [ %72, %._crit_edge333 ]
  %15 = phi float [ 0.000000e+00, %.lr.ph ], [ %73, %._crit_edge333 ]
  %16 = phi float [ 0.000000e+00, %.lr.ph ], [ %74, %._crit_edge333 ]
  %17 = phi float [ 0.000000e+00, %.lr.ph ], [ %75, %._crit_edge333 ]
  %18 = phi float [ 0.000000e+00, %.lr.ph ], [ %76, %._crit_edge333 ]
  %19 = phi float [ 0.000000e+00, %.lr.ph ], [ %136, %._crit_edge333 ]
  %20 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %21 = extractelement <8 x float> %20, i64 0
  %22 = extractelement <8 x float> %20, i64 1
  %23 = extractelement <8 x float> %20, i64 2
  %24 = extractelement <8 x float> %20, i64 3
  %25 = extractelement <8 x float> %20, i64 4
  %26 = extractelement <8 x float> %20, i64 5
  %27 = extractelement <8 x float> %20, i64 6
  %28 = extractelement <8 x float> %20, i64 7
  %29 = fmul float %21, 1.250000e-01
  %30 = fmul float %22, 1.250000e-01
  %31 = fmul float %23, 1.250000e-01
  %32 = fmul float %24, 1.250000e-01
  %33 = fmul float %25, 1.250000e-01
  %34 = fmul float %26, 1.250000e-01
  %35 = fmul float %27, 1.250000e-01
  %36 = fmul float %28, 1.250000e-01
  %37 = icmp slt i32 %3, %.pn1482
  %38 = icmp slt i32 %4, %.pn1482
  %39 = icmp slt i32 %5, %.pn1482
  %40 = icmp slt i32 %6, %.pn1482
  %41 = icmp slt i32 %7, %.pn1482
  %42 = icmp slt i32 %8, %.pn1482
  %43 = icmp slt i32 %9, %.pn1482
  %44 = icmp slt i32 %10, %.pn1482
  %45 = fmul float %29, 0x3FF7154760000000
  %46 = select i1 %37, float 0xFFF0000000000000, float %45
  %47 = fmul float %30, 0x3FF7154760000000
  %48 = select i1 %38, float 0xFFF0000000000000, float %47
  %49 = fmul float %31, 0x3FF7154760000000
  %50 = select i1 %39, float 0xFFF0000000000000, float %49
  %51 = fmul float %32, 0x3FF7154760000000
  %52 = select i1 %40, float 0xFFF0000000000000, float %51
  %53 = fmul float %33, 0x3FF7154760000000
  %54 = select i1 %41, float 0xFFF0000000000000, float %53
  %55 = fmul float %34, 0x3FF7154760000000
  %56 = select i1 %42, float 0xFFF0000000000000, float %55
  %57 = fmul float %35, 0x3FF7154760000000
  %58 = select i1 %43, float 0xFFF0000000000000, float %57
  %59 = fmul float %36, 0x3FF7154760000000
  %60 = select i1 %44, float 0xFFF0000000000000, float %59
  %61 = call float @llvm.genx.GenISA.WaveAll.f32(float %46, i8 12, i32 0)
  %62 = call float @llvm.genx.GenISA.WaveAll.f32(float %48, i8 12, i32 0)
  %63 = call float @llvm.genx.GenISA.WaveAll.f32(float %50, i8 12, i32 0)
  %64 = call float @llvm.genx.GenISA.WaveAll.f32(float %52, i8 12, i32 0)
  %65 = call float @llvm.genx.GenISA.WaveAll.f32(float %54, i8 12, i32 0)
  %66 = call float @llvm.genx.GenISA.WaveAll.f32(float %56, i8 12, i32 0)
  %67 = call float @llvm.genx.GenISA.WaveAll.f32(float %58, i8 12, i32 0)
  %68 = call float @llvm.genx.GenISA.WaveAll.f32(float %60, i8 12, i32 0)
  %69 = call float @llvm.maxnum.f32(float %11, float %61)
  %70 = call float @llvm.maxnum.f32(float %12, float %62)
  %71 = call float @llvm.maxnum.f32(float %13, float %63)
  %72 = call float @llvm.maxnum.f32(float %14, float %64)
  %73 = call float @llvm.maxnum.f32(float %15, float %65)
  %74 = call float @llvm.maxnum.f32(float %16, float %66)
  %75 = call float @llvm.maxnum.f32(float %17, float %67)
  %76 = call float @llvm.maxnum.f32(float %18, float %68)
  %77 = fcmp oeq float %69, 0.000000e+00
  %78 = fcmp oeq float %70, 0.000000e+00
  %79 = fcmp oeq float %71, 0.000000e+00
  %80 = fcmp oeq float %72, 0.000000e+00
  %81 = fcmp oeq float %73, 0.000000e+00
  %82 = fcmp oeq float %74, 0.000000e+00
  %83 = fcmp oeq float %75, 0.000000e+00
  %84 = fcmp oeq float %76, 0.000000e+00
  %85 = select i1 %77, float 0.000000e+00, float %69
  %86 = select i1 %78, float 0.000000e+00, float %70
  %87 = select i1 %79, float 0.000000e+00, float %71
  %88 = select i1 %80, float 0.000000e+00, float %72
  %89 = select i1 %81, float 0.000000e+00, float %73
  %90 = select i1 %82, float 0.000000e+00, float %74
  %91 = select i1 %83, float 0.000000e+00, float %75
  %92 = select i1 %84, float 0.000000e+00, float %76
  %93 = fsub float %46, %85
  %94 = fsub float %48, %86
  %95 = fsub float %50, %87
  %96 = fsub float %52, %88
  %97 = fsub float %54, %89
  %98 = fsub float %56, %90
  %99 = fsub float %58, %91
  %100 = fsub float %60, %92
  %101 = call float @llvm.exp2.f32(float %93)
  %102 = call float @llvm.exp2.f32(float %94)
  %103 = call float @llvm.exp2.f32(float %95)
  %104 = call float @llvm.exp2.f32(float %96)
  %105 = call float @llvm.exp2.f32(float %97)
  %106 = call float @llvm.exp2.f32(float %98)
  %107 = call float @llvm.exp2.f32(float %99)
  %108 = call float @llvm.exp2.f32(float %100)
  %109 = fmul float %19, 0.000000e+00
  %110 = fptrunc float %101 to half
  %111 = fptrunc float %102 to half
  %112 = fptrunc float %103 to half
  %113 = fptrunc float %104 to half
  %114 = fptrunc float %105 to half
  %115 = fptrunc float %106 to half
  %116 = fptrunc float %107 to half
  %117 = fptrunc float %108 to half
  %118 = insertelement <8 x float> zeroinitializer, float %109, i64 0
  %119 = insertelement <8 x float> %118, float 0.000000e+00, i64 0
  %120 = insertelement <8 x float> %119, float 0.000000e+00, i64 0
  %121 = insertelement <8 x float> %120, float 0.000000e+00, i64 0
  %122 = insertelement <8 x float> %121, float 0.000000e+00, i64 0
  %123 = insertelement <8 x float> %122, float 0.000000e+00, i64 0
  %124 = insertelement <8 x float> %123, float 0.000000e+00, i64 0
  %125 = insertelement <8 x float> %124, float 0.000000e+00, i64 0
  %126 = insertelement <8 x half> zeroinitializer, half %110, i64 0
  %127 = insertelement <8 x half> %126, half %111, i64 1
  %128 = insertelement <8 x half> %127, half %112, i64 2
  %129 = insertelement <8 x half> %128, half %113, i64 3
  %130 = insertelement <8 x half> %129, half %114, i64 4
  %131 = insertelement <8 x half> %130, half %115, i64 5
  %132 = insertelement <8 x half> %131, half %116, i64 6
  %133 = insertelement <8 x half> %132, half %117, i64 7
  %134 = bitcast <8 x half> %133 to <8 x i16>
  %135 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %125, <8 x i16> %134, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %136 = extractelement <8 x float> %135, i64 0
  br label %._crit_edge333
}

; Function Attrs: convergent inaccessiblememonly nounwind
declare float @llvm.genx.GenISA.WaveAll.f32(float, i8, i32) #1

; Function Attrs: convergent nounwind readnone willreturn
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1) #2

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.maxnum.f32(float, float) #3

; Function Attrs: nocallback nofree nosync nounwind readnone speculatable willreturn
declare float @llvm.exp2.f32(float) #3

attributes #0 = { convergent nounwind }
attributes #1 = { convergent inaccessiblememonly nounwind }
attributes #2 = { convergent nounwind readnone willreturn }
attributes #3 = { nocallback nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{!0}

!0 = distinct !{void (half addrspace(1)*, half addrspace(1)*, half addrspace(1)*, float addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)*, half addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i8 addrspace(2)*, i8*, i32, i32, i32, i32, i32, i32, i32, i32, i32)* bitcast (void (i16)* @triton_tem_fused_0 to void (half addrspace(1)*, half addrspace(1)*, half addrspace(1)*, float addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)*, i32 addrspace(1)*, half addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i8 addrspace(2)*, i8*, i32, i32, i32, i32, i32, i32, i32, i32, i32)*), !1}
!1 = distinct !{!2}
!2 = distinct !{!"sub_group_size", i32 16}
