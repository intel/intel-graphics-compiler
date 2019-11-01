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
; This test is expected to fail on LLVM7.
; XFAIL: *
; RUN: igc_opt -igc-replace-unsupported-intrinsics -verify -S %s -o %t
; RUN: FileCheck %s < %t

target triple = "igil_32_GEN8"

define void @test_fshl(i8 addrspace(1)* %res, i8 %a, i8 %b, i8 %c) #0 {
entry:
; CHECK-LABEL: define void @test_fshl
; CHECK:      [[modRes:%[0-9]+]] = urem i8 %c, 8
; CHECK:      [[subRes:%[0-9]+]] = sub i8 8, [[modRes]]
; CHECK:      [[shlRes:%[0-9]+]] = shl i8 %a, [[modRes]]
; CHECK:      [[shrRes:%[0-9]+]] = lshr i8 %b, [[subRes]]
; CHECK:      [[orRes:%[0-9]+]] = or i8 [[shlRes]], [[shrRes]]
; CHECK:      store i8 [[orRes]], i8 addrspace(1)* %res
; CHECK-NOT:  call {{.*}} @llvm.fshl

  %r = call i8 @llvm.fshl.i8(i8 %a, i8 %b, i8 %c)
  store i8 %r, i8 addrspace(1)* %res
  ret void
}

define void @test_fshr(i32 addrspace(1)* %res, i32 %a, i32 %b, i32 %c) #0 {
entry:
; CHECK-LABEL: define void @test_fshr
; CHECK:      [[modRes:%[0-9]+]] = urem i32 %c, 32
; CHECK:      [[subRes:%[0-9]+]] = sub i32 32, [[modRes]]
; CHECK:      [[shlRes:%[0-9]+]] = shl i32 %a, [[subRes]]
; CHECK:      [[shrRes:%[0-9]+]] = lshr i32 %b, [[modRes]]
; CHECK:      [[orRes:%[0-9]+]] = or i32 [[shlRes]], [[shrRes]]
; CHECK:      store i32 [[orRes]], i32 addrspace(1)* %res
; CHECK-NOT:  call {{.*}} @llvm.fshr

  %r = call i32 @llvm.fshr.i32(i32 %a, i32 %b, i32 %c)
  store i32 %r, i32 addrspace(1)* %res
  ret void
}

define void @test_fshl_vector(<2 x i64> addrspace(1)* %res, <2 x i64> %a, <2 x i64> %b, <2 x i64> %c) #0 {
entry:
; CHECK-LABEL: define void @test_fshl_vector
; CHECK:      [[modRes:%[0-9]+]] = urem <2 x i64> %c, <i64 64, i64 64>
; CHECK:      [[subRes:%[0-9]+]] = sub <2 x i64> <i64 64, i64 64>, [[modRes]]
; CHECK:      [[shlRes:%[0-9]+]] = shl <2 x i64> %a, [[modRes]]
; CHECK:      [[shrRes:%[0-9]+]] = lshr <2 x i64> %b, [[subRes]]
; CHECK:      [[orRes:%[0-9]+]] = or <2 x i64> [[shlRes]], [[shrRes]]
; CHECK:      store <2 x i64> [[orRes]], <2 x i64> addrspace(1)* %res
; CHECK-NOT:  call {{.*}} @llvm.fshl

  %r = call <2 x i64> @llvm.fshl.v2i64(<2 x i64> %a, <2 x i64> %b, <2 x i64> %c)
  store <2 x i64> %r, <2 x i64> addrspace(1)* %res
  ret void
}

declare i8 @llvm.fshl.i8(i8, i8, i8) #2
declare i32 @llvm.fshr.i32(i32, i32, i32) #2
declare <2 x i64> @llvm.fshl.v2i64(<2 x i64>, <2 x i64>, <2 x i64>) #2

attributes #0 = { alwaysinline nounwind }
attributes #1 = { nounwind }
attributes #2 = { nounwind readnone speculatable }
