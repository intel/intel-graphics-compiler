;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-process-func-attributes -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
;
; Checks that alwaysinline attibute is set for __builtin_ and not set for simple function
;

; CHECK: ; Function Attrs: alwaysinline
; CHECK-NEXT: define internal void @__builtin_spirv_func(){{.*}} [[ATTR1:#[0-9]*]]
define void @__builtin_spirv_func() #0 {
  ret void
}

; CHECK: ; Function Attrs: noinline optnone
; CHECK-NEXT: define internal spir_func void @foo(){{.*}} [[ATTR2:#[0-9]*]]
define internal spir_func void @foo() #0 {
  ret void
}

; CHECK-DAG: attributes [[ATTR1]] = { alwaysinline }
; CHECK-DAG: attributes [[ATTR2]] = { noinline optnone }
attributes #0 = { noinline optnone }

!igc.functions = !{}
