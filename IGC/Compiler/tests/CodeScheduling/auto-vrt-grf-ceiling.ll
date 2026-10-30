;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
;
; The automatic GRF targets are the VRT modes that vISA's automatic selection can
; use: the largest GRF number per thread count, with modes above 256 GRF only
; with efficient 64-bit addressing, and no mode above vISA's GRF ceiling. NVL has
; a 256-GRF ceiling by default; CRI has one only without EnableCRIDefault512GRF.
;
; RUN: igc_opt --opaque-pointers -platformNvl --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableEfficient64b=1 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=NVL
; RUN: igc_opt --opaque-pointers -platformNvl --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableEfficient64b=1 --regkey ForceVRTGRFCeiling=512 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=NVL-CEILING-512
; RUN: igc_opt --opaque-pointers -platformNvl --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableEfficient64b=0 --regkey ForceVRTGRFCeiling=512 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=NVL-NO-EFFICIENT-64B
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableEfficient64b=1 --regkey EnableCRIDefault512GRF=0 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=CRI-CEILING-256
;
; NVL modes: {32..96 GRF, 10 threads} {128, 8} {160, 6} {192, 5} {256, 4} {320, 3}
; {448, 2} {512, 2}. 320 and 448 GRF need 320/448-GRF support, which is off here.
;
; NVL-LABEL: Function auto_grf
; NVL: Auto GRF scheduling: enabled
; NVL: Auto GRF targets: {96 GRF, 10 threads} {128 GRF, 8 threads} {160 GRF, 6 threads} {192 GRF, 5 threads}
; NVL-SAME: {256 GRF, 4 threads}{{$}}
;
; A raised ceiling adds the 512-GRF mode.
;
; NVL-CEILING-512-LABEL: Function auto_grf
; NVL-CEILING-512: Auto GRF targets: {96 GRF, 10 threads} {128 GRF, 8 threads} {160 GRF, 6 threads}
; NVL-CEILING-512-SAME: {192 GRF, 5 threads} {256 GRF, 4 threads} {512 GRF, 2 threads}{{$}}
;
; Without efficient 64-bit addressing the targets end at 256 GRF even with a raised
; ceiling.
;
; NVL-NO-EFFICIENT-64B-LABEL: Function auto_grf
; NVL-NO-EFFICIENT-64B: Auto GRF targets: {96 GRF, 10 threads} {128 GRF, 8 threads} {160 GRF, 6 threads}
; NVL-NO-EFFICIENT-64B-SAME: {192 GRF, 5 threads} {256 GRF, 4 threads}{{$}}
;
; CRI modes: {32..192 GRF, 8 threads} {256 GRF, 4 threads} {512 GRF, 4 threads}. A
; 256-GRF ceiling drops the 512-GRF mode but keeps 256 GRF for 4 threads.
;
; CRI-CEILING-256-LABEL: Function auto_grf
; CRI-CEILING-256: Auto GRF targets: {192 GRF, 8 threads} {256 GRF, 4 threads}{{$}}

define spir_kernel void @auto_grf(ptr addrspace(1) %dst) {
entry:
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef,
      i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
    <8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
