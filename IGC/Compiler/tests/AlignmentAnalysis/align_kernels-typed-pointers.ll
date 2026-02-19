;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-fix-alignment -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

target triple = "igil_32_GEN9"

; Function Attrs: alwaysinline nounwind
; CHECK: @regular
define void @regular(i8 addrspace(1)* %src, <4 x i8> addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
entry:
  %scalar = extractelement <8 x i32> %payloadHeader, i32 0
  %groupId = extractelement <8 x i32> %r0, i32 1
  %localSize = extractelement <8 x i32> %payloadHeader, i32 3
  %0 = mul i32 %localSize, %groupId
  %1 = zext i16 %localIdX to i32
  %2 = add i32 %0, %1
  %3 = add i32 %2, %scalar
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %src, i32 %3
; CHECK: load
; CHECK: align 1
  %4 = load i8, i8 addrspace(1)* %arrayidx
  %add = add nsw i32 %3, 1
  %arrayidx1 = getelementptr inbounds i8, i8 addrspace(1)* %src, i32 %add
; CHECK: load
; CHECK: align 1
  %5 = load i8, i8 addrspace(1)* %arrayidx1
  %add3 = add nsw i32 %3, 2
  %arrayidx4 = getelementptr inbounds i8, i8 addrspace(1)* %src, i32 %add3
; CHECK: load
; CHECK: align 1
  %6 = load i8, i8 addrspace(1)* %arrayidx4
  %add6 = add nsw i32 %3, 3
  %arrayidx7 = getelementptr inbounds i8, i8 addrspace(1)* %src, i32 %add6
; CHECK: load
; CHECK: align 1
  %7 = load i8, i8 addrspace(1)* %arrayidx7
  %assembled.vect = insertelement <4 x i8> undef, i8 %4, i32 0
  %assembled.vect8 = insertelement <4 x i8> %assembled.vect, i8 %5, i32 1
  %assembled.vect9 = insertelement <4 x i8> %assembled.vect8, i8 %6, i32 2
  %assembled.vect10 = insertelement <4 x i8> %assembled.vect9, i8 %7, i32 3
  %arrayidx9 = getelementptr inbounds <4 x i8>, <4 x i8> addrspace(1)* %dst, i32 %3
; CHECK: store
; CHECK: align 4
  store <4 x i8> %assembled.vect10, <4 x i8> addrspace(1)* %arrayidx9
  ret void
}

; Function Attrs: alwaysinline nounwind
; CHECK: @virtload
define void @virtload(i8 addrspace(1)* %src, <4 x i8> addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
entry:
  %scalar = extractelement <8 x i32> %payloadHeader, i32 0
  %groupId = extractelement <8 x i32> %r0, i32 1
  %localSize = extractelement <8 x i32> %payloadHeader, i32 3
  %0 = mul i32 %localSize, %groupId
  %1 = zext i16 %localIdX to i32
  %2 = add i32 %0, %1
  %3 = add i32 %2, %scalar
  %add.ptr = getelementptr inbounds i8, i8 addrspace(1)* %src, i32 %3
  %4 = bitcast i8 addrspace(1)* %add.ptr to <4 x i8> addrspace(1)*
; CHECK: load
; CHECK: align {{1|4}}
  %5 = load <4 x i8>, <4 x i8> addrspace(1)* %4
  %arrayidx = getelementptr inbounds <4 x i8>, <4 x i8> addrspace(1)* %dst, i32 %3
; CHECK: store
; CHECK: align 4
  store <4 x i8> %5, <4 x i8> addrspace(1)* %arrayidx
  ret void
}

; Function Attrs: alwaysinline nounwind
; CHECK: @illegal
define void @illegal(i8 addrspace(1)* %src, <4 x i8> addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
entry:
  %scalar = extractelement <8 x i32> %payloadHeader, i32 0
  %groupId = extractelement <8 x i32> %r0, i32 1
  %localSize = extractelement <8 x i32> %payloadHeader, i32 3
  %0 = mul i32 %localSize, %groupId
  %1 = zext i16 %localIdX to i32
  %2 = add i32 %0, %1
  %3 = add i32 %2, %scalar
  %add.ptr = getelementptr inbounds i8, i8 addrspace(1)* %src, i32 %3
  %4 = bitcast i8 addrspace(1)* %add.ptr to <4 x i8> addrspace(1)*
; CHECK: load
; CHECK: align {{1|4}}
  %5 = load <4 x i8>, <4 x i8> addrspace(1)* %4
  %arrayidx = getelementptr inbounds <4 x i8>, <4 x i8> addrspace(1)* %dst, i32 %3
; CHECK: store
; CHECK: align 4
  store <4 x i8> %5, <4 x i8> addrspace(1)* %arrayidx
  ret void
}

; Function Attrs: alwaysinline nounwind
; CHECK: @virtload_aligned
define void @virtload_aligned(<4 x i8> addrspace(1)* %src, <4 x i8> addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
entry:
  %scalar = extractelement <8 x i32> %payloadHeader, i32 0
  %groupId = extractelement <8 x i32> %r0, i32 1
  %localSize = extractelement <8 x i32> %payloadHeader, i32 3
  %0 = mul i32 %localSize, %groupId
  %1 = zext i16 %localIdX to i32
  %2 = add i32 %0, %1
  %3 = add i32 %2, %scalar
  %4 = bitcast <4 x i8> addrspace(1)* %src to i8 addrspace(1)*
  %5 = shl i32 %3, 2
  %6 = getelementptr inbounds i8, i8 addrspace(1)* %4, i32 %5
  %7 = bitcast i8 addrspace(1)* %6 to <4 x i8> addrspace(1)*
; CHECK: load
; CHECK: align 4
  %8 = load <4 x i8>, <4 x i8> addrspace(1)* %7
  %arrayidx = getelementptr inbounds <4 x i8>, <4 x i8> addrspace(1)* %dst, i32 %3
; CHECK: store
; CHECK: align 4
  store <4 x i8> %8, <4 x i8> addrspace(1)* %arrayidx
  ret void
}

; Function Attrs: alwaysinline nounwind
; CHECK: @aligned
define void @aligned(<4 x i8> addrspace(1)* %src, <4 x i8> addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
entry:
  %scalar = extractelement <8 x i32> %payloadHeader, i32 0
  %groupId = extractelement <8 x i32> %r0, i32 1
  %localSize = extractelement <8 x i32> %payloadHeader, i32 3
  %0 = mul i32 %localSize, %groupId
  %1 = zext i16 %localIdX to i32
  %2 = add i32 %0, %1
  %3 = add i32 %2, %scalar
  %add.ptr = getelementptr inbounds <4 x i8>, <4 x i8> addrspace(1)* %src, i32 %3
; CHECK: load
; CHECK: align 4
  %4 = load <4 x i8>, <4 x i8> addrspace(1)* %add.ptr
  %arrayidx = getelementptr inbounds <4 x i8>, <4 x i8> addrspace(1)* %dst, i32 %3
; CHECK: store
; CHECK: align 4
  store <4 x i8> %4, <4 x i8> addrspace(1)* %arrayidx
  ret void
}

; CHECK: @regular_scalarized
define void @regular_scalarized(i8 addrspace(1)* %src, <4 x i8> addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
entry:
  %scalar = extractelement <8 x i32> %payloadHeader, i32 0
  %groupId = extractelement <8 x i32> %r0, i32 1
  %localSize = extractelement <8 x i32> %payloadHeader, i32 3
  %0 = mul i32 %localSize, %groupId
  %1 = zext i16 %localIdX to i32
  %2 = add i32 %0, %1
  %3 = add i32 %2, %scalar
  %arrayidx = getelementptr inbounds i8, i8 addrspace(1)* %src, i32 %3
; CHECK: load
; CHECK: align 1
  %4 = load i8, i8 addrspace(1)* %arrayidx
  %add = add nsw i32 %3, 1
  %arrayidx1 = getelementptr inbounds i8, i8 addrspace(1)* %src, i32 %add
; CHECK: load
; CHECK: align 1
  %5 = load i8, i8 addrspace(1)* %arrayidx1
  %add3 = add nsw i32 %3, 2
  %arrayidx4 = getelementptr inbounds i8, i8 addrspace(1)* %src, i32 %add3
; CHECK: load
; CHECK: align 1
  %6 = load i8, i8 addrspace(1)* %arrayidx4
  %add6 = add nsw i32 %3, 3
  %arrayidx7 = getelementptr inbounds i8, i8 addrspace(1)* %src, i32 %add6
; CHECK: load
; CHECK: align 1
  %7 = load i8, i8 addrspace(1)* %arrayidx7
  %ptrVec2ptrScl = getelementptr inbounds <4 x i8>, <4 x i8> addrspace(1)* %dst, i32 %3, i32 0
; CHECK: store
; CHECK: align 4
  store i8 %4, i8 addrspace(1)* %ptrVec2ptrScl
  %GEP_lane8 = getelementptr <4 x i8>, <4 x i8> addrspace(1)* %dst, i32 %3, i32 1
; CHECK: store
; CHECK: align 1
  store i8 %5, i8 addrspace(1)* %GEP_lane8
  %GEP_lane9 = getelementptr <4 x i8>, <4 x i8> addrspace(1)* %dst, i32 %3, i32 2
; CHECK: store
; CHECK: align 2
  store i8 %6, i8 addrspace(1)* %GEP_lane9
  %GEP_lane10 = getelementptr <4 x i8>, <4 x i8> addrspace(1)* %dst, i32 %3, i32 3
; CHECK: store
; CHECK: align 1
  store i8 %7, i8 addrspace(1)* %GEP_lane10
  ret void
}

attributes #0 = { alwaysinline nounwind }
