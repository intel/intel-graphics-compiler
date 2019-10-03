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
; RUN: igc_opt -igc-fix-alignment -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

target triple = "igil_32_GEN9"

; Function Attrs: alwaysinline nounwind
define void @alloca() #0 {
entry:
  %alloc = alloca <4 x i32>
; CHECK: align 16
  %val = load <4 x i32>, <4 x i32>* %alloc
  ret void
}

define void @param(i8* %byteptr, <4 x i8>* %dwordptr) #0 {
; CHECK: align 1
  %val1 = load i8, i8* %byteptr
; CHECK: align 4
  %val4 = load <4 x i8>, <4 x i8>* %dwordptr
  ret void
}

define void @bitcast_down() #0 {
entry:
  %alloc = alloca <4 x i32>
  %cast = bitcast <4 x i32>* %alloc to i8*
; CHECK: align 16
  %val = load i8, i8* %cast
  ret void
}

define void @bitcast_up() #0 {
entry:
  %alloc = alloca i8
  %cast = bitcast i8* %alloc to <4 x i32>*
; CHECK: align 1
  %val = load <4 x i32>, <4 x i32>* %cast
  ret void
}


define void @ptrtointtoptr() #0 {
entry:
  %alloc = alloca <4 x i32>
  %int = ptrtoint <4 x i32>* %alloc to i32
  %ptr = inttoptr i32 %int to i8*
; CHECK: align 16
  %val = load i8, i8* %ptr
  ret void
}

define void @add(i32 %unknown) #0 {
entry:
  %alloc = alloca <4 x i32>
  %int = ptrtoint <4 x i32>* %alloc to i32
  %add1 = add i32 %int, 1
  %ptr1 = inttoptr i32 %add1 to i8*
; CHECK: align 1
  %val1 = load i8, i8* %ptr1

  %add4 = add i32 %int, 4
  %ptr4 = inttoptr i32 %add4 to i8*
; CHECK: align 4
  %val4 = load i8, i8* %ptr4

  %add17 = add i32 %int, 17
  %ptr17 = inttoptr i32 %add17 to i8*
; CHECK: align 1
  %val17 = load i8, i8* %ptr17

  %add256 = add i32 %int, 256
  %ptr256 = inttoptr i32 %add256 to i8*
; CHECK: align 16
  %val256 = load i8, i8* %ptr256

  %addunk = add i32 %int, %unknown
  %ptrunk = inttoptr i32 %addunk to i8*
; CHECK: align 1
  %valunk = load i8, i8* %ptrunk
  ret void
}

define void @mul(i32 %unknown) #0 {
entry:
  %alloc = alloca <4 x i8>
  %int = ptrtoint <4 x i8>* %alloc to i32
  %mul1 = mul i32 %int, 1
  %ptr1 = inttoptr i32 %mul1 to i8*
; CHECK: align 4
  %val1 = load i8, i8* %ptr1

  %mul4 = mul i32 %int, 4
  %ptr4 = inttoptr i32 %mul4 to i8*
; CHECK: align 16
  %val4 = load i8, i8* %ptr4

  %mul17 = mul i32 %int, 17
  %ptr17 = inttoptr i32 %mul17 to i8*
; CHECK: align 4
  %val17 = load i8, i8* %ptr17

  %mul108 = mul i32 %int, 108
  %ptr108 = inttoptr i32 %mul108 to i8*
; CHECK: align 16
  %val108 = load i8, i8* %ptr108

  %mulunk = mul i32 %int, %unknown
  %ptrunk = inttoptr i32 %mulunk to i8*
; CHECK: align 4
  %valunk = load i8, i8* %ptrunk

  ret void
}

define void @muladd(i32* %src, i32 %offset)
{
  %int = ptrtoint i32* %src to i32
  %mul = mul i32 %offset, 16
  %add = add i32 %int, %mul
  %ptr = inttoptr i32 %add to i8*
; CHECK: align 4
  %val = load i8, i8* %ptr
  ret void
}

define void @shl(i32 %unknown) #0 {
entry:
  %alloc = alloca <4 x i8>
  %int = ptrtoint <4 x i8>* %alloc to i32
  %shl1 = shl i32 %int, 0
  %ptr1 = inttoptr i32 %shl1 to i8*
; CHECK: align 4
  %val1 = load i8, i8* %ptr1

  %shl2 = shl i32 %int, 2
  %ptr4 = inttoptr i32 %shl2 to i8*
; CHECK: align 16
  %val4 = load i8, i8* %ptr4

  %shlunk = shl i32 %int, %unknown
  %ptrunk = inttoptr i32 %shlunk to i8*
; CHECK: align 4
  %valunk = load i8, i8* %ptrunk

  ret void
}

define void @and (i8* %ptr) #0 {
entry:
  %int = ptrtoint i8* %ptr to i32
  ; ~0x03
  %and3 = and i32 %int, 4294967292
  %ptr3 = inttoptr i32 %and3 to <16 x i8>*
; CHECK: align 4
  %val3 = load <16 x i8>, <16 x i8>* %ptr3

  %andsilly = and i32 %int, 255
  %ptrsilly = inttoptr i32 %andsilly to <16 x i8>*
; CHECK: align 1
  %valsilly = load <16 x i8>, <16 x i8>* %ptrsilly

  ret void
}

define void @select(<4 x i8>* %char4ptr, <16 x i8>* %char16ptr, i1 %flag) #0 {
  %cast = bitcast <4 x i8>* %char4ptr to <16 x i8>*
  %ptr = select i1 %flag, <16 x i8>* %char16ptr, <16 x i8>* %cast
; CHECK: align 4
  %val = load <16 x i8>, <16 x i8>* %ptr
  ret void
}

define void @loadval(<4 x i8>* %src, i32* %offset) #0 {
; CHECK: align 4
  %off = load i32, i32* %offset
  %int = ptrtoint <4 x i8>* %src to i32
  %add = add i32 %int, %off
  %ptr = inttoptr i32 %add to <4 x i8>*
; CHECK: align 1
  %val = load <4 x i8>, <4 x i8>* %ptr
  ret void
}


define void @simplephi(<16 x i8>* %src, i1 %flag) #0 {
  %int = ptrtoint <16 x i8>* %src to i32
  br i1 %flag, label %true, label %false

true:
  %add4 = add i32 %int, 4
  %ptr4 = inttoptr i32 %add4 to i8*
; CHECK: align 4
  %val4 = load i8, i8* %ptr4
  br label %end

false:
  %add16 = add i32 %int, 16
  %ptr16 = inttoptr i32 %add16 to i8*
; CHECK: align 16
  %val16 = load i8, i8* %ptr16
  br label %end

end:
  %p = phi i8* [ %ptr4, %true], [ %ptr16, %false]
; CHECK: align 4
  store i8 0, i8* %p
  ret void
}

define void @loop(<4 x i8>* %src, i1 %flag) #0 {
entry:
  br label %body

body:
  %reduced = phi <4 x i8>* [ %src, %entry ], [ %addptr, %body ]
  %improved = phi <4 x i8>* [ %src, %entry ], [ %mulptr, %body ]
; CHECK: align 1
  %valreduced = load <4 x i8>, <4 x i8>* %reduced
; CHECK: align 4
  %valimproved = load <4 x i8>, <4 x i8>* %improved
  %int = ptrtoint <4 x i8>* %improved to i32
  %add = add i32 %int, 1
  %addptr = inttoptr i32 %add to <4 x i8>*
  %mul = mul i32 %int, 32
  %mulptr = inttoptr i32 %mul to <4 x i8>*
  br i1 %flag, label %body, label %end

end:
  ret void
}

define void @gep1(<4 x i8>* %src, i32 %offset) {
  %ptr = getelementptr <4 x i8>, <4 x i8>* %src, i32 %offset
; CHECK: align 4
  %val = load <4 x i8>, <4 x i8>* %ptr
  ret void
}

define void @gep2(<4 x i8>* %src, i32 %offset) {
  %bytesrc = bitcast <4 x i8>* %src to i8*
  %ptr = getelementptr i8, i8* %bytesrc, i32 %offset
; CHECK: align 1
  %val = load i8, i8* %ptr
  ret void
}

define void @gep3(<4 x i8>* %src) {
  %bytesrc = bitcast <4 x i8>* %src to i8*
  %ptr = getelementptr i8, i8* %bytesrc, i32 4
; CHECK: align 4
  %val = load i8, i8* %ptr
  ret void
}

@globalarr = addrspace(3) global [10 x i32] [i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9], align 16
define void @globalgep() {
  %start = getelementptr [10 x i32], [10 x i32] addrspace(3)* @globalarr, i32 0, i32 0
; CHECK: align 16
  %val = load i32, i32 addrspace(3)* %start
  ret void
}

attributes #0 = { alwaysinline nounwind }
