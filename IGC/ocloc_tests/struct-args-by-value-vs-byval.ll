;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Checks that struct and array arguments of non-inlined functions are passed the
; same way whether they are passed by value (value.ll, which SPIR-V allows) or as
; byval pointers (byval.ll, what Clang emits). Both files define the same
; functions and go through the same checks:
;
;   argument type                                    bits  subroutine  stackcall
;   { i64, i64 }                                      128  registers   registers
;   { i64, i64, i64 }                                 192  registers   memory
;   { i64, i64, i64, i64, i64, i64, i64, i64 }        512  registers   memory
;   { i64, i64, i64, i64, i64, i64, i64, i64, i64 }   576  memory      memory
;   { i64, <4 x i32> }                                     memory      memory
;   { i64, [2 x i32] }                                     memory      memory
;   { i64, { i32, i32 } }                                  memory      memory
;   [2 x i64]                                              memory      memory
;
; Stackcalls pass structs of up to 128 bits in registers, subroutines up to 512
; bits. Arrays and structs with aggregate or vector members are passed in memory.
;
; The checks are on the callee signature in the last LLVM IR, before EmitPass: a
; struct parameter is passed in registers, a pointer parameter in memory.

; REQUIRES: regkeys, bmg-supported, llvm-16-plus
; RUN: split-file %s %t
; RUN: llvm-as %OPAQUE_PTR_FLAG% %t/value.ll -o %t/value.bc
; RUN: llvm-as %OPAQUE_PTR_FLAG% %t/byval.ll -o %t/byval.bc
; RUN: ocloc compile -llvm_input -file %t/value.bc -device bmg -options "-igc_opts 'EnableOpaquePointersBackend=1 FunctionControl=2 PrintToConsole=1 PrintBefore=EmitPass'" 2>&1 | FileCheck %s --check-prefixes=CHECK,SUBROUTINE
; RUN: ocloc compile -llvm_input -file %t/byval.bc -device bmg -options "-igc_opts 'EnableOpaquePointersBackend=1 FunctionControl=2 PrintToConsole=1 PrintBefore=EmitPass'" 2>&1 | FileCheck %s --check-prefixes=CHECK,SUBROUTINE
; RUN: ocloc compile -llvm_input -file %t/value.bc -device bmg -options "-igc_opts 'EnableOpaquePointersBackend=1 FunctionControl=3 PrintToConsole=1 PrintBefore=EmitPass'" 2>&1 | FileCheck %s --check-prefixes=CHECK,STACKCALL
; RUN: ocloc compile -llvm_input -file %t/byval.bc -device bmg -options "-igc_opts 'EnableOpaquePointersBackend=1 FunctionControl=3 PrintToConsole=1 PrintBefore=EmitPass'" 2>&1 | FileCheck %s --check-prefixes=CHECK,STACKCALL

; CHECK-DAG:       define internal spir_func i64 @f_s128(%s128 %s)
; SUBROUTINE-DAG:  define internal spir_func i64 @f_s192(%s192 %s)
; STACKCALL-DAG:   define internal spir_func i64 @f_s192(ptr {{.*}}%s)
; SUBROUTINE-DAG:  define internal spir_func i64 @f_s512(%s512 %s)
; STACKCALL-DAG:   define internal spir_func i64 @f_s512(ptr {{.*}}%s)
; CHECK-DAG:       define internal spir_func i64 @f_s576(ptr {{.*}}%s)
; CHECK-DAG:       define internal spir_func i64 @f_vec(ptr {{.*}}%s)
; CHECK-DAG:       define internal spir_func i64 @f_arr(ptr {{.*}}%s)
; CHECK-DAG:       define internal spir_func i64 @f_nested(ptr {{.*}}%s)
; CHECK-DAG:       define internal spir_func i64 @f_array(ptr {{.*}}%s)

;--- value.ll
%s128 = type { i64, i64 }
%s192 = type { i64, i64, i64 }
%s512 = type { i64, i64, i64, i64, i64, i64, i64, i64 }
%s576 = type { i64, i64, i64, i64, i64, i64, i64, i64, i64 }
%vec = type { i64, <4 x i32> }
%arr = type { i64, [2 x i32] }
%nested = type { i64, { i32, i32 } }

define internal spir_func i64 @f_s128(%s128 %s) {
  %m = extractvalue %s128 %s, 0
  ret i64 %m
}

define spir_kernel void @k_s128(ptr addrspace(1) %in, ptr addrspace(1) %out) {
  %s = load %s128, ptr addrspace(1) %in
  %r = call spir_func i64 @f_s128(%s128 %s)
  store i64 %r, ptr addrspace(1) %out
  ret void
}

define internal spir_func i64 @f_s192(%s192 %s) {
  %m = extractvalue %s192 %s, 0
  ret i64 %m
}

define spir_kernel void @k_s192(ptr addrspace(1) %in, ptr addrspace(1) %out) {
  %s = load %s192, ptr addrspace(1) %in
  %r = call spir_func i64 @f_s192(%s192 %s)
  store i64 %r, ptr addrspace(1) %out
  ret void
}

define internal spir_func i64 @f_s512(%s512 %s) {
  %m = extractvalue %s512 %s, 0
  ret i64 %m
}

define spir_kernel void @k_s512(ptr addrspace(1) %in, ptr addrspace(1) %out) {
  %s = load %s512, ptr addrspace(1) %in
  %r = call spir_func i64 @f_s512(%s512 %s)
  store i64 %r, ptr addrspace(1) %out
  ret void
}

define internal spir_func i64 @f_s576(%s576 %s) {
  %m = extractvalue %s576 %s, 0
  ret i64 %m
}

define spir_kernel void @k_s576(ptr addrspace(1) %in, ptr addrspace(1) %out) {
  %s = load %s576, ptr addrspace(1) %in
  %r = call spir_func i64 @f_s576(%s576 %s)
  store i64 %r, ptr addrspace(1) %out
  ret void
}

define internal spir_func i64 @f_vec(%vec %s) {
  %m = extractvalue %vec %s, 0
  ret i64 %m
}

define spir_kernel void @k_vec(ptr addrspace(1) %in, ptr addrspace(1) %out) {
  %s = load %vec, ptr addrspace(1) %in
  %r = call spir_func i64 @f_vec(%vec %s)
  store i64 %r, ptr addrspace(1) %out
  ret void
}

define internal spir_func i64 @f_arr(%arr %s) {
  %m = extractvalue %arr %s, 0
  ret i64 %m
}

define spir_kernel void @k_arr(ptr addrspace(1) %in, ptr addrspace(1) %out) {
  %s = load %arr, ptr addrspace(1) %in
  %r = call spir_func i64 @f_arr(%arr %s)
  store i64 %r, ptr addrspace(1) %out
  ret void
}

define internal spir_func i64 @f_nested(%nested %s) {
  %m = extractvalue %nested %s, 0
  ret i64 %m
}

define spir_kernel void @k_nested(ptr addrspace(1) %in, ptr addrspace(1) %out) {
  %s = load %nested, ptr addrspace(1) %in
  %r = call spir_func i64 @f_nested(%nested %s)
  store i64 %r, ptr addrspace(1) %out
  ret void
}

define internal spir_func i64 @f_array([2 x i64] %s) {
  %m = extractvalue [2 x i64] %s, 0
  ret i64 %m
}

define spir_kernel void @k_array(ptr addrspace(1) %in, ptr addrspace(1) %out) {
  %s = load [2 x i64], ptr addrspace(1) %in
  %r = call spir_func i64 @f_array([2 x i64] %s)
  store i64 %r, ptr addrspace(1) %out
  ret void
}

;--- byval.ll
%s128 = type { i64, i64 }
%s192 = type { i64, i64, i64 }
%s512 = type { i64, i64, i64, i64, i64, i64, i64, i64 }
%s576 = type { i64, i64, i64, i64, i64, i64, i64, i64, i64 }
%vec = type { i64, <4 x i32> }
%arr = type { i64, [2 x i32] }
%nested = type { i64, { i32, i32 } }

define internal spir_func i64 @f_s128(ptr byval(%s128) %s) {
  %m = load i64, ptr %s
  ret i64 %m
}

define spir_kernel void @k_s128(ptr addrspace(1) %in, ptr addrspace(1) %out) {
  %p = alloca %s128
  %s = load %s128, ptr addrspace(1) %in
  store %s128 %s, ptr %p
  %r = call spir_func i64 @f_s128(ptr byval(%s128) %p)
  store i64 %r, ptr addrspace(1) %out
  ret void
}

define internal spir_func i64 @f_s192(ptr byval(%s192) %s) {
  %m = load i64, ptr %s
  ret i64 %m
}

define spir_kernel void @k_s192(ptr addrspace(1) %in, ptr addrspace(1) %out) {
  %p = alloca %s192
  %s = load %s192, ptr addrspace(1) %in
  store %s192 %s, ptr %p
  %r = call spir_func i64 @f_s192(ptr byval(%s192) %p)
  store i64 %r, ptr addrspace(1) %out
  ret void
}

define internal spir_func i64 @f_s512(ptr byval(%s512) %s) {
  %m = load i64, ptr %s
  ret i64 %m
}

define spir_kernel void @k_s512(ptr addrspace(1) %in, ptr addrspace(1) %out) {
  %p = alloca %s512
  %s = load %s512, ptr addrspace(1) %in
  store %s512 %s, ptr %p
  %r = call spir_func i64 @f_s512(ptr byval(%s512) %p)
  store i64 %r, ptr addrspace(1) %out
  ret void
}

define internal spir_func i64 @f_s576(ptr byval(%s576) %s) {
  %m = load i64, ptr %s
  ret i64 %m
}

define spir_kernel void @k_s576(ptr addrspace(1) %in, ptr addrspace(1) %out) {
  %p = alloca %s576
  %s = load %s576, ptr addrspace(1) %in
  store %s576 %s, ptr %p
  %r = call spir_func i64 @f_s576(ptr byval(%s576) %p)
  store i64 %r, ptr addrspace(1) %out
  ret void
}

define internal spir_func i64 @f_vec(ptr byval(%vec) %s) {
  %m = load i64, ptr %s
  ret i64 %m
}

define spir_kernel void @k_vec(ptr addrspace(1) %in, ptr addrspace(1) %out) {
  %p = alloca %vec
  %s = load %vec, ptr addrspace(1) %in
  store %vec %s, ptr %p
  %r = call spir_func i64 @f_vec(ptr byval(%vec) %p)
  store i64 %r, ptr addrspace(1) %out
  ret void
}

define internal spir_func i64 @f_arr(ptr byval(%arr) %s) {
  %m = load i64, ptr %s
  ret i64 %m
}

define spir_kernel void @k_arr(ptr addrspace(1) %in, ptr addrspace(1) %out) {
  %p = alloca %arr
  %s = load %arr, ptr addrspace(1) %in
  store %arr %s, ptr %p
  %r = call spir_func i64 @f_arr(ptr byval(%arr) %p)
  store i64 %r, ptr addrspace(1) %out
  ret void
}

define internal spir_func i64 @f_nested(ptr byval(%nested) %s) {
  %m = load i64, ptr %s
  ret i64 %m
}

define spir_kernel void @k_nested(ptr addrspace(1) %in, ptr addrspace(1) %out) {
  %p = alloca %nested
  %s = load %nested, ptr addrspace(1) %in
  store %nested %s, ptr %p
  %r = call spir_func i64 @f_nested(ptr byval(%nested) %p)
  store i64 %r, ptr addrspace(1) %out
  ret void
}

define internal spir_func i64 @f_array(ptr byval([2 x i64]) %s) {
  %m = load i64, ptr %s
  ret i64 %m
}

define spir_kernel void @k_array(ptr addrspace(1) %in, ptr addrspace(1) %out) {
  %p = alloca [2 x i64]
  %s = load [2 x i64], ptr addrspace(1) %in
  store [2 x i64] %s, ptr %p
  %r = call spir_func i64 @f_array(ptr byval([2 x i64]) %p)
  store i64 %r, ptr addrspace(1) %out
  ret void
}
