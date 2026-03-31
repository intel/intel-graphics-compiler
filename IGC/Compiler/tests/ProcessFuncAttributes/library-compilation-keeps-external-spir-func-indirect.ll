;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-cleanup-indirectly-referenced-functions -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
;
; In library compilation mode (IsLibraryCompilation = true), a spir_func with
; external linkage must keep its "referenced-indirectly" attribute even when its
; address is never taken locally.  A function that is internal (not external)
; must still have the attribute removed when its address is not taken.

; @exported_lib_func is external + spir_func + library mode → attribute must be kept.
; CHECK-LABEL: define spir_func void @exported_lib_func(
; CHECK-SAME: ) #[[KEEP:[0-9]+]]

; @internal_lib_func is internal + spir_func + library mode → attribute must be removed.
; After removal the function carries no "referenced-indirectly" attribute.
; CHECK-LABEL: define internal spir_func void @internal_lib_func(
; CHECK-NOT:   "referenced-indirectly"
; CHECK-LABEL: define spir_kernel void @kernel(

; Confirm that the preserved attribute set contains both expected attrs.
; CHECK: attributes #[[KEEP]] = {{.*}}"referenced-indirectly"{{.*}}"visaStackCall"

; External spir_func in library mode: "referenced-indirectly" must be preserved.
define spir_func void @exported_lib_func() #0 {
  ret void
}

; Internal spir_func in library mode: condition not met, attribute must be removed.
define internal spir_func void @internal_lib_func() #0 {
  ret void
}

define spir_kernel void @kernel() {
  call spir_func void @exported_lib_func()
  call spir_func void @internal_lib_func()
  ret void
}

attributes #0 = { "referenced-indirectly" "visaStackCall" }

!IGCMetadata = !{!0}
!igc.functions = !{!10}

; ModuleMD with IsLibraryCompilation = true
!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"IsLibraryCompilation", i1 true}

!10 = !{ptr @kernel, !11}
!11 = !{!12}
!12 = !{!"function_type", i32 0}
