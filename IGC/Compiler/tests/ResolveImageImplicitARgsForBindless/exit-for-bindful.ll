;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -resolve-image-implicit-args-for-bindless --opaque-pointers -S %s | FileCheck %s

target triple = "spir64-unknown-unknown"

%spirv.Sampler = type opaque


define spir_func void @test_width(i64 %bindless_img) {
  ; CHECK-NOT: @llvm.genx.GenISA.ldraw
  %1 = call i32 @__builtin_IB_get_image_width(i64 %bindless_img)
  ret void
}


declare spir_func i32 @__builtin_IB_get_image_width(i64 noundef) local_unnamed_addr

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"UseBindlessImage", i1 false}
