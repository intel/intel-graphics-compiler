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

define void @foo(i3* %pc, i3* %pa, i3* %pb) {
  %a = load i3* %pa
  %b = load i3* %pb
  %r = add i3 %a, %b
  store i3 %r, i3* %pc
  ret void
}

; CHECK-LABEL: define void @foo
; CHECK: %a.promote0 = load i8* %pa.ptrcast
; CHECK: %b.promote0 = load i8* %pb.ptrcast
; CHECK: %r.promote = add i8 %a.promote0, %b.promote0
; CHECK: store i8 %r.promote, i8* %pc.ptrcast


define void @bar(i128* %pc, i128* %pa, i128* %pb) {
  %a = load i128* %pa
  %b = load i128* %pb
  %r = and i128 %a, %b
  store i128 %r, i128* %pc
  ret void
}

; CHECK-LABEL: define void @bar
; CHECK: %a.ex0 = load i64* %pa.ex0
; CHECK: %a.ex1 = load i64* %pa.ex1
; CHECK: %b.ex0 = load i64* %pb.ex0
; CHECK: %b.ex1 = load i64* %pb.ex1
; CHECK: %r.ex0 = and i64 %a.ex0, %b.ex0
; CHECK: %r.ex1 = and i64 %a.ex1, %b.ex1
; CHECK: store i64 %r.ex0, i64* %pc.ex0
; CHECK: store i64 %r.ex1, i64* %pc.ex1


define void @bax(i91* %pc, i91* %pa, i91* %pb) {
  %a = load i91* %pa
  %b = load i91* %pb
  %r = and i91 %a, %b
  store i91 %r, i91* %pc
  ret void
}

; CHECK-LABEL: define void @bax
; CHECK: %a.ex0 = load i64* %pa.ex0
; CHECK: %a.ex1.promote0 = load i32* %pa.ex1.ptrcast.ptrcast.ptrcast
; CHECK: %b.ex0 = load i64* %pb.ex0
; CHECK: %b.ex1.promote0 = load i32* %pb.ex1.ptrcast.ptrcast.ptrcast
; CHECK: %r.ex0 = and i64 %a.ex0, %b.ex0
; CHECK: %r.ex1.promote = and i32 %a.ex1.promote0, %b.ex1.promote0
; CHECK: store i64 %r.ex0, i64* %pc.ex0
; CHECK: store i32 %r.ex1.promote, i32* %pc.ex1.ptrcast.ptrcast.ptrcast


define i64 @promoteload(i56* %src) {
  %x = load i56* %src
  %r = zext i56 %x to i64
  ret i64 %r
}

; CHECK: define i64 @promoteload
; CHECK: %src.ptrcast = bitcast i56* %src to i8*
; CHECK: %src.ptrcast.ptrcast = bitcast i56* %src to i32*
; CHECK: %x.promote0 = load i32* %src.ptrcast.ptrcast, align 8
; CHECK: %x.promote0.zext = zext i32 %x.promote0 to i64
; CHECK: %src.ptrcast.off4 = getelementptr inbounds i8* %src.ptrcast, i32 4
; CHECK: %src.ptrcast.off4.ptrcast = bitcast i8* %src.ptrcast.off4 to i16*
; CHECK: %x.promote1 = load i16* %src.ptrcast.off4.ptrcast, align 4
; CHECK: %x.promote1.zext = zext i16 %x.promote1 to i64
; CHECK: %x.promote1.zext.shl = shl nuw nsw i64 %x.promote1.zext, 32
; CHECK: %x.promote1.zext.shl.concat = or i64 %x.promote0.zext, %x.promote1.zext.shl
; CHECK: %src.ptrcast.off6 = getelementptr inbounds i8* %src.ptrcast, i32 6
; CHECK: %x.promote2 = load i8* %src.ptrcast.off6, align 2
; CHECK: %x.promote2.zext = zext i8 %x.promote2 to i64
; CHECK: %x.promote2.zext.shl = shl nuw nsw i64 %x.promote2.zext, 48
; CHECK: %x.promote2.zext.shl.concat = or i64 %x.promote1.zext.shl.concat, %x.promote2.zext.shl
; CHECK: ret i64 %x.promote2.zext.shl.concat


define void @promotestore(i56* %dst, i64 %src) {
  %x = trunc i64 %src to i56
  store i56 %x, i56* %dst
  ret void
}

; CHECK: define void @promotestore
; CHECK: %dst.ptrcast = bitcast i56* %dst to i8*
; CHECK: %dst.ptrcast.ptrcast = bitcast i56* %dst to i32*
; CHECK: %src.trunc = trunc i64 %src to i32
; CHECK: store i32 %src.trunc, i32* %dst.ptrcast.ptrcast, align 8
; CHECK: %dst.ptrcast.off4 = getelementptr inbounds i8* %dst.ptrcast, i32 4
; CHECK: %dst.ptrcast.off4.ptrcast = bitcast i8* %dst.ptrcast.off4 to i16*
; CHECK: %src.lshr = lshr i64 %src, 32
; CHECK: %src.lshr.trunc = trunc i64 %src.lshr to i16
; CHECK: store i16 %src.lshr.trunc, i16* %dst.ptrcast.off4.ptrcast, align 4
; CHECK: %dst.ptrcast.off6 = getelementptr inbounds i8* %dst.ptrcast, i32 6
; CHECK: %src.lshr1 = lshr i64 %src, 48
; CHECK: %src.lshr1.trunc = trunc i64 %src.lshr1 to i8
; CHECK: store i8 %src.lshr1.trunc, i8* %dst.ptrcast.off6, align 2
; CHECK: ret void
