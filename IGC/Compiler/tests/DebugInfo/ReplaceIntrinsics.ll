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
; RUN: igc_opt -igc-replace-unsupported-intrinsics -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that ReplaceIntrinsics pass handles line debug info.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target triple = "igil_32_GEN8"

define void @testMemcpy1(i32 addrspace(1)* %dst, i32 addrspace(1)* %src, i32 %count) #0 {
entry:
  call void @llvm.memcpy.p1i32.p1i32.i32(i32 addrspace(1)* %dst, i32 addrspace(1)* %src, i32 %count, i32 16, i1 false), !dbg !1
  ret void

; CHECK: @testMemcpy1
; CHECK: [[pIV1:%[a-zA-Z0-9_.]+]] = alloca i32
; CHECK: [[memcpy_src1:%[a-zA-Z0-9_.]+]] = bitcast i32 addrspace(1)* %src to i8 addrspace(1)*, !dbg !1
; CHECK: [[memcpy_dst1:%[a-zA-Z0-9_.]+]] = bitcast i32 addrspace(1)* %dst to i8 addrspace(1)*, !dbg !1
; CHECK: store i32 0, i32* [[pIV1]], !dbg !1
; CHECK: [[cond1:%[a-zA-Z0-9_.]+]] = icmp ult i32 0, %count, !dbg !1
; CHECK: br i1 [[cond1]], label %[[memcpy_body:[a-zA-Z0-9_.]+]], label %[[memcpy_post:[a-zA-Z0-9_.]+]], !dbg !1

; CHECK: [[memcpy_body]]:                                      ; preds = %[[memcpy_body]], %entry
; CHECK: [[IV1:%[a-zA-Z0-9_.]+]] = load i32, i32* [[pIV1]], !dbg !1
; CHECK: [[src_gep1:%[a-zA-Z0-9_.]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_src1]], i32 [[IV1]], !dbg !1
; CHECK: [[dst_gep1:%[a-zA-Z0-9_.]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_dst1]], i32 [[IV1]], !dbg !1
; CHECK: [[res1:%[a-zA-Z0-9_.]+]] = load i8, i8 addrspace(1)* [[src_gep1]], align 16, !dbg !1
; CHECK: store i8 [[res1]], i8 addrspace(1)* [[dst_gep1]], align 16, !dbg !1
; CHECK: [[inc1:%[a-zA-Z0-9_.]+]] = add i32 %IV, 1, !dbg !1
; CHECK: store i32 [[inc1]], i32* [[pIV1]], !dbg !1
; CHECK: [[cond2:%[a-zA-Z0-9_.]+]] = icmp ult i32 [[inc1]], %count, !dbg !1
; CHECK: br i1 [[cond2]], label %[[memcpy_body]], label %[[memcpy_post]], !dbg !1

; CHECK: [[memcpy_post]]:                                      ; preds = %[[memcpy_body]], %entry
; CHECK: ret void
}

define void @testMemcpy2(i32 addrspace(1)* %dst, i32 addrspace(1)* %src) #0 {
entry:
  call void @llvm.memcpy.p1i32.p1i32.i32(i32 addrspace(1)* %dst, i32 addrspace(1)* %src, i32 63, i32 16, i1 false), !dbg !2
  ret void

; CHECK: @testMemcpy2
; CHECK: [[memcpy_vsrc:%[a-zA-Z0-9_.]+]] = bitcast i32 addrspace(1)* %src to <8 x i32> addrspace(1)*, !dbg !2
; CHECK: [[memcpy_vdst:%[a-zA-Z0-9_.]+]] = bitcast i32 addrspace(1)* %dst to <8 x i32> addrspace(1)*, !dbg !2
; CHECK: [[memcpy2_src_gep1:%[a-zA-Z0-9_.]+]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[memcpy_vsrc]], i32 0, !dbg !2
; CHECK: [[memcpy2_dst_gep1:%[a-zA-Z0-9_.]+]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[memcpy_vdst]], i32 0, !dbg !2
; CHECK: [[memcpy2_res1:%[a-zA-Z0-9_.]+]] = load <8 x i32>, <8 x i32> addrspace(1)* [[memcpy2_src_gep1]], align 16, !dbg !2
; CHECK: store <8 x i32> [[memcpy2_res1]], <8 x i32> addrspace(1)* [[memcpy2_dst_gep1]], align 16, !dbg !2
; CHECK: [[memcpy_src:%[a-zA-Z0-9_.]+]] = bitcast i32 addrspace(1)* %src to i8 addrspace(1)*, !dbg !2
; CHECK: [[memcpy_dst:%[a-zA-Z0-9_.]+]] = bitcast i32 addrspace(1)* %dst to i8 addrspace(1)*, !dbg !2
; CHECK: [[memcpy2_src_gep2:%[a-zA-Z0-9_.]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_src]], i32 32, !dbg !2
; CHECK: [[memcpy2_dst_gep2:%[a-zA-Z0-9_.]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_dst]], i32 32, !dbg !2
; CHECK: [[memcpy_rem:%[a-zA-Z0-9_.]+]] = bitcast i8 addrspace(1)* [[memcpy2_src_gep2]] to <4 x i32> addrspace(1)*, !dbg !2
; CHECK: [[memcpy_rem1:%[a-zA-Z0-9_.]+]] = bitcast i8 addrspace(1)* [[memcpy2_dst_gep2]] to <4 x i32> addrspace(1)*, !dbg !2
; CHECK: [[memcpy2_res2:%[a-zA-Z0-9_.]+]] = load <4 x i32>, <4 x i32> addrspace(1)* [[memcpy_rem]], align 16, !dbg !2
; CHECK: store <4 x i32> [[memcpy2_res2]], <4 x i32> addrspace(1)* [[memcpy_rem1]], align 16, !dbg !2
; CHECK: [[memcpy2_src_gep3:%[a-zA-Z0-9_.]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_src]], i32 48, !dbg !2
; CHECK: [[memcpy2_dst_gep3:%[a-zA-Z0-9_.]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_dst]], i32 48, !dbg !2
; CHECK: [[memcpy_rem2:%[a-zA-Z0-9_.]+]] = bitcast i8 addrspace(1)* [[memcpy2_src_gep3]] to <3 x i32> addrspace(1)*, !dbg !2
; CHECK: [[memcpy_rem3:%[a-zA-Z0-9_.]+]] = bitcast i8 addrspace(1)* [[memcpy2_dst_gep3]] to <3 x i32> addrspace(1)*, !dbg !2
; CHECK: [[memcpy2_res3:%[a-zA-Z0-9_.]+]] = load <3 x i32>, <3 x i32> addrspace(1)* [[memcpy_rem2]], align 4, !dbg !2
; CHECK: store <3 x i32> [[memcpy2_res3]], <3 x i32> addrspace(1)* [[memcpy_rem3]], align 4, !dbg !2
; CHECK: [[memcpy2_src_gep5:%[a-zA-Z0-9_.]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_src]], i32 60, !dbg !2
; CHECK: [[memcpy2_dst_gep5:%[a-zA-Z0-9_.]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_dst]], i32 60, !dbg !2
; CHECK: [[memcpy_rem6:%[a-zA-Z0-9_.]+]] = bitcast i8 addrspace(1)* [[memcpy2_src_gep5]] to <2 x i8> addrspace(1)*, !dbg !2
; CHECK: [[memcpy_rem7:%[a-zA-Z0-9_.]+]] = bitcast i8 addrspace(1)* [[memcpy2_dst_gep5]] to <2 x i8> addrspace(1)*, !dbg !2
; CHECK: [[memcpy2_res5:%[a-zA-Z0-9_.]+]] = load <2 x i8>, <2 x i8> addrspace(1)* [[memcpy_rem6]], align 2, !dbg !2
; CHECK: store <2 x i8> [[memcpy2_res5]], <2 x i8> addrspace(1)* [[memcpy_rem7]], align 2, !dbg !2
; CHECK: [[memcpy2_src_gep6:%[a-zA-Z0-9_.]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_src]], i32 62, !dbg !2
; CHECK: [[memcpy2_dst_gep6:%[a-zA-Z0-9_.]+]] = getelementptr i8, i8 addrspace(1)* [[memcpy_dst]], i32 62, !dbg !2
; CHECK: [[memcpy2_res6:%[a-zA-Z0-9_.]+]] = load i8, i8 addrspace(1)* [[memcpy2_src_gep6]], align 1, !dbg !2
; CHECK: store i8 [[memcpy2_res6]], i8 addrspace(1)* [[memcpy2_dst_gep6]], align 1, !dbg !2
}

define void @testMemset1(i32 addrspace(1)* %dst, i8 %src, i32 %count) #0 {
entry:
  call void @llvm.memset.p1i32.i32(i32 addrspace(1)* %dst, i8 %src, i32 %count, i32 16, i1 false), !dbg !3
  ret void

; CHECK: @testMemset1
; CHECK: [[memset1_pIV:%[a-zA-Z0-9_.]+]] = alloca i32
; CHECK: [[memset_dst:%[a-zA-Z0-9_.]+]] = bitcast i32 addrspace(1)* %dst to i8 addrspace(1)*, !dbg !3
; CHECK: store i32 0, i32* [[memset1_pIV]], !dbg !3
; CHECK: [[memset1_cond1:%[a-zA-Z0-9_.]+]] = icmp ult i32 0, %count, !dbg !3
; CHECK: br i1 [[memset1_cond1]], label %[[memset_body:[a-zA-Z0-9_.]+]], label %[[memset_post:[a-zA-Z0-9_.]+]], !dbg !3

; CHECK: [[memset_body]]:                                      ; preds = %[[memset_body]], %entry
; CHECK: [[memset1_IV:%[a-zA-Z0-9_.]+]] = load i32, i32* [[memset1_pIV]], !dbg !3
; CHECK: [[memset1_gep1:%[a-zA-Z0-9_.]+]] = getelementptr i8, i8 addrspace(1)* [[memset_dst]], i32 [[memset1_IV]], !dbg !3
; CHECK: store i8 %src, i8 addrspace(1)* [[memset1_gep1]], align 16, !dbg !3
; CHECK: [[memset1_inc1:%[a-zA-Z0-9_.]+]] = add i32 [[memset1_IV]], 1, !dbg !3
; CHECK: store i32 [[memset1_inc1]], i32* [[memset1_pIV]], !dbg !3
; CHECK: [[memset1_cond2:%[a-zA-Z0-9_.]+]] = icmp ult i32 [[memset1_inc1]], %count, !dbg !3
; CHECK: br i1 [[memset1_cond2]], label %[[memset_body]], label %[[memset_post]], !dbg !3

; CHECK: [[memset_post]]:                                      ; preds = %[[memset_body]], %entry
; CHECK: ret void
}

define void @testMemset2(i32 addrspace(1)* %dst, i8 %src) #0 {
entry:
  call void @llvm.memset.p1i32.i32(i32 addrspace(1)* %dst, i8 %src, i32 63, i32 16, i1 false), !dbg !4
  ret void

; CHECK: @testMemset2
; CHECK: [[memset2_v8_zext:%[a-zA-Z0-9_.]+]] = zext i8 %src to i32, !dbg !4
; CHECK: [[memset2_v8_shl1:%[a-zA-Z0-9_.]+]] = shl i32 [[memset2_v8_zext]], 8, !dbg !4
; CHECK: [[memset2_v8_add1:%[a-zA-Z0-9_.]+]] = add i32 [[memset2_v8_shl1]], [[memset2_v8_zext]], !dbg !4
; CHECK: [[memset2_v8_shl2:%[a-zA-Z0-9_.]+]] = shl i32 [[memset2_v8_add1]], 8, !dbg !4
; CHECK: [[memset2_v8_add2:%[a-zA-Z0-9_.]+]] = add i32 [[memset2_v8_shl2]], [[memset2_v8_zext]], !dbg !4
; CHECK: [[memset2_v8_shl3:%[a-zA-Z0-9_.]+]] = shl i32 [[memset2_v8_add2]], 8, !dbg !4
; CHECK: [[memset2_v8_ssrc:%[a-zA-Z0-9_.]+]] = add i32 [[memset2_v8_shl3]], [[memset2_v8_zext]], !dbg !4
; CHECK: [[memset2_v8_insert1:%[a-zA-Z0-9_.]+]] = insertelement <8 x i32> undef, i32 [[memset2_v8_ssrc]], i32 0, !dbg !4
; CHECK: [[memset2_v8_insert2:%[a-zA-Z0-9_.]+]] = insertelement <8 x i32> [[memset2_v8_insert1]], i32 [[memset2_v8_ssrc]], i32 1, !dbg !4
; CHECK: [[memset2_v8_insert3:%[a-zA-Z0-9_.]+]] = insertelement <8 x i32> [[memset2_v8_insert2]], i32 [[memset2_v8_ssrc]], i32 2, !dbg !4
; CHECK: [[memset2_v8_insert4:%[a-zA-Z0-9_.]+]] = insertelement <8 x i32> [[memset2_v8_insert3]], i32 [[memset2_v8_ssrc]], i32 3, !dbg !4
; CHECK: [[memset2_v8_insert5:%[a-zA-Z0-9_.]+]] = insertelement <8 x i32> [[memset2_v8_insert4]], i32 [[memset2_v8_ssrc]], i32 4, !dbg !4
; CHECK: [[memset2_v8_insert6:%[a-zA-Z0-9_.]+]] = insertelement <8 x i32> [[memset2_v8_insert5]], i32 [[memset2_v8_ssrc]], i32 5, !dbg !4
; CHECK: [[memset2_v8_insert7:%[a-zA-Z0-9_.]+]] = insertelement <8 x i32> [[memset2_v8_insert6]], i32 [[memset2_v8_ssrc]], i32 6, !dbg !4
; CHECK: [[memset2_v8_vsrc:%[a-zA-Z0-9_.]+]] = insertelement <8 x i32> [[memset2_v8_insert7]], i32 [[memset2_v8_ssrc]], i32 7, !dbg !4
; CHECK: [[memset2_v8_vdst:%[a-zA-Z0-9_.]+]] = bitcast i32 addrspace(1)* %dst to <8 x i32> addrspace(1)*, !dbg !4
; CHECK: [[memset2_v8_gep:%[a-zA-Z0-9_.]+]] = getelementptr <8 x i32>, <8 x i32> addrspace(1)* [[memset2_v8_vdst]], i32 0, !dbg !4
; CHECK: store <8 x i32> [[memset2_v8_vsrc]], <8 x i32> addrspace(1)* [[memset2_v8_gep]], align 16, !dbg !4

; CHECK: [[memset2_dst:%[a-zA-Z0-9_.]+]] = bitcast i32 addrspace(1)* %dst to i8 addrspace(1)*, !dbg !4

; CHECK: [[memset2_v4_gep:%[a-zA-Z0-9_.]+]] = getelementptr i8, i8 addrspace(1)* [[memset2_dst]], i32 32, !dbg !4
; CHECK: [[memset2_v4_zext:%[a-zA-Z0-9_.]+]] = zext i8 %src to i32, !dbg !4
; CHECK: [[memset2_v4_shl1:%[a-zA-Z0-9_.]+]] = shl i32 [[memset2_v4_zext]], 8, !dbg !4
; CHECK: [[memset2_v4_add1:%[a-zA-Z0-9_.]+]] = add i32 [[memset2_v4_shl1]], [[memset2_v4_zext]], !dbg !4
; CHECK: [[memset2_v4_shl2:%[a-zA-Z0-9_.]+]] = shl i32 [[memset2_v4_add1]], 8, !dbg !4
; CHECK: [[memset2_v4_add2:%[a-zA-Z0-9_.]+]] = add i32 [[memset2_v4_shl2]], [[memset2_v4_zext]], !dbg !4
; CHECK: [[memset2_v4_shl3:%[a-zA-Z0-9_.]+]] = shl i32 [[memset2_v4_add2]], 8, !dbg !4
; CHECK: [[memset2_v4_ssrc:%[a-zA-Z0-9_.]+]] = add i32 [[memset2_v4_shl3]], [[memset2_v4_zext]], !dbg !4
; CHECK: [[memset2_v4_insert1:%[a-zA-Z0-9_.]+]] = insertelement <4 x i32> undef, i32 [[memset2_v4_ssrc]], i32 0, !dbg !4
; CHECK: [[memset2_v4_insert2:%[a-zA-Z0-9_.]+]] = insertelement <4 x i32> [[memset2_v4_insert1]], i32 [[memset2_v4_ssrc]], i32 1, !dbg !4
; CHECK: [[memset2_v4_insert3:%[a-zA-Z0-9_.]+]] = insertelement <4 x i32> [[memset2_v4_insert2]], i32 [[memset2_v4_ssrc]], i32 2, !dbg !4
; CHECK: [[memset2_v4_vsrc:%[a-zA-Z0-9_.]+]] = insertelement <4 x i32> [[memset2_v4_insert3]], i32 [[memset2_v4_ssrc]], i32 3, !dbg !4
; CHECK: [[memset2_v4_vdst:%[a-zA-Z0-9_.]+]] = bitcast i8 addrspace(1)* [[memset2_v4_gep]] to <4 x i32> addrspace(1)*, !dbg !4
; CHECK: store <4 x i32> [[memset2_v4_vsrc]], <4 x i32> addrspace(1)* [[memset2_v4_vdst]], align 16, !dbg !4

; CHECK: [[memset2_v2_gep:%[a-zA-Z0-9_.]+]] = getelementptr i8, i8 addrspace(1)* [[memset2_dst]], i32 48, !dbg !4
; CHECK: [[memset2_v2_zext:%[a-zA-Z0-9_.]+]] = zext i8 %src to i32, !dbg !4
; CHECK: [[memset2_v2_shl1:%[a-zA-Z0-9_.]+]] = shl i32 [[memset2_v2_zext]], 8, !dbg !4
; CHECK: [[memset2_v2_add1:%[a-zA-Z0-9_.]+]] = add i32 [[memset2_v2_shl1]], [[memset2_v2_zext]], !dbg !4
; CHECK: [[memset2_v2_shl2:%[a-zA-Z0-9_.]+]] = shl i32 [[memset2_v2_add1]], 8, !dbg !4
; CHECK: [[memset2_v2_add2:%[a-zA-Z0-9_.]+]] = add i32 [[memset2_v2_shl2]], [[memset2_v2_zext]], !dbg !4
; CHECK: [[memset2_v2_shl3:%[a-zA-Z0-9_.]+]] = shl i32 [[memset2_v2_add2]], 8, !dbg !4
; CHECK: [[memset2_v2_ssrc:%[a-zA-Z0-9_.]+]] = add i32 [[memset2_v2_shl3]], [[memset2_v2_zext]], !dbg !4
; CHECK: [[memset2_v2_insert1:%[a-zA-Z0-9_.]+]] = insertelement <3 x i32> undef, i32 [[memset2_v2_ssrc]], i32 0, !dbg !4
; CHECK: [[memset2_v2_insert2:%[a-zA-Z0-9_.]+]] = insertelement <3 x i32> [[memset2_v2_insert1]], i32 [[memset2_v2_ssrc]], i32 1, !dbg !4
; CHECK: [[memset2_v2_vsrc:%[a-zA-Z0-9_.]+]] = insertelement <3 x i32> [[memset2_v2_insert2]], i32 [[memset2_v2_ssrc]], i32 2, !dbg !4
; CHECK: [[memset2_v2_vdst:%[a-zA-Z0-9_.]+]] = bitcast i8 addrspace(1)* [[memset2_v2_gep]] to <3 x i32> addrspace(1)*, !dbg !4
; CHECK: store <3 x i32> [[memset2_v2_vsrc]], <3 x i32> addrspace(1)* [[memset2_v2_vdst]], align 4, !dbg !4

; CHECK: [[memset2_c2_gep:%[a-zA-Z0-9_.]+]] = getelementptr i8, i8 addrspace(1)* [[memset_dst]], i32 60, !dbg !4
; CHECK: [[memset2_c2_insert1:%[a-zA-Z0-9_.]+]] = insertelement <2 x i8> undef, i8 %src, i32 0, !dbg !4
; CHECK: [[memset2_c2_vsrc:%[a-zA-Z0-9_.]+]] = insertelement <2 x i8> [[memset2_c2_insert1]], i8 %src, i32 1, !dbg !4
; CHECK: [[memset2_c2_vdst:%[a-zA-Z0-9_.]+]] = bitcast i8 addrspace(1)* [[memset2_c2_gep]] to <2 x i8> addrspace(1)*, !dbg !4
; CHECK: store <2 x i8> [[memset2_c2_vsrc]], <2 x i8> addrspace(1)* [[memset2_c2_vdst]], align 2, !dbg !4

; CHECK: [[memset2_c1_gep:%[a-zA-Z0-9_.]+]] = getelementptr i8, i8 addrspace(1)* [[memset_dst]], i32 62, !dbg !4
; CHECK: store i8 %src, i8 addrspace(1)* [[memset2_c1_gep]], align 1, !dbg !4
}

declare void @llvm.memcpy.p1i32.p1i32.i32(i32 addrspace(1)* nocapture, i32 addrspace(1)* nocapture, i32, i32, i1) #1

declare void @llvm.memset.p1i32.i32(i32 addrspace(1)* nocapture, i8, i32, i32, i1) #1

attributes #0 = { alwaysinline nounwind }
attributes #1 = { nounwind }

;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3, !4}

!0 = !{}
!1 = !{i32 5, i32 0, !0, null}
!2 = !{i32 9, i32 0, !0, null}
!3 = !{i32 13, i32 0, !0, null}
!4 = !{i32 17, i32 0, !0, null}
