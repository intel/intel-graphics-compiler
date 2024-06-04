;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: llc %s -march=genx64 -mcpu=XeHPC -vc-skip-ocl-runtime-info \
; RUN: -finalizer-opts='-dumpcommonisa -isaasmToConsole' -o /dev/null | FileCheck %s

; CHECK: .decl [[DATA:V[0-9]+]] v_type=G type=f num_elts=128 align=GRF
; CHECK: .decl [[ACC:V[0-9]+]] v_type=G type=f num_elts=128 align=wordx32 alias=<[[DATA]], 0>
; CHECK: raw_sends.15.1.0.8 (M1, 1)  0x0:ud 0x2820403:ud V{{[0-9]+}}.0 %null.0 [[DATA]].0
; CHECK-NEXT: dpas.hf.hf.8.8 (M1, 16) [[ACC]].0 [[ACC]].0 V{{[0-9]+}}.0 V{{[0-9]+}}(0,0)
; CHECK-NEXT: dpas.hf.hf.8.8 (M1, 16) [[ACC]].0 [[ACC]].0 V{{[0-9]+}}.0 V{{[0-9]+}}(0,0)
; CHECK-NEXT: raw_sends.15.1.8.0 (M1, 1)  0x0:ud 0x2020407:ud V{{[0-9]+}}.0 [[DATA]].0 %null.0

target datalayout = "e-p:64:64-p3:32:32-p6:32:32-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

declare <128 x float> @llvm.genx.raw.send2.v128f32.i1.v8i32(i8, i8, i1, i8, i8, i8, i32, i32, <8 x i32>, <128 x float>)

declare <128 x float> @llvm.genx.dpas2.v128f32.v128f32.v128i32.v64i32(<128 x float>, <128 x i32>, <64 x i32>, i32, i32, i32, i32, i32, i32)

declare void @llvm.genx.raw.sends2.noresult.i1.v8i32.v128f32(i8, i8, i1, i8, i8, i8, i32, i32, <8 x i32>, <128 x float>)

define dllexport spir_kernel void @test([256 x half] addrspace(1)* %0, [512 x half] addrspace(1)* %1, [128 x float] addrspace(1)* %2, i64 %impl.arg.private.base) {
  %4 = call <128 x float> @llvm.genx.raw.send2.v128f32.i1.v8i32(i8 0, i8 0, i1 false, i8 1, i8 8, i8 15, i32 0, i32 42075139, <8 x i32> zeroinitializer, <128 x float> zeroinitializer)
  %5 = call <128 x float> @llvm.genx.dpas2.v128f32.v128f32.v128i32.v64i32(<128 x float> %4, <128 x i32> undef, <64 x i32> undef, i32 10, i32 10, i32 8, i32 8, i32 0, i32 0)
  %6 = call <128 x float> @llvm.genx.dpas2.v128f32.v128f32.v128i32.v64i32(<128 x float> %5, <128 x i32> undef, <64 x i32> undef, i32 10, i32 10, i32 8, i32 8, i32 0, i32 0)
  call void @llvm.genx.raw.sends2.noresult.i1.v8i32.v128f32(i8 0, i8 0, i1 true, i8 1, i8 8, i8 15, i32 0, i32 33686535, <8 x i32> undef, <128 x float> %6)
  ret void
}

!genx.kernels = !{!0}
!genx.kernel.internal = !{!5}

!0 = !{void ([256 x half] addrspace(1)*, [512 x half] addrspace(1)*, [128 x float] addrspace(1)*, i64)* @test, !"test", !1, i32 0, !2, !3, !4, i32 0}
!1 = !{i32 0, i32 0, i32 0, i32 96}
!2 = !{i32 136, i32 144, i32 152, i32 128}
!3 = !{i32 0, i32 0, i32 0}
!4 = !{!"svmptr_t", !"svmptr_t", !"svmptr_t"}
!5 = !{void ([256 x half] addrspace(1)*, [512 x half] addrspace(1)*, [128 x float] addrspace(1)*, i64)* @test, !6, !7, !8, !9}
!6 = !{i32 0, i32 0, i32 0, i32 0}
!7 = !{i32 0, i32 1, i32 2, i32 3}
!8 = !{}
!9 = !{i32 255, i32 255, i32 255, i32 255}
