;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %not_legacy %opt_legacy_typed %use_old_pass_manager% -GenXTranslateSPIRVBuiltins \
; RUN: -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 \
; RUN: -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S %s 2>&1 | FileCheck %s
; RUN: %not_new_pm %opt_new_pm_typed -passes=GenXTranslateSPIRVBuiltins \
; RUN: -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 \
; RUN: -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S %s 2>&1 | FileCheck %s
; ------------------------------------------------
; GenXTranslateSPIRVBuiltins
; ------------------------------------------------

; Function Attrs: mustprogress nofree nosync nounwind readnone willreturn
declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32) #2

; CHECK: LLVM ERROR: GenXTranslateSPIRVBuiltins failed for: <  call spir_func void @__devicelib_assert_fail
; CHECK: Unexpected function argument #9 type: i32, expected: i64

; Function Attrs: noinline nounwind
define spir_func void @__assert_fail(i16 addrspace(4)* %0, [11 x i8] addrspace(4)* %1, i32 %2, [56 x i8] addrspace(4)* %3) {
  %5 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0) #3
  call spir_func void @__devicelib_assert_fail(i16 addrspace(4)* %0, [11 x i8] addrspace(4)* %1, i32 %2, [56 x i8] addrspace(4)* %3, i64 %5, i64 %5, i64 %5, i64 %5, i64 %5, i32 %2) #1
  ret void
}

; Function Attrs: noinline nounwind
define spir_func void @__devicelib_assert_fail(i16 addrspace(4)* %0, [11 x i8] addrspace(4)* %1, i32 %2, [56 x i8] addrspace(4)* %3, i64 %4, i64 %5, i64 %6, i64 %7, i64 %8, i32 %9) #1 {
  ret void
}

attributes #2 = { mustprogress nofree nosync nounwind readnone willreturn }
attributes #3 = { nounwind readnone willreturn }
