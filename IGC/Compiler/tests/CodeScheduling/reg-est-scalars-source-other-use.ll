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
; RUN:   --regkey ForceOCLSIMDWidth=16 --igc-code-scheduling -disable-output %s 2>&1 | FileCheck %s
;
; Every element of %src is extracted to a scalar. The scalars use the registers
; of %src, so %src is live until the last scalar dies and until its own last
; use. %src is a no-op bitcast of the load %ld, so the pressure tracker sees %ld
; die, not %src. In each function the estimate must release %ld once, so the
; block ends at the pressure it starts with. The DPAS on %a only makes the block
; a candidate for scheduling.

; The extracts are the only users of %src. %s15, where the last scalar dies,
; releases %ld.
;
; CHECK-LABEL: Function bitcast_scalars_source
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ({{[0-9]+}}, -512 {{.*}} %s15 = fadd
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @bitcast_scalars_source(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %src = bitcast <16 x i16> %ld to <16 x half>
  %e0 = extractelement <16 x half> %src, i32 0
  %e1 = extractelement <16 x half> %src, i32 1
  %e2 = extractelement <16 x half> %src, i32 2
  %e3 = extractelement <16 x half> %src, i32 3
  %e4 = extractelement <16 x half> %src, i32 4
  %e5 = extractelement <16 x half> %src, i32 5
  %e6 = extractelement <16 x half> %src, i32 6
  %e7 = extractelement <16 x half> %src, i32 7
  %e8 = extractelement <16 x half> %src, i32 8
  %e9 = extractelement <16 x half> %src, i32 9
  %e10 = extractelement <16 x half> %src, i32 10
  %e11 = extractelement <16 x half> %src, i32 11
  %e12 = extractelement <16 x half> %src, i32 12
  %e13 = extractelement <16 x half> %src, i32 13
  %e14 = extractelement <16 x half> %src, i32 14
  %e15 = extractelement <16 x half> %src, i32 15
  %s1 = fadd half %e0, %e1
  %s2 = fadd half %s1, %e2
  %s3 = fadd half %s2, %e3
  %s4 = fadd half %s3, %e4
  %s5 = fadd half %s4, %e5
  %s6 = fadd half %s5, %e6
  %s7 = fadd half %s6, %e7
  %s8 = fadd half %s7, %e8
  %s9 = fadd half %s8, %e9
  %s10 = fadd half %s9, %e10
  %s11 = fadd half %s10, %e11
  %s12 = fadd half %s11, %e12
  %s13 = fadd half %s12, %e13
  %s14 = fadd half %s13, %e14
  %s15 = fadd half %s14, %e15
  %sum.ptr = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 32
  store half %s15, ptr addrspace(1) %sum.ptr, align 2
  ret void
}

; %ld is also stored as a whole, so it can die after the last scalar. %s15
; releases nothing, and the store of %ld releases it: 512 bytes, plus 128 for
; %ld.ptr.
;
; CHECK-LABEL: Function bitcast_scalars_source_used_later
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ({{[0-9]+}}, 0 {{.*}} %s15 = fadd
; CHECK: ({{[0-9]+}}, -640 {{.*}} store <16 x i16> %ld,
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @bitcast_scalars_source_used_later(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %src = bitcast <16 x i16> %ld to <16 x half>
  %e0 = extractelement <16 x half> %src, i32 0
  %e1 = extractelement <16 x half> %src, i32 1
  %e2 = extractelement <16 x half> %src, i32 2
  %e3 = extractelement <16 x half> %src, i32 3
  %e4 = extractelement <16 x half> %src, i32 4
  %e5 = extractelement <16 x half> %src, i32 5
  %e6 = extractelement <16 x half> %src, i32 6
  %e7 = extractelement <16 x half> %src, i32 7
  %e8 = extractelement <16 x half> %src, i32 8
  %e9 = extractelement <16 x half> %src, i32 9
  %e10 = extractelement <16 x half> %src, i32 10
  %e11 = extractelement <16 x half> %src, i32 11
  %e12 = extractelement <16 x half> %src, i32 12
  %e13 = extractelement <16 x half> %src, i32 13
  %e14 = extractelement <16 x half> %src, i32 14
  %e15 = extractelement <16 x half> %src, i32 15
  %s1 = fadd half %e0, %e1
  %s2 = fadd half %s1, %e2
  %s3 = fadd half %s2, %e3
  %s4 = fadd half %s3, %e4
  %s5 = fadd half %s4, %e5
  %s6 = fadd half %s5, %e6
  %s7 = fadd half %s6, %e7
  %s8 = fadd half %s7, %e8
  %s9 = fadd half %s8, %e9
  %s10 = fadd half %s9, %e10
  %s11 = fadd half %s10, %e11
  %s12 = fadd half %s11, %e12
  %s13 = fadd half %s12, %e13
  %s14 = fadd half %s13, %e14
  %s15 = fadd half %s14, %e15
  %sum.ptr = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 32
  store half %s15, ptr addrspace(1) %sum.ptr, align 2
  %ld.ptr = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <16 x i16> %ld, ptr addrspace(1) %ld.ptr, align 32
  ret void
}

declare <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
