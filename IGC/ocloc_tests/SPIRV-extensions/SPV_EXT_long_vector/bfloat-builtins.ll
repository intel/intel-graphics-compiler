;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; BiF implements bfloat builtins on i16, and BfloatBuiltinsResolution redirects
; the calls only after LongVectorBuiltinScalarization, so the scalar bfloat
; variant counts as defined in BiF.

; REQUIRES: llvm-spirv, regkeys, cri-supported, opaque-pointers
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc -o %t.spv --spirv-ext=+SPV_INTEL_sigmoid,+SPV_KHR_bfloat16,+SPV_INTEL_bfloat16_arithmetic,+SPV_EXT_long_vector
; RUN: ocloc compile -device cri -spirv_input -file %t.spv -options " -igc_opts 'PrintToConsole=1 PrintAfter=igc-long-vector-builtin-scalarization'" 2>&1 \
; RUN:   | FileCheck %s --implicit-check-not="undefined reference"

; CHECK-LABEL: define spir_kernel void @sigmoid(
; CHECK-COUNT-5: call spir_func bfloat @_Z21__spirv_FSigmoidINTELDF16b(bfloat
; CHECK:       long.vector.loop:
; CHECK:         call spir_func bfloat @_Z21__spirv_FSigmoidINTELDF16b(bfloat
; CHECK:       Build succeeded

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spir64-unknown-unknown"

declare spir_func <5 x bfloat> @_Z21__spirv_FSigmoidINTELDv5_DF16b(<5 x bfloat>)
declare spir_func <17 x bfloat> @_Z21__spirv_FSigmoidINTELDv17_DF16b(<17 x bfloat>)

define spir_kernel void @sigmoid(ptr addrspace(1) %io5, ptr addrspace(1) %io17) {
  %v5 = load <5 x bfloat>, ptr addrspace(1) %io5, align 16
  %r5 = call spir_func <5 x bfloat> @_Z21__spirv_FSigmoidINTELDv5_DF16b(<5 x bfloat> %v5)
  store <5 x bfloat> %r5, ptr addrspace(1) %io5, align 16
  %v17 = load <17 x bfloat>, ptr addrspace(1) %io17, align 64
  %r17 = call spir_func <17 x bfloat> @_Z21__spirv_FSigmoidINTELDv17_DF16b(<17 x bfloat> %v17)
  store <17 x bfloat> %r17, ptr addrspace(1) %io17, align 64
  ret void
}
