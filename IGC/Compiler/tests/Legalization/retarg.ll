;=========================== begin_copyright_notice ============================
;
; Copyright (c) 2014-2021 Intel Corporation
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"),
; to deal in the Software without restriction, including without limitation
; the rights to use, copy, modify, merge, publish, distribute, sublicense,
; and/or sell copies of the Software, and to permit persons to whom
; the Software is furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included
; in all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
; FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
; IN THE SOFTWARE.
;
;============================ end_copyright_notice =============================

; RUN: igc_opt %s -S -o - -igc-type-legalizer -instcombine | FileCheck %s
; XFAIL: *
; As it's decided NOT to handle the case where return/argument values need
; promoting or expanding.

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define i3 @f1(i3 %a, i3 %b) {
  %r = add i3 %a, %b
  ret i3 %r
}

; CHECK-LABEL: define i3 @f1
; CHECK: %a.promote = call i8 @llvm.genx.GenISA.val.promote.i8.i3(i3 %a)
; CHECK: %b.promote = call i8 @llvm.genx.GenISA.val.promote.i8.i3(i3 %b)
; CHECK: %r.promote = add i8 %a.promote, %b.promote
; CHECK: %r.promote.demote = call i3 @llvm.genx.GenISA.val.demote.i3.i8(i8 %r.promote)
; CHECK: ret i3 %r.promote.demote

define i123 @f2(i123 %a, i123 %b) {
  %r = and i123 %a, %b
  ret i123 %r
}

; CHECK-LABEL: define i123 @f2
; CHECK: %a.ex0 = call i64 @llvm.genx.GenISA.val.expand.i64.i123(i123 %a, i32 0)
; CHECK: %a.ex1 = call i59 @llvm.genx.GenISA.val.expand.i59.i123(i123 %a, i32 1)
; CHECK: %a.ex1.promote = call i64 @llvm.genx.GenISA.val.promote.i64.i59(i59 %a.ex1)
; CHECK: %b.ex0 = call i64 @llvm.genx.GenISA.val.expand.i64.i123(i123 %b, i32 0)
; CHECK: %b.ex1 = call i59 @llvm.genx.GenISA.val.expand.i59.i123(i123 %b, i32 1)
; CHECK: %b.ex1.promote = call i64 @llvm.genx.GenISA.val.promote.i64.i59(i59 %b.ex1)
; CHECK: %r.ex0 = and i64 %a.ex0, %b.ex0
; CHECK: %r.ex1.promote = and i64 %a.ex1.promote, %b.ex1.promote
; CHECK: %r.ex1.promote.demote = call i59 @llvm.genx.GenISA.val.demote.i59.i64(i64 %r.ex1.promote)
; CHECK: %r.ex.compact = call i123 (...)* @llvm.genx.GenISA.val.compact.i123(i64 %r.ex0, i59 %r.ex1.promote.demote)
; CHECK: ret i123 %r.ex.compact
