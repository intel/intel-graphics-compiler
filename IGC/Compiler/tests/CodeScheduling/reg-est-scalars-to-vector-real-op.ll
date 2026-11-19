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
; The scalars inserted into a vector hang: the pressure tracker releases them
; together when the last of them dies. A scalar that is a no-op bitcast dies as
; the value it casts. A constant has no register and an argument is never
; released, so neither may keep the other scalars from being released. In each
; function the block must end at the pressure it starts with, in the original
; order and in the order of the greedy MW attempt. The DPAS on %a only makes the
; block a candidate for scheduling.

; The scalar of the first insertelement is a bitcast.
;
; CHECK-LABEL: Function bitcast_first_scalar
; CHECK: Original schedule
; CHECK: ([[OGSTART:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[OGSTART]], 0 {{.*}} OG:{{.*}} ret void
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @bitcast_first_scalar(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %p.x0 = getelementptr inbounds float, ptr addrspace(1) %A, i64 64
  %x0 = load float, ptr addrspace(1) %p.x0, align 4
  %p.x1 = getelementptr inbounds float, ptr addrspace(1) %A, i64 65
  %x1 = load float, ptr addrspace(1) %p.x1, align 4
  %p.x2 = getelementptr inbounds float, ptr addrspace(1) %A, i64 66
  %x2 = load float, ptr addrspace(1) %p.x2, align 4
  %p.x3 = getelementptr inbounds float, ptr addrspace(1) %A, i64 67
  %x3 = load float, ptr addrspace(1) %p.x3, align 4
  %s0 = bitcast float %x0 to i32
  %s1 = fptosi float %x1 to i32
  %s2 = fptosi float %x2 to i32
  %s3 = fptosi float %x3 to i32
  %v0 = insertelement <4 x i32> poison, i32 %s0, i32 0
  %v1 = insertelement <4 x i32> %v0, i32 %s1, i32 1
  %v2 = insertelement <4 x i32> %v1, i32 %s2, i32 2
  %v3 = insertelement <4 x i32> %v2, i32 %s3, i32 3
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <4 x i32> %v3, ptr addrspace(1) %p0, align 16
  ret void
}

; A later scalar is a bitcast.
;
; CHECK-LABEL: Function bitcast_later_scalar
; CHECK: Original schedule
; CHECK: ([[OGSTART:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[OGSTART]], 0 {{.*}} OG:{{.*}} ret void
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @bitcast_later_scalar(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %p.x0 = getelementptr inbounds float, ptr addrspace(1) %A, i64 64
  %x0 = load float, ptr addrspace(1) %p.x0, align 4
  %p.x1 = getelementptr inbounds float, ptr addrspace(1) %A, i64 65
  %x1 = load float, ptr addrspace(1) %p.x1, align 4
  %p.x2 = getelementptr inbounds float, ptr addrspace(1) %A, i64 66
  %x2 = load float, ptr addrspace(1) %p.x2, align 4
  %p.x3 = getelementptr inbounds float, ptr addrspace(1) %A, i64 67
  %x3 = load float, ptr addrspace(1) %p.x3, align 4
  %s0 = fptosi float %x0 to i32
  %s1 = fptosi float %x1 to i32
  %s2 = bitcast float %x2 to i32
  %s3 = fptosi float %x3 to i32
  %v0 = insertelement <4 x i32> poison, i32 %s0, i32 0
  %v1 = insertelement <4 x i32> %v0, i32 %s1, i32 1
  %v2 = insertelement <4 x i32> %v1, i32 %s2, i32 2
  %v3 = insertelement <4 x i32> %v2, i32 %s3, i32 3
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <4 x i32> %v3, ptr addrspace(1) %p0, align 16
  ret void
}

; The last element is a constant.
;
; CHECK-LABEL: Function constant_element
; CHECK: Original schedule
; CHECK: ([[OGSTART:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[OGSTART]], 0 {{.*}} OG:{{.*}} ret void
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @constant_element(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %p.x0 = getelementptr inbounds float, ptr addrspace(1) %A, i64 64
  %x0 = load float, ptr addrspace(1) %p.x0, align 4
  %p.x1 = getelementptr inbounds float, ptr addrspace(1) %A, i64 65
  %x1 = load float, ptr addrspace(1) %p.x1, align 4
  %p.x2 = getelementptr inbounds float, ptr addrspace(1) %A, i64 66
  %x2 = load float, ptr addrspace(1) %p.x2, align 4
  %s0 = fptosi float %x0 to i32
  %s1 = fptosi float %x1 to i32
  %s2 = fptosi float %x2 to i32
  %v0 = insertelement <4 x i32> poison, i32 %s0, i32 0
  %v1 = insertelement <4 x i32> %v0, i32 %s1, i32 1
  %v2 = insertelement <4 x i32> %v1, i32 %s2, i32 2
  %v3 = insertelement <4 x i32> %v2, i32 0, i32 3
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <4 x i32> %v3, ptr addrspace(1) %p0, align 16
  ret void
}

; The first element is a constant.
;
; CHECK-LABEL: Function constant_first_element
; CHECK: Original schedule
; CHECK: ([[OGSTART:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[OGSTART]], 0 {{.*}} OG:{{.*}} ret void
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @constant_first_element(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %p.x0 = getelementptr inbounds float, ptr addrspace(1) %A, i64 64
  %x0 = load float, ptr addrspace(1) %p.x0, align 4
  %p.x1 = getelementptr inbounds float, ptr addrspace(1) %A, i64 65
  %x1 = load float, ptr addrspace(1) %p.x1, align 4
  %p.x2 = getelementptr inbounds float, ptr addrspace(1) %A, i64 66
  %x2 = load float, ptr addrspace(1) %p.x2, align 4
  %s0 = fptosi float %x0 to i32
  %s1 = fptosi float %x1 to i32
  %s2 = fptosi float %x2 to i32
  %v0 = insertelement <4 x i32> poison, i32 7, i32 0
  %v1 = insertelement <4 x i32> %v0, i32 %s0, i32 1
  %v2 = insertelement <4 x i32> %v1, i32 %s1, i32 2
  %v3 = insertelement <4 x i32> %v2, i32 %s2, i32 3
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <4 x i32> %v3, ptr addrspace(1) %p0, align 16
  ret void
}

; The last element is a kernel argument.
;
; CHECK-LABEL: Function argument_element
; CHECK: Original schedule
; CHECK: ([[OGSTART:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[OGSTART]], 0 {{.*}} OG:{{.*}} ret void
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @argument_element(ptr addrspace(1) %A, ptr addrspace(1) %dst, i32 %k) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %p.x0 = getelementptr inbounds float, ptr addrspace(1) %A, i64 64
  %x0 = load float, ptr addrspace(1) %p.x0, align 4
  %p.x1 = getelementptr inbounds float, ptr addrspace(1) %A, i64 65
  %x1 = load float, ptr addrspace(1) %p.x1, align 4
  %p.x2 = getelementptr inbounds float, ptr addrspace(1) %A, i64 66
  %x2 = load float, ptr addrspace(1) %p.x2, align 4
  %s0 = fptosi float %x0 to i32
  %s1 = fptosi float %x1 to i32
  %s2 = fptosi float %x2 to i32
  %v0 = insertelement <4 x i32> poison, i32 %s0, i32 0
  %v1 = insertelement <4 x i32> %v0, i32 %s1, i32 1
  %v2 = insertelement <4 x i32> %v1, i32 %s2, i32 2
  %v3 = insertelement <4 x i32> %v2, i32 %k, i32 3
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <4 x i32> %v3, ptr addrspace(1) %p0, align 16
  ret void
}

; A scalar is a bitcast of %e2, an extract of %v, which is extracted to scalars.
; The extract adds no pressure and uses the registers of %v, so it must hang
; with %v. In the original schedule %t0 is placed before any extract of %v.
;
; CHECK-LABEL: Function extract_bitcast_into_vector
; CHECK: Original schedule
; CHECK: ([[OGSTART:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[OGSTART]], 0 {{.*}} OG:{{.*}} ret void
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @extract_bitcast_into_vector(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %p.x = getelementptr inbounds i16, ptr addrspace(1) %A, i64 64
  %x = load i16, ptr addrspace(1) %p.x, align 2
  %t0 = insertelement <2 x i16> poison, i16 %x, i32 0
  %p.v = getelementptr inbounds half, ptr addrspace(1) %A, i64 128
  %v = load <4 x half>, ptr addrspace(1) %p.v, align 8
  %e0 = extractelement <4 x half> %v, i32 0
  %e1 = extractelement <4 x half> %v, i32 1
  %e2 = extractelement <4 x half> %v, i32 2
  %e3 = extractelement <4 x half> %v, i32 3
  %c2 = bitcast half %e2 to i16
  %t1 = insertelement <2 x i16> %t0, i16 %c2, i32 1
  %s1 = fadd half %e0, %e1
  %s3 = fadd half %s1, %e3
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <2 x i16> %t1, ptr addrspace(1) %p0, align 4
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store half %s3, ptr addrspace(1) %p1, align 2
  ret void
}

; %s1 also lives out of the block, so it is never released here and must not
; keep the other scalars from being released at %v3. The block ends higher by the
; size of %s1, 2 registers.
;
; CHECK-LABEL: Function liveout_scalar
; CHECK: Original schedule: entry
; CHECK: ([[#ENTRYSTART:]], 0 {{.*}} %base = ptrtoint
; CHECK: , -192 {{.*}} OG:{{.*}} %v3 = insertelement
; CHECK: ([[#ENTRYSTART+2]], 0 {{.*}} OG:{{.*}} br label %next

define spir_kernel void @liveout_scalar(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %p.x0 = getelementptr inbounds float, ptr addrspace(1) %A, i64 64
  %x0 = load float, ptr addrspace(1) %p.x0, align 4
  %p.x1 = getelementptr inbounds float, ptr addrspace(1) %A, i64 65
  %x1 = load float, ptr addrspace(1) %p.x1, align 4
  %p.x2 = getelementptr inbounds float, ptr addrspace(1) %A, i64 66
  %x2 = load float, ptr addrspace(1) %p.x2, align 4
  %p.x3 = getelementptr inbounds float, ptr addrspace(1) %A, i64 67
  %x3 = load float, ptr addrspace(1) %p.x3, align 4
  %s0 = fptosi float %x0 to i32
  %s1 = fptosi float %x1 to i32
  %s2 = fptosi float %x2 to i32
  %s3 = fptosi float %x3 to i32
  %v0 = insertelement <4 x i32> poison, i32 %s0, i32 0
  %v1 = insertelement <4 x i32> %v0, i32 %s1, i32 1
  %v2 = insertelement <4 x i32> %v1, i32 %s2, i32 2
  %v3 = insertelement <4 x i32> %v2, i32 %s3, i32 3
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <4 x i32> %v3, ptr addrspace(1) %p0, align 16
  br label %next

next:
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store i32 %s1, ptr addrspace(1) %p1, align 4
  ret void
}

; Only %s0 hangs, because the other elements are constants. So %s0 is released at
; %v0, and the estimate of %v0 must equal the update: 256 bytes for the vector
; minus 64 for %s0.
;
; CHECK-LABEL: Function single_tracked_scalar
; CHECK: Original schedule
; CHECK: , 192 {{.*}} OG:{{.*}} %v0 = insertelement
; CHECK: Greedy MW attempt
; CHECK: , 192 {{.*}} MW:{{.*}} %v0 = insertelement

define spir_kernel void @single_tracked_scalar(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %p.x0 = getelementptr inbounds float, ptr addrspace(1) %A, i64 64
  %x0 = load float, ptr addrspace(1) %p.x0, align 4
  %s0 = fptosi float %x0 to i32
  %v0 = insertelement <4 x i32> poison, i32 %s0, i32 0
  %v1 = insertelement <4 x i32> %v0, i32 0, i32 1
  %v2 = insertelement <4 x i32> %v1, i32 0, i32 2
  %v3 = insertelement <4 x i32> %v2, i32 0, i32 3
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <4 x i32> %v3, ptr addrspace(1) %p0, align 16
  ret void
}

; A scalar is a bitcast of %d, a one-element no-op subvector of %ld. %d uses the
; registers of %ld, so it must hang with %ld. In the original schedule %t0 is
; placed before %d.
;
; CHECK-LABEL: Function noop_subvector_bitcast_scalar
; CHECK: Original schedule
; CHECK: ([[OGSTART:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[OGSTART]], 0 {{.*}} OG:{{.*}} ret void
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @noop_subvector_bitcast_scalar(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %p.x = getelementptr inbounds i16, ptr addrspace(1) %A, i64 64
  %x = load i16, ptr addrspace(1) %p.x, align 2
  %t0 = insertelement <2 x i16> poison, i16 %x, i32 0
  %e = extractelement <16 x i16> %ld, i32 3
  %d = insertelement <1 x i16> poison, i16 %e, i32 0
  %s = bitcast <1 x i16> %d to i16
  %t1 = insertelement <2 x i16> %t0, i16 %s, i32 1
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <2 x i16> %t1, ptr addrspace(1) %p0, align 4
  ret void
}

; %s0, %s1 and %s2 are each inserted into two vectors, partly through bitcasts.
; Each is released once.
;
; CHECK-LABEL: Function scalar_in_two_vectors
; CHECK: Original schedule
; CHECK: ([[OGSTART:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[OGSTART]], 0 {{.*}} OG:{{.*}} ret void
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @scalar_in_two_vectors(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %p.x0 = getelementptr inbounds float, ptr addrspace(1) %A, i64 64
  %x0 = load float, ptr addrspace(1) %p.x0, align 4
  %p.x1 = getelementptr inbounds float, ptr addrspace(1) %A, i64 65
  %x1 = load float, ptr addrspace(1) %p.x1, align 4
  %p.x2 = getelementptr inbounds float, ptr addrspace(1) %A, i64 66
  %x2 = load float, ptr addrspace(1) %p.x2, align 4
  %s0 = fptosi float %x0 to i32
  %s1 = fptosi float %x1 to i32
  %s2 = fptosi float %x2 to i32
  %c1 = bitcast i32 %s1 to float
  %a0 = insertelement <2 x i32> poison, i32 %s0, i32 0
  %b0 = insertelement <2 x float> poison, float %c1, i32 0
  %a1 = insertelement <2 x i32> %a0, i32 %s1, i32 1
  %c2 = bitcast i32 %s2 to float
  %b1 = insertelement <2 x float> %b0, float %c2, i32 1
  %c0 = bitcast i32 %s0 to float
  %v0 = insertelement <2 x float> poison, float %c0, i32 0
  %v1 = insertelement <2 x float> %v0, float %c2, i32 1
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <2 x i32> %a1, ptr addrspace(1) %p0, align 8
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <2 x float> %b1, ptr addrspace(1) %p1, align 8
  %p2 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 192
  store <2 x float> %v1, ptr addrspace(1) %p2, align 8
  ret void
}

; %s0 and %s3 are also added after the vector is stored, so the scalars are
; released at %t, where both die: 256 bytes, minus 64 for %t. The estimate must
; see both die, as the update does.
;
; CHECK-LABEL: Function two_scalars_die_together
; CHECK: Original schedule
; CHECK: ([[OGSTART:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: , -192 {{.*}} OG:{{.*}} %t = add i32 %s0, %s3
; CHECK: ([[OGSTART]], 0 {{.*}} OG:{{.*}} ret void
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: , -192 {{.*}} MW:{{.*}} %t = add i32 %s0, %s3
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @two_scalars_die_together(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %p.x0 = getelementptr inbounds float, ptr addrspace(1) %A, i64 64
  %x0 = load float, ptr addrspace(1) %p.x0, align 4
  %p.x1 = getelementptr inbounds float, ptr addrspace(1) %A, i64 65
  %x1 = load float, ptr addrspace(1) %p.x1, align 4
  %p.x2 = getelementptr inbounds float, ptr addrspace(1) %A, i64 66
  %x2 = load float, ptr addrspace(1) %p.x2, align 4
  %p.x3 = getelementptr inbounds float, ptr addrspace(1) %A, i64 67
  %x3 = load float, ptr addrspace(1) %p.x3, align 4
  %s0 = fptosi float %x0 to i32
  %s1 = fptosi float %x1 to i32
  %s2 = fptosi float %x2 to i32
  %s3 = fptosi float %x3 to i32
  %v0 = insertelement <4 x i32> poison, i32 %s0, i32 0
  %v1 = insertelement <4 x i32> %v0, i32 %s1, i32 1
  %v2 = insertelement <4 x i32> %v1, i32 %s2, i32 2
  %v3 = insertelement <4 x i32> %v2, i32 %s3, i32 3
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <4 x i32> %v3, ptr addrspace(1) %p0, align 16
  %t = add i32 %s0, %s3
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store i32 %t, ptr addrspace(1) %p1, align 4
  ret void
}

declare <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
