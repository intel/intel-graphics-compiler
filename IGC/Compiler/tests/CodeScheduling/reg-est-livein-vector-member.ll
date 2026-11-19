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
; A live-in value that uses the registers of a vector is counted once, at the
; vector: a no-op subvector, an extract, or a no-op cast of one of them, also
; when the subvector or the extract itself is not live-in, and also when the
; value is a subvector of another such value. In the functions that check the
; end of %entry against the first instruction of %next, %next starts at the
; pressure %entry ends with: every argument that lives into %entry also lives
; into %next, and the first instruction of %next does not change the pressure.

; %y is a no-op subvector of %r, and its cast %c lives out. %c keeps all the
; registers of %r.
;
; CHECK-LABEL: Function livein_cast_of_subvector
; CHECK: Original schedule: entry
; CHECK: ([[END:[0-9]+]], 0 {{.*}} OG:{{.*}} br label %next
; CHECK: Original schedule: next
; CHECK: ([[END]], 0 {{.*}} OG:{{.*}} store i64 %base

define spir_kernel void @livein_cast_of_subvector(ptr addrspace(1) %dst, i64 %base, <8 x i16> %a, <8 x i32> %b) {
entry:
  %d0 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  %r = fmul <8 x float> %d0, %d0
  %x0 = extractelement <8 x float> %r, i32 0
  %y0 = insertelement <4 x float> poison, float %x0, i32 0
  %x1 = extractelement <8 x float> %r, i32 1
  %y1 = insertelement <4 x float> %y0, float %x1, i32 1
  %x2 = extractelement <8 x float> %r, i32 2
  %y2 = insertelement <4 x float> %y1, float %x2, i32 2
  %x3 = extractelement <8 x float> %r, i32 3
  %y = insertelement <4 x float> %y2, float %x3, i32 3
  %c = bitcast <4 x float> %y to <4 x i32>
  store <8 x float> %d0, ptr addrspace(1) %dst, align 32
  br label %next

next:
  store i64 %base, ptr addrspace(1) %dst, align 8
  %d1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %d1, ptr addrspace(1) %dst, align 32
  %p = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 256
  store <4 x i32> %c, ptr addrspace(1) %p, align 16
  ret void
}

; %d0 is split into scalars, and the cast %c3 of the scalar %e3 lives out. %c3
; keeps all the registers of %d0.
;
; CHECK-LABEL: Function livein_cast_of_vector_extract
; CHECK: Original schedule: entry
; CHECK: ([[END:[0-9]+]], 0 {{.*}} OG:{{.*}} br label %next
; CHECK: Original schedule: next
; CHECK: ([[END]], 0 {{.*}} OG:{{.*}} store i64 %base

define spir_kernel void @livein_cast_of_vector_extract(ptr addrspace(1) %dst, i64 %base, <8 x i16> %a, <8 x i32> %b) {
entry:
  %d0 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  %e0 = extractelement <8 x float> %d0, i32 0
  %e1 = extractelement <8 x float> %d0, i32 1
  %e2 = extractelement <8 x float> %d0, i32 2
  %e3 = extractelement <8 x float> %d0, i32 3
  %e4 = extractelement <8 x float> %d0, i32 4
  %e5 = extractelement <8 x float> %d0, i32 5
  %e6 = extractelement <8 x float> %d0, i32 6
  %e7 = extractelement <8 x float> %d0, i32 7
  %s1 = fadd float %e0, %e1
  %s2 = fadd float %s1, %e2
  %s3 = fadd float %s2, %e3
  %s4 = fadd float %s3, %e4
  %s5 = fadd float %s4, %e5
  %s6 = fadd float %s5, %e6
  %s7 = fadd float %s6, %e7
  store float %s7, ptr addrspace(1) %dst, align 4
  %c3 = bitcast float %e3 to i32
  br label %next

next:
  store i64 %base, ptr addrspace(1) %dst, align 8
  %d1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %d1, ptr addrspace(1) %dst, align 32
  %p = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 256
  store i32 %c3, ptr addrspace(1) %p, align 4
  ret void
}

; The scalars of %r and the cast %rc of %r live out. They all use the registers
; of %r, which are counted once.
;
; CHECK-LABEL: Function livein_extracts_and_vector_cast
; CHECK: Original schedule: entry
; CHECK: ([[END:[0-9]+]], 0 {{.*}} OG:{{.*}} br label %next
; CHECK: Original schedule: next
; CHECK: ([[END]], 0 {{.*}} OG:{{.*}} store i64 %base

define spir_kernel void @livein_extracts_and_vector_cast(ptr addrspace(1) %dst, i64 %base, <8 x i16> %a, <8 x i32> %b) {
entry:
  %d0 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %d0, ptr addrspace(1) %dst, align 32
  %r = fmul <8 x float> %d0, %d0
  %rc = bitcast <8 x float> %r to <8 x i32>
  %e0 = extractelement <8 x float> %r, i32 0
  %e1 = extractelement <8 x float> %r, i32 1
  %e2 = extractelement <8 x float> %r, i32 2
  %e3 = extractelement <8 x float> %r, i32 3
  %e4 = extractelement <8 x float> %r, i32 4
  %e5 = extractelement <8 x float> %r, i32 5
  %e6 = extractelement <8 x float> %r, i32 6
  %e7 = extractelement <8 x float> %r, i32 7
  br label %next

next:
  store i64 %base, ptr addrspace(1) %dst, align 8
  %d1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %d1, ptr addrspace(1) %dst, align 32
  %s1 = fadd float %e0, %e1
  %s2 = fadd float %s1, %e2
  %s3 = fadd float %s2, %e3
  %s4 = fadd float %s3, %e4
  %s5 = fadd float %s4, %e5
  %s6 = fadd float %s5, %e6
  %s7 = fadd float %s6, %e7
  store float %s7, ptr addrspace(1) %dst, align 4
  %p = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 256
  store <8 x i32> %rc, ptr addrspace(1) %p, align 32
  ret void
}

; %c is a cast of the no-op subvector %y of %r, and %z is a no-op subvector of
; %c. Both live out, and both use the registers of %r.
;
; CHECK-LABEL: Function livein_subvector_of_livein_cast
; CHECK: Original schedule: entry
; CHECK: ([[END:[0-9]+]], 0 {{.*}} OG:{{.*}} br label %next
; CHECK: Original schedule: next
; CHECK: ([[END]], 0 {{.*}} OG:{{.*}} store i64 %base

define spir_kernel void @livein_subvector_of_livein_cast(ptr addrspace(1) %dst, i64 %base, <8 x i16> %a, <8 x i32> %b) {
entry:
  %d0 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  %r = fmul <8 x float> %d0, %d0
  %x0 = extractelement <8 x float> %r, i32 0
  %y0 = insertelement <4 x float> poison, float %x0, i32 0
  %x1 = extractelement <8 x float> %r, i32 1
  %y1 = insertelement <4 x float> %y0, float %x1, i32 1
  %x2 = extractelement <8 x float> %r, i32 2
  %y2 = insertelement <4 x float> %y1, float %x2, i32 2
  %x3 = extractelement <8 x float> %r, i32 3
  %y = insertelement <4 x float> %y2, float %x3, i32 3
  %c = bitcast <4 x float> %y to <4 x i32>
  %w0 = extractelement <4 x i32> %c, i32 0
  %z0 = insertelement <2 x i32> poison, i32 %w0, i32 0
  %w1 = extractelement <4 x i32> %c, i32 1
  %z = insertelement <2 x i32> %z0, i32 %w1, i32 1
  store <8 x float> %d0, ptr addrspace(1) %dst, align 32
  br label %next

next:
  store i64 %base, ptr addrspace(1) %dst, align 8
  %d1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %d1, ptr addrspace(1) %dst, align 32
  %p = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 256
  store <4 x i32> %c, ptr addrspace(1) %p, align 16
  %q = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 512
  store <2 x i32> %z, ptr addrspace(1) %q, align 8
  ret void
}

; As above, but %c lives into %next only as the incoming value of the PHI %ph,
; and %next splits %z into scalars. The scalars hang in the group of %r, which
; %next counts once and releases once: the last scalar releases the 512 B of
; %r, less the 64 B of %s, and when nothing lives out, the block ends with the
; arguments only, at the pressure %entry starts with.
;
; CHECK-LABEL: Function livein_subvector_of_phi_incoming_value
; CHECK: Initial CurrentPressure in registers: [[START:[0-9]+]]
; CHECK: Original schedule: entry
; CHECK: Original schedule: next
; CHECK: ({{[0-9]+}}, -448 {{.*}} OG:{{.*}} %s = add i32 %s0, %s1
; CHECK: ([[START]], 0 {{.*}} OG:{{.*}} ret void

define spir_kernel void @livein_subvector_of_phi_incoming_value(ptr addrspace(1) %dst, i64 %base, <8 x i16> %a, <8 x i32> %b) {
entry:
  %d0 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  %r = fmul <8 x float> %d0, %d0
  %x0 = extractelement <8 x float> %r, i32 0
  %y0 = insertelement <4 x float> poison, float %x0, i32 0
  %x1 = extractelement <8 x float> %r, i32 1
  %y1 = insertelement <4 x float> %y0, float %x1, i32 1
  %x2 = extractelement <8 x float> %r, i32 2
  %y2 = insertelement <4 x float> %y1, float %x2, i32 2
  %x3 = extractelement <8 x float> %r, i32 3
  %y = insertelement <4 x float> %y2, float %x3, i32 3
  %c = bitcast <4 x float> %y to <4 x i32>
  %w0 = extractelement <4 x i32> %c, i32 0
  %z0 = insertelement <2 x i32> poison, i32 %w0, i32 0
  %w1 = extractelement <4 x i32> %c, i32 1
  %z = insertelement <2 x i32> %z0, i32 %w1, i32 1
  store <8 x float> %d0, ptr addrspace(1) %dst, align 32
  br label %next

next:
  %ph = phi <4 x i32> [ %c, %entry ]
  store i64 %base, ptr addrspace(1) %dst, align 8
  %d1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %d1, ptr addrspace(1) %dst, align 32
  %s0 = extractelement <2 x i32> %z, i32 0
  %s1 = extractelement <2 x i32> %z, i32 1
  %s = add i32 %s0, %s1
  %p = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 256
  %q = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 512
  store i32 %s, ptr addrspace(1) %q, align 4
  store <4 x i32> %ph, ptr addrspace(1) %p, align 16
  ret void
}

; %w, a no-op cast of %u, lives into %next only as the incoming value of the PHI
; %ph, and its extract %e0 lives into %next together with %u. %e0 hangs in the
; group of %u, which %next counts once: %s7 releases nothing, because %u is
; still live.
;
; CHECK-LABEL: Function livein_extract_of_phi_incoming_cast
; CHECK: Original schedule: next
; CHECK: ({{[0-9]+}}, 64 {{.*}} OG:{{.*}} %s7 = add i32 %e0, %e0

define spir_kernel void @livein_extract_of_phi_incoming_cast(ptr addrspace(1) %dst, <8 x i16> %a, <8 x i32> %b) {
entry:
  %u = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  %w = bitcast <8 x float> %u to <8 x i32>
  %e0 = extractelement <8 x i32> %w, i32 0
  %e1 = extractelement <8 x i32> %w, i32 1
  %e2 = extractelement <8 x i32> %w, i32 2
  %e3 = extractelement <8 x i32> %w, i32 3
  %e4 = extractelement <8 x i32> %w, i32 4
  %e5 = extractelement <8 x i32> %w, i32 5
  %e6 = extractelement <8 x i32> %w, i32 6
  %e7 = extractelement <8 x i32> %w, i32 7
  %t1 = add i32 %e1, %e2
  %t2 = add i32 %t1, %e3
  %t3 = add i32 %t2, %e4
  %t4 = add i32 %t3, %e5
  %t5 = add i32 %t4, %e6
  %t6 = add i32 %t5, %e7
  %pt = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 512
  store i32 %t6, ptr addrspace(1) %pt, align 4
  br label %next
next:
  %ph = phi <8 x i32> [ %w, %entry ]
  %d1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %d1, ptr addrspace(1) %dst, align 32
  %s7 = add i32 %e0, %e0
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store i32 %s7, ptr addrspace(1) %p0, align 4
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <8 x float> %u, ptr addrspace(1) %p1, align 32
  %p2 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 256
  store <8 x i32> %ph, ptr addrspace(1) %p2, align 32
  ret void
}

; As above, but %u does not live into %next: its extract %f0 does, together with
; %e0. Both hang in the group of %u, which %next counts once: %s7 releases
; nothing, and %s8 releases the 512 B of %u, less the 64 B of %s8.
;
; CHECK-LABEL: Function livein_extracts_of_phi_incoming_cast_of_owner
; CHECK: Original schedule: next
; CHECK: ({{[0-9]+}}, 64 {{.*}} OG:{{.*}} %s7 = add i32 %e0, %e0
; CHECK: ({{[0-9]+}}, -448 {{.*}} OG:{{.*}} %s8 = fadd float %f0, %f0

define spir_kernel void @livein_extracts_of_phi_incoming_cast_of_owner(ptr addrspace(1) %dst, <8 x i16> %a, <8 x i32> %b) {
entry:
  %u = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  %w = bitcast <8 x float> %u to <8 x i32>
  %e0 = extractelement <8 x i32> %w, i32 0
  %e1 = extractelement <8 x i32> %w, i32 1
  %e2 = extractelement <8 x i32> %w, i32 2
  %e3 = extractelement <8 x i32> %w, i32 3
  %e4 = extractelement <8 x i32> %w, i32 4
  %e5 = extractelement <8 x i32> %w, i32 5
  %e6 = extractelement <8 x i32> %w, i32 6
  %e7 = extractelement <8 x i32> %w, i32 7
  %f0 = extractelement <8 x float> %u, i32 0
  %f1 = extractelement <8 x float> %u, i32 1
  %f2 = extractelement <8 x float> %u, i32 2
  %f3 = extractelement <8 x float> %u, i32 3
  %f4 = extractelement <8 x float> %u, i32 4
  %f5 = extractelement <8 x float> %u, i32 5
  %f6 = extractelement <8 x float> %u, i32 6
  %f7 = extractelement <8 x float> %u, i32 7
  %t1 = add i32 %e1, %e2
  %t2 = add i32 %t1, %e3
  %t3 = add i32 %t2, %e4
  %t4 = add i32 %t3, %e5
  %t5 = add i32 %t4, %e6
  %t6 = add i32 %t5, %e7
  %g1 = fadd float %f1, %f2
  %g2 = fadd float %g1, %f3
  %g3 = fadd float %g2, %f4
  %g4 = fadd float %g3, %f5
  %g5 = fadd float %g4, %f6
  %g6 = fadd float %g5, %f7
  %pt = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 512
  store i32 %t6, ptr addrspace(1) %pt, align 4
  %pg = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 576
  store float %g6, ptr addrspace(1) %pg, align 4
  br label %next
next:
  %ph = phi <8 x i32> [ %w, %entry ]
  %d1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %d1, ptr addrspace(1) %dst, align 32
  %s7 = add i32 %e0, %e0
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store i32 %s7, ptr addrspace(1) %p0, align 4
  %s8 = fadd float %f0, %f0
  %p3 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 96
  store float %s8, ptr addrspace(1) %p3, align 4
  %p2 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 256
  store <8 x i32> %ph, ptr addrspace(1) %p2, align 32
  ret void
}

; %h and %bf, a half and a bfloat view of the load %ld, live into %next only as
; the incoming values of PHIs, and their extracts %h0 and %b0 live into %next;
; %ld does not. Both extracts hang in the group of %ld, which %next counts once:
; %sh releases nothing, and %sb releases the 256 B of %ld, less the 32 B of %sb.
;
; CHECK-LABEL: Function livein_extracts_of_phi_incoming_sibling_casts
; CHECK: Original schedule: next
; CHECK: ({{[0-9]+}}, 32 {{.*}} OG:{{.*}} %sh = fadd half %h0, %h0
; CHECK: ({{[0-9]+}}, -224 {{.*}} OG:{{.*}} %sb = fadd bfloat %b0, %b0

define spir_kernel void @livein_extracts_of_phi_incoming_sibling_casts(ptr addrspace(1) %dst, <8 x i16> %a, <8 x i32> %b) {
entry:
  %src = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 8192
  %ld = load <8 x i16>, ptr addrspace(1) %src, align 16
  %h = bitcast <8 x i16> %ld to <8 x half>
  %bf = bitcast <8 x i16> %ld to <8 x bfloat>
  %h0 = extractelement <8 x half> %h, i32 0
  %h1 = extractelement <8 x half> %h, i32 1
  %h2 = extractelement <8 x half> %h, i32 2
  %h3 = extractelement <8 x half> %h, i32 3
  %h4 = extractelement <8 x half> %h, i32 4
  %h5 = extractelement <8 x half> %h, i32 5
  %h6 = extractelement <8 x half> %h, i32 6
  %h7 = extractelement <8 x half> %h, i32 7
  %b0 = extractelement <8 x bfloat> %bf, i32 0
  %b1 = extractelement <8 x bfloat> %bf, i32 1
  %b2 = extractelement <8 x bfloat> %bf, i32 2
  %b3 = extractelement <8 x bfloat> %bf, i32 3
  %b4 = extractelement <8 x bfloat> %bf, i32 4
  %b5 = extractelement <8 x bfloat> %bf, i32 5
  %b6 = extractelement <8 x bfloat> %bf, i32 6
  %b7 = extractelement <8 x bfloat> %bf, i32 7
  %t1 = fadd half %h1, %h2
  %t2 = fadd half %t1, %h3
  %t3 = fadd half %t2, %h4
  %t4 = fadd half %t3, %h5
  %t5 = fadd half %t4, %h6
  %t6 = fadd half %t5, %h7
  %g1 = fadd bfloat %b1, %b2
  %g2 = fadd bfloat %g1, %b3
  %g3 = fadd bfloat %g2, %b4
  %g4 = fadd bfloat %g3, %b5
  %g5 = fadd bfloat %g4, %b6
  %g6 = fadd bfloat %g5, %b7
  %pt = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 512
  store half %t6, ptr addrspace(1) %pt, align 2
  %pg = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 576
  store bfloat %g6, ptr addrspace(1) %pg, align 2
  br label %next
next:
  %phh = phi <8 x half> [ %h, %entry ]
  %phb = phi <8 x bfloat> [ %bf, %entry ]
  %d1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %d1, ptr addrspace(1) %dst, align 32
  %sh = fadd half %h0, %h0
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store half %sh, ptr addrspace(1) %p0, align 2
  %sb = fadd bfloat %b0, %b0
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 96
  store bfloat %sb, ptr addrspace(1) %p1, align 2
  %p2 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 1024
  store <8 x half> %phh, ptr addrspace(1) %p2, align 16
  %p3 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 2048
  store <8 x bfloat> %phb, ptr addrspace(1) %p3, align 16
  ret void
}

; %h, a no-op cast of the argument %v, is used only in the block before, and
; its extract %h0 lives into %next; %v does not. %h0 hangs in the group of %v,
; and %sh releases the 256 B of %v, less the 32 B of %sh: an argument that is
; not live-in is not live to the end of the block.
;
; CHECK-LABEL: Function livein_extract_of_argument_cast
; CHECK: Original schedule: next
; CHECK: ({{[0-9]+}}, -224 {{.*}} OG:{{.*}} %sh = fadd half %h0, %h0

define spir_kernel void @livein_extract_of_argument_cast(ptr addrspace(1) %dst, <8 x i16> %a, <8 x i32> %b, <8 x i16> %v) {
entry:
  %h = bitcast <8 x i16> %v to <8 x half>
  %h0 = extractelement <8 x half> %h, i32 0
  %h1 = extractelement <8 x half> %h, i32 1
  %h2 = extractelement <8 x half> %h, i32 2
  %h3 = extractelement <8 x half> %h, i32 3
  %h4 = extractelement <8 x half> %h, i32 4
  %h5 = extractelement <8 x half> %h, i32 5
  %h6 = extractelement <8 x half> %h, i32 6
  %h7 = extractelement <8 x half> %h, i32 7
  %t1 = fadd half %h1, %h2
  %t2 = fadd half %t1, %h3
  %t3 = fadd half %t2, %h4
  %t4 = fadd half %t3, %h5
  %t5 = fadd half %t4, %h6
  %t6 = fadd half %t5, %h7
  %pt = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 512
  store half %t6, ptr addrspace(1) %pt, align 2
  br label %next
next:
  %d1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %d1, ptr addrspace(1) %dst, align 32
  %sh = fadd half %h0, %h0
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store half %sh, ptr addrspace(1) %p0, align 2
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
