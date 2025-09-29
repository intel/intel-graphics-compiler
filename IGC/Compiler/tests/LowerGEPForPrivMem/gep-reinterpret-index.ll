;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-priv-mem-to-reg -S < %s 2>&1 | FileCheck %s

; Verify correct scalarized GEP index advancement for reinterpreted vectors.
; Before the fix, when storing a <8 x i32> vector into a promoted private alloca
; whose base lane type is double (8 bytes), the legacy scalarization advanced
; the linear index by 8 (number of i32 elements) instead of 4 (bytesCovered / promotedLaneBytes),
; creating gaps (0..3, 8..11, ...) and skipping indices 4..7.
; After the fix, the second vector store must populate lanes 4..7.

; CHECK-LABEL: @test(
; CHECK: alloca <16 x double>
; First vector packet populates lanes 0..3
; CHECK: insertelement <16 x double> {{.*}}, double {{.*}}, i32 0
; CHECK: insertelement <16 x double> {{.*}}, double {{.*}}, i32 1
; CHECK: insertelement <16 x double> {{.*}}, double {{.*}}, i32 2
; CHECK: insertelement <16 x double> {{.*}}, double {{.*}}, i32 3
; CHECK: store <16 x double>
; Second vector packet must immediately fill lanes 4..7 (no gap / no i32 8 yet)
; CHECK: insertelement <16 x double> {{.*}}, double {{.*}}, i32 4
; CHECK: insertelement <16 x double> {{.*}}, double {{.*}}, i32 5
; CHECK: insertelement <16 x double> {{.*}}, double {{.*}}, i32 6
; CHECK: insertelement <16 x double> {{.*}}, double {{.*}}, i32 7

define spir_kernel void @test(ptr addrspace(1) %src) {
entry:
  ; Private array to be promoted: 16 doubles (128 bytes). Each <8 x i32> (32 bytes) spans 4 double lanes.
  %priv = alloca [16 x double], align 8

  ; Base pointer to first double.
  %base = getelementptr inbounds [16 x double], ptr %priv, i32 0, i32 0

  ; Load first packet and store.
  %src.vec0 = load <8 x i32>, ptr addrspace(1) %src, align 32
  store <8 x i32> %src.vec0, ptr %base, align 32

  ; Second packet (should map immediately after first: lanes 4..7 post-promotion).
  %src.vec1.ptr = getelementptr <8 x i32>, ptr addrspace(1) %src, i32 1
  %src.vec1 = load <8 x i32>, ptr addrspace(1) %src.vec1.ptr, align 32
  %vec.cast.1 = getelementptr <8 x i32>, ptr %base, i32 1
  store <8 x i32> %src.vec1, ptr %vec.cast.1, align 32

  ret void
}

!igc.functions = !{!1}
!1 = !{ptr @test, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
