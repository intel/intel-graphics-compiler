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
; RUN: igc_opt -igc-conv-ocl-to-common -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that BuiltinsConverter pass handles variable debug info.
;; Handlesd case: inline sampler defined in program scope.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target triple = "igil_32_GEN9"

%opencl.image2d_t = type opaque

@sampler = internal constant i32 8, align 4

; Function Attrs: alwaysinline nounwind
define void @test(<4 x i32> addrspace(1)* %dst, %opencl.image2d_t addrspace(1)* %image, <2 x float> %arg_value_float2) #0 {
entry:
  %0 = ptrtoint %opencl.image2d_t addrspace(1)* %image to i32, !dbg !1
  %1 = load i32, i32* @sampler, align 4, !dbg !2
  %resFloat = call <4 x float> @__builtin_IB_OCL_2d_sample_l(i32 %0, i32 %1, <2 x float> %arg_value_float2, float 0.000000e+00) #1, !dbg !3
  %res = bitcast <4 x float> %resFloat to <4 x i32>, !dbg !4
  store <4 x i32> %res, <4 x i32> addrspace(1)* %dst, align 16, !dbg !5
  ret void, !dbg !6

; CHECK: call void @llvm.dbg.value(metadata [[m46:![0-9]+]], i64 0, metadata [[m47:![0-9]+]])
; CHECK: [[CoordX:%[a-zA-Z0-9]+]] = extractelement <2 x float> %arg_value_float2, i32 0, !dbg !3
; CHECK: [[CoordY:%[a-zA-Z0-9]+]] = extractelement <2 x float> %arg_value_float2, i32 1, !dbg !3
; CHECK: [[resFloat:%[a-zA-Z0-9]+]] = call <4 x float> @genx.GenISA.sampleL.v4f32.f32(float 0.000000e+00, float [[CoordX]], float [[CoordY]], float 0.000000e+00, float 0.000000e+00, i32 0, i32 0, i32 0, i32 0, i32 0), !dbg !3
; CHECK: [[res:%[a-zA-Z0-9]+]] = bitcast <4 x float> [[resFloat]] to <4 x i32>, !dbg !4

}

declare <4 x float> @__builtin_IB_OCL_2d_sample_l(i32, i32, <2 x float>, float)

attributes #0 = { alwaysinline nounwind }

;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3, !4, !5, !6, !7}
!llvm.dbg.cu = !{!8}
!igc.functions = !{!32}

!0 = distinct !{i32 0}
!1 = !{i32 13, i32 0, !0, null}
!2 = !{i32 16, i32 0, !0, null}
!3 = !{i32 18, i32 0, !0, null}
!4 = !{i32 19, i32 0, !0, null}
!5 = !{i32 20, i32 0, !0, null}
!6 = !{i32 21, i32 0, !0, null}
!7 = !{!"filename.cl", !"dirname"}
!8 = !{i32 786449, !7, i32 12, !"clang version 3.4 ", i1 true, !"", i32 0, !0, !0, !9, !28, !0, !""}
!9 = !{!10}
!10 = !{i32 786478, !7, !11, !"test", !"test", !"", i32 13, !12, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 true, null, null, null, null, i32 14}
!11 = !{i32 786473, !7}        
!12 = !{i32 786453, i32 0, i32 0, !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, !13, i32 0, i32 0}
!13 = !{null, !14, !21, !23}
!14 = !{i32 786447, null, null, !"", i32 0, i64 32, i64 32, i64 0, i32 0, !15}
!15 = !{i32 786454, !7, null, !"uint4", i32 204, i64 0, i64 0, i64 0, i32 0, !16}
!16 = !{i32 786433, null, null, !"", i32 0, i64 128, i64 128, i32 0, i32 2048, !17, !19, i32 0, i32 0}
!17 = !{i32 786454, !7, null, !"uint", i32 58, i64 0, i64 0, i64 0, i32 0, !18}
!18 = !{i32 786468, null, null, !"unsigned int", i32 0, i64 32, i64 32, i64 0, i32 0, i32 7}
!19 = !{!20}
!20 = !{i32 786465, i64 0, i64 4}       
!21 = !{i32 786447, null, null, !"", i32 0, i64 32, i64 0, i64 0, i32 0, !22}
!22 = !{i32 786451, !7, null, !"opencl_image2d_t", i32 0, i64 0, i64 0, i32 0, i32 4, null, null, i32 0}
!23 = !{i32 786454, !7, null, !"float2", i32 217, i64 0, i64 0, i64 0, i32 0, !24}
!24 = !{i32 786433, null, null, !"", i32 0, i64 64, i64 64, i32 0, i32 2048, !25, !26, i32 0, i32 0}
!25 = !{i32 786468, null, null, !"float", i32 0, i64 32, i64 32, i64 0, i32 0, i32 4}
!26 = !{!27}
!27 = !{i32 786465, i64 0, i64 2}       
!28 = !{!29}
!29 = !{i32 786484, i32 0, null, !"sampler", !"sampler", !"", !11, i32 11, !30, i32 0, i32 1, i32* @sampler, null}
!30 = !{i32 786470, null, null, !"", i32 0, i64 0, i64 0, i64 0, i32 0, !31}
!31 = !{i32 786468, null, null, !"opencl_sampler_t", i32 0, i64 32, i64 32, i64 0, i32 0, i32 7}
!32 = !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t addrspace(1)*, <2 x float>)* @test, !33}
!33 = !{!34, !35, !36}
!34 = !{!"function_type", i32 0}
!35 = !{!"implicit_arg_desc"}
!36 = !{!"resource_alloc", !37, !38, !39, !40}
!37 = !{!"uavs_num", i32 1}
!38 = !{!"srvs_num", i32 1}
!39 = !{!"samplers_num", i32 0}
!40 = !{!"arg_allocs", !41, !42, !43}
!41 = !{i32 1, null, i32 0}
!42 = !{i32 2, i32 0, i32 0}
!43 = !{i32 0, null, null}

!igc.version = !{!45}
!igc.input.ir = !{!44}
!44 = !{!"ocl", i32 1, i32 2}
!45 = !{i32 1, i32 0}

; CHECK: !36 = !{!"resource_alloc", !37, !38, !39, !40, [[m44:![0-9]+]]}
; CHECK: [[m44]] = !{[[m45:![0-9]+]]}
; CHECK: [[m45]] = !{i32 8, i32 0}
; CHECK: [[m46]] = !{i32 8}
; CHECK: [[m47]] = !{i32 786688, !10, !"sampler", !11, i32 11, !30, i32 0, i32 0}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; Test is based on following source
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; const sampler_t sampler = CLK_NORMALIZED_COORDS_TRUE | CLK_FILTER_NEAREST | CLK_ADDRESS_NONE;
;;__kernel void test(global uint4 *dst, read_only image2d_t image, float2 arg_value_float2)
;;{
;;  dst[0] = read_imageui (image, sampler, arg_value_float2);
;;}
