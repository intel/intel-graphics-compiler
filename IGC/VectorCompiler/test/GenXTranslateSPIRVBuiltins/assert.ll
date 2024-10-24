;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt_typed_ptrs %use_old_pass_manager% -GenXTranslateSPIRVBuiltins \
; RUN: -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 \
; RUN: -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXTranslateSPIRVBuiltins \
; RUN: -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 \
; RUN: -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS
; ------------------------------------------------
; GenXTranslateSPIRVBuiltins
; ------------------------------------------------

; Function Attrs: mustprogress nofree nosync nounwind readnone willreturn
declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32) #2

; Function Attrs: noinline nounwind
; CHECK-LABEL: void @__assert_fail
define spir_func void @__assert_fail([31 x i8] addrspace(2)* %0, [11 x i8] addrspace(2)* %1, i32 %2, [56 x i8] addrspace(2)* %3) {
  %5 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0) #3
  ; Multi-call must be handled
; CHECK-TYPED-PTRS: [[ARG1:%[^ ]*]] = addrspacecast [31 x i8] addrspace(2)* %0 to i8 addrspace(4)*
; CHECK-TYPED-PTRS: [[ARG2:%[^ ]*]] = addrspacecast [11 x i8] addrspace(2)* %1 to i8 addrspace(4)*
; CHECK-TYPED-PTRS: [[ARG3:%[^ ]*]] = addrspacecast [56 x i8] addrspace(2)* %3 to i8 addrspace(4)*
; CHECK-TYPED-PTRS: call spir_func void @__devicelib_assert_fail(i8 addrspace(4)* [[ARG1]], i8 addrspace(4)* [[ARG2]], i32 %2, i8 addrspace(4)* [[ARG3]]
; CHECK-OPAQUE-PTRS: [[ARG1:%[^ ]*]] = addrspacecast ptr addrspace(2) %0 to ptr addrspace(4)
; CHECK-OPAQUE-PTRS: [[ARG2:%[^ ]*]] = addrspacecast ptr addrspace(2) %1 to ptr addrspace(4)
; CHECK-OPAQUE-PTRS: [[ARG3:%[^ ]*]] = addrspacecast ptr addrspace(2) %3 to ptr addrspace(4)
; CHECK-OPAQUE-PTRS: call spir_func void @__devicelib_assert_fail(ptr addrspace(4) [[ARG1]], ptr addrspace(4) [[ARG2]], i32 %2, ptr addrspace(4) [[ARG3]]
  call spir_func void @__devicelib_assert_fail([31 x i8] addrspace(2)* %0, [11 x i8] addrspace(2)* %1, i32 %2, [56 x i8] addrspace(2)* %3, i64 %5, i64 %5, i64 %5, i64 %5, i64 %5, i64 %5) #1
; CHECK-TYPED-PTRS: void @__devicelib_assert_fail(i8 addrspace(4)*{{.*}}, i8 addrspace(4)*{{.*}}, i32{{.*}}, i8 addrspace(4)*
; CHECK-OPAQUE-PTRS: void @__devicelib_assert_fail(ptr addrspace(4){{.*}}, ptr addrspace(4){{.*}}, i32{{.*}}, ptr addrspace(4)
  call spir_func void @__devicelib_assert_fail([31 x i8] addrspace(2)* %0, [11 x i8] addrspace(2)* %1, i32 %2, [56 x i8] addrspace(2)* %3, i64 %5, i64 %5, i64 %5, i64 %5, i64 %5, i64 %5) #1
  ret void
}

; Function Attrs: noinline nounwind
; CHECK-LABEL: define {{.*}} void @__devicelib_assert_fail
define spir_func void @__devicelib_assert_fail([31 x i8] addrspace(2)* %0, [11 x i8] addrspace(2)* %1, i32 %2, [56 x i8] addrspace(2)* %3, i64 %4, i64 %5, i64 %6, i64 %7, i64 %8, i64 %9) #1 {
; CHECK: tail call void @llvm.debugtrap()
; CHECK: tail call void @llvm.trap()
  ret void
}

attributes #2 = { mustprogress nofree nosync nounwind readnone willreturn }
attributes #3 = { nounwind readnone willreturn }
