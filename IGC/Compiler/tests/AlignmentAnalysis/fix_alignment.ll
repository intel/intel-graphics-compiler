;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-fix-alignment -S %s -o %t
; RUN: FileCheck %s --input-file=%t

target datalayout = "e-p:32:32-p1:64:64-p2:64:64-p3:32:32-p4:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256"
target triple = "spir64-unknown-unknown"

; Function Attrs: alwaysinline nounwind
; CHECK-LABEL: @alloca
define void @alloca() #0 {
entry:
  %alloc = alloca <4 x i32>
; CHECK: load
; CHECK: align 16
  %val = load <4 x i32>, ptr %alloc
  ret void
}

; CHECK-LABEL: @param
define void @param(ptr align 1 %byteptr, ptr align 4 %dwordptr) #0 {
; CHECK: load
; CHECK: align 1
  %val1 = load i8, ptr %byteptr
; CHECK: load
; CHECK: align 4
  %val4 = load <4 x i8>, ptr %dwordptr
  ret void
}

; CHECK-LABEL: @ptrtointtoptr
define void @ptrtointtoptr() #0 {
entry:
  %alloc = alloca <4 x i32>
  %int = ptrtoint ptr %alloc to i32
  %ptr = inttoptr i32 %int to ptr
; CHECK: load
; CHECK: align 16
  %val = load i8, ptr %ptr
  ret void
}

; CHECK-LABEL: @add
define void @add(i32 %unknown) #0 {
entry:
  %alloc = alloca <4 x i32>
  %int = ptrtoint ptr %alloc to i32
  %add1 = add i32 %int, 1
  %ptr1 = inttoptr i32 %add1 to ptr
; CHECK: load
; CHECK: align 1
  %val1 = load i8, ptr %ptr1

  %add4 = add i32 %int, 4
  %ptr4 = inttoptr i32 %add4 to ptr
; CHECK: load
; CHECK: align 4
  %val4 = load i8, ptr %ptr4

  %add17 = add i32 %int, 17
  %ptr17 = inttoptr i32 %add17 to ptr
; CHECK: load
; CHECK: align 1
  %val17 = load i8, ptr %ptr17

  %add256 = add i32 %int, 256
  %ptr256 = inttoptr i32 %add256 to ptr
; CHECK: load
; CHECK: align 16
  %val256 = load i8, ptr %ptr256

  %addunk = add i32 %int, %unknown
  %ptrunk = inttoptr i32 %addunk to ptr
; CHECK: load
; CHECK: align 1
  %valunk = load i8, ptr %ptrunk
  ret void
}

; CHECK-LABEL: @mul
define void @mul(i32 %unknown) #0 {
entry:
  %alloc = alloca <4 x i8>
  %int = ptrtoint ptr %alloc to i32
  %mul1 = mul i32 %int, 1
  %ptr1 = inttoptr i32 %mul1 to ptr
; CHECK: load
; CHECK: align {{1|4}}
  %val1 = load i8, ptr %ptr1

  %mul4 = mul i32 %int, 4
  %ptr4 = inttoptr i32 %mul4 to ptr
; CHECK: load
; CHECK: align {{1|16}}
  %val4 = load i8, ptr %ptr4

  %mul17 = mul i32 %int, 17
  %ptr17 = inttoptr i32 %mul17 to ptr
; CHECK: load
; CHECK: align {{1|4}}
  %val17 = load i8, ptr %ptr17

  %mul108 = mul i32 %int, 108
  %ptr108 = inttoptr i32 %mul108 to ptr
; CHECK: load
; CHECK: align {{1|16}}
  %val108 = load i8, ptr %ptr108

  %mulunk = mul i32 %int, %unknown
  %ptrunk = inttoptr i32 %mulunk to ptr
; CHECK: load
; CHECK: align {{1|4}}
  %valunk = load i8, ptr %ptrunk

  ret void
}

; CHECK-LABEL: @muladd
define void @muladd(ptr %src, i32 %offset)
{
  %int = ptrtoint ptr %src to i32
  %mul = mul i32 %offset, 16
  %add = add i32 %int, %mul
  %ptr = inttoptr i32 %add to ptr
; CHECK: load
; CHECK: align {{1|4}}
  %val = load i8, ptr %ptr
  ret void
}

; CHECK-LABEL: @shl
define void @shl(i32 %unknown) #0 {
entry:
  %alloc = alloca <4 x i8>
  %int = ptrtoint ptr %alloc to i32
  %shl1 = shl i32 %int, 0
  %ptr1 = inttoptr i32 %shl1 to ptr
; CHECK: load
; CHECK: align {{1|4}}
  %val1 = load i8, ptr %ptr1

  %shl2 = shl i32 %int, 2
  %ptr4 = inttoptr i32 %shl2 to ptr
; CHECK: load
; CHECK: align {{1|16}}
  %val4 = load i8, ptr %ptr4

  %shlunk = shl i32 %int, %unknown
  %ptrunk = inttoptr i32 %shlunk to ptr
; CHECK: load
; CHECK: align 4
  %valunk = load i8, ptr %ptrunk

  ret void
}

; CHECK-LABEL: @and
define void @and (ptr %ptr) #0 {
entry:
  %int = ptrtoint ptr %ptr to i32
  ; ~0x03
  %and3 = and i32 %int, 4294967292
  %ptr3 = inttoptr i32 %and3 to ptr
; CHECK: load
; CHECK: align {{4|16}}
  %val3 = load <16 x i8>, ptr %ptr3

  %andsilly = and i32 %int, 255
  %ptrsilly = inttoptr i32 %andsilly to ptr
; CHECK: load
; CHECK: align {{1|16}}
  %valsilly = load <16 x i8>, ptr %ptrsilly

  ret void
}

; CHECK-LABEL: @select
define void @select(ptr align 4 %char4ptr, ptr align 16 %char16ptr, i1 %flag) #0 {
  %ptr = select i1 %flag, ptr %char16ptr, ptr %char4ptr
; CHECK: load
; CHECK: align 16
  %val = load <16 x i8>, ptr %ptr
  ret void
}

; CHECK-LABEL: @loadval
define void @loadval(<4 x i8>* %src, i32* %offset) #0 {
; CHECK: load
; CHECK: align 4
  %off = load i32, i32* %offset
  %int = ptrtoint <4 x i8>* %src to i32
  %add = add i32 %int, %off
  %ptr = inttoptr i32 %add to <4 x i8>*
; CHECK: load
; CHECK: align {{1|4}}
  %val = load <4 x i8>, <4 x i8>* %ptr
  ret void
}

; CHECK-LABEL: @simplephi
define void @simplephi(ptr align 16 %src, i1 %flag) #0 {
  %int = ptrtoint ptr %src to i32
  br i1 %flag, label %true, label %false

true:
  %add4 = add i32 %int, 4
  %ptr4 = inttoptr i32 %add4 to ptr
; CHECK: load
; CHECK: align 4
  %val4 = load i8, ptr %ptr4
  br label %end

false:
  %add16 = add i32 %int, 16
  %ptr16 = inttoptr i32 %add16 to ptr
; CHECK: load
; CHECK: align 16
  %val16 = load i8, ptr %ptr16
  br label %end

end:
  %p = phi ptr [ %ptr4, %true], [ %ptr16, %false]
; CHECK: store
; CHECK: align 4
  store i8 0, ptr %p
  ret void
}

; CHECK-LABEL: @loop
define void @loop(<4 x i8>* %src, i1 %flag) #0 {
entry:
  br label %body

body:
  %reduced = phi <4 x i8>* [ %src, %entry ], [ %addptr, %body ]
  %improved = phi <4 x i8>* [ %src, %entry ], [ %mulptr, %body ]
; CHECK: load
; CHECK: align {{1|4}}
  %valreduced = load <4 x i8>, <4 x i8>* %reduced
; CHECK: load
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

; CHECK-LABEL: @gep1
define void @gep1(ptr %src, i32 %offset) {
  %ptr = getelementptr <4 x i8>, ptr %src, i32 %offset
; CHECK: load
; CHECK: align 4
  %val = load <4 x i8>, ptr %ptr
  ret void
}

; CHECK-LABEL: @gep2
define void @gep2(ptr %src, i32 %offset) {
  %ptr = getelementptr i8, ptr %src, i32 %offset
; CHECK: load
; CHECK: align 1
  %val = load i8, ptr %ptr
  ret void
}

; CHECK-LABEL: @gep3
define void @gep3(ptr align 4 %src) {
  %ptr = getelementptr i8, ptr %src, i32 4
; CHECK: load
; CHECK: align 4
  %val = load i8, ptr %ptr
  ret void
}

; CHECK-LABEL: @globalgep
@globalarr = addrspace(3) global [10 x i32] [i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9], align 16
define void @globalgep() {
  %start = getelementptr [10 x i32], ptr addrspace(3) @globalarr, i32 0, i32 0
; CHECK: load
; CHECK: align 16
  %val = load i32, ptr addrspace(3) %start
  ret void
}

; -----------------------

; Function Attrs: alwaysinline nounwind
; CHECK: @regular
define void @regular(ptr addrspace(1) align 1 %src, ptr addrspace(1) align 4 %dst, <8 x i32> %arg1, <8 x i32> %arg2, i16 %arg3, i16 %arg4, i16 %arg5) #0 {
entry:
  %scalar = extractelement <8 x i32> %arg2, i32 0
  %groupId = extractelement <8 x i32> %arg1, i32 1
  %localSize = extractelement <8 x i32> %arg2, i32 3
  %0 = mul i32 %localSize, %groupId
  %1 = zext i16 %arg3 to i32
  %2 = add i32 %0, %1
  %3 = add i32 %2, %scalar
  %arrayidx = getelementptr inbounds i8, ptr addrspace(1) %src, i32 %3
; CHECK: load
; CHECK: align 1
  %4 = load i8, ptr addrspace(1) %arrayidx
  %add = add nsw i32 %3, 1
  %arrayidx1 = getelementptr inbounds i8, ptr addrspace(1) %src, i32 %add
; CHECK: load
; CHECK: align 1
  %5 = load i8, ptr addrspace(1) %arrayidx1
  %add3 = add nsw i32 %3, 2
  %arrayidx4 = getelementptr inbounds i8, ptr addrspace(1) %src, i32 %add3
; CHECK: load
; CHECK: align 1
  %6 = load i8, ptr addrspace(1) %arrayidx4
  %add6 = add nsw i32 %3, 3
  %arrayidx7 = getelementptr inbounds i8, ptr addrspace(1) %src, i32 %add6
; CHECK: load
; CHECK: align 1
  %7 = load i8, ptr addrspace(1) %arrayidx7
  %assembled.vect = insertelement <4 x i8> undef, i8 %4, i32 0
  %assembled.vect8 = insertelement <4 x i8> %assembled.vect, i8 %5, i32 1
  %assembled.vect9 = insertelement <4 x i8> %assembled.vect8, i8 %6, i32 2
  %assembled.vect10 = insertelement <4 x i8> %assembled.vect9, i8 %7, i32 3
  %arrayidx9 = getelementptr inbounds <4 x i8>, ptr addrspace(1) %dst, i32 %3
; CHECK: store
; CHECK: align 4
  store <4 x i8> %assembled.vect10, ptr addrspace(1) %arrayidx9
  ret void
}

; Function Attrs: alwaysinline nounwind
; CHECK: @virtload
define void @virtload(ptr addrspace(1) align 1 %src, ptr addrspace(1) align 4 %dst, <8 x i32> %arg1, <8 x i32> %arg2, i16 %arg3, i16 %arg4, i16 %arg5) #0 {
entry:
  %scalar = extractelement <8 x i32> %arg2, i32 0
  %groupId = extractelement <8 x i32> %arg1, i32 1
  %localSize = extractelement <8 x i32> %arg2, i32 3
  %0 = mul i32 %localSize, %groupId
  %1 = zext i16 %arg3 to i32
  %2 = add i32 %0, %1
  %3 = add i32 %2, %scalar
  %add.ptr = getelementptr inbounds i8, ptr addrspace(1) %src, i32 %3
; CHECK: load
; CHECK: align {{1|4}}
  %4 = load <4 x i8>, ptr addrspace(1) %add.ptr
  %arrayidx = getelementptr inbounds <4 x i8>, ptr addrspace(1) %dst, i32 %3
; CHECK: store
; CHECK: align 4
  store <4 x i8> %4, ptr addrspace(1) %arrayidx
  ret void
}

; Function Attrs: alwaysinline nounwind
; CHECK: @illegal
define void @illegal(ptr addrspace(1) align 1 %src, ptr addrspace(1) align 4 %dst, <8 x i32> %arg1, <8 x i32> %arg2, i16 %arg3, i16 %arg4, i16 %arg5) #0 {
entry:
  %scalar = extractelement <8 x i32> %arg2, i32 0
  %groupId = extractelement <8 x i32> %arg1, i32 1
  %localSize = extractelement <8 x i32> %arg2, i32 3
  %0 = mul i32 %localSize, %groupId
  %1 = zext i16 %arg3 to i32
  %2 = add i32 %0, %1
  %3 = add i32 %2, %scalar
  %add.ptr = getelementptr inbounds i8, ptr addrspace(1) %src, i32 %3
; CHECK: load
; CHECK: align {{1|4}}
  %4 = load <4 x i8>, <4 x i8> addrspace(1)* %add.ptr
  %arrayidx = getelementptr inbounds <4 x i8>, ptr addrspace(1) %dst, i32 %3
; CHECK: store
; CHECK: align 4
  store <4 x i8> %4, ptr addrspace(1) %arrayidx
  ret void
}

; Function Attrs: alwaysinline nounwind
; CHECK: @virtload_aligned
define void @virtload_aligned(ptr addrspace(1) align 4 %src, ptr addrspace(1) align 4 %dst, <8 x i32> %arg1, <8 x i32> %arg2, i16 %arg3, i16 %arg4, i16 %arg5) #0 {
entry:
  %scalar = extractelement <8 x i32> %arg2, i32 0
  %groupId = extractelement <8 x i32> %arg1, i32 1
  %localSize = extractelement <8 x i32> %arg2, i32 3
  %0 = mul i32 %localSize, %groupId
  %1 = zext i16 %arg3 to i32
  %2 = add i32 %0, %1
  %3 = add i32 %2, %scalar
  %4 = shl i32 %3, 2
  %5 = getelementptr inbounds i8, ptr addrspace(1) %src, i32 %4
; CHECK: load
; CHECK: align 4
  %6 = load <4 x i8>, ptr addrspace(1) %5
  %arrayidx = getelementptr inbounds <4 x i8>, ptr addrspace(1) %dst, i32 %3
; CHECK: store
; CHECK: align 4
  store <4 x i8> %6, ptr addrspace(1) %arrayidx
  ret void
}

; Function Attrs: alwaysinline nounwind
; CHECK: @aligned
define void @aligned(ptr addrspace(1) align 4 %src, ptr addrspace(1) align 4 %dst, <8 x i32> %arg1, <8 x i32> %arg2, i16 %arg3, i16 %arg4, i16 %arg5) #0 {
entry:
  %scalar = extractelement <8 x i32> %arg2, i32 0
  %groupId = extractelement <8 x i32> %arg1, i32 1
  %localSize = extractelement <8 x i32> %arg2, i32 3
  %0 = mul i32 %localSize, %groupId
  %1 = zext i16 %arg3 to i32
  %2 = add i32 %0, %1
  %3 = add i32 %2, %scalar
  %add.ptr = getelementptr inbounds <4 x i8>, ptr addrspace(1) %src, i32 %3
; CHECK: load
; CHECK: align 4
  %4 = load <4 x i8>, ptr addrspace(1) %add.ptr
  %arrayidx = getelementptr inbounds <4 x i8>, ptr addrspace(1) %dst, i32 %3
; CHECK: store
; CHECK: align 4
  store <4 x i8> %4, ptr addrspace(1) %arrayidx
  ret void
}

; CHECK: @regular_scalarized
define void @regular_scalarized(ptr addrspace(1) align 1 %src, ptr addrspace(1) align 4 %dst, <8 x i32> %arg1, <8 x i32> %arg2, i16 %arg3, i16 %arg4, i16 %arg5) #0 {
entry:
  %scalar = extractelement <8 x i32> %arg2, i32 0
  %groupId = extractelement <8 x i32> %arg1, i32 1
  %localSize = extractelement <8 x i32> %arg2, i32 3
  %0 = mul i32 %localSize, %groupId
  %1 = zext i16 %arg3 to i32
  %2 = add i32 %0, %1
  %3 = add i32 %2, %scalar
  %arrayidx = getelementptr inbounds i8, ptr addrspace(1) %src, i32 %3
; CHECK: load
; CHECK: align 1
  %4 = load i8, ptr addrspace(1) %arrayidx
  %add = add nsw i32 %3, 1
  %arrayidx1 = getelementptr inbounds i8, ptr addrspace(1) %src, i32 %add
; CHECK: load
; CHECK: align 1
  %5 = load i8, ptr addrspace(1) %arrayidx1
  %add3 = add nsw i32 %3, 2
  %arrayidx4 = getelementptr inbounds i8, ptr addrspace(1) %src, i32 %add3
; CHECK: load
; CHECK: align 1
  %6 = load i8, ptr addrspace(1) %arrayidx4
  %add6 = add nsw i32 %3, 3
  %arrayidx7 = getelementptr inbounds i8, ptr addrspace(1) %src, i32 %add6
; CHECK: load
; CHECK: align 1
  %7 = load i8, ptr addrspace(1) %arrayidx7
  %ptrVec2ptrScl = getelementptr inbounds <4 x i8>, ptr addrspace(1) %dst, i32 %3, i32 0
; CHECK: store
; CHECK: align 4
  store i8 %4, ptr addrspace(1) %ptrVec2ptrScl
  %GEP_lane8 = getelementptr <4 x i8>, ptr addrspace(1) %dst, i32 %3, i32 1
; CHECK: store
; CHECK: align 1
  store i8 %5, ptr addrspace(1) %GEP_lane8
  %GEP_lane9 = getelementptr <4 x i8>, ptr addrspace(1) %dst, i32 %3, i32 2
; CHECK: store
; CHECK: align 2
  store i8 %6, ptr addrspace(1) %GEP_lane9
  %GEP_lane10 = getelementptr <4 x i8>, ptr addrspace(1) %dst, i32 %3, i32 3
; CHECK: store
; CHECK: align 1
  store i8 %7, ptr addrspace(1) %GEP_lane10
  ret void
}

attributes #0 = { alwaysinline nounwind }
