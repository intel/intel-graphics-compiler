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
; A vector shuffle splits the source vector %src into subvectors. %src is live
; until its last no-op subvector dies, until its last copied subvector is
; copied, and until its own last use. In each function the estimate must release
; %src once, so the block ends at the pressure it starts with.

; A no-op vector shuffle splits %src into the subvectors %lo and %hi without
; copies. %src also has another user: as a whole, it is the B matrix of %dpas3,
; which continues the DPAS chain after %hi dies. %dpas2 does not decrease the
; pressure, and %dpas3 releases %src.
;
; CHECK-LABEL: Function shuffle_source_used_later
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: %dpas1 = call
; CHECK-NEXT: ([[MAXRP:[0-9]+]], 0 {{.*}} %dpas2 = call
; CHECK-NEXT: ([[MAXRP]], 0 {{.*}} %src.b = bitcast
; CHECK-NEXT: ([[MAXRP]], -512 {{.*}} %dpas3 = call
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained, complete=1, maxRP=[[MAXRP]],

define spir_kernel void @shuffle_source_used_later(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %src = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
  %e0 = extractelement <16 x i16> %src, i32 0
  %lo.0 = insertelement <8 x i16> undef, i16 %e0, i32 0
  %e1 = extractelement <16 x i16> %src, i32 1
  %lo.1 = insertelement <8 x i16> %lo.0, i16 %e1, i32 1
  %e2 = extractelement <16 x i16> %src, i32 2
  %lo.2 = insertelement <8 x i16> %lo.1, i16 %e2, i32 2
  %e3 = extractelement <16 x i16> %src, i32 3
  %lo.3 = insertelement <8 x i16> %lo.2, i16 %e3, i32 3
  %e4 = extractelement <16 x i16> %src, i32 4
  %lo.4 = insertelement <8 x i16> %lo.3, i16 %e4, i32 4
  %e5 = extractelement <16 x i16> %src, i32 5
  %lo.5 = insertelement <8 x i16> %lo.4, i16 %e5, i32 5
  %e6 = extractelement <16 x i16> %src, i32 6
  %lo.6 = insertelement <8 x i16> %lo.5, i16 %e6, i32 6
  %e7 = extractelement <16 x i16> %src, i32 7
  %lo = insertelement <8 x i16> %lo.6, i16 %e7, i32 7
  %e8 = extractelement <16 x i16> %src, i32 8
  %hi.0 = insertelement <8 x i16> undef, i16 %e8, i32 0
  %e9 = extractelement <16 x i16> %src, i32 9
  %hi.1 = insertelement <8 x i16> %hi.0, i16 %e9, i32 1
  %e10 = extractelement <16 x i16> %src, i32 10
  %hi.2 = insertelement <8 x i16> %hi.1, i16 %e10, i32 2
  %e11 = extractelement <16 x i16> %src, i32 11
  %hi.3 = insertelement <8 x i16> %hi.2, i16 %e11, i32 3
  %e12 = extractelement <16 x i16> %src, i32 12
  %hi.4 = insertelement <8 x i16> %hi.3, i16 %e12, i32 4
  %e13 = extractelement <16 x i16> %src, i32 13
  %hi.5 = insertelement <8 x i16> %hi.4, i16 %e13, i32 5
  %e14 = extractelement <16 x i16> %src, i32 14
  %hi.6 = insertelement <8 x i16> %hi.5, i16 %e14, i32 6
  %e15 = extractelement <16 x i16> %src, i32 15
  %hi = insertelement <8 x i16> %hi.6, i16 %e15, i32 7
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %lo, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas1, <8 x i16> %hi, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  %src.b = bitcast <16 x i16> %src to <8 x i32>
  %dpas3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas2, <8 x i16> zeroinitializer, <8 x i32> %src.b, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas3, ptr addrspace(1) %dst, align 32
  ret void
}

; The same no-op shuffle, but %src as a whole is the B matrix of %dpas2, before
; %hi dies at %dpas3. %dpas3 releases %src.
;
; CHECK-LABEL: Function shuffle_source_used_first
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ({{[0-9]+}}, -512 {{.*}} %dpas3 = call
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @shuffle_source_used_first(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %src = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
  %lo.e0 = extractelement <16 x i16> %src, i32 0
  %lo.0 = insertelement <8 x i16> undef, i16 %lo.e0, i32 0
  %lo.e1 = extractelement <16 x i16> %src, i32 1
  %lo.1 = insertelement <8 x i16> %lo.0, i16 %lo.e1, i32 1
  %lo.e2 = extractelement <16 x i16> %src, i32 2
  %lo.2 = insertelement <8 x i16> %lo.1, i16 %lo.e2, i32 2
  %lo.e3 = extractelement <16 x i16> %src, i32 3
  %lo.3 = insertelement <8 x i16> %lo.2, i16 %lo.e3, i32 3
  %lo.e4 = extractelement <16 x i16> %src, i32 4
  %lo.4 = insertelement <8 x i16> %lo.3, i16 %lo.e4, i32 4
  %lo.e5 = extractelement <16 x i16> %src, i32 5
  %lo.5 = insertelement <8 x i16> %lo.4, i16 %lo.e5, i32 5
  %lo.e6 = extractelement <16 x i16> %src, i32 6
  %lo.6 = insertelement <8 x i16> %lo.5, i16 %lo.e6, i32 6
  %lo.e7 = extractelement <16 x i16> %src, i32 7
  %lo = insertelement <8 x i16> %lo.6, i16 %lo.e7, i32 7
  %hi.e0 = extractelement <16 x i16> %src, i32 8
  %hi.0 = insertelement <8 x i16> undef, i16 %hi.e0, i32 0
  %hi.e1 = extractelement <16 x i16> %src, i32 9
  %hi.1 = insertelement <8 x i16> %hi.0, i16 %hi.e1, i32 1
  %hi.e2 = extractelement <16 x i16> %src, i32 10
  %hi.2 = insertelement <8 x i16> %hi.1, i16 %hi.e2, i32 2
  %hi.e3 = extractelement <16 x i16> %src, i32 11
  %hi.3 = insertelement <8 x i16> %hi.2, i16 %hi.e3, i32 3
  %hi.e4 = extractelement <16 x i16> %src, i32 12
  %hi.4 = insertelement <8 x i16> %hi.3, i16 %hi.e4, i32 4
  %hi.e5 = extractelement <16 x i16> %src, i32 13
  %hi.5 = insertelement <8 x i16> %hi.4, i16 %hi.e5, i32 5
  %hi.e6 = extractelement <16 x i16> %src, i32 14
  %hi.6 = insertelement <8 x i16> %hi.5, i16 %hi.e6, i32 6
  %hi.e7 = extractelement <16 x i16> %src, i32 15
  %hi = insertelement <8 x i16> %hi.6, i16 %hi.e7, i32 7
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %lo, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  %src.b = bitcast <16 x i16> %src to <8 x i32>
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas1, <8 x i16> zeroinitializer, <8 x i32> %src.b, i32 1, i32 1, i32 1, i32 1, i1 false)
  %dpas3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas2, <8 x i16> %hi, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas3, ptr addrspace(1) %dst, align 32
  ret void
}

; A copying vector shuffle builds %ev and %od out of the even and the odd
; elements of %src, and %src as a whole is the B matrix of %dpas3. Copying the
; last element into %od does not release %src; %dpas3 does.
;
; CHECK-LABEL: Function copy_shuffle_source_used_later
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ({{[0-9]+}}, 0 {{.*}} %od = insertelement <8 x i16> %od.6
; CHECK: ({{[0-9]+}}, -512 {{.*}} %dpas3 = call
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @copy_shuffle_source_used_later(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %src = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
  %ev.e0 = extractelement <16 x i16> %src, i32 0
  %ev.0 = insertelement <8 x i16> undef, i16 %ev.e0, i32 0
  %ev.e1 = extractelement <16 x i16> %src, i32 2
  %ev.1 = insertelement <8 x i16> %ev.0, i16 %ev.e1, i32 1
  %ev.e2 = extractelement <16 x i16> %src, i32 4
  %ev.2 = insertelement <8 x i16> %ev.1, i16 %ev.e2, i32 2
  %ev.e3 = extractelement <16 x i16> %src, i32 6
  %ev.3 = insertelement <8 x i16> %ev.2, i16 %ev.e3, i32 3
  %ev.e4 = extractelement <16 x i16> %src, i32 8
  %ev.4 = insertelement <8 x i16> %ev.3, i16 %ev.e4, i32 4
  %ev.e5 = extractelement <16 x i16> %src, i32 10
  %ev.5 = insertelement <8 x i16> %ev.4, i16 %ev.e5, i32 5
  %ev.e6 = extractelement <16 x i16> %src, i32 12
  %ev.6 = insertelement <8 x i16> %ev.5, i16 %ev.e6, i32 6
  %ev.e7 = extractelement <16 x i16> %src, i32 14
  %ev = insertelement <8 x i16> %ev.6, i16 %ev.e7, i32 7
  %od.e0 = extractelement <16 x i16> %src, i32 1
  %od.0 = insertelement <8 x i16> undef, i16 %od.e0, i32 0
  %od.e1 = extractelement <16 x i16> %src, i32 3
  %od.1 = insertelement <8 x i16> %od.0, i16 %od.e1, i32 1
  %od.e2 = extractelement <16 x i16> %src, i32 5
  %od.2 = insertelement <8 x i16> %od.1, i16 %od.e2, i32 2
  %od.e3 = extractelement <16 x i16> %src, i32 7
  %od.3 = insertelement <8 x i16> %od.2, i16 %od.e3, i32 3
  %od.e4 = extractelement <16 x i16> %src, i32 9
  %od.4 = insertelement <8 x i16> %od.3, i16 %od.e4, i32 4
  %od.e5 = extractelement <16 x i16> %src, i32 11
  %od.5 = insertelement <8 x i16> %od.4, i16 %od.e5, i32 5
  %od.e6 = extractelement <16 x i16> %src, i32 13
  %od.6 = insertelement <8 x i16> %od.5, i16 %od.e6, i32 6
  %od.e7 = extractelement <16 x i16> %src, i32 15
  %od = insertelement <8 x i16> %od.6, i16 %od.e7, i32 7
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %ev, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas1, <8 x i16> %od, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  %src.b = bitcast <16 x i16> %src to <8 x i32>
  %dpas3 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas2, <8 x i16> zeroinitializer, <8 x i32> %src.b, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas3, ptr addrspace(1) %dst, align 32
  ret void
}

; The last use of %src is an extractelement that is not part of the shuffle.
; Its index depends on %dpas2, so it comes after %hi dies. %dpas2 does not
; release %src. %x releases %src (512 bytes) and %k (64) and adds itself (32).
;
; CHECK-LABEL: Function shuffle_source_extracted_later
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ({{[0-9]+}}, 0 {{.*}} %dpas2 = call
; CHECK: ({{[0-9]+}}, -544 {{.*}} %x = extractelement
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @shuffle_source_extracted_later(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %src = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
  %lo.e0 = extractelement <16 x i16> %src, i32 0
  %lo.0 = insertelement <8 x i16> undef, i16 %lo.e0, i32 0
  %lo.e1 = extractelement <16 x i16> %src, i32 1
  %lo.1 = insertelement <8 x i16> %lo.0, i16 %lo.e1, i32 1
  %lo.e2 = extractelement <16 x i16> %src, i32 2
  %lo.2 = insertelement <8 x i16> %lo.1, i16 %lo.e2, i32 2
  %lo.e3 = extractelement <16 x i16> %src, i32 3
  %lo.3 = insertelement <8 x i16> %lo.2, i16 %lo.e3, i32 3
  %lo.e4 = extractelement <16 x i16> %src, i32 4
  %lo.4 = insertelement <8 x i16> %lo.3, i16 %lo.e4, i32 4
  %lo.e5 = extractelement <16 x i16> %src, i32 5
  %lo.5 = insertelement <8 x i16> %lo.4, i16 %lo.e5, i32 5
  %lo.e6 = extractelement <16 x i16> %src, i32 6
  %lo.6 = insertelement <8 x i16> %lo.5, i16 %lo.e6, i32 6
  %lo.e7 = extractelement <16 x i16> %src, i32 7
  %lo = insertelement <8 x i16> %lo.6, i16 %lo.e7, i32 7
  %hi.e0 = extractelement <16 x i16> %src, i32 8
  %hi.0 = insertelement <8 x i16> undef, i16 %hi.e0, i32 0
  %hi.e1 = extractelement <16 x i16> %src, i32 9
  %hi.1 = insertelement <8 x i16> %hi.0, i16 %hi.e1, i32 1
  %hi.e2 = extractelement <16 x i16> %src, i32 10
  %hi.2 = insertelement <8 x i16> %hi.1, i16 %hi.e2, i32 2
  %hi.e3 = extractelement <16 x i16> %src, i32 11
  %hi.3 = insertelement <8 x i16> %hi.2, i16 %hi.e3, i32 3
  %hi.e4 = extractelement <16 x i16> %src, i32 12
  %hi.4 = insertelement <8 x i16> %hi.3, i16 %hi.e4, i32 4
  %hi.e5 = extractelement <16 x i16> %src, i32 13
  %hi.5 = insertelement <8 x i16> %hi.4, i16 %hi.e5, i32 5
  %hi.e6 = extractelement <16 x i16> %src, i32 14
  %hi.6 = insertelement <8 x i16> %hi.5, i16 %hi.e6, i32 6
  %hi.e7 = extractelement <16 x i16> %src, i32 15
  %hi = insertelement <8 x i16> %hi.6, i16 %hi.e7, i32 7
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %lo, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas1, <8 x i16> %hi, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas2, ptr addrspace(1) %dst, align 32
  %f = extractelement <8 x float> %dpas2, i32 0
  %k = fptoui float %f to i32
  %x = extractelement <16 x i16> %src, i32 %k
  %dst.x = getelementptr i8, ptr addrspace(1) %dst, i64 32
  store i16 %x, ptr addrspace(1) %dst.x, align 2
  ret void
}

; %src has a copied subvector %rv, its reversed upper half, and a no-op
; subvector %lo, its lower half. Building %lo does not release %src; %dpas2,
; where %lo dies, does.
;
; CHECK-LABEL: Function mixed_shuffle_source
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ({{[0-9]+}}, 0 {{.*}} %lo = insertelement <8 x i16> %lo.6
; CHECK: ({{[0-9]+}}, -512 {{.*}} %dpas2 = call
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @mixed_shuffle_source(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %src = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
  %rv.e0 = extractelement <16 x i16> %src, i32 15
  %rv.0 = insertelement <8 x i16> undef, i16 %rv.e0, i32 0
  %rv.e1 = extractelement <16 x i16> %src, i32 14
  %rv.1 = insertelement <8 x i16> %rv.0, i16 %rv.e1, i32 1
  %rv.e2 = extractelement <16 x i16> %src, i32 13
  %rv.2 = insertelement <8 x i16> %rv.1, i16 %rv.e2, i32 2
  %rv.e3 = extractelement <16 x i16> %src, i32 12
  %rv.3 = insertelement <8 x i16> %rv.2, i16 %rv.e3, i32 3
  %rv.e4 = extractelement <16 x i16> %src, i32 11
  %rv.4 = insertelement <8 x i16> %rv.3, i16 %rv.e4, i32 4
  %rv.e5 = extractelement <16 x i16> %src, i32 10
  %rv.5 = insertelement <8 x i16> %rv.4, i16 %rv.e5, i32 5
  %rv.e6 = extractelement <16 x i16> %src, i32 9
  %rv.6 = insertelement <8 x i16> %rv.5, i16 %rv.e6, i32 6
  %rv.e7 = extractelement <16 x i16> %src, i32 8
  %rv = insertelement <8 x i16> %rv.6, i16 %rv.e7, i32 7
  %lo.e0 = extractelement <16 x i16> %src, i32 0
  %lo.0 = insertelement <8 x i16> undef, i16 %lo.e0, i32 0
  %lo.e1 = extractelement <16 x i16> %src, i32 1
  %lo.1 = insertelement <8 x i16> %lo.0, i16 %lo.e1, i32 1
  %lo.e2 = extractelement <16 x i16> %src, i32 2
  %lo.2 = insertelement <8 x i16> %lo.1, i16 %lo.e2, i32 2
  %lo.e3 = extractelement <16 x i16> %src, i32 3
  %lo.3 = insertelement <8 x i16> %lo.2, i16 %lo.e3, i32 3
  %lo.e4 = extractelement <16 x i16> %src, i32 4
  %lo.4 = insertelement <8 x i16> %lo.3, i16 %lo.e4, i32 4
  %lo.e5 = extractelement <16 x i16> %src, i32 5
  %lo.5 = insertelement <8 x i16> %lo.4, i16 %lo.e5, i32 5
  %lo.e6 = extractelement <16 x i16> %src, i32 6
  %lo.6 = insertelement <8 x i16> %lo.5, i16 %lo.e6, i32 6
  %lo.e7 = extractelement <16 x i16> %src, i32 7
  %lo = insertelement <8 x i16> %lo.6, i16 %lo.e7, i32 7
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %rv, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas1, <8 x i16> %lo, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas2, ptr addrspace(1) %dst, align 32
  ret void
}

; %src is a bitcast of the load %ld that does not change the number of
; elements, so the pressure tracker sees %ld die at the last extractelement of
; the shuffle. Neither that extractelement nor %dpas1 releases %ld; %dpas2,
; where %hi dies, does.
;
; CHECK-LABEL: Function bitcast_shuffle_source
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ({{[0-9]+}}, 0 {{.*}} %hi.e7 = extractelement
; CHECK: ({{[0-9]+}}, 512 {{.*}} %dpas1 = call
; CHECK: ({{[0-9]+}}, -512 {{.*}} %dpas2 = call
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @bitcast_shuffle_source(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
  %src = bitcast <16 x i16> %ld to <16 x half>
  %lo.e0 = extractelement <16 x half> %src, i32 0
  %lo.0 = insertelement <8 x half> undef, half %lo.e0, i32 0
  %lo.e1 = extractelement <16 x half> %src, i32 1
  %lo.1 = insertelement <8 x half> %lo.0, half %lo.e1, i32 1
  %lo.e2 = extractelement <16 x half> %src, i32 2
  %lo.2 = insertelement <8 x half> %lo.1, half %lo.e2, i32 2
  %lo.e3 = extractelement <16 x half> %src, i32 3
  %lo.3 = insertelement <8 x half> %lo.2, half %lo.e3, i32 3
  %lo.e4 = extractelement <16 x half> %src, i32 4
  %lo.4 = insertelement <8 x half> %lo.3, half %lo.e4, i32 4
  %lo.e5 = extractelement <16 x half> %src, i32 5
  %lo.5 = insertelement <8 x half> %lo.4, half %lo.e5, i32 5
  %lo.e6 = extractelement <16 x half> %src, i32 6
  %lo.6 = insertelement <8 x half> %lo.5, half %lo.e6, i32 6
  %lo.e7 = extractelement <16 x half> %src, i32 7
  %lo = insertelement <8 x half> %lo.6, half %lo.e7, i32 7
  %hi.e0 = extractelement <16 x half> %src, i32 8
  %hi.0 = insertelement <8 x half> undef, half %hi.e0, i32 0
  %hi.e1 = extractelement <16 x half> %src, i32 9
  %hi.1 = insertelement <8 x half> %hi.0, half %hi.e1, i32 1
  %hi.e2 = extractelement <16 x half> %src, i32 10
  %hi.2 = insertelement <8 x half> %hi.1, half %hi.e2, i32 2
  %hi.e3 = extractelement <16 x half> %src, i32 11
  %hi.3 = insertelement <8 x half> %hi.2, half %hi.e3, i32 3
  %hi.e4 = extractelement <16 x half> %src, i32 12
  %hi.4 = insertelement <8 x half> %hi.3, half %hi.e4, i32 4
  %hi.e5 = extractelement <16 x half> %src, i32 13
  %hi.5 = insertelement <8 x half> %hi.4, half %hi.e5, i32 5
  %hi.e6 = extractelement <16 x half> %src, i32 14
  %hi.6 = insertelement <8 x half> %hi.5, half %hi.e6, i32 6
  %hi.e7 = extractelement <16 x half> %src, i32 15
  %hi = insertelement <8 x half> %hi.6, half %hi.e7, i32 7
  %lo.i = bitcast <8 x half> %lo to <8 x i16>
  %hi.i = bitcast <8 x half> %hi to <8 x i16>
  %dpas1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %lo.i, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %dpas1, <8 x i16> %hi.i, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas2, ptr addrspace(1) %dst, align 32
  ret void
}

declare <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
