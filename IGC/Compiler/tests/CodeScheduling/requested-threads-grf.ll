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
; A "num-thread-per-eu" request fixes the GRF number, so the scheduler plans for
; that number instead of choosing among the automatic VRT targets. The request
; takes the largest GRF number of the modes with that many threads that vISA's
; automatic selection can use; modes above 256 GRF need efficient 64-bit
; addressing. When automatic selection can use none of them, the request takes
; the first one, as vISA's thread mapping does. A thread count without a mode
; takes the next larger count, or the largest count if every mode has fewer
; threads. The GRF ceiling of vISA's automatic selection does not limit a thread
; request. Without VRT the modes are 128 GRF with 8 threads and 256 GRF with 4.
;
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableEfficient64b=1 --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=CRI
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableEfficient64b=0 --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=CRI-NO-EFFICIENT-64B
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableEfficient64b=1 --regkey EnableCRIDefault512GRF=0 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=CRI-CEILING-256
; RUN: igc_opt --opaque-pointers -platformPtl --regkey DisableCodeScheduling=0 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=PTL
; RUN: igc_opt --opaque-pointers -platformNvl --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableEfficient64b=1 --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=NVL
; RUN: igc_opt --opaque-pointers -platformNvl --regkey DisableCodeScheduling=0 \
; RUN:   --regkey EnableEfficient64b=0 --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=NVL-NO-EFFICIENT-64B
; RUN: igc_opt --opaque-pointers -platformbmg --regkey DisableCodeScheduling=0 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 \
; RUN:   --igc-code-scheduling --verify -disable-output %s 2>&1 \
; RUN:   | FileCheck %s --check-prefix=BMG
;
; CRI modes: {32..192 GRF, 8 threads} {256 GRF, 4 threads} {512 GRF, 4 threads}.
; 10 threads get the most threads CRI has, 5 threads get 8, and 3 and 2 threads
; get 4.
;
; CRI-LABEL: Function no_request
; CRI: Auto GRF scheduling: enabled
; CRI-LABEL: Function threads_10
; CRI: Auto GRF scheduling: disabled
; CRI-NEXT: Fixed GRF scheduling: grf=192
; CRI-LABEL: Function threads_8
; CRI: Auto GRF scheduling: disabled
; CRI-NEXT: Fixed GRF scheduling: grf=192
; CRI-LABEL: Function threads_5
; CRI: Auto GRF scheduling: disabled
; CRI-NEXT: Fixed GRF scheduling: grf=192
; CRI-LABEL: Function threads_4
; CRI: Auto GRF scheduling: disabled
; CRI-NEXT: Fixed GRF scheduling: grf=512
; CRI-LABEL: Function threads_3
; CRI: Auto GRF scheduling: disabled
; CRI-NEXT: Fixed GRF scheduling: grf=512
; CRI-LABEL: Function threads_2
; CRI: Auto GRF scheduling: disabled
; CRI-NEXT: Fixed GRF scheduling: grf=512
;
; Without efficient 64-bit addressing vISA cannot use 512 GRF, so 4 threads get 256.
;
; CRI-NO-EFFICIENT-64B-LABEL: Function threads_4
; CRI-NO-EFFICIENT-64B: Auto GRF scheduling: disabled
; CRI-NO-EFFICIENT-64B-NEXT: Fixed GRF scheduling: grf=256
; CRI-NO-EFFICIENT-64B-LABEL: Function threads_3
; CRI-NO-EFFICIENT-64B: Auto GRF scheduling: disabled
; CRI-NO-EFFICIENT-64B-NEXT: Fixed GRF scheduling: grf=256
; CRI-NO-EFFICIENT-64B-LABEL: Function threads_2
; CRI-NO-EFFICIENT-64B: Auto GRF scheduling: disabled
; CRI-NO-EFFICIENT-64B-NEXT: Fixed GRF scheduling: grf=256
;
; A 256-GRF ceiling does not limit a thread request: 4 threads still get 512 GRF.
;
; CRI-CEILING-256-LABEL: Function threads_4
; CRI-CEILING-256: Auto GRF scheduling: disabled
; CRI-CEILING-256-NEXT: Fixed GRF scheduling: grf=512
;
; PTL modes: {32..96 GRF, 10 threads} {128, 8} {160, 6} {192, 5} {256, 4}.
;
; PTL-LABEL: Function threads_10
; PTL: Auto GRF scheduling: disabled
; PTL-NEXT: Fixed GRF scheduling: grf=96
; PTL-LABEL: Function threads_8
; PTL: Auto GRF scheduling: disabled
; PTL-NEXT: Fixed GRF scheduling: grf=128
; PTL-LABEL: Function threads_5
; PTL: Auto GRF scheduling: disabled
; PTL-NEXT: Fixed GRF scheduling: grf=192
; PTL-LABEL: Function threads_4
; PTL: Auto GRF scheduling: disabled
; PTL-NEXT: Fixed GRF scheduling: grf=256
; PTL-LABEL: Function threads_3
; PTL: Auto GRF scheduling: disabled
; PTL-NEXT: Fixed GRF scheduling: grf=256
; PTL-LABEL: Function threads_2
; PTL: Auto GRF scheduling: disabled
; PTL-NEXT: Fixed GRF scheduling: grf=256
;
; NVL modes: the PTL modes, {320, 3}, {448, 2} and {512, 2}. vISA's automatic
; selection on NVL has a 256-GRF ceiling, which does not limit a thread request.
; 320 and 448 GRF need 320/448-GRF support, which is off here, so 3 threads take
; the first 3-thread mode, 320. Without efficient 64-bit addressing automatic
; selection can use neither 2-thread mode, so 2 threads take the first one, 448.
;
; NVL-LABEL: Function threads_3
; NVL: Auto GRF scheduling: disabled
; NVL-NEXT: Fixed GRF scheduling: grf=320
; NVL-LABEL: Function threads_2
; NVL: Auto GRF scheduling: disabled
; NVL-NEXT: Fixed GRF scheduling: grf=512
;
; NVL-NO-EFFICIENT-64B-LABEL: Function threads_3
; NVL-NO-EFFICIENT-64B: Auto GRF scheduling: disabled
; NVL-NO-EFFICIENT-64B-NEXT: Fixed GRF scheduling: grf=320
; NVL-NO-EFFICIENT-64B-LABEL: Function threads_2
; NVL-NO-EFFICIENT-64B: Auto GRF scheduling: disabled
; NVL-NO-EFFICIENT-64B-NEXT: Fixed GRF scheduling: grf=448
;
; BMG has no VRT: {128 GRF, 8 threads} {256 GRF, 4 threads}. 10 and 5 threads get
; 8, and 3 and 2 threads get 4. A kernel without a request keeps 128 GRF.
;
; BMG-LABEL: Function no_request
; BMG: Fixed GRF scheduling: grf=128
; BMG-LABEL: Function threads_10
; BMG: Auto GRF scheduling: disabled
; BMG-NEXT: Fixed GRF scheduling: grf=128
; BMG-LABEL: Function threads_8
; BMG: Auto GRF scheduling: disabled
; BMG-NEXT: Fixed GRF scheduling: grf=128
; BMG-LABEL: Function threads_5
; BMG: Auto GRF scheduling: disabled
; BMG-NEXT: Fixed GRF scheduling: grf=128
; BMG-LABEL: Function threads_4
; BMG: Auto GRF scheduling: disabled
; BMG-NEXT: Fixed GRF scheduling: grf=256
; BMG-LABEL: Function threads_3
; BMG: Auto GRF scheduling: disabled
; BMG-NEXT: Fixed GRF scheduling: grf=256
; BMG-LABEL: Function threads_2
; BMG: Auto GRF scheduling: disabled
; BMG-NEXT: Fixed GRF scheduling: grf=256

define spir_kernel void @no_request(ptr addrspace(1) %dst) {
entry:
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef,
      i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  ret void
}

define spir_kernel void @threads_10(ptr addrspace(1) %dst) #0 {
entry:
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef,
      i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  ret void
}

define spir_kernel void @threads_8(ptr addrspace(1) %dst) #1 {
entry:
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef,
      i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  ret void
}

define spir_kernel void @threads_5(ptr addrspace(1) %dst) #3 {
entry:
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef,
      i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  ret void
}

define spir_kernel void @threads_4(ptr addrspace(1) %dst) #2 {
entry:
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef,
      i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  ret void
}

define spir_kernel void @threads_3(ptr addrspace(1) %dst) #5 {
entry:
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef,
      i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  ret void
}

define spir_kernel void @threads_2(ptr addrspace(1) %dst) #4 {
entry:
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
      <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef,
      i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
    <8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

attributes #0 = { "num-thread-per-eu"="10" }
attributes #1 = { "num-thread-per-eu"="8" }
attributes #2 = { "num-thread-per-eu"="4" }
attributes #3 = { "num-thread-per-eu"="5" }
attributes #4 = { "num-thread-per-eu"="2" }
attributes #5 = { "num-thread-per-eu"="3" }
