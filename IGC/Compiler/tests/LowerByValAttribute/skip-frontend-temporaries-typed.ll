;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-lower-byval-attribute -S %s | FileCheck %s

; With typed pointers a frontend writes its byval temporary through a bitcast to i8*.

%struct.S = type { i32, [10 x i32], i32 }

; CHECK-LABEL: define spir_func void @memcpy_temp(
; CHECK: %tmp = alloca %struct.S
; CHECK-NOT: alloca
; CHECK: call spir_func void @f(%struct.S* byval(%struct.S) align 4 %tmp)
define spir_func void @memcpy_temp(%struct.S addrspace(4)* %p) {
  %tmp = alloca %struct.S, align 4
  %dst = bitcast %struct.S* %tmp to i8*
  %src = bitcast %struct.S addrspace(4)* %p to i8 addrspace(4)*
  call void @llvm.memcpy.p0i8.p4i8.i64(i8* align 4 %dst, i8 addrspace(4)* align 4 %src, i64 48, i1 false)
  call spir_func void @f(%struct.S* byval(%struct.S) align 4 %tmp)
  ret void
}

; CHECK-LABEL: define spir_func i32 @read_after_call(
; CHECK: [[COPY:%[0-9]+]] = alloca %struct.S
; CHECK: call void @llvm.memcpy.p0i8.p0i8.i64(
; CHECK: call spir_func void @f(%struct.S* byval(%struct.S) align 4 [[COPY]])
define spir_func i32 @read_after_call(%struct.S* %p) {
  %s = alloca %struct.S, align 4
  %dst = bitcast %struct.S* %s to i8*
  %src = bitcast %struct.S* %p to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %dst, i8* align 4 %src, i64 48, i1 false)
  call spir_func void @f(%struct.S* byval(%struct.S) align 4 %s)
  %x = getelementptr inbounds %struct.S, %struct.S* %s, i32 0, i32 0
  %v = load i32, i32* %x, align 4
  ret i32 %v
}

define spir_func void @f(%struct.S* byval(%struct.S) align 4 %s) #0 {
  %x = getelementptr inbounds %struct.S, %struct.S* %s, i32 0, i32 0
  store i32 222, i32* %x, align 4
  ret void
}

declare void @llvm.memcpy.p0i8.p0i8.i64(i8*, i8*, i64, i1)
declare void @llvm.memcpy.p0i8.p4i8.i64(i8*, i8 addrspace(4)*, i64, i1)

attributes #0 = { noinline }
