;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-lower-byval-attribute -S %s | FileCheck %s

; A temporary that is only written right before the call and never read by the caller is a copy a frontend
; already made, so no second copy is needed. Anything else the caller could observe after the call still gets
; a copy.

%struct.S = type { i32, [100 x i32], i32 }

@gs = internal global %struct.S zeroinitializer, align 4

; CHECK-LABEL: define spir_func void @memcpy_temp(
; CHECK: %tmp = alloca %struct.S
; CHECK-NOT: alloca
; CHECK: call spir_func void @f(ptr byval(%struct.S) align 4 %tmp)
define spir_func void @memcpy_temp(ptr addrspace(4) %p) {
  %tmp = alloca %struct.S, align 4
  call void @llvm.memcpy.p0.p4.i64(ptr align 4 %tmp, ptr addrspace(4) align 4 %p, i64 408, i1 false)
  call spir_func void @f(ptr byval(%struct.S) align 4 %tmp)
  ret void
}

; CHECK-LABEL: define spir_func void @sret_temp(
; CHECK: %tmp = alloca %struct.S
; CHECK-NOT: alloca
; CHECK: call spir_func void @f(ptr byval(%struct.S) align 4 %tmp)
define spir_func void @sret_temp() {
  %tmp = alloca %struct.S, align 4
  call spir_func void @mk(ptr sret(%struct.S) align 4 %tmp)
  call spir_func void @f(ptr byval(%struct.S) align 4 %tmp)
  ret void
}

; CHECK-LABEL: define spir_func void @memset_temp(
; CHECK: %tmp = alloca %struct.S
; CHECK-NOT: alloca
; CHECK: call spir_func void @f(ptr byval(%struct.S) align 4 %tmp)
define spir_func void @memset_temp() {
  %tmp = alloca %struct.S, align 4
  call void @llvm.memset.p0.i64(ptr align 4 %tmp, i8 0, i64 408, i1 false)
  call spir_func void @f(ptr byval(%struct.S) align 4 %tmp)
  ret void
}

; CHECK-LABEL: define spir_func void @memcpy_temp_in_loop(
; CHECK: %tmp = alloca %struct.S
; CHECK-NOT: alloca
; CHECK: call spir_func void @f(ptr byval(%struct.S) align 4 %tmp)
define spir_func void @memcpy_temp_in_loop(ptr addrspace(4) %p, i1 %c) {
entry:
  %tmp = alloca %struct.S, align 4
  br label %loop

loop:
  call void @llvm.memcpy.p0.p4.i64(ptr align 4 %tmp, ptr addrspace(4) align 4 %p, i64 408, i1 false)
  call spir_func void @f(ptr byval(%struct.S) align 4 %tmp)
  br i1 %c, label %loop, label %exit

exit:
  ret void
}

; CHECK-LABEL: define spir_func i32 @read_after_call(
; CHECK: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 %s, i64 408, i1 false)
; CHECK-NEXT: call spir_func void @f(ptr byval(%struct.S) align 4 [[COPY]])
define spir_func i32 @read_after_call(ptr %p) {
  %s = alloca %struct.S, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %s, ptr align 4 %p, i64 408, i1 false)
  call spir_func void @f(ptr byval(%struct.S) align 4 %s)
  %v = load i32, ptr %s, align 4
  ret i32 %v
}

; CHECK-LABEL: define spir_func void @passed_to_two_calls(
; CHECK: [[COPY0:%[0-9]+]] = alloca %struct.S, align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY0]], ptr align 4 %s, i64 408, i1 false)
; CHECK-NEXT: call spir_func void @f(ptr byval(%struct.S) align 4 [[COPY0]])
; CHECK-NEXT: [[COPY1:%[0-9]+]] = alloca %struct.S, align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY1]], ptr align 4 %s, i64 408, i1 false)
; CHECK-NEXT: call spir_func void @f(ptr byval(%struct.S) align 4 [[COPY1]])
define spir_func void @passed_to_two_calls(ptr %p) {
  %s = alloca %struct.S, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %s, ptr align 4 %p, i64 408, i1 false)
  call spir_func void @f(ptr byval(%struct.S) align 4 %s)
  call spir_func void @f(ptr byval(%struct.S) align 4 %s)
  ret void
}

; CHECK-LABEL: define spir_func void @written_before_loop(
; CHECK: loop:
; CHECK-NEXT: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 %s, i64 408, i1 false)
; CHECK-NEXT: call spir_func void @f(ptr byval(%struct.S) align 4 [[COPY]])
define spir_func void @written_before_loop(ptr %p, i1 %c) {
entry:
  %s = alloca %struct.S, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %s, ptr align 4 %p, i64 408, i1 false)
  br label %loop

loop:
  call spir_func void @f(ptr byval(%struct.S) align 4 %s)
  br i1 %c, label %loop, label %exit

exit:
  ret void
}

; CHECK-LABEL: define spir_func void @written_after_call(
; CHECK: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 %s, i64 408, i1 false)
; CHECK-NEXT: call spir_func void @f(ptr byval(%struct.S) align 4 [[COPY]])
define spir_func void @written_after_call(ptr %p) {
  %s = alloca %struct.S, align 4
  call spir_func void @f(ptr byval(%struct.S) align 4 %s)
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %s, ptr align 4 %p, i64 408, i1 false)
  ret void
}

; CHECK-LABEL: define spir_func void @also_passed_by_pointer(
; CHECK: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 %s, i64 408, i1 false)
; CHECK-NEXT: call spir_func void @g(ptr byval(%struct.S) align 4 [[COPY]], ptr %s)
define spir_func void @also_passed_by_pointer(ptr %p) {
  %s = alloca %struct.S, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %s, ptr align 4 %p, i64 408, i1 false)
  call spir_func void @g(ptr byval(%struct.S) align 4 %s, ptr %s)
  ret void
}

; CHECK-LABEL: define spir_func void @volatile_memcpy(
; CHECK: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 %s, i64 408, i1 false)
; CHECK-NEXT: call spir_func void @f(ptr byval(%struct.S) align 4 [[COPY]])
define spir_func void @volatile_memcpy(ptr %p) {
  %s = alloca %struct.S, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %s, ptr align 4 %p, i64 408, i1 true)
  call spir_func void @f(ptr byval(%struct.S) align 4 %s)
  ret void
}

; CHECK-LABEL: define spir_func void @generic_byval_operand(
; CHECK: %tmp = alloca %struct.S
; CHECK-NOT: alloca
; CHECK: call spir_func void @fg(ptr addrspace(4) byval(%struct.S) align 4 %asc)
define spir_func void @generic_byval_operand(ptr addrspace(4) %p) {
  %tmp = alloca %struct.S, align 4
  call void @llvm.memcpy.p0.p4.i64(ptr align 4 %tmp, ptr addrspace(4) align 4 %p, i64 408, i1 false)
  %asc = addrspacecast ptr %tmp to ptr addrspace(4)
  call spir_func void @fg(ptr addrspace(4) byval(%struct.S) align 4 %asc)
  ret void
}

; CHECK-LABEL: define spir_func void @written_through_cast(
; CHECK: %tmp = alloca %struct.S
; CHECK-NOT: alloca
; CHECK: call spir_func void @f(ptr byval(%struct.S) align 4 %tmp)
define spir_func void @written_through_cast(ptr addrspace(4) %p) {
  %tmp = alloca %struct.S, align 4
  %asc = addrspacecast ptr %tmp to ptr addrspace(4)
  call void @llvm.memcpy.p4.p4.i64(ptr addrspace(4) align 4 %asc, ptr addrspace(4) align 4 %p, i64 408, i1 false)
  call spir_func void @f(ptr byval(%struct.S) align 4 %tmp)
  ret void
}

; CHECK-LABEL: define spir_func void @multiple_writes(
; CHECK: %tmp = alloca %struct.S
; CHECK-NOT: alloca
; CHECK: call spir_func void @f(ptr byval(%struct.S) align 4 %tmp)
define spir_func void @multiple_writes(ptr %p) {
  %tmp = alloca %struct.S, align 4
  call void @llvm.memset.p0.i64(ptr align 4 %tmp, i8 0, i64 408, i1 false)
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %tmp, ptr align 4 %p, i64 4, i1 false)
  call spir_func void @f(ptr byval(%struct.S) align 4 %tmp)
  ret void
}

; CHECK-LABEL: define spir_func void @two_temps_in_one_call(
; CHECK: %tmp0 = alloca %struct.S
; CHECK: %tmp1 = alloca %struct.S
; CHECK-NOT: alloca
; CHECK: call spir_func void @h(ptr byval(%struct.S) align 4 %tmp0, ptr byval(%struct.S) align 4 %tmp1)
define spir_func void @two_temps_in_one_call(ptr addrspace(4) %p, ptr addrspace(4) %q) {
  %tmp0 = alloca %struct.S, align 4
  %tmp1 = alloca %struct.S, align 4
  call void @llvm.memcpy.p0.p4.i64(ptr align 4 %tmp0, ptr addrspace(4) align 4 %p, i64 408, i1 false)
  call void @llvm.memcpy.p0.p4.i64(ptr align 4 %tmp1, ptr addrspace(4) align 4 %q, i64 408, i1 false)
  call spir_func void @h(ptr byval(%struct.S) align 4 %tmp0, ptr byval(%struct.S) align 4 %tmp1)
  ret void
}

; Only the private object passed directly gets a copy.
; CHECK-LABEL: define spir_func void @temp_and_private_in_one_call(
; CHECK: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 %q, i64 408, i1 false)
; CHECK-NEXT: call spir_func void @h(ptr byval(%struct.S) align 4 %tmp, ptr byval(%struct.S) align 4 [[COPY]])
define spir_func void @temp_and_private_in_one_call(ptr addrspace(4) %p, ptr %q) {
  %tmp = alloca %struct.S, align 4
  call void @llvm.memcpy.p0.p4.i64(ptr align 4 %tmp, ptr addrspace(4) align 4 %p, i64 408, i1 false)
  call spir_func void @h(ptr byval(%struct.S) align 4 %tmp, ptr byval(%struct.S) align 4 %q)
  ret void
}

; CHECK-LABEL: define spir_func void @passed_twice_to_one_call(
; CHECK: [[COPY0:%[0-9]+]] = alloca %struct.S, align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY0]], ptr align 4 %tmp, i64 408, i1 false)
; CHECK-NEXT: [[COPY1:%[0-9]+]] = alloca %struct.S, align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY1]], ptr align 4 %tmp, i64 408, i1 false)
; CHECK-NEXT: call spir_func void @h(ptr byval(%struct.S) align 4 [[COPY0]], ptr byval(%struct.S) align 4 [[COPY1]])
define spir_func void @passed_twice_to_one_call(ptr addrspace(4) %p) {
  %tmp = alloca %struct.S, align 4
  call void @llvm.memcpy.p0.p4.i64(ptr align 4 %tmp, ptr addrspace(4) align 4 %p, i64 408, i1 false)
  call spir_func void @h(ptr byval(%struct.S) align 4 %tmp, ptr byval(%struct.S) align 4 %tmp)
  ret void
}

; Only memcpy, memset and sret count as writes, so an object initialized by stores gets a copy.
; CHECK-LABEL: define spir_func void @written_by_store(
; CHECK: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 %s, i64 408, i1 false)
; CHECK-NEXT: call spir_func void @f(ptr byval(%struct.S) align 4 [[COPY]])
define spir_func void @written_by_store(i32 %v) {
  %s = alloca %struct.S, align 4
  store i32 %v, ptr %s, align 4
  call spir_func void @f(ptr byval(%struct.S) align 4 %s)
  ret void
}

; CHECK-LABEL: define spir_func void @written_through_gep(
; CHECK: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 %s, i64 408, i1 false)
; CHECK-NEXT: call spir_func void @f(ptr byval(%struct.S) align 4 [[COPY]])
define spir_func void @written_through_gep(ptr %p) {
  %s = alloca %struct.S, align 4
  %z = getelementptr inbounds %struct.S, ptr %s, i32 0, i32 2
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %z, ptr align 4 %p, i64 4, i1 false)
  call spir_func void @f(ptr byval(%struct.S) align 4 %s)
  ret void
}

; CHECK-LABEL: define spir_func void @address_escapes(
; CHECK: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 %s, i64 408, i1 false)
; CHECK-NEXT: call spir_func void @f(ptr byval(%struct.S) align 4 [[COPY]])
define spir_func void @address_escapes(ptr %p, ptr addrspace(1) %out) {
  %s = alloca %struct.S, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %s, ptr align 4 %p, i64 408, i1 false)
  %asc = addrspacecast ptr %s to ptr addrspace(4)
  store ptr addrspace(4) %asc, ptr addrspace(1) %out, align 8
  call spir_func void @f(ptr byval(%struct.S) align 4 %s)
  ret void
}

; CHECK-LABEL: define spir_func void @read_by_memcpy_after_call(
; CHECK: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 %s, i64 408, i1 false)
; CHECK-NEXT: call spir_func void @f(ptr byval(%struct.S) align 4 [[COPY]])
define spir_func void @read_by_memcpy_after_call(ptr %p, ptr %q) {
  %s = alloca %struct.S, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %s, ptr align 4 %p, i64 408, i1 false)
  call spir_func void @f(ptr byval(%struct.S) align 4 %s)
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %q, ptr align 4 %s, i64 408, i1 false)
  ret void
}

; A call other than an sret one may read the object or keep its address.
; CHECK-LABEL: define spir_func void @passed_to_another_call_before(
; CHECK: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 %s, i64 408, i1 false)
; CHECK-NEXT: call spir_func void @f(ptr byval(%struct.S) align 4 [[COPY]])
define spir_func void @passed_to_another_call_before(ptr %p) {
  %s = alloca %struct.S, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %s, ptr align 4 %p, i64 408, i1 false)
  call spir_func void @use(ptr %s)
  call spir_func void @f(ptr byval(%struct.S) align 4 %s)
  ret void
}

; CHECK-LABEL: define spir_func void @sret_after_call(
; CHECK: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 %tmp, i64 408, i1 false)
; CHECK-NEXT: call spir_func void @f(ptr byval(%struct.S) align 4 [[COPY]])
define spir_func void @sret_after_call() {
  %tmp = alloca %struct.S, align 4
  call spir_func void @f(ptr byval(%struct.S) align 4 %tmp)
  call spir_func void @mk(ptr sret(%struct.S) align 4 %tmp)
  ret void
}

; A function's own byval parameter may be read again after the call.
; CHECK-LABEL: define spir_func void @own_param_passed_on(
; CHECK: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 %s, i64 408, i1 false)
; CHECK-NEXT: call spir_func void @f(ptr byval(%struct.S) align 4 [[COPY]])
define spir_func void @own_param_passed_on(ptr byval(%struct.S) align 4 %s) {
  call spir_func void @f(ptr byval(%struct.S) align 4 %s)
  ret void
}

; An OpenMP frontend may pass a generic pointer into global memory directly, without a temporary.
; CHECK-LABEL: define spir_func void @generic_pointer_operand(
; CHECK: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p4.i64(ptr align 4 [[COPY]], ptr addrspace(4) align 4 %p, i64 408, i1 false)
; CHECK-NEXT: [[ASC:%[0-9]+]] = addrspacecast ptr [[COPY]] to ptr addrspace(4)
; CHECK-NEXT: call spir_func void @fg(ptr addrspace(4) byval(%struct.S) align 4 [[ASC]])
define spir_func void @generic_pointer_operand(ptr addrspace(1) %g) {
  %p = addrspacecast ptr addrspace(1) %g to ptr addrspace(4)
  call spir_func void @fg(ptr addrspace(4) byval(%struct.S) align 4 %p)
  ret void
}

; CHECK-LABEL: define spir_func void @global_operand(
; CHECK: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 @gs, i64 408, i1 false)
; CHECK-NEXT: call spir_func void @f(ptr byval(%struct.S) align 4 [[COPY]])
define spir_func void @global_operand() {
  call spir_func void @f(ptr byval(%struct.S) align 4 @gs)
  ret void
}

; CHECK-LABEL: define spir_func void @indirect_call(
; CHECK: [[COPY:%[0-9]+]] = alloca %struct.S, align 4
; CHECK-NEXT: call void @llvm.memcpy.p0.p0.i64(ptr align 4 [[COPY]], ptr align 4 %p, i64 408, i1 false)
; CHECK-NEXT: call spir_func void %fp(ptr byval(%struct.S) align 4 [[COPY]])
define spir_func void @indirect_call(ptr %fp, ptr %p) {
  call spir_func void %fp(ptr byval(%struct.S) align 4 %p)
  ret void
}

; CHECK-LABEL: define spir_func void @readonly_param(
; CHECK-NOT: alloca
; CHECK: call spir_func i32 @fro(ptr byval(%struct.S) align 4 %p)
define spir_func void @readonly_param(ptr %p) {
  %v = call spir_func i32 @fro(ptr byval(%struct.S) align 4 %p)
  ret void
}

; CHECK-LABEL: define spir_func void @undef_operand(
; CHECK-NOT: alloca
; CHECK: call spir_func void @f(ptr byval(%struct.S) align 4 undef)
define spir_func void @undef_operand() {
  call spir_func void @f(ptr byval(%struct.S) align 4 undef)
  ret void
}

define spir_func void @f(ptr byval(%struct.S) align 4 %s) #0 {
  store i32 222, ptr %s, align 4
  ret void
}

define spir_func void @g(ptr byval(%struct.S) align 4 %s, ptr %q) #0 {
  store i32 222, ptr %s, align 4
  ret void
}

define spir_func void @fg(ptr addrspace(4) byval(%struct.S) align 4 %s) #0 {
  store i32 222, ptr addrspace(4) %s, align 4
  ret void
}

define spir_func void @h(ptr byval(%struct.S) align 4 %a, ptr byval(%struct.S) align 4 %b) #0 {
  store i32 222, ptr %a, align 4
  store i32 222, ptr %b, align 4
  ret void
}

define spir_func i32 @fro(ptr readonly byval(%struct.S) align 4 %s) #0 {
  %v = load i32, ptr %s, align 4
  ret i32 %v
}

define spir_func void @use(ptr %s) #0 {
  ret void
}

define spir_func void @mk(ptr sret(%struct.S) align 4 %r) #0 {
  store i32 1, ptr %r, align 4
  ret void
}

declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)
declare void @llvm.memcpy.p0.p4.i64(ptr, ptr addrspace(4), i64, i1)
declare void @llvm.memcpy.p4.p4.i64(ptr addrspace(4), ptr addrspace(4), i64, i1)
declare void @llvm.memset.p0.i64(ptr, i8, i64, i1)

attributes #0 = { noinline }
