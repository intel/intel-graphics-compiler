;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-gen-ir-lowering -S < %s | FileCheck %s
;
; A bfloat clamp to [0, 1] must not be folded into GenISA_fsat, because vISA
; does not support saturation on a bfloat destination. Since LLVM 23 such
; clamps reach IGC in bfloat (e.g. from the bfloat smoothstep builtin).

define bfloat @clamp_min_of_max(bfloat %x) {
; CHECK-LABEL: @clamp_min_of_max(
; CHECK-NOT:     @llvm.genx.GenISA.fsat
; CHECK:         [[MAX:%.*]] = call bfloat @llvm.maxnum.bf16(bfloat %x, bfloat {{.*}})
; CHECK:         [[MIN:%.*]] = call bfloat @llvm.minnum.bf16(bfloat [[MAX]], bfloat {{.*}})
; CHECK:         ret bfloat [[MIN]]
  %max = call bfloat @llvm.maxnum.bf16(bfloat %x, bfloat 0xR0000)
  %min = call bfloat @llvm.minnum.bf16(bfloat %max, bfloat 0xR3F80)
  ret bfloat %min
}

define bfloat @clamp_max_of_min(bfloat %x) {
; CHECK-LABEL: @clamp_max_of_min(
; CHECK-NOT:     @llvm.genx.GenISA.fsat
; CHECK:         [[MIN:%.*]] = call bfloat @llvm.minnum.bf16(bfloat %x, bfloat {{.*}})
; CHECK:         [[MAX:%.*]] = call bfloat @llvm.maxnum.bf16(bfloat [[MIN]], bfloat {{.*}})
; CHECK:         ret bfloat [[MAX]]
  %min = call bfloat @llvm.minnum.bf16(bfloat %x, bfloat 0xR3F80)
  %max = call bfloat @llvm.maxnum.bf16(bfloat %min, bfloat 0xR0000)
  ret bfloat %max
}

declare bfloat @llvm.maxnum.bf16(bfloat, bfloat)
declare bfloat @llvm.minnum.bf16(bfloat, bfloat)

; GenIRLowering only runs on functions listed in igc.functions.
!igc.functions = !{!0, !1}
!0 = !{ptr @clamp_min_of_max, !2}
!1 = !{ptr @clamp_max_of_min, !2}
!2 = !{}
