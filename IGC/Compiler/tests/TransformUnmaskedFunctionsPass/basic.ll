;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --transform-unmasked -S < %s | FileCheck %s
; ------------------------------------------------

; Check that inline attribute is removed marked unmasked functions

; CHECK: define {{.*}} @foo() [[ATTR:#[0-9]*]]
; CHECK: [[ATTR]] = { noinline nounwind "sycl-unmasked" }

define spir_kernel void @test_const() {
entry:
  %call = call spir_func i32 @foo()
  ret void
}

; Function Attrs: nounwind
define spir_func i32 @foo() #0 {
entry:
  ret i32 undef
}

attributes #0 = { alwaysinline nounwind "sycl-unmasked" }
