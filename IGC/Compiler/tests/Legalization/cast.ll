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
; RUN: igc_opt %s -S -o - -igc-type-legalizer -instcombine | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

; zext/sext
; dst\src legal promote expand
; legal   N.A.  1       N.A.
; promote 3     4       N.A.
; expand  6     7       8

define void @zext1(i32* %dst, i5* %src) {
  %x = load i5* %src
  %zx = zext i5 %x to i32
  store i32 %zx, i32* %dst
  ret void
}

; CHECK-LABEL: define void @zext1
; CHECK: %src.ptrcast = bitcast i5* %src to i8*
; CHECK: %x.promote0 = load i8* %src.ptrcast, align 1
; CHECK: %x.promote0.concat.zext = and i8 %x.promote0, 31
; CHECK: %zx = zext i8 %x.promote0.concat.zext to i32
; CHECK: store i32 %zx, i32* %dst, align 4
; CHECK: ret void


define void @zext3(i27* %dst, i8* %src) {
  %x = load i8* %src
  %zx = zext i8 %x to i27
  store i27 %zx, i27* %dst
  ret void
}

; CHECK-LABEL: define void @zext3
; CHECK: %x = load i8* %src, align 1
; CHECK: %zx.promote = zext i8 %x to i32
; CHECK: %dst.ptrcast.ptrcast = bitcast i27* %dst to i32*
; CHECK: store i32 %zx.promote, i32* %dst.ptrcast.ptrcast, align 4
; CHECK: ret void


define void @zext4(i27* %dst, i5* %src) {
  %x = load i5* %src
  %zx = zext i5 %x to i27
  store i27 %zx, i27* %dst
  ret void
}

; CHECK-LABEL: define void @zext4
; CHECK: %src.ptrcast = bitcast i5* %src to i8*
; CHECK: %x.promote0 = load i8* %src.ptrcast, align 1
; CHECK: %x.promote0.concat.zext = and i8 %x.promote0, 31
; CHECK: %zx.promote = zext i8 %x.promote0.concat.zext to i32
; CHECK: %dst.ptrcast.ptrcast = bitcast i27* %dst to i32*
; CHECK: store i32 %zx.promote, i32* %dst.ptrcast.ptrcast, align 4
; CHECK: ret void


define void @zext6(i91* %dst, i8* %src) {
  %x = load i8* %src
  %zx = zext i8 %x to i91
  store i91 %zx, i91* %dst
  ret void
}

; CHECK-LABEL: define void @zext6
; CHECK: %x = load i8* %src, align 1
; CHECK: %x.zext = zext i8 %x to i64
; CHECK: %dst.ex0 = bitcast i91* %dst to i64*
; CHECK: store i64 %x.zext, i64* %dst.ex0, align 8
; CHECK: %dst.ex1 = getelementptr inbounds i64* %dst.ex0, i32 1
; CHECK: %dst.ex1.ptrcast.ptrcast.ptrcast = bitcast i64* %dst.ex1 to i32*
; CHECK: store i32 0, i32* %dst.ex1.ptrcast.ptrcast.ptrcast, align 8
; CHECK: ret void


define void @zext7(i91* %dst, i5* %src) {
  %x = load i5* %src
  %zx = zext i5 %x to i91
  store i91 %zx, i91* %dst
  ret void
}

; CHECK-LABEL: define void @zext7
; CHECK: %src.ptrcast = bitcast i5* %src to i8*
; CHECK: %x.promote0 = load i8* %src.ptrcast, align 1
; CHECK: %x.promote0.concat.zext = and i8 %x.promote0, 31
; CHECK: %x.promote0.concat.zext.zext = zext i8 %x.promote0.concat.zext to i64
; CHECK: %dst.ex0 = bitcast i91* %dst to i64*
; CHECK: store i64 %x.promote0.concat.zext.zext, i64* %dst.ex0, align 8
; CHECK: %dst.ex1 = getelementptr inbounds i64* %dst.ex0, i32 1
; CHECK: %dst.ex1.ptrcast.ptrcast.ptrcast = bitcast i64* %dst.ex1 to i32*
; CHECK: store i32 0, i32* %dst.ex1.ptrcast.ptrcast.ptrcast, align 8
; CHECK: ret void


define void @zext8(i91* %dst, i65* %src) {
  %x = load i65* %src
  %zx = zext i65 %x to i91
  store i91 %zx, i91* %dst
  ret void
}

; CHECK-LABEL: define void @zext8
; CHECK: %src.ex0 = bitcast i65* %src to i64*
; CHECK: %x.ex0 = load i64* %src.ex0, align 8
; CHECK: %src.ex1 = getelementptr inbounds i64* %src.ex0, i32 1
; CHECK: %src.ex1.ptrcast = bitcast i64* %src.ex1 to i1*
; CHECK: %x.ex1 = load i1* %src.ex1.ptrcast, align 8
; CHECK: %x.ex1.zext.promote = zext i1 %x.ex1 to i32
; CHECK: %dst.ex0 = bitcast i91* %dst to i64*
; CHECK: store i64 %x.ex0, i64* %dst.ex0, align 8
; CHECK: %dst.ex1 = getelementptr inbounds i64* %dst.ex0, i32 1
; CHECK: %dst.ex1.ptrcast.ptrcast.ptrcast = bitcast i64* %dst.ex1 to i32*
; CHECK: store i32 %x.ex1.zext.promote, i32* %dst.ex1.ptrcast.ptrcast.ptrcast, align 8
; CHECK: ret void


; trunc
; dst\src legal promote expand
; legal   N.A.  1       2    
; promote 3     4       5   
; expand  N.A.  N.A     8

define void @trunc1(i32* %dst, i55* %src) {
  %x = load i55* %src
  %tx = trunc i55 %x to i32
  store i32 %tx, i32* %dst
  ret void
}

; CHECK-LABEL: define void @trunc1
; CHECK: %src.ptrcast.ptrcast = bitcast i55* %src to i32*
; CHECK: %x.promote0 = load i32* %src.ptrcast.ptrcast, align 8
; CHECK: store i32 %x.promote0, i32* %dst, align 4
; CHECK: ret void


define void @trunc2(i32* %dst, i87* %src) {
  %x = load i87* %src
  %tx = trunc i87 %x to i32
  store i32 %tx, i32* %dst
  ret void
}

; CHECK-LABEL: define void @trunc2
; CHECK: %src.ex0 = bitcast i87* %src to i64*
; CHECK: %x.ex0 = load i64* %src.ex0, align 8
; CHECK: %tx = trunc i64 %x.ex0 to i32
; CHECK: store i32 %tx, i32* %dst, align 4
; CHECK: ret void


define void @trunc3(i3* %dst, i32* %src) {
  %x = load i32* %src
  %tx = trunc i32 %x to i3
  store i3 %tx, i3* %dst
  ret void
}

; CHECK-LABEL: define void @trunc3
; CHECK: %x = load i32* %src, align 4
; CHECK: %tx.promote = trunc i32 %x to i8
; CHECK: %dst.ptrcast = bitcast i3* %dst to i8*
; CHECK: store i8 %tx.promote, i8* %dst.ptrcast, align 1
; CHECK: ret void


define void @trunc4(i3* %dst, i27* %src) {
  %x = load i27* %src
  %tx = trunc i27 %x to i3
  store i3 %tx, i3* %dst
  ret void
}

; CHECK-LABEL: define void @trunc4
; CHECK: %src.ptrcast.ptrcast = bitcast i27* %src to i32*
; CHECK: %x.promote0 = load i32* %src.ptrcast.ptrcast, align 4
; CHECK: %tx.promote = trunc i32 %x.promote0 to i8
; CHECK: %dst.ptrcast = bitcast i3* %dst to i8*
; CHECK: store i8 %tx.promote, i8* %dst.ptrcast, align 1
; CHECK: ret void


define void @trunc5(i3* %dst, i87* %src) {
  %x = load i87* %src
  %tx = trunc i87 %x to i3
  store i3 %tx, i3* %dst
  ret void
}

; CHECK-LABEL: define void @trunc5
; CHECK: %src.ex0 = bitcast i87* %src to i64*
; CHECK: %x.ex0 = load i64* %src.ex0, align 8
; CHECK: %tx.promote = trunc i64 %x.ex0 to i8
; CHECK: %dst.ptrcast = bitcast i3* %dst to i8*
; CHECK: store i8 %tx.promote, i8* %dst.ptrcast, align 1
; CHECK: ret void


define void @trunc8(i65* %dst, i87* %src) {
  %x = load i87* %src
  %tx = trunc i87 %x to i65
  store i65 %tx, i65* %dst
  ret void
}

; CHECK-LABEL: define void @trunc8
; CHECK: %src.ex0 = bitcast i87* %src to i64*
; CHECK: %x.ex0 = load i64* %src.ex0, align 8
; CHECK: %src.ex1 = getelementptr inbounds i64* %src.ex0, i32 1
; CHECK: %src.ex1.ptrcast.ptrcast.ptrcast = bitcast i64* %src.ex1 to i16*
; CHECK: %x.ex1.promote0 = load i16* %src.ex1.ptrcast.ptrcast.ptrcast, align 8
; CHECK: %1 = and i16 %x.ex1.promote0, 1
; CHECK: %tx.ex1 = icmp ne i16 %1, 0
; CHECK: %dst.ex0 = bitcast i65* %dst to i64*
; CHECK: store i64 %x.ex0, i64* %dst.ex0, align 8
; CHECK: %dst.ex1 = getelementptr inbounds i64* %dst.ex0, i32 1
; CHECK: %dst.ex1.ptrcast = bitcast i64* %dst.ex1 to i1*
; CHECK: store i1 %tx.ex1, i1* %dst.ex1.ptrcast, align 8
; CHECK: ret void
