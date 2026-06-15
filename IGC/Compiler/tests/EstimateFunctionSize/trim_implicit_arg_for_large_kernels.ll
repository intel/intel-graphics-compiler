;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; Test for trimming implicit-arg functions in large kernels.
;
; A function that uses an implicit argument is force-inlined by default
; (ControlInlineImplicitArgs), so it is normally excluded from trimming. When the
; kernel is still above threshold after trimming all ordinary functions, the
; TrimImplicitArgFunctionsForLargeKernels flag lets the trimmer undo its work and
; retry while also allowing the implicit-arg function to be trimmed.
;
; REQUIRES: regkeys
; RUN: igc_opt --EstimateFunctionSize --regkey PrintControlKernelTotalSize=0xF --regkey SubroutineThreshold=50 --regkey KernelTotalSizeThreshold=50 --regkey LargeKernelThresholdMultiplier=1 --regkey ControlInlineTinySize=10 --regkey TrimImplicitArgFunctionsForLargeKernels=0 -disable-output 2>&1 < %s | FileCheck %s -check-prefix=CHECK-NORETRY
; RUN: igc_opt --EstimateFunctionSize --regkey PrintControlKernelTotalSize=0xF --regkey SubroutineThreshold=50 --regkey KernelTotalSizeThreshold=50 --regkey LargeKernelThresholdMultiplier=1 --regkey ControlInlineTinySize=10 --regkey TrimImplicitArgFunctionsForLargeKernels=1 -disable-output 2>&1 < %s | FileCheck %s -check-prefix=CHECK-RETRY

; Without the flag: the implicit-arg function is force-inlined and excluded from
; trimming, and no implicit-arg retry happens.
; CHECK-NORETRY: Can't trim (not best effort inline), implicit_arg_fn, Function Attribute: Force inline
; CHECK-NORETRY-NOT: untrimming and retrying including implicit-arg functions
; CHECK-NORETRY-NOT: Trim the function, implicit_arg_fn

; With the flag: the kernel is still above threshold after the first pass, so the
; trimmer retries and trims the implicit-arg function as well.
; CHECK-RETRY: untrimming and retrying including implicit-arg functions
; CHECK-RETRY: Trim the function, implicit_arg_fn

declare spir_func i32 @__builtin_IB_get_local_id_x()

define internal spir_func void @ordinary_one() {
  %v1 = add i32 0, 0
  %v2 = add i32 %v1, 1
  %v3 = add i32 %v2, 1
  %v4 = add i32 %v3, 1
  %v5 = add i32 %v4, 1
  %v6 = add i32 %v5, 1
  %v7 = add i32 %v6, 1
  %v8 = add i32 %v7, 1
  %v9 = add i32 %v8, 1
  %v10 = add i32 %v9, 1
  %v11 = add i32 %v10, 1
  %v12 = add i32 %v11, 1
  %v13 = add i32 %v12, 1
  %v14 = add i32 %v13, 1
  %v15 = add i32 %v14, 1
  %v16 = add i32 %v15, 1
  %v17 = add i32 %v16, 1
  %v18 = add i32 %v17, 1
  %v19 = add i32 %v18, 1
  %v20 = add i32 %v19, 1
  ret void
}

define internal spir_func void @ordinary_two() {
  %v1 = add i32 0, 0
  %v2 = add i32 %v1, 1
  %v3 = add i32 %v2, 1
  %v4 = add i32 %v3, 1
  %v5 = add i32 %v4, 1
  %v6 = add i32 %v5, 1
  %v7 = add i32 %v6, 1
  %v8 = add i32 %v7, 1
  %v9 = add i32 %v8, 1
  %v10 = add i32 %v9, 1
  %v11 = add i32 %v10, 1
  %v12 = add i32 %v11, 1
  %v13 = add i32 %v12, 1
  %v14 = add i32 %v13, 1
  %v15 = add i32 %v14, 1
  %v16 = add i32 %v15, 1
  %v17 = add i32 %v16, 1
  %v18 = add i32 %v17, 1
  %v19 = add i32 %v18, 1
  %v20 = add i32 %v19, 1
  ret void
}

define internal spir_func void @implicit_arg_fn() {
  %id = call spir_func i32 @__builtin_IB_get_local_id_x()
  %v2 = add i32 %id, 1
  %v3 = add i32 %v2, 1
  %v4 = add i32 %v3, 1
  %v5 = add i32 %v4, 1
  %v6 = add i32 %v5, 1
  %v7 = add i32 %v6, 1
  %v8 = add i32 %v7, 1
  %v9 = add i32 %v8, 1
  %v10 = add i32 %v9, 1
  %v11 = add i32 %v10, 1
  %v12 = add i32 %v11, 1
  %v13 = add i32 %v12, 1
  %v14 = add i32 %v13, 1
  %v15 = add i32 %v14, 1
  %v16 = add i32 %v15, 1
  %v17 = add i32 %v16, 1
  %v18 = add i32 %v17, 1
  %v19 = add i32 %v18, 1
  %v20 = add i32 %v19, 1
  ret void
}

define spir_kernel void @big_kernel() {
  call spir_func void @ordinary_one()
  call spir_func void @ordinary_one()
  call spir_func void @ordinary_one()
  call spir_func void @ordinary_two()
  call spir_func void @ordinary_two()
  call spir_func void @ordinary_two()
  call spir_func void @implicit_arg_fn()
  call spir_func void @implicit_arg_fn()
  call spir_func void @implicit_arg_fn()
  ret void
}
