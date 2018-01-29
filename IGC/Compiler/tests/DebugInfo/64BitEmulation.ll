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
; RUN: opt -igc-resolve-64bit -S %s -o - | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This LIT test checks that 64BitEmulation pass handles line debug info.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a64:64:64-f80:128:128-n8:16:32:64"
target triple = "igil_32_GEN8"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; sdiv cases for these types: long, long2, long3, long4, long8, long16
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define void @test_sdiv_1(i64* %dst, i64 %src1, i64 %src2) #0 {
  %res = sdiv i64 %src1, %src2, !dbg !1
  store i64 %res, i64* %dst, align 8, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = sdiv
; CHECK: [[res_sdiv_1:%[a-zA-Z0-9]+]] = call i64 @_Z18__builtin_IB_sdivll(i64 %src1, i64 %src2), !dbg !1
}

define void @test_sdiv_2(<2 x i64>* %dst, <2 x i64> %src1, <2 x i64> %src2) #0 {
  %res = sdiv <2 x i64> %src1, %src2, !dbg !1
  store <2 x i64> %res, <2 x i64>* %dst, align 16, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = sdiv
; CHECK: [[res_sdiv_2:%[a-zA-Z0-9]+]] = call <2 x i64> @_Z18__builtin_IB_sdivDv2_lS_(<2 x i64> %src1, <2 x i64> %src2), !dbg !1
}

define void @test_sdiv_3(<3 x i64>* %dst, <3 x i64> %src1, <3 x i64> %src2) #0 {
  %res = sdiv <3 x i64> %src1, %src2, !dbg !1
  store <3 x i64> %res, <3 x i64>* %dst, align 32, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = sdiv
; CHECK: [[res_sdiv_3:%[a-zA-Z0-9]+]] = call <3 x i64> @_Z18__builtin_IB_sdivDv3_lS_(<3 x i64> %src1, <3 x i64> %src2), !dbg !1
}

define void @test_sdiv_4(<4 x i64>* %dst, <4 x i64> %src1, <4 x i64> %src2) #0 {
  %res = sdiv <4 x i64> %src1, %src2, !dbg !1
  store <4 x i64> %res, <4 x i64>* %dst, align 32, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = sdiv
; CHECK: [[res_sdiv_4:%[a-zA-Z0-9]+]] = call <4 x i64> @_Z18__builtin_IB_sdivDv4_lS_(<4 x i64> %src1, <4 x i64> %src2), !dbg !1
}

define void @test_sdiv_8(<8 x i64>* %dst, <8 x i64> %src1, <8 x i64> %src2) #0 {
  %res = sdiv <8 x i64> %src1, %src2, !dbg !1
  store <8 x i64> %res, <8 x i64>* %dst, align 64, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = sdiv
; CHECK: [[res_sdiv_8:%[a-zA-Z0-9]+]] = call <8 x i64> @_Z18__builtin_IB_sdivDv8_lS_(<8 x i64> %src1, <8 x i64> %src2), !dbg !1
}

define void @test_sdiv_16(<16 x i64>* %dst, <16 x i64> %src1, <16 x i64> %src2) #0 {
  %res = sdiv <16 x i64> %src1, %src2, !dbg !1
  store <16 x i64> %res, <16 x i64>* %dst, align 128, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = sdiv
; CHECK: [[res_sdiv_16:%[a-zA-Z0-9]+]] = call <16 x i64> @_Z18__builtin_IB_sdivDv16_lS_(<16 x i64> %src1, <16 x i64> %src2), !dbg !1
}


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; udiv cases for these types: long, long2, long3, long4, long8, long16
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define void @test_udiv_1(i64* %dst, i64 %src1, i64 %src2) #0 {
  %res = udiv i64 %src1, %src2, !dbg !1
  store i64 %res, i64* %dst, align 8, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = udiv
; CHECK: [[res_udiv_1:%[a-zA-Z0-9]+]] = call i64 @_Z18__builtin_IB_udivmm(i64 %src1, i64 %src2), !dbg !1
}

define void @test_udiv_2(<2 x i64>* %dst, <2 x i64> %src1, <2 x i64> %src2) #0 {
  %res = udiv <2 x i64> %src1, %src2, !dbg !1
  store <2 x i64> %res, <2 x i64>* %dst, align 16, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = udiv
; CHECK: [[res_udiv_2:%[a-zA-Z0-9]+]] = call <2 x i64> @_Z18__builtin_IB_udivDv2_mS_(<2 x i64> %src1, <2 x i64> %src2), !dbg !1
}

define void @test_udiv_3(<3 x i64>* %dst, <3 x i64> %src1, <3 x i64> %src2) #0 {
  %res = udiv <3 x i64> %src1, %src2, !dbg !1
  store <3 x i64> %res, <3 x i64>* %dst, align 32, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = udiv
; CHECK: [[res_udiv_3:%[a-zA-Z0-9]+]] = call <3 x i64> @_Z18__builtin_IB_udivDv3_mS_(<3 x i64> %src1, <3 x i64> %src2), !dbg !1
}

define void @test_udiv_4(<4 x i64>* %dst, <4 x i64> %src1, <4 x i64> %src2) #0 {
  %res = udiv <4 x i64> %src1, %src2, !dbg !1
  store <4 x i64> %res, <4 x i64>* %dst, align 32, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = udiv
; CHECK: [[res_udiv_4:%[a-zA-Z0-9]+]] = call <4 x i64> @_Z18__builtin_IB_udivDv4_mS_(<4 x i64> %src1, <4 x i64> %src2), !dbg !1
}

define void @test_udiv_8(<8 x i64>* %dst, <8 x i64> %src1, <8 x i64> %src2) #0 {
  %res = udiv <8 x i64> %src1, %src2, !dbg !1
  store <8 x i64> %res, <8 x i64>* %dst, align 64, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = udiv
; CHECK: [[res_udiv_8:%[a-zA-Z0-9]+]] = call <8 x i64> @_Z18__builtin_IB_udivDv8_mS_(<8 x i64> %src1, <8 x i64> %src2), !dbg !1
}

define void @test_udiv_16(<16 x i64>* %dst, <16 x i64> %src1, <16 x i64> %src2) #0 {
  %res = udiv <16 x i64> %src1, %src2, !dbg !1
  store <16 x i64> %res, <16 x i64>* %dst, align 128, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = udiv
; CHECK: [[res_udiv_16:%[a-zA-Z0-9]+]] = call <16 x i64> @_Z18__builtin_IB_udivDv16_mS_(<16 x i64> %src1, <16 x i64> %src2), !dbg !1
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; srem cases for these types: long, long2, long3, long4, long8, long16
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define void @test_srem_1(i64* %dst, i64 %src1, i64 %src2) #0 {
  %res = srem i64 %src1, %src2, !dbg !1
  store i64 %res, i64* %dst, align 8, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = srem
; CHECK: [[res_srem_1:%[a-zA-Z0-9]+]] = call i64 @_Z18__builtin_IB_smodll(i64 %src1, i64 %src2), !dbg !1
}

define void @test_srem_2(<2 x i64>* %dst, <2 x i64> %src1, <2 x i64> %src2) #0 {
  %res = srem <2 x i64> %src1, %src2, !dbg !1
  store <2 x i64> %res, <2 x i64>* %dst, align 16, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = srem
; CHECK: [[res_srem_2:%[a-zA-Z0-9]+]] = call <2 x i64> @_Z18__builtin_IB_smodDv2_lS_(<2 x i64> %src1, <2 x i64> %src2), !dbg !1
}

define void @test_srem_3(<3 x i64>* %dst, <3 x i64> %src1, <3 x i64> %src2) #0 {
  %res = srem <3 x i64> %src1, %src2, !dbg !1
  store <3 x i64> %res, <3 x i64>* %dst, align 32, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = srem
; CHECK: [[res_srem_3:%[a-zA-Z0-9]+]] = call <3 x i64> @_Z18__builtin_IB_smodDv3_lS_(<3 x i64> %src1, <3 x i64> %src2), !dbg !1
}

define void @test_srem_4(<4 x i64>* %dst, <4 x i64> %src1, <4 x i64> %src2) #0 {
  %res = srem <4 x i64> %src1, %src2, !dbg !1
  store <4 x i64> %res, <4 x i64>* %dst, align 32, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = srem
; CHECK: [[res_srem_4:%[a-zA-Z0-9]+]] = call <4 x i64> @_Z18__builtin_IB_smodDv4_lS_(<4 x i64> %src1, <4 x i64> %src2), !dbg !1
}

define void @test_srem_8(<8 x i64>* %dst, <8 x i64> %src1, <8 x i64> %src2) #0 {
  %res = srem <8 x i64> %src1, %src2, !dbg !1
  store <8 x i64> %res, <8 x i64>* %dst, align 64, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = srem
; CHECK: [[res_srem_8:%[a-zA-Z0-9]+]] = call <8 x i64> @_Z18__builtin_IB_smodDv8_lS_(<8 x i64> %src1, <8 x i64> %src2), !dbg !1
}

define void @test_srem_16(<16 x i64>* %dst, <16 x i64> %src1, <16 x i64> %src2) #0 {
  %res = srem <16 x i64> %src1, %src2, !dbg !1
  store <16 x i64> %res, <16 x i64>* %dst, align 128, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = srem
; CHECK: [[res_srem_16:%[a-zA-Z0-9]+]] = call <16 x i64> @_Z18__builtin_IB_smodDv16_lS_(<16 x i64> %src1, <16 x i64> %src2), !dbg !1
}


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; urem cases for these types: long, long2, long3, long4, long8, long16
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define void @test_urem_1(i64* %dst, i64 %src1, i64 %src2) #0 {
  %res = urem i64 %src1, %src2, !dbg !1
  store i64 %res, i64* %dst, align 8, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = urem
; CHECK: [[res_urem_1:%[a-zA-Z0-9]+]] = call i64 @_Z18__builtin_IB_umodmm(i64 %src1, i64 %src2), !dbg !1
}

define void @test_urem_2(<2 x i64>* %dst, <2 x i64> %src1, <2 x i64> %src2) #0 {
  %res = urem <2 x i64> %src1, %src2, !dbg !1
  store <2 x i64> %res, <2 x i64>* %dst, align 16, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = urem
; CHECK: [[res_urem_2:%[a-zA-Z0-9]+]] = call <2 x i64> @_Z18__builtin_IB_umodDv2_mS_(<2 x i64> %src1, <2 x i64> %src2), !dbg !1
}

define void @test_urem_3(<3 x i64>* %dst, <3 x i64> %src1, <3 x i64> %src2) #0 {
  %res = urem <3 x i64> %src1, %src2, !dbg !1
  store <3 x i64> %res, <3 x i64>* %dst, align 32, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = urem
; CHECK: [[res_urem_3:%[a-zA-Z0-9]+]] = call <3 x i64> @_Z18__builtin_IB_umodDv3_mS_(<3 x i64> %src1, <3 x i64> %src2), !dbg !1
}

define void @test_urem_4(<4 x i64>* %dst, <4 x i64> %src1, <4 x i64> %src2) #0 {
  %res = urem <4 x i64> %src1, %src2, !dbg !1
  store <4 x i64> %res, <4 x i64>* %dst, align 32, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = urem
; CHECK: [[res_urem_4:%[a-zA-Z0-9]+]] = call <4 x i64> @_Z18__builtin_IB_umodDv4_mS_(<4 x i64> %src1, <4 x i64> %src2), !dbg !1
}

define void @test_urem_8(<8 x i64>* %dst, <8 x i64> %src1, <8 x i64> %src2) #0 {
  %res = urem <8 x i64> %src1, %src2, !dbg !1
  store <8 x i64> %res, <8 x i64>* %dst, align 64, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = urem
; CHECK: [[res_urem_8:%[a-zA-Z0-9]+]] = call <8 x i64> @_Z18__builtin_IB_umodDv8_mS_(<8 x i64> %src1, <8 x i64> %src2), !dbg !1
}

define void @test_urem_16(<16 x i64>* %dst, <16 x i64> %src1, <16 x i64> %src2) #0 {
  %res = urem <16 x i64> %src1, %src2, !dbg !1
  store <16 x i64> %res, <16 x i64>* %dst, align 128, !dbg !2
  ret void, !dbg !3

; CHECK-NOT: = urem
; CHECK: [[res_urem_16:%[a-zA-Z0-9]+]] = call <16 x i64> @_Z18__builtin_IB_umodDv16_mS_(<16 x i64> %src1, <16 x i64> %src2), !dbg !1
}

attributes #0 = { nounwind }

;; This hack named metadata is needed to assure metadata order
!hack_order = !{!0, !1, !2, !3}

!0 = metadata !{}
!1 = metadata !{i32 5, i32 0, metadata !0, null}
!2 = metadata !{i32 6, i32 0, metadata !0, null}
!3 = metadata !{i32 7, i32 0, metadata !0, null}
