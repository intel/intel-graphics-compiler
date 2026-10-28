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
; Test: when MaxWeight chooses a DPAS, CodeScheduling first schedules the ready
; conversions (fptrunc, float to bf16) whose result is only an A or B operand
; of DPAS instructions.
;
; Such a conversion gets its MaxWeight from the first of these DPAS. When that
; DPAS continues a DPAS chain, MaxWeight schedules the conversion after the
; DPAS of the other chains, right before its own DPAS. The conversion
; then separates DPAS that could issue back to back, and the register
; allocator can give its result the registers of the operand that the
; previous DPAS read.
;
; Not taken first: a conversion with a user that is not a DPAS, a conversion
; that feeds a DPAS accumulator, and any conversion in register pressure
; scheduling.
;
; No DPAS forwarding.
; RUN: igc_opt --opaque-pointers -platformbmg --regkey DisableCodeScheduling=0 \
; RUN:   --regkey TotalGRFNum=128 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:   --regkey CodeSchedulingRPThreshold=-512 \
; RUN:   --igc-code-scheduling --verify -S %s 2>&1 | FileCheck %s --check-prefixes=CHECK,BMG
;
; DPAS forwarding: the first DPAS of a forward pair waits for the operands of
; the second one.
; RUN: igc_opt --opaque-pointers -platformCri --regkey DisableCodeScheduling=0 \
; RUN:   --regkey TotalGRFNum=128 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:   --regkey CodeSchedulingRPThreshold=-512 \
; RUN:   --igc-code-scheduling --verify -S %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CRI
;
; Register pressure scheduling.
; RUN: igc_opt --opaque-pointers -platformbmg --regkey DisableCodeScheduling=0 \
; RUN:   --regkey TotalGRFNum=128 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:   --regkey CodeSchedulingRPThreshold=-512 --regkey CodeSchedulingForceRPOnly=1 \
; RUN:   --igc-code-scheduling --verify -S %s 2>&1 | FileCheck %s --check-prefix=RPONLY

; Two DPAS chains. The second DPAS of each chain reads a converted operand.
; Without the heuristic, %x.conv2 and %y.conv2 come after the chain heads
; %x.dpas1 and %y.dpas1 and separate them from %x.dpas2 and %y.dpas2. Now all
; conversions come first and the four DPAS are back to back.
;
; CHECK-LABEL: define spir_kernel void @second_operands(
; CHECK-NOT:   sub.group.dpas
; CHECK:       %x.conv2 = fptrunc
; CHECK-NOT:   sub.group.dpas
; CHECK:       %y.conv2 = fptrunc
; CHECK-NEXT:  %y.a2 = bitcast
; BMG-NEXT:    %x.dpas1 = call
; BMG-NEXT:    %y.dpas1 = call
; BMG-NEXT:    %x.dpas2 = call
; BMG-NEXT:    %y.dpas2 = call
; CRI-NEXT:    %x.dpas1 = call
; CRI-NEXT:    %x.dpas2 = call
; CRI-NEXT:    %y.dpas1 = call
; CRI-NEXT:    %y.dpas2 = call
;
; RPONLY-LABEL: define spir_kernel void @second_operands(
; RPONLY:       %x.dpas1 = call
; RPONLY:       %x.conv2 = fptrunc
define spir_kernel void @second_operands(<8 x float> %x.in1, <8 x float> %x.in2, <8 x float> %y.in1, <8 x float> %y.in2, <8 x i32> %b, ptr addrspace(1) %out) {
entry:
  %x.conv1 = fptrunc <8 x float> %x.in1 to <8 x half>
  %x.a1 = bitcast <8 x half> %x.conv1 to <8 x i16>
  %y.conv1 = fptrunc <8 x float> %y.in1 to <8 x half>
  %y.a1 = bitcast <8 x half> %y.conv1 to <8 x i16>
  %x.dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %x.a1, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  %y.dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %y.a1, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  %x.conv2 = fptrunc <8 x float> %x.in2 to <8 x half>
  %x.a2 = bitcast <8 x half> %x.conv2 to <8 x i16>
  %x.dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x.dpas1, <8 x i16> %x.a2, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  %y.conv2 = fptrunc <8 x float> %y.in2 to <8 x half>
  %y.a2 = bitcast <8 x half> %y.conv2 to <8 x i16>
  %y.dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %y.dpas1, <8 x i16> %y.a2, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  %sum = fadd <8 x float> %x.dpas2, %y.dpas2
  store <8 x float> %sum, ptr addrspace(1) %out
  ret void
}

; With DPAS forwarding, %x.dpas1 and %x.dpas2 form a forward pair, and
; %x.dpas1 waits until %x.dpas2 has its operand %x.a2. Without the heuristic,
; MaxWeight schedules the pair (%y.dpas1, %y.dpas2) first and %x.conv2 between
; the two pairs. Now %x.conv2 comes first, and the four DPAS are back to back.
;
; CRI-LABEL: define spir_kernel void @forward_pair(
; CRI-NEXT:  entry:
; CRI-NEXT:    %x.conv2 = fptrunc
; CRI-NEXT:    %x.a2 = bitcast
; CRI-NEXT:    %y.dpas1 = call
; CRI-NEXT:    %y.dpas2 = call
; CRI-NEXT:    %x.dpas1 = call
; CRI-NEXT:    %x.dpas2 = call
define spir_kernel void @forward_pair(<8 x i16> %a, <8 x i32> %b, <8 x i32> %b2, <8 x float> %x.in2, ptr addrspace(1) %out) {
entry:
  %y.dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b2, i32 12, i32 12, i32 8, i32 8, i1 false)
  %y.dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %y.dpas1, <8 x i16> %a, <8 x i32> %b2, i32 12, i32 12, i32 8, i32 8, i1 false)
  %x.dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  %x.conv2 = fptrunc <8 x float> %x.in2 to <8 x half>
  %x.a2 = bitcast <8 x half> %x.conv2 to <8 x i16>
  %x.dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x.dpas1, <8 x i16> %x.a2, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  %sum = fadd <8 x float> %x.dpas2, %y.dpas2
  store <8 x float> %sum, ptr addrspace(1) %out
  ret void
}

; The float to bf16 conversion gives the A operand without a bitcast.
;
; CHECK-LABEL: define spir_kernel void @bf16_operand(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %x.conv2 = call <8 x i16> @llvm.genx.GenISA.ftobf
define spir_kernel void @bf16_operand(<8 x i16> %a, <8 x float> %x.in2, <8 x i32> %b, ptr addrspace(1) %out) {
entry:
  %x.dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %y.dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %x.conv2 = call <8 x i16> @llvm.genx.GenISA.ftobf.v8i16.v8f32(<8 x float> %x.in2, i32 0)
  %x.dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x.dpas1, <8 x i16> %x.conv2, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %y.dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %y.dpas1, <8 x i16> %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 8, i1 false)
  %sum = fadd <8 x float> %x.dpas2, %y.dpas2
  store <8 x float> %sum, ptr addrspace(1) %out
  ret void
}

; The conversion %x.conv2 is ready from the start, but it is taken only when
; MaxWeight chooses a DPAS: %y.mul and %y.conv1 lead to the longer chain
; %y.dpas1 - %y.dpas3 and come first, then %x.conv2 comes before %y.dpas1.
;
; CHECK-LABEL: define spir_kernel void @before_chosen_dpas(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %y.mul = fmul
; CHECK-NEXT:    %y.conv1 = fptrunc
; CHECK-NEXT:    %y.a1 = bitcast
; CHECK-NEXT:    %x.conv2 = fptrunc
; CHECK-NEXT:    %x.a2 = bitcast
; CHECK-NEXT:    %y.dpas1 = call
define spir_kernel void @before_chosen_dpas(<8 x i16> %a, <8 x float> %x.in2, <8 x float> %y.in1, <8 x i32> %b, ptr addrspace(1) %out) {
entry:
  %x.dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  %x.conv2 = fptrunc <8 x float> %x.in2 to <8 x half>
  %x.a2 = bitcast <8 x half> %x.conv2 to <8 x i16>
  %x.dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x.dpas1, <8 x i16> %x.a2, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  %y.mul = fmul <8 x float> %y.in1, %y.in1
  %y.conv1 = fptrunc <8 x float> %y.mul to <8 x half>
  %y.a1 = bitcast <8 x half> %y.conv1 to <8 x i16>
  %y.dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %y.a1, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  %y.dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %y.dpas1, <8 x i16> %a, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  %y.dpas3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %y.dpas2, <8 x i16> %a, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  %sum = fadd <8 x float> %x.dpas2, %y.dpas3
  store <8 x float> %sum, ptr addrspace(1) %out
  ret void
}

; %x.conv2 is also stored, and %acc.conv gives the accumulator %acc of
; %z.dpas1: neither is taken first, so the order is the MaxWeight order.
;
; CHECK-LABEL: define spir_kernel void @not_only_dpas_operand(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    %{{x|y}}.dpas1 = call
; CHECK:         %x.conv2 = fptrunc
; CHECK:         %x.dpas2 = call
; CHECK:         %acc.conv = fptrunc
define spir_kernel void @not_only_dpas_operand(<8 x i16> %a, <8 x float> %x.in2, <8 x float> %acc.in, <8 x i32> %b, ptr addrspace(1) %out, ptr addrspace(1) %out2) {
entry:
  %x.dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  %y.dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  %x.conv2 = fptrunc <8 x float> %x.in2 to <8 x half>
  %x.a2 = bitcast <8 x half> %x.conv2 to <8 x i16>
  %x.dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %x.dpas1, <8 x i16> %x.a2, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  store <8 x half> %x.conv2, ptr addrspace(1) %out2
  %acc.conv = fptrunc <8 x float> %acc.in to <8 x half>
  %acc = bitcast <8 x half> %acc.conv to <8 x i16>
  %z.dpas1 = call <8 x i16> @llvm.genx.GenISA.sub.group.dpas.v8i16.v8i16.v8i16.v8i32(<8 x i16> %acc, <8 x i16> %a, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  %y.dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %y.dpas1, <8 x i16> %a, <8 x i32> %b, i32 12, i32 12, i32 8, i32 8, i1 false)
  %sum = fadd <8 x float> %x.dpas2, %y.dpas2
  store <8 x float> %sum, ptr addrspace(1) %out
  %out.z = getelementptr inbounds i8, ptr addrspace(1) %out, i64 64
  store <8 x i16> %z.dpas1, ptr addrspace(1) %out.z
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare <8 x i16> @llvm.genx.GenISA.sub.group.dpas.v8i16.v8i16.v8i16.v8i32(<8 x i16>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare <8 x i16> @llvm.genx.GenISA.ftobf.v8i16.v8f32(<8 x float>, i32)
