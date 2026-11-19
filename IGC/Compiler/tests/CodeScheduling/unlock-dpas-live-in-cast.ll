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
; RUN: igc_opt --opaque-pointers --regkey DisableCodeScheduling=0 --regkey EnableCodeSchedulingIfNoSpills=1 \
; RUN:   --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 --regkey CodeSchedulingRPThreshold=-512 \
; RUN:   --regkey ForceOCLSIMDWidth=16 --regkey CodeSchedulingForceRPOnly=1 --igc-code-scheduling -disable-output %s 2>&1 | FileCheck %s
;
; The register pressure heuristics place first a load that unlocks a DPAS, a DPAS
; whose other operands are placed. An operand from another block is available
; at the start of the block, also when the pressure counts its registers at the
; vector it is a part of. In %next, the other operand of %dy is live-in, so %yb
; alone unlocks %dy, and %yb is placed first.
;
; %c2 is a no-op cast of a live-in value.
;
; CHECK-LABEL: Function unlock_dpas_of_live_in_cast
; CHECK: Original schedule: next
; CHECK: Greedy RP attempt
; CHECK-NEXT: ({{[0-9]+}}, {{[0-9]+}} {{.*}}) RP:{{.*}} %yb = call

define spir_kernel void @unlock_dpas_of_live_in_cast(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ldA = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 4, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %c1 = bitcast <8 x i16> %ld to <8 x half>
  %c2 = bitcast <8 x half> %c1 to <8 x i16>
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %da = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %da, ptr addrspace(1) %dst, align 32
  br label %next
next:
  %ya = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 64, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %yb = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 80, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dy = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %c2, <8 x i32> %yb, i32 1, i32 1, i32 1, i32 1, i1 false)
  %dx = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %ldA, <8 x i32> %ya, i32 1, i32 1, i32 1, i32 1, i1 false)
  %p1_dx = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <8 x float> %dx, ptr addrspace(1) %p1_dx, align 16
  %p2_dy = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <8 x float> %dy, ptr addrspace(1) %p2_dy, align 16
  %p3 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 192
  store <8 x i16> %ld, ptr addrspace(1) %p3, align 16
  br label %exit
exit:
  ret void
}

; %c is a no-op cast of a no-op subvector of %ld. The pressure counts %ld, not
; %c.
;
; CHECK-LABEL: Function unlock_dpas_of_live_in_subvector_cast
; CHECK: Original schedule: next
; CHECK: Greedy RP attempt
; CHECK-NEXT: ({{[0-9]+}}, {{[0-9]+}} {{.*}}) RP:{{.*}} %yb = call

define spir_kernel void @unlock_dpas_of_live_in_subvector_cast(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ldA = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 4, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 4)
  %ldh = bitcast <16 x i16> %ld to <16 x half>
  %x0 = extractelement <16 x half> %ldh, i32 0
  %s0 = insertelement <8 x half> undef, half %x0, i32 0
  %x1 = extractelement <16 x half> %ldh, i32 1
  %s1 = insertelement <8 x half> %s0, half %x1, i32 1
  %x2 = extractelement <16 x half> %ldh, i32 2
  %s2 = insertelement <8 x half> %s1, half %x2, i32 2
  %x3 = extractelement <16 x half> %ldh, i32 3
  %s3 = insertelement <8 x half> %s2, half %x3, i32 3
  %x4 = extractelement <16 x half> %ldh, i32 4
  %s4 = insertelement <8 x half> %s3, half %x4, i32 4
  %x5 = extractelement <16 x half> %ldh, i32 5
  %s5 = insertelement <8 x half> %s4, half %x5, i32 5
  %x6 = extractelement <16 x half> %ldh, i32 6
  %s6 = insertelement <8 x half> %s5, half %x6, i32 6
  %x7 = extractelement <16 x half> %ldh, i32 7
  %s = insertelement <8 x half> %s6, half %x7, i32 7
  %c = bitcast <8 x half> %s to <8 x i16>
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %da = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %da, ptr addrspace(1) %dst, align 32
  br label %next
next:
  %ya = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 64, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %yb = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 80, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dy = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %c, <8 x i32> %yb, i32 1, i32 1, i32 1, i32 1, i1 false)
  %dx = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %ldA, <8 x i32> %ya, i32 1, i32 1, i32 1, i32 1, i1 false)
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <8 x float> %dx, ptr addrspace(1) %p1, align 16
  %p2 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <8 x float> %dy, ptr addrspace(1) %p2, align 16
  br label %exit
exit:
  ret void
}

; %c is a no-op subvector of %ld.
;
; CHECK-LABEL: Function unlock_dpas_of_live_in_subvector
; CHECK: Original schedule: next
; CHECK: Greedy RP attempt
; CHECK-NEXT: ({{[0-9]+}}, {{[0-9]+}} {{.*}}) RP:{{.*}} %yb = call

define spir_kernel void @unlock_dpas_of_live_in_subvector(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ldA = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 4, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 4)
  %x0 = extractelement <16 x i16> %ld, i32 0
  %s0 = insertelement <8 x i16> undef, i16 %x0, i32 0
  %x1 = extractelement <16 x i16> %ld, i32 1
  %s1 = insertelement <8 x i16> %s0, i16 %x1, i32 1
  %x2 = extractelement <16 x i16> %ld, i32 2
  %s2 = insertelement <8 x i16> %s1, i16 %x2, i32 2
  %x3 = extractelement <16 x i16> %ld, i32 3
  %s3 = insertelement <8 x i16> %s2, i16 %x3, i32 3
  %x4 = extractelement <16 x i16> %ld, i32 4
  %s4 = insertelement <8 x i16> %s3, i16 %x4, i32 4
  %x5 = extractelement <16 x i16> %ld, i32 5
  %s5 = insertelement <8 x i16> %s4, i16 %x5, i32 5
  %x6 = extractelement <16 x i16> %ld, i32 6
  %s6 = insertelement <8 x i16> %s5, i16 %x6, i32 6
  %x7 = extractelement <16 x i16> %ld, i32 7
  %c = insertelement <8 x i16> %s6, i16 %x7, i32 7
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %da = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %da, ptr addrspace(1) %dst, align 32
  br label %next
next:
  %ya = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 64, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %yb = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 80, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dy = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %c, <8 x i32> %yb, i32 1, i32 1, i32 1, i32 1, i1 false)
  %dx = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %ldA, <8 x i32> %ya, i32 1, i32 1, i32 1, i32 1, i1 false)
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <8 x float> %dx, ptr addrspace(1) %p1, align 16
  %p2 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <8 x float> %dy, ptr addrspace(1) %p2, align 16
  br label %exit
exit:
  ret void
}

declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
