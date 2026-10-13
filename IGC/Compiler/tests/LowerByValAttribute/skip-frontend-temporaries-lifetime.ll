;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-22-plus
; RUN: igc_opt --opaque-pointers -igc-lower-byval-attribute -S %s | FileCheck %s

; Lifetime markers don't read the object, so they don't prevent skipping the copy of a frontend temporary.

%struct.S = type { i32, [100 x i32], i32 }

; CHECK-LABEL: define spir_func void @memcpy_temp_with_lifetime(
; CHECK: %tmp = alloca %struct.S
; CHECK-NOT: alloca
; CHECK: call spir_func void @f(ptr byval(%struct.S) align 4 %tmp)
define spir_func void @memcpy_temp_with_lifetime(ptr addrspace(4) %p) {
  %tmp = alloca %struct.S, align 4
  call void @llvm.lifetime.start.p0(ptr %tmp)
  call void @llvm.memcpy.p0.p4.i64(ptr align 4 %tmp, ptr addrspace(4) align 4 %p, i64 408, i1 false)
  call spir_func void @f(ptr byval(%struct.S) align 4 %tmp)
  call void @llvm.lifetime.end.p0(ptr %tmp)
  ret void
}

define spir_func void @f(ptr byval(%struct.S) align 4 %s) #0 {
  store i32 222, ptr %s, align 4
  ret void
}

declare void @llvm.memcpy.p0.p4.i64(ptr, ptr addrspace(4), i64, i1)
declare void @llvm.lifetime.start.p0(ptr)
declare void @llvm.lifetime.end.p0(ptr)

attributes #0 = { noinline }
