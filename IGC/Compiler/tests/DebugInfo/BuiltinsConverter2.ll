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
;; This LIT test checks that BuiltinsConverter pass handles variable debug info.
;; Handlesd case: inline sampler defined in program scope.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a64:64:64-f80:128:128-n8:16:32:64"
target triple = "igil_32_GEN9"

%opencl.image2d_t = type opaque

@sampler = internal constant i32 8, align 4

; Function Attrs: alwaysinline nounwind
define void @test(<4 x i32> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %image, <2 x float> %arg_value_float2) #0 {
entry:
  %0 = ptrtoint %opencl.image2d_t addrspace(1)* %image to i32, !dbg !1
  %1 = load i32* @sampler, align 4, !dbg !2
  %resFloat = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %0, i32 %1, <2 x float> %arg_value_float2, float 0.000000e+00) #1, !dbg !3
  %res = bitcast <4 x float> %resFloat to <4 x i32>, !dbg !4
  store <4 x i32> %res, <4 x i32> addrspace(1)* %dst, align 16, !dbg !5
  ret void, !dbg !6

; CHECK: call void @llvm.dbg.value(metadata [[m46:![0-9]+]], i64 0, metadata [[m47:![0-9]+]])
; CHECK: [[CoordX:%[a-zA-Z0-9]+]] = extractelement <2 x float> %arg_value_float2, i32 0, !dbg !3
; CHECK: [[CoordY:%[a-zA-Z0-9]+]] = extractelement <2 x float> %arg_value_float2, i32 1, !dbg !3
; CHECK: [[resFloat:%[a-zA-Z0-9]+]] = call <4 x float> @llvm.genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[CoordX]], float [[CoordY]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 0, i32 0, i32 0, i32 0), !dbg !3
; CHECK: [[res:%[a-zA-Z0-9]+]] = bitcast <4 x float> [[resFloat]] to <4 x i32>, !dbg !4

}

declare <4 x float> @__builtin_IB_OCL_2d_sample_l(i32, i32, <2 x float>, float)

attributes #0 = { alwaysinline nounwind }

;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3, !4, !5, !6, !7}
!llvm.dbg.cu = !{!8}
!igc.functions = !{!32}

!0 = metadata !{i32 0}
!1 = metadata !{i32 13, i32 0, metadata !0, null}
!2 = metadata !{i32 16, i32 0, metadata !0, null}
!3 = metadata !{i32 18, i32 0, metadata !0, null}
!4 = metadata !{i32 19, i32 0, metadata !0, null}
!5 = metadata !{i32 20, i32 0, metadata !0, null}
!6 = metadata !{i32 21, i32 0, metadata !0, null}
!7 = metadata !{metadata !"filename.cl", metadata !"dirname"}
!8 = metadata !{i32 786449, metadata !7, i32 12, metadata !"clang version 3.4 ", i1 true, metadata !"", i32 0, metadata !0, metadata !0, metadata !9, metadata !28, metadata !0, metadata !""}
!9 = metadata !{metadata !10}
!10 = metadata !{i32 786478, metadata !7, metadata !11, metadata !"test", metadata !"test", metadata !"", i32 13, metadata !12, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, null, null, null, null, i32 14}
!11 = metadata !{i32 786473, metadata !7}        
!12 = metadata !{i32 786453, i32 0, i32 0, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !13, i32 0, i32 0}
!13 = metadata !{null, metadata !14, metadata !21, metadata !23}
!14 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 32, i64 32, i64 0, i32 0, metadata !15}
!15 = metadata !{i32 786454, metadata !7, null, metadata !"uint4", i32 204, i64 0, i64 0, i64 0, i32 0, metadata !16}
!16 = metadata !{i32 786433, null, null, metadata !"", i32 0, i64 128, i64 128, i32 0, i32 2048, metadata !17, metadata !19, i32 0, i32 0}
!17 = metadata !{i32 786454, metadata !7, null, metadata !"uint", i32 58, i64 0, i64 0, i64 0, i32 0, metadata !18}
!18 = metadata !{i32 786468, null, null, metadata !"unsigned int", i32 0, i64 32, i64 32, i64 0, i32 0, i32 7}
!19 = metadata !{metadata !20}
!20 = metadata !{i32 786465, i64 0, i64 4}       
!21 = metadata !{i32 786447, null, null, metadata !"", i32 0, i64 32, i64 0, i64 0, i32 0, metadata !22}
!22 = metadata !{i32 786451, metadata !7, null, metadata !"opencl_image2d_t", i32 0, i64 0, i64 0, i32 0, i32 4, null, null, i32 0}
!23 = metadata !{i32 786454, metadata !7, null, metadata !"float2", i32 217, i64 0, i64 0, i64 0, i32 0, metadata !24}
!24 = metadata !{i32 786433, null, null, metadata !"", i32 0, i64 64, i64 64, i32 0, i32 2048, metadata !25, metadata !26, i32 0, i32 0}
!25 = metadata !{i32 786468, null, null, metadata !"float", i32 0, i64 32, i64 32, i64 0, i32 0, i32 4}
!26 = metadata !{metadata !27}
!27 = metadata !{i32 786465, i64 0, i64 2}       
!28 = metadata !{metadata !29}
!29 = metadata !{i32 786484, i32 0, null, metadata !"sampler", metadata !"sampler", metadata !"", metadata !11, i32 11, metadata !30, i32 0, i32 1, i32* @sampler, null}
!30 = metadata !{i32 786470, null, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, metadata !31}
!31 = metadata !{i32 786468, null, null, metadata !"opencl_sampler_t", i32 0, i64 32, i64 32, i64 0, i32 0, i32 7}
!32 = metadata !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t addrspace(1)*, <2 x float>)* @test, metadata !33}
!33 = metadata !{metadata !34, metadata !35, metadata !36}
!34 = metadata !{metadata !"function_type", i32 0}
!35 = metadata !{metadata !"implicit_arg_desc"}
!36 = metadata !{metadata !"resource_alloc", metadata !37, metadata !38, metadata !39, metadata !40}
!37 = metadata !{metadata !"uavs_num", i32 1}
!38 = metadata !{metadata !"srvs_num", i32 1}
!39 = metadata !{metadata !"samplers_num", i32 0}
!40 = metadata !{metadata !"arg_allocs", metadata !41, metadata !42, metadata !43}
!41 = metadata !{i32 1, null, i32 0}
!42 = metadata !{i32 2, i32 0, i32 0}
!43 = metadata !{i32 0, null, null}

; CHECK: !36 = metadata !{metadata !"resource_alloc", metadata !37, metadata !38, metadata !39, metadata !40, metadata [[m44:![0-9]+]]}
; CHECK: [[m44]] = metadata !{metadata [[m45:![0-9]+]]}
; CHECK: [[m45]] = metadata !{i32 8, i32 0}
; CHECK: [[m46]] = metadata !{i32 8}
; CHECK: [[m47]] = metadata !{i32 786688, metadata !10, metadata !"sampler", metadata !11, i32 11, metadata !30, i32 0, i32 0}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; Test is based on following source
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; const sampler_t sampler = CLK_NORMALIZED_COORDS_TRUE | CLK_FILTER_NEAREST | CLK_ADDRESS_NONE;
;;__kernel void test(global uint4 *dst, read_only image2d_t image, float2 arg_value_float2)
;;{
;;  dst[0] = read_imageui (image, sampler, arg_value_float2);
;;}
