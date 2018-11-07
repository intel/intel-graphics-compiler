;===================== begin_copyright_notice ==================================

;Copyright (c) 2017 Intel Corporation

;Permission is hereby granted, free of charge, to any person obtaining a
;copy of this software and associated documentation files (the
;"Software"), to deal in the Software without restriction, including
;without limitation the rights to use, copy, modify, merge, publish,
;distribute, sublicense, and/or sell copies of the Software, and to
;permit persons to whom the Software is furnished to do so, subject to
;the following conditions:

;The above copyright notice and this permission notice shall be included
;in all copies or substantial portions of the Software.

;THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
;OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
;MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
;IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
;CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
;TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
;SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


;======================= end_copyright_notice ==================================
; RUN: opt -igc-conv-ocl-to-common -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that BuiltinsConverter pass handles line debug info.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a64:64:64-f80:128:128-n8:16:32:64"
target triple = "igil_32_GEN8"

%opencl.image2d_t = type opaque

; Function Attrs: alwaysinline nounwind
define void @test(<4 x float> addrspace(1)* %buf1, <4 x i32> addrspace(1)* %buf2, %opencl.image2d_t addrspace(1)* %img1, %opencl.image2d_t addrspace(1)* %img2, i32 %sampler, <2 x i32> %coord, <2 x float> %fcoord, i32 %smpAddress) #0 {
entry:
  %0 = ptrtoint %opencl.image2d_t addrspace(1)* %img1 to i32
  %1 = ptrtoint %opencl.image2d_t addrspace(1)* %img2 to i32

  %res1 = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %0, i32 %sampler, <2 x float> %fcoord, float 0.000000e+00) #0, !dbg !1
  %arrayidx1 = getelementptr inbounds <4 x float> addrspace(1)* %buf1, i32 0, !dbg !2
  store <4 x float> %res1, <4 x float> addrspace(1)* %arrayidx1, align 16, !dbg !3

; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_sample_l
; CHECK: [[CoordX1:%[a-zA-Z0-9]+]] = extractelement <2 x float> %fcoord, i32 0, !dbg !1
; CHECK: [[CoordY1:%[a-zA-Z0-9]+]] = extractelement <2 x float> %fcoord, i32 1, !dbg !1
; CHECK: [[res1:%[a-zA-Z0-9]+]] = call <4 x float> @llvm.genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[CoordX1]], float [[CoordY1]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 0, i32 0, i32 0, i32 0), !dbg !1
; CHECK: [[arrayidx1:%[a-zA-Z0-9]+]] = getelementptr inbounds <4 x float> addrspace(1)* %buf1, i32 0, !dbg !2
; CHECK: store <4 x float> [[res1]], <4 x float> addrspace(1)* [[arrayidx1]], align 16, !dbg !3

  %res2 = call <4 x float> @__builtin_IB_OCL_2d_ld(i32 %0, <2 x i32> %coord, i32 0) #0, !dbg !1
  %arrayidx2 = getelementptr inbounds <4 x float> addrspace(1)* %buf1, i32 1, !dbg !2
  store <4 x float> %res2, <4 x float> addrspace(1)* %arrayidx2, align 16, !dbg !3

; CHECK-NOT: call <4 x float> @__builtin_IB_OCL_2d_ld
; CHECK: [[CoordX2:%[a-zA-Z0-9]+]] = extractelement <2 x i32> %coord, i32 0, !dbg !1
; CHECK: [[CoordY2:%[a-zA-Z0-9]+]] = extractelement <2 x i32> %coord, i32 1, !dbg !1
; CHECK: [[ptr2:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2), !dbg !1
; CHECK: [[res2:%[a-zA-Z0-9]+]] = call <4 x float> @llvm.genx.GenISA.ldptr.v4f32.p196608f32(i32 [[CoordX2]], i32 [[CoordY2]], i32 0, i32 0, float addrspace(196608)* [[ptr2]], i32 0, i32 0, i32 0), !dbg !1
; CHECK: [[arrayidx2:%[a-zA-Z0-9]+]] = getelementptr inbounds <4 x float> addrspace(1)* %buf1, i32 1, !dbg !2
; CHECK: store <4 x float> [[res2]], <4 x float> addrspace(1)* [[arrayidx2]], align 16, !dbg !3

  %res3 = call <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %0, <2 x i32> %coord, i32 0) #0, !dbg !1
  %arrayidx3 = getelementptr inbounds <4 x i32> addrspace(1)* %buf2, i32 0, !dbg !2
  store <4 x i32> %res3, <4 x i32> addrspace(1)* %arrayidx3, align 16, !dbg !3

; CHECK-NOT: call <4 x i32> @__builtin_IB_OCL_2d_ldui
; CHECK: [[CoordX3:%[a-zA-Z0-9]+]] = extractelement <2 x i32> %coord, i32 0, !dbg !1
; CHECK: [[CoordY3:%[a-zA-Z0-9]+]] = extractelement <2 x i32> %coord, i32 1, !dbg !1
; CHECK: [[ptr3:%[a-zA-Z0-9]+]] = call float addrspace(196608)* @llvm.genx.GenISA.GetBufferPtr.p196608f32(i32 0, i32 2), !dbg !1
; CHECK: [[res3:%[a-zA-Z0-9]+]] = call <4 x i32> @llvm.genx.GenISA.ldui.ptr.p196608f32(i32 [[CoordX3]], i32 [[CoordY3]], i32 0, i32 0, float addrspace(196608)* [[ptr3]], i32 0, i32 0, i32 0), !dbg !1
; CHECK: [[arrayidx3:%[a-zA-Z0-9]+]] = getelementptr inbounds <4 x i32> addrspace(1)* %buf2, i32 0, !dbg !2
; CHECK: store <4 x i32> [[res3]], <4 x i32> addrspace(1)* [[arrayidx3]], align 16, !dbg !3

  %arrayidx4 = getelementptr inbounds <4 x i32> addrspace(1)* %buf2, i32 1, !dbg !1
  %res4 = load <4 x i32> addrspace(1)* %arrayidx4, align 16, !dbg !2
  call void @__builtin_IB_write_2d_i(i32 %1, i32 1, i32 1, <2 x i32> %coord, <4 x i32> %res4, i32 0) #0, !dbg !3

; CHECK-NOT: call void @__builtin_IB_write_2d_i
; CHECK: [[arrayidx4:%[a-zA-Z0-9]+]] = getelementptr inbounds <4 x i32> addrspace(1)* %buf2, i32 1, !dbg !1
; CHECK: [[res4:%[a-zA-Z0-9]+]] = load <4 x i32> addrspace(1)* [[arrayidx4]], align 16, !dbg !2
; CHECK: [[ptr4:%[a-zA-Z0-9]+]] = call float addrspace(131074)* @llvm.genx.GenISA.GetBufferPtr.p131074f32(i32 2, i32 1), !dbg !3
; CHECK: [[CoordX4:%[a-zA-Z0-9]+]] = extractelement <2 x i32> %coord, i32 0, !dbg !3
; CHECK: [[CoordY4:%[a-zA-Z0-9]+]] = extractelement <2 x i32> %coord, i32 1, !dbg !3
; CHECK: [[floatColor:%[a-zA-Z0-9]+]] = bitcast <4 x i32> [[res4]] to <4 x float>, !dbg !3
; CHECK: [[ColorX:%[a-zA-Z0-9]+]] = extractelement <4 x float> [[floatColor]], i32 0, !dbg !3
; CHECK: [[ColorY:%[a-zA-Z0-9]+]] = extractelement <4 x float> [[floatColor]], i32 1, !dbg !3
; CHECK: [[ColorZ:%[a-zA-Z0-9]+]] = extractelement <4 x float> [[floatColor]], i32 2, !dbg !3
; CHECK: [[ColorW:%[a-zA-Z0-9]+]] = extractelement <4 x float> [[floatColor]], i32 3, !dbg !3
; CHECK: call void @llvm.genx.GenISA.typedwrite.p131074f32(float addrspace(131074)* [[ptr4]], i32 [[CoordX4]], i32 [[CoordY4]], i32 0, i32 0, float [[ColorX]], float [[ColorY]], float [[ColorZ]], float [[ColorW]]), !dbg !3

  %tmp1 = extractelement <2 x float> %fcoord, i32 0, !dbg !1
  %tmp2 = call float @__builtin_IB_frnd_ni(float %tmp1) #0, !dbg !2
  %res5 = insertelement <4 x float> undef, float %tmp2, i32 0, !dbg !3
  %arrayidx5 = getelementptr inbounds <4 x float> addrspace(1)* %buf1, i32 2, !dbg !3
  store <4 x float> %res5, <4 x float> addrspace(1)* %arrayidx5, align 16, !dbg !3

; CHECK-NOT: call float @__builtin_IB_frnd_ni
; CHECK: [[tmp1:%[a-zA-Z0-9]+]] = extractelement <2 x float> %fcoord, i32 0, !dbg !1
; CHECK: [[tmp2:%[a-zA-Z0-9]+]] = call float @llvm.floor.f32(float [[tmp1]]), !dbg !2
; CHECK: [[res5:%[a-zA-Z0-9]+]] = insertelement <4 x float> undef, float [[tmp2]], i32 0, !dbg !3
; CHECK: [[arrayidx5:%[a-zA-Z0-9]+]] = getelementptr inbounds <4 x float> addrspace(1)* %buf1, i32 2, !dbg !3
; CHECK: store <4 x float> [[res5]], <4 x float> addrspace(1)* [[arrayidx5]], align 16, !dbg !3

  ret void
}

declare void @__builtin_IB_write_2d_i(i32, i32, i32, <2 x i32>, <4 x i32>, i32)

declare <4 x float> @__builtin_IB_OCL_2d_sample_l(i32, i32, <2 x float>, float)

declare <4 x i32> @__builtin_IB_OCL_2d_ldui(i32, <2 x i32>, i32)

declare <4 x float> @__builtin_IB_OCL_2d_ld(i32, <2 x i32>, i32)

declare float @__builtin_IB_frnd_ni(float)

attributes #0 = { nounwind }

;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9, !10, !11, !12, !13, !14, !15, !16, !17, !18, !19, !20}

!igc.functions = !{!4}

!0 = metadata !{}
!1 = metadata !{i32 5, i32 0, metadata !0, null}
!2 = metadata !{i32 6, i32 0, metadata !0, null}
!3 = metadata !{i32 7, i32 0, metadata !0, null}
!4 = metadata !{void (<4 x float> addrspace(1)*, <4 x i32> addrspace(1)*, %opencl.image2d_t addrspace(1)*, %opencl.image2d_t addrspace(1)*, i32, <2 x i32>, <2 x float>, i32)* @test, metadata !5}
!5 = metadata !{metadata !6, metadata !7, metadata !10}
!6 = metadata !{metadata !"function_type", i32 0}
!7 = metadata !{metadata !"implicit_arg_desc", metadata !8}
!8 = metadata !{i32 27, metadata !9} ;; SAMPLER_ADDRESS
!9 = metadata !{metadata !"explicit_arg_num", i32 4}
!10 = metadata !{metadata !"resource_alloc", metadata !11, metadata !12, metadata !13, metadata !14}
!11 = metadata !{metadata !"uavs_num", i32 3}
!12 = metadata !{metadata !"srvs_num", i32 1}
!13 = metadata !{metadata !"samplers_num", i32 1}
!14 = metadata !{metadata !"arg_allocs", metadata !15, metadata !16, metadata !17, metadata !18, metadata !19, metadata !20, metadata !20, metadata !20}
!15 = metadata !{i32 1, null, i32 0}
!16 = metadata !{i32 1, null, i32 1}
!17 = metadata !{i32 2, i32 0, i32 0}
!18 = metadata !{i32 1, i32 0, i32 2}
!19 = metadata !{i32 3, i32 0, i32 0}
!20 = metadata !{i32 0, null, null}
