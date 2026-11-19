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
; Vectors that are one value behind no-op bitcasts use the registers of that
; value. A no-op subvector, made of consecutive elements, uses the registers of
; its vector too. When several of these vectors are extracted to scalars or
; shuffled, the pressure tracker must release the registers once. In each
; function the block must end at the pressure it starts with, in the original
; order and in the order of the greedy MW attempt, unless noted otherwise. The
; DPAS on %a only makes the block a candidate for scheduling.

; %ld is extracted directly and through a bitcast.
;
; CHECK-LABEL: Function load_and_bitcast_extracted
; CHECK: Original schedule
; CHECK: ([[OGSTART:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[OGSTART]], 0 {{.*}} OG:{{.*}} ret void
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @load_and_bitcast_extracted(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %h = bitcast <8 x i16> %ld to <8 x half>
  %i0 = extractelement <8 x i16> %ld, i32 0
  %i1 = extractelement <8 x i16> %ld, i32 1
  %i2 = extractelement <8 x i16> %ld, i32 2
  %i3 = extractelement <8 x i16> %ld, i32 3
  %i4 = extractelement <8 x i16> %ld, i32 4
  %i5 = extractelement <8 x i16> %ld, i32 5
  %i6 = extractelement <8 x i16> %ld, i32 6
  %i7 = extractelement <8 x i16> %ld, i32 7
  %i.sum1 = add i16 %i0, %i1
  %i.sum2 = add i16 %i.sum1, %i2
  %i.sum3 = add i16 %i.sum2, %i3
  %i.sum4 = add i16 %i.sum3, %i4
  %i.sum5 = add i16 %i.sum4, %i5
  %i.sum6 = add i16 %i.sum5, %i6
  %i.sum7 = add i16 %i.sum6, %i7
  %f0 = extractelement <8 x half> %h, i32 0
  %f1 = extractelement <8 x half> %h, i32 1
  %f2 = extractelement <8 x half> %h, i32 2
  %f3 = extractelement <8 x half> %h, i32 3
  %f4 = extractelement <8 x half> %h, i32 4
  %f5 = extractelement <8 x half> %h, i32 5
  %f6 = extractelement <8 x half> %h, i32 6
  %f7 = extractelement <8 x half> %h, i32 7
  %f.sum1 = fadd half %f0, %f1
  %f.sum2 = fadd half %f.sum1, %f2
  %f.sum3 = fadd half %f.sum2, %f3
  %f.sum4 = fadd half %f.sum3, %f4
  %f.sum5 = fadd half %f.sum4, %f5
  %f.sum6 = fadd half %f.sum5, %f6
  %f.sum7 = fadd half %f.sum6, %f7
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store i16 %i.sum7, ptr addrspace(1) %p0, align 2
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store half %f.sum7, ptr addrspace(1) %p1, align 2
  ret void
}

; A bitcast of %ld is extracted, and %ld is shuffled.
;
; CHECK-LABEL: Function bitcast_extracted_load_shuffled
; CHECK: Original schedule
; CHECK: ([[OGSTART:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[OGSTART]], 0 {{.*}} OG:{{.*}} ret void
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @bitcast_extracted_load_shuffled(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %h = bitcast <8 x i16> %ld to <8 x half>
  %f0 = extractelement <8 x half> %h, i32 0
  %f1 = extractelement <8 x half> %h, i32 1
  %f2 = extractelement <8 x half> %h, i32 2
  %f3 = extractelement <8 x half> %h, i32 3
  %f4 = extractelement <8 x half> %h, i32 4
  %f5 = extractelement <8 x half> %h, i32 5
  %f6 = extractelement <8 x half> %h, i32 6
  %f7 = extractelement <8 x half> %h, i32 7
  %f.sum1 = fadd half %f0, %f1
  %f.sum2 = fadd half %f.sum1, %f2
  %f.sum3 = fadd half %f.sum2, %f3
  %f.sum4 = fadd half %f.sum3, %f4
  %f.sum5 = fadd half %f.sum4, %f5
  %f.sum6 = fadd half %f.sum5, %f6
  %f.sum7 = fadd half %f.sum6, %f7
  %sh.e0 = extractelement <8 x i16> %ld, i32 1
  %sh.e1 = extractelement <8 x i16> %ld, i32 0
  %sh.e2 = extractelement <8 x i16> %ld, i32 3
  %sh.e3 = extractelement <8 x i16> %ld, i32 2
  %sh.e4 = extractelement <8 x i16> %ld, i32 5
  %sh.e5 = extractelement <8 x i16> %ld, i32 4
  %sh.e6 = extractelement <8 x i16> %ld, i32 7
  %sh.e7 = extractelement <8 x i16> %ld, i32 6
  %sh0 = insertelement <8 x i16> poison, i16 %sh.e0, i32 0
  %sh1 = insertelement <8 x i16> %sh0, i16 %sh.e1, i32 1
  %sh2 = insertelement <8 x i16> %sh1, i16 %sh.e2, i32 2
  %sh3 = insertelement <8 x i16> %sh2, i16 %sh.e3, i32 3
  %sh4 = insertelement <8 x i16> %sh3, i16 %sh.e4, i32 4
  %sh5 = insertelement <8 x i16> %sh4, i16 %sh.e5, i32 5
  %sh6 = insertelement <8 x i16> %sh5, i16 %sh.e6, i32 6
  %sh7 = insertelement <8 x i16> %sh6, i16 %sh.e7, i32 7
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store half %f.sum7, ptr addrspace(1) %p0, align 2
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <8 x i16> %sh7, ptr addrspace(1) %p1, align 16
  ret void
}

; %ld is split into no-op halves, and the low half is shuffled.
;
; CHECK-LABEL: Function noop_half_shuffled
; CHECK: Original schedule
; CHECK: ([[OGSTART:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[OGSTART]], 0 {{.*}} OG:{{.*}} ret void
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @noop_half_shuffled(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %lo.e0 = extractelement <8 x i16> %ld, i32 0
  %lo.e1 = extractelement <8 x i16> %ld, i32 1
  %lo.e2 = extractelement <8 x i16> %ld, i32 2
  %lo.e3 = extractelement <8 x i16> %ld, i32 3
  %lo0 = insertelement <4 x i16> poison, i16 %lo.e0, i32 0
  %lo1 = insertelement <4 x i16> %lo0, i16 %lo.e1, i32 1
  %lo2 = insertelement <4 x i16> %lo1, i16 %lo.e2, i32 2
  %lo3 = insertelement <4 x i16> %lo2, i16 %lo.e3, i32 3
  %hi.e0 = extractelement <8 x i16> %ld, i32 4
  %hi.e1 = extractelement <8 x i16> %ld, i32 5
  %hi.e2 = extractelement <8 x i16> %ld, i32 6
  %hi.e3 = extractelement <8 x i16> %ld, i32 7
  %hi0 = insertelement <4 x i16> poison, i16 %hi.e0, i32 0
  %hi1 = insertelement <4 x i16> %hi0, i16 %hi.e1, i32 1
  %hi2 = insertelement <4 x i16> %hi1, i16 %hi.e2, i32 2
  %hi3 = insertelement <4 x i16> %hi2, i16 %hi.e3, i32 3
  %sh.e0 = extractelement <4 x i16> %lo3, i32 1
  %sh.e1 = extractelement <4 x i16> %lo3, i32 0
  %sh.e2 = extractelement <4 x i16> %lo3, i32 3
  %sh.e3 = extractelement <4 x i16> %lo3, i32 2
  %sh0 = insertelement <4 x i16> poison, i16 %sh.e0, i32 0
  %sh1 = insertelement <4 x i16> %sh0, i16 %sh.e1, i32 1
  %sh2 = insertelement <4 x i16> %sh1, i16 %sh.e2, i32 2
  %sh3 = insertelement <4 x i16> %sh2, i16 %sh.e3, i32 3
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <4 x i16> %hi3, ptr addrspace(1) %p0, align 16
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <4 x i16> %sh3, ptr addrspace(1) %p1, align 16
  ret void
}

; %ld is extracted to scalars, and one scalar is unused. An extract without uses
; does not hang, so %ld is released when the last used scalar dies.
;
; CHECK-LABEL: Function unused_extract
; CHECK: Original schedule
; CHECK: ([[OGSTART:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[OGSTART]], 0 {{.*}} OG:{{.*}} ret void
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @unused_extract(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %i0 = extractelement <8 x i16> %ld, i32 0
  %i1 = extractelement <8 x i16> %ld, i32 1
  %i2 = extractelement <8 x i16> %ld, i32 2
  %i3 = extractelement <8 x i16> %ld, i32 3
  %i4 = extractelement <8 x i16> %ld, i32 4
  %i5 = extractelement <8 x i16> %ld, i32 5
  %i6 = extractelement <8 x i16> %ld, i32 6
  %i7 = extractelement <8 x i16> %ld, i32 7
  %i.sum1 = add i16 %i0, %i1
  %i.sum2 = add i16 %i.sum1, %i2
  %i.sum3 = add i16 %i.sum2, %i3
  %i.sum4 = add i16 %i.sum3, %i4
  %i.sum5 = add i16 %i.sum4, %i5
  %i.sum6 = add i16 %i.sum5, %i6
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store i16 %i.sum6, ptr addrspace(1) %p0, align 2
  ret void
}

; %e is the only extract of %h, a bitcast of %ld. Placing %e releases nothing:
; %ld hangs until %d dies. The estimate must see %ld behind the bitcast, as it
; does without one.
;
; CHECK-LABEL: Function bitcast_source_one_element
; CHECK: Original schedule
; CHECK: ([[OGSTART:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[OGSTART]], 0 {{.*}} OG:{{.*}} ret void
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[#]], 0 {{.*}} MW:{{.*}} %e = extractelement <16 x half> %h
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @bitcast_source_one_element(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %h = bitcast <16 x i16> %ld to <16 x half>
  %e = extractelement <16 x half> %h, i32 5
  %d = insertelement <1 x half> poison, half %e, i32 0
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <1 x half> %d, ptr addrspace(1) %p0, align 2
  ret void
}

; The same for a vector that is extracted to scalars.
;
; CHECK-LABEL: Function bitcast_source_one_element_extracted
; CHECK: Original schedule
; CHECK: ([[OGSTART:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[OGSTART]], 0 {{.*}} OG:{{.*}} ret void
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[#]], 0 {{.*}} MW:{{.*}} %e = extractelement <1 x float> %h
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @bitcast_source_one_element_extracted(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %p.v = getelementptr inbounds i32, ptr addrspace(1) %A, i64 64
  %v = load <1 x i32>, ptr addrspace(1) %p.v, align 4
  %h = bitcast <1 x i32> %v to <1 x float>
  %e = extractelement <1 x float> %h, i32 0
  %r = fadd float %e, 1.0
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store float %r, ptr addrspace(1) %p0, align 4
  ret void
}

; %v is extracted in both blocks. In %next, %e0 and %e1 of %entry cannot die, so
; %v is released when %e2 and %e3 die: 256 bytes, minus 64 for %s2. The
; estimate must see both die at %s2, as the update does.
;
; CHECK-LABEL: Function cross_block_extracts
; CHECK: Original schedule: next
; CHECK: , -192 {{.*}} OG:{{.*}} %s2 = fadd float %e2, %e3
; CHECK: Greedy MW attempt
; CHECK: , -192 {{.*}} MW:{{.*}} %s2 = fadd float %e2, %e3

define spir_kernel void @cross_block_extracts(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %p.v = getelementptr inbounds float, ptr addrspace(1) %A, i64 64
  %v = load <4 x float>, ptr addrspace(1) %p.v, align 16
  %e0 = extractelement <4 x float> %v, i32 0
  %e1 = extractelement <4 x float> %v, i32 1
  %s1 = fadd float %e0, %e1
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store float %s1, ptr addrspace(1) %p1, align 4
  br label %next

next:
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas2, ptr addrspace(1) %dst, align 32
  %e2 = extractelement <4 x float> %v, i32 2
  %e3 = extractelement <4 x float> %v, i32 3
  %s2 = fadd float %e2, %e3
  %p2 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store float %s2, ptr addrspace(1) %p2, align 4
  ret void
}

; %ld is split into no-op halves, %lo in %entry and %hi in %next. In %next, %lo
; cannot die, so %ld is released when %hi dies: 256 bytes, plus 128 for %p1.
;
; CHECK-LABEL: Function cross_block_subvectors
; CHECK: Original schedule: next
; CHECK: , -384 {{.*}} OG:{{.*}} store <4 x i16> %hi3

define spir_kernel void @cross_block_subvectors(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  %lo.e0 = extractelement <8 x i16> %ld, i32 0
  %lo.e1 = extractelement <8 x i16> %ld, i32 1
  %lo.e2 = extractelement <8 x i16> %ld, i32 2
  %lo.e3 = extractelement <8 x i16> %ld, i32 3
  %lo0 = insertelement <4 x i16> poison, i16 %lo.e0, i32 0
  %lo1 = insertelement <4 x i16> %lo0, i16 %lo.e1, i32 1
  %lo2 = insertelement <4 x i16> %lo1, i16 %lo.e2, i32 2
  %lo3 = insertelement <4 x i16> %lo2, i16 %lo.e3, i32 3
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <4 x i16> %lo3, ptr addrspace(1) %p0, align 8
  br label %next

next:
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  %hi.e0 = extractelement <8 x i16> %ld, i32 4
  %hi.e1 = extractelement <8 x i16> %ld, i32 5
  %hi.e2 = extractelement <8 x i16> %ld, i32 6
  %hi.e3 = extractelement <8 x i16> %ld, i32 7
  %hi0 = insertelement <4 x i16> poison, i16 %hi.e0, i32 0
  %hi1 = insertelement <4 x i16> %hi0, i16 %hi.e1, i32 1
  %hi2 = insertelement <4 x i16> %hi1, i16 %hi.e2, i32 2
  %hi3 = insertelement <4 x i16> %hi2, i16 %hi.e3, i32 3
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <4 x i16> %hi3, ptr addrspace(1) %p1, align 8
  ret void
}

; Two elements of %c, a bitcast of %ld, are copied to %y, then one element of
; %ld to %w. Placing %z releases nothing: %ld hangs until %z dies. The estimate
; must see that %z joins the hanging values of %ld, as the update does.
;
; CHECK-LABEL: Function two_copies_of_bitcast_source
; CHECK: Original schedule
; CHECK: ([[OGSTART:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[OGSTART]], 0 {{.*}} OG:{{.*}} ret void
; CHECK: Greedy MW attempt
; CHECK: ([[START:[0-9]+]], 0 {{.*}} %base = ptrtoint
; CHECK: ([[#]], 0 {{.*}} MW:{{.*}} %z = extractelement <8 x float> %ld
; CHECK: ([[START]], 0 {{.*}} ret void
; CHECK-NEXT: Attempt 0 (greedy-mw): result=retained,

define spir_kernel void @two_copies_of_bitcast_source(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %pl = getelementptr inbounds float, ptr addrspace(1) %A, i64 64
  %ld = load <8 x float>, ptr addrspace(1) %pl, align 32
  %c = bitcast <8 x float> %ld to <8 x i32>
  %x0 = extractelement <8 x i32> %c, i32 0
  %x2 = extractelement <8 x i32> %c, i32 2
  %y0 = insertelement <2 x i32> poison, i32 %x0, i32 0
  %y1 = insertelement <2 x i32> %y0, i32 %x2, i32 1
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <2 x i32> %y1, ptr addrspace(1) %p0, align 8
  %z = extractelement <8 x float> %ld, i32 5
  %w = insertelement <1 x float> poison, float %z, i32 0
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <1 x float> %w, ptr addrspace(1) %p1, align 4
  ret void
}

; The no-op subvector %lo is started in %entry and completed in %next. In %next,
; %ld is released when %lo dies: 256 bytes, plus 128 for %p0.
;
; CHECK-LABEL: Function subvector_completed_in_next_block
; CHECK: Original schedule: next
; CHECK: , -384 {{.*}} OG:{{.*}} store <4 x i16> %lo3

define spir_kernel void @subvector_completed_in_next_block(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  %lo.e0 = extractelement <8 x i16> %ld, i32 0
  %lo.e1 = extractelement <8 x i16> %ld, i32 1
  %lo0 = insertelement <4 x i16> poison, i16 %lo.e0, i32 0
  %lo1 = insertelement <4 x i16> %lo0, i16 %lo.e1, i32 1
  br label %next

next:
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  %lo.e2 = extractelement <8 x i16> %ld, i32 2
  %lo.e3 = extractelement <8 x i16> %ld, i32 3
  %lo2 = insertelement <4 x i16> %lo1, i16 %lo.e2, i32 2
  %lo3 = insertelement <4 x i16> %lo2, i16 %lo.e3, i32 3
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <4 x i16> %lo3, ptr addrspace(1) %p0, align 8
  ret void
}

; The same for a copy %lo, which has registers of its own: %next counts %lo1 at
; its start, releases %ld when the last element is copied and %lo when it dies,
; and ends at the pressure %entry starts with.
;
; CHECK-LABEL: Function copy_completed_in_next_block
; CHECK: Original schedule: entry
; CHECK: ([[START:[0-9]+]], 0 {{.*}} OG:{{.*}} %base = ptrtoint
; CHECK: Original schedule: next
; CHECK: , -256 {{.*}} OG:{{.*}} %lo3 = insertelement
; CHECK: ([[START]], 0 {{.*}} OG:{{.*}} ret void

define spir_kernel void @copy_completed_in_next_block(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  %lo.e0 = extractelement <8 x i16> %ld, i32 0
  %lo.e1 = extractelement <8 x i16> %ld, i32 2
  %lo0 = insertelement <4 x i16> poison, i16 %lo.e0, i32 0
  %lo1 = insertelement <4 x i16> %lo0, i16 %lo.e1, i32 1
  br label %next

next:
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  %lo.e2 = extractelement <8 x i16> %ld, i32 4
  %lo.e3 = extractelement <8 x i16> %ld, i32 6
  %lo2 = insertelement <4 x i16> %lo1, i16 %lo.e2, i32 2
  %lo3 = insertelement <4 x i16> %lo2, i16 %lo.e3, i32 3
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <4 x i16> %lo3, ptr addrspace(1) %p0, align 8
  ret void
}

; The no-op subvector %lo of %ld is complete in %entry and dies in %next before
; %hi is built. A dead value cannot release %ld, so %ld is released when %hi
; dies: 256 bytes, plus 128 for %p1.
;
; CHECK-LABEL: Function dead_livein_subvector
; CHECK: Original schedule: next
; CHECK: , -384 {{.*}} OG:{{.*}} store <4 x i16> %hi3

define spir_kernel void @dead_livein_subvector(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  %lo.e0 = extractelement <8 x i16> %ld, i32 0
  %lo.e1 = extractelement <8 x i16> %ld, i32 1
  %lo.e2 = extractelement <8 x i16> %ld, i32 2
  %lo.e3 = extractelement <8 x i16> %ld, i32 3
  %lo0 = insertelement <4 x i16> poison, i16 %lo.e0, i32 0
  %lo1 = insertelement <4 x i16> %lo0, i16 %lo.e1, i32 1
  %lo2 = insertelement <4 x i16> %lo1, i16 %lo.e2, i32 2
  %lo3 = insertelement <4 x i16> %lo2, i16 %lo.e3, i32 3
  br label %next

next:
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <4 x i16> %lo3, ptr addrspace(1) %p0, align 8
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  %hi.e0 = extractelement <8 x i16> %ld, i32 4
  %hi.e1 = extractelement <8 x i16> %ld, i32 5
  %hi.e2 = extractelement <8 x i16> %ld, i32 6
  %hi.e3 = extractelement <8 x i16> %ld, i32 7
  %hi0 = insertelement <4 x i16> poison, i16 %hi.e0, i32 0
  %hi1 = insertelement <4 x i16> %hi0, i16 %hi.e1, i32 1
  %hi2 = insertelement <4 x i16> %hi1, i16 %hi.e2, i32 2
  %hi3 = insertelement <4 x i16> %hi2, i16 %hi.e3, i32 3
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <4 x i16> %hi3, ptr addrspace(1) %p1, align 8
  ret void
}

; The no-op subvector %lo of %ld comes from %entry and is copied to %m in %next.
; %m does not release %ld, which %hi still uses: %ld is released when %hi dies.
;
; CHECK-LABEL: Function livein_subvector_copied
; CHECK: Original schedule: next
; CHECK: , 0 {{.*}} OG:{{.*}} %m1 = insertelement
; CHECK: , -384 {{.*}} OG:{{.*}} store <4 x i16> %hi3

define spir_kernel void @livein_subvector_copied(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  %lo.e0 = extractelement <8 x i16> %ld, i32 0
  %lo.e1 = extractelement <8 x i16> %ld, i32 1
  %lo.e2 = extractelement <8 x i16> %ld, i32 2
  %lo.e3 = extractelement <8 x i16> %ld, i32 3
  %lo0 = insertelement <4 x i16> poison, i16 %lo.e0, i32 0
  %lo1 = insertelement <4 x i16> %lo0, i16 %lo.e1, i32 1
  %lo2 = insertelement <4 x i16> %lo1, i16 %lo.e2, i32 2
  %lo3 = insertelement <4 x i16> %lo2, i16 %lo.e3, i32 3
  br label %next

next:
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  %n0 = extractelement <4 x i16> %lo3, i32 0
  %n2 = extractelement <4 x i16> %lo3, i32 2
  %m0 = insertelement <2 x i16> poison, i16 %n0, i32 0
  %m1 = insertelement <2 x i16> %m0, i16 %n2, i32 1
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <2 x i16> %m1, ptr addrspace(1) %p0, align 4
  %hi.e0 = extractelement <8 x i16> %ld, i32 4
  %hi.e1 = extractelement <8 x i16> %ld, i32 5
  %hi.e2 = extractelement <8 x i16> %ld, i32 6
  %hi.e3 = extractelement <8 x i16> %ld, i32 7
  %hi0 = insertelement <4 x i16> poison, i16 %hi.e0, i32 0
  %hi1 = insertelement <4 x i16> %hi0, i16 %hi.e1, i32 1
  %hi2 = insertelement <4 x i16> %hi1, i16 %hi.e2, i32 2
  %hi3 = insertelement <4 x i16> %hi2, i16 %hi.e3, i32 3
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <4 x i16> %hi3, ptr addrspace(1) %p1, align 8
  ret void
}

; %m, a no-op subvector of the no-op subvector %lo of %ld, is started in %entry
; and completed in %next. %m and %lo use the registers of %ld, so %next counts
; only %ld at its start, releases it when %hi dies, and ends at the pressure
; %entry starts with.
;
; CHECK-LABEL: Function nested_subvector_completed_in_next_block
; CHECK: Original schedule: entry
; CHECK: ([[START:[0-9]+]], 0 {{.*}} OG:{{.*}} %base = ptrtoint
; CHECK: Original schedule: next
; CHECK: , -384 {{.*}} OG:{{.*}} store <4 x i16> %hi3
; CHECK: ([[START]], 0 {{.*}} OG:{{.*}} ret void

define spir_kernel void @nested_subvector_completed_in_next_block(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  %lo.e0 = extractelement <8 x i16> %ld, i32 0
  %lo.e1 = extractelement <8 x i16> %ld, i32 1
  %lo.e2 = extractelement <8 x i16> %ld, i32 2
  %lo.e3 = extractelement <8 x i16> %ld, i32 3
  %lo0 = insertelement <4 x i16> poison, i16 %lo.e0, i32 0
  %lo1 = insertelement <4 x i16> %lo0, i16 %lo.e1, i32 1
  %lo2 = insertelement <4 x i16> %lo1, i16 %lo.e2, i32 2
  %lo3 = insertelement <4 x i16> %lo2, i16 %lo.e3, i32 3
  %m.e0 = extractelement <4 x i16> %lo3, i32 0
  %m0 = insertelement <2 x i16> poison, i16 %m.e0, i32 0
  br label %next

next:
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  %m.e1 = extractelement <4 x i16> %lo3, i32 1
  %m1 = insertelement <2 x i16> %m0, i16 %m.e1, i32 1
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <2 x i16> %m1, ptr addrspace(1) %p0, align 4
  %hi.e0 = extractelement <8 x i16> %ld, i32 4
  %hi.e1 = extractelement <8 x i16> %ld, i32 5
  %hi.e2 = extractelement <8 x i16> %ld, i32 6
  %hi.e3 = extractelement <8 x i16> %ld, i32 7
  %hi0 = insertelement <4 x i16> poison, i16 %hi.e0, i32 0
  %hi1 = insertelement <4 x i16> %hi0, i16 %hi.e1, i32 1
  %hi2 = insertelement <4 x i16> %hi1, i16 %hi.e2, i32 2
  %hi3 = insertelement <4 x i16> %hi2, i16 %hi.e3, i32 3
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <4 x i16> %hi3, ptr addrspace(1) %p1, align 8
  ret void
}

; The no-op halves %lo and %hi of %ld are built in %entry and extracted in
; %next, where %ld itself is not live. %next counts %ld once and releases it
; once, when the extracts of both halves are dead, and ends at the pressure
; %entry starts with.
;
; CHECK-LABEL: Function livein_sibling_subvectors
; CHECK: Original schedule: entry
; CHECK: ([[START:[0-9]+]], 0 {{.*}} OG:{{.*}} %base = ptrtoint
; CHECK: Original schedule: next
; CHECK: , -224 {{.*}} OG:{{.*}} %hs1 = add i16 %h2, %h3
; CHECK: ([[START]], 0 {{.*}} OG:{{.*}} ret void

define spir_kernel void @livein_sibling_subvectors(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  %lo.e0 = extractelement <8 x i16> %ld, i32 0
  %lo.e1 = extractelement <8 x i16> %ld, i32 1
  %lo.e2 = extractelement <8 x i16> %ld, i32 2
  %lo.e3 = extractelement <8 x i16> %ld, i32 3
  %lo0 = insertelement <4 x i16> poison, i16 %lo.e0, i32 0
  %lo1 = insertelement <4 x i16> %lo0, i16 %lo.e1, i32 1
  %lo2 = insertelement <4 x i16> %lo1, i16 %lo.e2, i32 2
  %lo3 = insertelement <4 x i16> %lo2, i16 %lo.e3, i32 3
  %hi.e0 = extractelement <8 x i16> %ld, i32 4
  %hi.e1 = extractelement <8 x i16> %ld, i32 5
  %hi.e2 = extractelement <8 x i16> %ld, i32 6
  %hi.e3 = extractelement <8 x i16> %ld, i32 7
  %hi0 = insertelement <4 x i16> poison, i16 %hi.e0, i32 0
  %hi1 = insertelement <4 x i16> %hi0, i16 %hi.e1, i32 1
  %hi2 = insertelement <4 x i16> %hi1, i16 %hi.e2, i32 2
  %hi3 = insertelement <4 x i16> %hi2, i16 %hi.e3, i32 3
  br label %next

next:
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  %l0 = extractelement <4 x i16> %lo3, i32 0
  %l1 = extractelement <4 x i16> %lo3, i32 1
  %l2 = extractelement <4 x i16> %lo3, i32 2
  %l3 = extractelement <4 x i16> %lo3, i32 3
  %ls0 = add i16 %l0, %l1
  %ls1 = add i16 %l2, %l3
  %ls = add i16 %ls0, %ls1
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store i16 %ls, ptr addrspace(1) %p0, align 2
  %h0 = extractelement <4 x i16> %hi3, i32 0
  %h1 = extractelement <4 x i16> %hi3, i32 1
  %h2 = extractelement <4 x i16> %hi3, i32 2
  %h3 = extractelement <4 x i16> %hi3, i32 3
  %hs0 = add i16 %h0, %h1
  %hs1 = add i16 %h2, %h3
  %hs = add i16 %hs0, %hs1
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store i16 %hs, ptr addrspace(1) %p1, align 2
  ret void
}

; The same, but %ld is also stored in %next before the halves are extracted.
;
; CHECK-LABEL: Function livein_sibling_subvectors_owner_first
; CHECK: Original schedule: entry
; CHECK: ([[START:[0-9]+]], 0 {{.*}} OG:{{.*}} %base = ptrtoint
; CHECK: Original schedule: next
; CHECK: ([[START]], 0 {{.*}} OG:{{.*}} ret void

define spir_kernel void @livein_sibling_subvectors_owner_first(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  %lo.e0 = extractelement <8 x i16> %ld, i32 0
  %lo.e1 = extractelement <8 x i16> %ld, i32 1
  %lo.e2 = extractelement <8 x i16> %ld, i32 2
  %lo.e3 = extractelement <8 x i16> %ld, i32 3
  %lo0 = insertelement <4 x i16> poison, i16 %lo.e0, i32 0
  %lo1 = insertelement <4 x i16> %lo0, i16 %lo.e1, i32 1
  %lo2 = insertelement <4 x i16> %lo1, i16 %lo.e2, i32 2
  %lo3 = insertelement <4 x i16> %lo2, i16 %lo.e3, i32 3
  %hi.e0 = extractelement <8 x i16> %ld, i32 4
  %hi.e1 = extractelement <8 x i16> %ld, i32 5
  %hi.e2 = extractelement <8 x i16> %ld, i32 6
  %hi.e3 = extractelement <8 x i16> %ld, i32 7
  %hi0 = insertelement <4 x i16> poison, i16 %hi.e0, i32 0
  %hi1 = insertelement <4 x i16> %hi0, i16 %hi.e1, i32 1
  %hi2 = insertelement <4 x i16> %hi1, i16 %hi.e2, i32 2
  %hi3 = insertelement <4 x i16> %hi2, i16 %hi.e3, i32 3
  br label %next

next:
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  %pld = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 256
  store <8 x i16> %ld, ptr addrspace(1) %pld, align 16
  %l0 = extractelement <4 x i16> %lo3, i32 0
  %l1 = extractelement <4 x i16> %lo3, i32 1
  %l2 = extractelement <4 x i16> %lo3, i32 2
  %l3 = extractelement <4 x i16> %lo3, i32 3
  %ls0 = add i16 %l0, %l1
  %ls1 = add i16 %l2, %l3
  %ls = add i16 %ls0, %ls1
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store i16 %ls, ptr addrspace(1) %p0, align 2
  %h0 = extractelement <4 x i16> %hi3, i32 0
  %h1 = extractelement <4 x i16> %hi3, i32 1
  %h2 = extractelement <4 x i16> %hi3, i32 2
  %h3 = extractelement <4 x i16> %hi3, i32 3
  %hs0 = add i16 %h0, %h1
  %hs1 = add i16 %h2, %h3
  %hs = add i16 %hs0, %hs1
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store i16 %hs, ptr addrspace(1) %p1, align 2
  ret void
}

; The no-op half %lo of %ld comes from %entry and is only stored in %next, so no
; instruction of a vector pattern is in %next. %ld is released when %lo dies.
;
; CHECK-LABEL: Function livein_subvector_plain_use
; CHECK: Original schedule: entry
; CHECK: ([[START:[0-9]+]], 0 {{.*}} OG:{{.*}} %base = ptrtoint
; CHECK: Original schedule: next
; CHECK: ([[START]], 0 {{.*}} OG:{{.*}} ret void

define spir_kernel void @livein_subvector_plain_use(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  %lo.e0 = extractelement <8 x i16> %ld, i32 0
  %lo.e1 = extractelement <8 x i16> %ld, i32 1
  %lo.e2 = extractelement <8 x i16> %ld, i32 2
  %lo.e3 = extractelement <8 x i16> %ld, i32 3
  %lo0 = insertelement <4 x i16> poison, i16 %lo.e0, i32 0
  %lo1 = insertelement <4 x i16> %lo0, i16 %lo.e1, i32 1
  %lo2 = insertelement <4 x i16> %lo1, i16 %lo.e2, i32 2
  %lo3 = insertelement <4 x i16> %lo2, i16 %lo.e3, i32 3
  br label %next

next:
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <4 x i16> %lo3, ptr addrspace(1) %p0, align 8
  ret void
}

; Only the last insertelement of the copy %c is in %next. Its extract, from
; %entry, is the last value that reads %ld, so the estimate and the update both
; release %ld there.
;
; CHECK-LABEL: Function copy_last_insert_in_next_block
; CHECK: Original schedule: entry
; CHECK: ([[START:[0-9]+]], 0 {{.*}} OG:{{.*}} %base = ptrtoint
; CHECK: Original schedule: next
; CHECK: , -256 {{.*}} OG:{{.*}} %c3 = insertelement
; CHECK: ([[START]], 0 {{.*}} OG:{{.*}} ret void
; CHECK: Greedy MW attempt
; CHECK: , -256 {{.*}} MW:{{.*}} %c3 = insertelement

define spir_kernel void @copy_last_insert_in_next_block(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  %c.e0 = extractelement <8 x i16> %ld, i32 0
  %c.e1 = extractelement <8 x i16> %ld, i32 2
  %c.e2 = extractelement <8 x i16> %ld, i32 4
  %c.e3 = extractelement <8 x i16> %ld, i32 6
  %c0 = insertelement <4 x i16> poison, i16 %c.e0, i32 0
  %c1 = insertelement <4 x i16> %c0, i16 %c.e1, i32 1
  %c2 = insertelement <4 x i16> %c1, i16 %c.e2, i32 2
  br label %next

next:
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  %c3 = insertelement <4 x i16> %c2, i16 %c.e3, i32 3
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <4 x i16> %c3, ptr addrspace(1) %p0, align 8
  ret void
}

; %e0, extracted from %v in %entry, dies in %next before %e2 and %e3 are
; extracted. %v is released when %e2 and %e3 die: 256 bytes, minus 64 for %s2.
;
; CHECK-LABEL: Function dead_livein_extract
; CHECK: Original schedule: next
; CHECK: , -192 {{.*}} OG:{{.*}} %s2 = fadd float %e2, %e3

define spir_kernel void @dead_livein_extract(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas, ptr addrspace(1) %dst, align 32
  %p.v = getelementptr inbounds float, ptr addrspace(1) %A, i64 64
  %v = load <4 x float>, ptr addrspace(1) %p.v, align 16
  %e0 = extractelement <4 x float> %v, i32 0
  %e1 = extractelement <4 x float> %v, i32 1
  %s1 = fadd float %e0, %e1
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store float %s1, ptr addrspace(1) %p1, align 4
  br label %next

next:
  %p3 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 192
  store float %e0, ptr addrspace(1) %p3, align 4
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas2, ptr addrspace(1) %dst, align 32
  %e2 = extractelement <4 x float> %v, i32 2
  %e3 = extractelement <4 x float> %v, i32 3
  %s2 = fadd float %e2, %e3
  %p2 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store float %s2, ptr addrspace(1) %p2, align 4
  ret void
}

; The elements copied to %y are extracted in %entry and inserted in %next. The
; extracts use the registers of %ld, so %next counts %ld at its start and
; releases it at the last insertelement, where the last extract dies. %next ends
; at the pressure %entry starts with.
;
; CHECK-LABEL: Function extracts_in_predecessor
; CHECK: Original schedule: entry
; CHECK: ([[START:[0-9]+]], 0 {{.*}} OG:{{.*}} %base = ptrtoint
; CHECK: Original schedule: next
; CHECK: , -256 {{.*}} OG:{{.*}} %y1 = insertelement
; CHECK: ([[START]], 0 {{.*}} OG:{{.*}} ret void

define spir_kernel void @extracts_in_predecessor(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  %x0 = extractelement <8 x i16> %ld, i32 0
  %x2 = extractelement <8 x i16> %ld, i32 2
  br label %next

next:
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  %y0 = insertelement <2 x i16> poison, i16 %x0, i32 0
  %y1 = insertelement <2 x i16> %y0, i16 %x2, i32 1
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <2 x i16> %y1, ptr addrspace(1) %p0, align 4
  ret void
}

; The no-op subvector %lo is built in %entry and extracted to scalars in %next.
; In %next, %ld is used only by the Phi %p, which does not keep it live there.
; %lo keeps the registers of %ld until the last scalar dies at %ls1: 256 bytes,
; minus 32 for %ls1. %next ends at the pressure %entry starts with.
;
; CHECK-LABEL: Function livein_subvector_owner_phi_use
; CHECK: Original schedule: entry
; CHECK: ([[START:[0-9]+]], 0 {{.*}} OG:{{.*}} %base = ptrtoint
; CHECK: Original schedule: next
; CHECK: , -224 {{.*}} OG:{{.*}} %ls1 = add
; CHECK: ([[START]], 0 {{.*}} OG:{{.*}} ret void

define spir_kernel void @livein_subvector_owner_phi_use(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  %lo.e0 = extractelement <8 x i16> %ld, i32 0
  %lo.e1 = extractelement <8 x i16> %ld, i32 1
  %lo.e2 = extractelement <8 x i16> %ld, i32 2
  %lo.e3 = extractelement <8 x i16> %ld, i32 3
  %lo0 = insertelement <4 x i16> poison, i16 %lo.e0, i32 0
  %lo1 = insertelement <4 x i16> %lo0, i16 %lo.e1, i32 1
  %lo2 = insertelement <4 x i16> %lo1, i16 %lo.e2, i32 2
  %lo3 = insertelement <4 x i16> %lo2, i16 %lo.e3, i32 3
  %cmp = icmp eq ptr addrspace(1) %A, %dst
  br i1 %cmp, label %next, label %skip

skip:
  %base1 = ptrtoint ptr addrspace(1) %dst to i64
  %ld2 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base1, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  br label %next

next:
  %p = phi <8 x i16> [ %ld, %entry ], [ %ld2, %skip ]
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  %l0 = extractelement <4 x i16> %lo3, i32 0
  %l1 = extractelement <4 x i16> %lo3, i32 1
  %l2 = extractelement <4 x i16> %lo3, i32 2
  %l3 = extractelement <4 x i16> %lo3, i32 3
  %ls0 = add i16 %l0, %l1
  %ls1 = add i16 %l2, %l3
  %ls = add i16 %ls0, %ls1
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store i16 %ls, ptr addrspace(1) %p0, align 2
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <8 x i16> %p, ptr addrspace(1) %p1, align 16
  ret void
}

; The same with a no-op bitcast %c of %ld: %lo is a no-op subvector of %c, and
; %c is the incoming value of the Phi %p.
;
; CHECK-LABEL: Function livein_cast_owner_phi_use
; CHECK: Original schedule: entry
; CHECK: ([[START:[0-9]+]], 0 {{.*}} OG:{{.*}} %base = ptrtoint
; CHECK: Original schedule: next
; CHECK: , -224 {{.*}} OG:{{.*}} %ls1 = fadd
; CHECK: ([[START]], 0 {{.*}} OG:{{.*}} ret void

define spir_kernel void @livein_cast_owner_phi_use(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  %c = bitcast <8 x i16> %ld to <8 x half>
  %lo.e0 = extractelement <8 x half> %c, i32 0
  %lo.e1 = extractelement <8 x half> %c, i32 1
  %lo.e2 = extractelement <8 x half> %c, i32 2
  %lo.e3 = extractelement <8 x half> %c, i32 3
  %lo0 = insertelement <4 x half> poison, half %lo.e0, i32 0
  %lo1 = insertelement <4 x half> %lo0, half %lo.e1, i32 1
  %lo2 = insertelement <4 x half> %lo1, half %lo.e2, i32 2
  %lo3 = insertelement <4 x half> %lo2, half %lo.e3, i32 3
  %cmp = icmp eq ptr addrspace(1) %A, %dst
  br i1 %cmp, label %next, label %skip

skip:
  %base1 = ptrtoint ptr addrspace(1) %dst to i64
  %ld2 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base1, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %c2 = bitcast <8 x i16> %ld2 to <8 x half>
  br label %next

next:
  %p = phi <8 x half> [ %c, %entry ], [ %c2, %skip ]
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  %l0 = extractelement <4 x half> %lo3, i32 0
  %l1 = extractelement <4 x half> %lo3, i32 1
  %l2 = extractelement <4 x half> %lo3, i32 2
  %l3 = extractelement <4 x half> %lo3, i32 3
  %ls0 = fadd half %l0, %l1
  %ls1 = fadd half %l2, %l3
  %ls = fadd half %ls0, %ls1
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store half %ls, ptr addrspace(1) %p0, align 2
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <8 x half> %p, ptr addrspace(1) %p1, align 16
  ret void
}

; The no-op subvector %lo is built in %entry except for its last insertelement
; %lo3, which is in %next. %ld has no use in %next, but %lo2 and %lo.e3 use its
; registers, so %entry ends at the pressure %next starts with. %lo3 keeps the
; registers until the store releases them: 256 bytes, plus 128 for %p0. The
; estimate at %lo3, like the update, releases nothing.
;
; CHECK-LABEL: Function noop_last_insert_in_next_block
; CHECK: Original schedule: entry
; CHECK: ([[START:[0-9]+]], 0 {{.*}} OG:{{.*}} %base = ptrtoint
; CHECK: ([[END:[0-9]+]], 0 {{.*}} OG:{{.*}} br label %next
; CHECK: Original schedule: next
; CHECK: ([[END]], 0 {{.*}} OG:{{.*}} %base2 = ptrtoint
; CHECK: , -384 {{.*}} OG:{{.*}} store <4 x i16> %lo3
; CHECK: ([[START]], 0 {{.*}} OG:{{.*}} ret void
; CHECK: Greedy MW attempt
; CHECK: ({{[0-9]+}}, 0 {{.*}} MW:{{.*}} %lo3 = insertelement

define spir_kernel void @noop_last_insert_in_next_block(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  %lo.e0 = extractelement <8 x i16> %ld, i32 0
  %lo.e1 = extractelement <8 x i16> %ld, i32 1
  %lo.e2 = extractelement <8 x i16> %ld, i32 2
  %lo.e3 = extractelement <8 x i16> %ld, i32 3
  %lo0 = insertelement <4 x i16> poison, i16 %lo.e0, i32 0
  %lo1 = insertelement <4 x i16> %lo0, i16 %lo.e1, i32 1
  %lo2 = insertelement <4 x i16> %lo1, i16 %lo.e2, i32 2
  br label %next

next:
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  %lo3 = insertelement <4 x i16> %lo2, i16 %lo.e3, i32 3
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <4 x i16> %lo3, ptr addrspace(1) %p0, align 8
  ret void
}

; The same for a one-element no-op subvector %one: its extract %e is in %entry
; and its only insertelement is in %next.
;
; CHECK-LABEL: Function one_element_insert_in_next_block
; CHECK: Original schedule: entry
; CHECK: ([[START:[0-9]+]], 0 {{.*}} OG:{{.*}} %base = ptrtoint
; CHECK: ([[END:[0-9]+]], 0 {{.*}} OG:{{.*}} br label %next
; CHECK: Original schedule: next
; CHECK: ([[END]], 0 {{.*}} OG:{{.*}} %base2 = ptrtoint
; CHECK: , -384 {{.*}} OG:{{.*}} store <1 x i16> %one
; CHECK: ([[START]], 0 {{.*}} OG:{{.*}} ret void
; CHECK: Greedy MW attempt
; CHECK: ({{[0-9]+}}, 0 {{.*}} MW:{{.*}} %one = insertelement

define spir_kernel void @one_element_insert_in_next_block(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  %e = extractelement <8 x i16> %ld, i32 3
  br label %next

next:
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  %one = insertelement <1 x i16> poison, i16 %e, i32 0
  %p0 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <1 x i16> %one, ptr addrspace(1) %p0, align 2
  ret void
}

; %ld and its no-op bitcast %c are both live-in to %next and both used there.
; %c has the registers of %ld, so %next counts them once at its start, and
; releases them at the store of %c, the last use of either: 256 bytes, plus 128
; for %p2. %next ends at the pressure %entry starts with.
;
; CHECK-LABEL: Function livein_cast_and_source
; CHECK: Original schedule: entry
; CHECK: ([[START:[0-9]+]], 0 {{.*}} OG:{{.*}} %base = ptrtoint
; CHECK: Original schedule: next
; CHECK: , -384 {{.*}} OG:{{.*}} store <8 x half> %c
; CHECK: ([[START]], 0 {{.*}} OG:{{.*}} ret void

define spir_kernel void @livein_cast_and_source(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %c = bitcast <8 x i16> %ld to <8 x half>
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  br label %next

next:
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <8 x i16> %ld, ptr addrspace(1) %p1, align 16
  %p2 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <8 x half> %c, ptr addrspace(1) %p2, align 16
  ret void
}

; The same with a no-op subvector %lo of %ld and a plain use of %c.
;
; CHECK-LABEL: Function livein_cast_and_subvector
; CHECK: Original schedule: entry
; CHECK: ([[START:[0-9]+]], 0 {{.*}} OG:{{.*}} %base = ptrtoint
; CHECK: Original schedule: next
; CHECK: , -384 {{.*}} OG:{{.*}} store <8 x half> %c
; CHECK: ([[START]], 0 {{.*}} OG:{{.*}} ret void

define spir_kernel void @livein_cast_and_subvector(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %c = bitcast <8 x i16> %ld to <8 x half>
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  br label %next

next:
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  %lo.e0 = extractelement <8 x i16> %ld, i32 0
  %lo.e1 = extractelement <8 x i16> %ld, i32 1
  %lo0 = insertelement <2 x i16> poison, i16 %lo.e0, i32 0
  %lo1 = insertelement <2 x i16> %lo0, i16 %lo.e1, i32 1
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <2 x i16> %lo1, ptr addrspace(1) %p1, align 4
  %p2 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <8 x half> %c, ptr addrspace(1) %p2, align 16
  ret void
}

; %p and its addrspacecast %c are both live-in to %next. An addrspacecast is
; not a no-op cast that the users of %p look through, so %c and %p are counted
; separately and each is released at its own last use. %next ends at the
; pressure %entry starts with.
; The test passes without the live-in cast handling and guards it against
; counting this case wrongly.
;
; CHECK-LABEL: Function livein_addrspacecast_and_source
; CHECK: Original schedule: entry
; CHECK: ([[START:[0-9]+]], 0 {{.*}} OG:{{.*}} %base = ptrtoint
; CHECK: Original schedule: next
; CHECK: ([[START]], 0 {{.*}} OG:{{.*}} ret void

define spir_kernel void @livein_addrspacecast_and_source(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %lid = call i16 @llvm.genx.GenISA.simdLaneId()
  %lz = zext i16 %lid to i64
  %q = getelementptr i64, ptr addrspace(1) %A, i64 %lz
  %v = load i64, ptr addrspace(1) %q, align 8
  %p = inttoptr i64 %v to ptr addrspace(1)
  %c = addrspacecast ptr addrspace(1) %p to ptr addrspace(4)
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  br label %next
next:
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  store i32 1, ptr addrspace(1) %p, align 4
  %w = load i32, ptr addrspace(4) %c, align 4
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store i32 %w, ptr addrspace(1) %p1, align 4
  ret void
}

; %ld and its no-op bitcast %c are both live-in to %next, and %c lives out of
; it. %c uses the registers of %ld, so %ld stays live to the end of %next and %c
; is not counted again: %next ends at the pressure %entry ends with.
; The test passes without the live-in cast handling and guards it against
; counting this case wrongly.
;
; CHECK-LABEL: Function livein_cast_liveout
; CHECK: Original schedule: entry
; CHECK: ([[END:[0-9]+]], 0 {{.*}} OG:{{.*}} br label %next
; CHECK: Original schedule: next
; CHECK: ([[END]], 0 {{.*}} OG:{{.*}} br label %exit

define spir_kernel void @livein_cast_liveout(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %c = bitcast <8 x i16> %ld to <8 x half>

  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  br label %next
next:
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  %p1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <8 x i16> %ld, ptr addrspace(1) %p1, align 16
  br label %exit
exit:
  %p2 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <8 x half> %c, ptr addrspace(1) %p2, align 16
  ret void
}

; %c2 is a no-op cast of the no-op cast %c1 of %ld. %ld and %c2 are live-in to
; %next, and %c1 is only the incoming value of the Phi %ph. All three share the
; registers of %ld, so %next counts them once and releases them at the last
; use of any of the three, and ends at the pressure %entry starts with.
; The test passes without the live-in cast handling and guards it against
; counting this case wrongly.
;
; CHECK-LABEL: Function livein_cast_chain_phi_incoming
; CHECK: Original schedule: entry
; CHECK: ([[START:[0-9]+]], 0 {{.*}} OG:{{.*}} %base = ptrtoint
; CHECK: Original schedule: next
; CHECK: ([[START]], 0 {{.*}} OG:{{.*}} br label %exit

define spir_kernel void @livein_cast_chain_phi_incoming(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %c1 = bitcast <8 x i16> %ld to <8 x half>
  %c2 = bitcast <8 x half> %c1 to <8 x i16>
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  br label %next
next:
  %ph = phi <8 x half> [ %c1, %entry ]
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  %sp1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <8 x i16> %c2, ptr addrspace(1) %sp1, align 16
  %sp2 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <8 x i16> %ld, ptr addrspace(1) %sp2, align 16
  %sp3 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 192
  store <8 x half> %ph, ptr addrspace(1) %sp3, align 16
  br label %exit
exit:
  ret void
}

; %ld is not live-in to %next, but its no-op subvector %s.i3 and its no-op
; bitcast %c are. %s.i3 counts the registers of %ld once, and %c shares them, so
; %next releases them at the store of %c, the last use of either.
;
; CHECK-LABEL: Function livein_cast_of_subvector_owner
; CHECK: Original schedule: entry
; CHECK: ([[START:[0-9]+]], 0 {{.*}} OG:{{.*}} %base = ptrtoint
; CHECK: Original schedule: next
; CHECK: ([[START]], 0 {{.*}} OG:{{.*}} br label %exit

define spir_kernel void @livein_cast_of_subvector_owner(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %c = bitcast <8 x i16> %ld to <8 x half>
  %s.e0 = extractelement <8 x i16> %ld, i32 0
  %s.e1 = extractelement <8 x i16> %ld, i32 1
  %s.e2 = extractelement <8 x i16> %ld, i32 2
  %s.e3 = extractelement <8 x i16> %ld, i32 3
  %s.i0 = insertelement <4 x i16> poison, i16 %s.e0, i32 0
  %s.i1 = insertelement <4 x i16> %s.i0, i16 %s.e1, i32 1
  %s.i2 = insertelement <4 x i16> %s.i1, i16 %s.e2, i32 2
  %s.i3 = insertelement <4 x i16> %s.i2, i16 %s.e3, i32 3
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.a = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.a, ptr addrspace(1) %dst, align 32
  br label %next
next:
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %dpas.b = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %dpas.b, ptr addrspace(1) %dst, align 32
  %sp7 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <4 x i16> %s.i3, ptr addrspace(1) %sp7, align 16
  %sp8 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <8 x half> %c, ptr addrspace(1) %sp8, align 16
  br label %exit
exit:
  ret void
}

; %ld is not live-in to %next, but its no-op subvector %s.i3, the subvector %t.i3
; of its no-op bitcast %v, and %v itself are. %ld owns the registers, %v shares
; them, and the subvectors use them, so %next counts them once and releases them
; at the last use of any of the three. %next ends at the pressure %entry
; starts with.
;
; CHECK-LABEL: Function livein_cast_and_vectors_of_owner
; CHECK: Original schedule: entry
; CHECK: ([[START:[0-9]+]], 0 {{.*}} OG:{{.*}} %base = ptrtoint
; CHECK: Original schedule: next
; CHECK: ([[START]], 0 {{.*}} OG:{{.*}} br label %exit

define spir_kernel void @livein_cast_and_vectors_of_owner(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %v = bitcast <8 x i16> %ld to <8 x half>

  %s.e0 = extractelement <8 x i16> %ld, i32 4
  %s.e1 = extractelement <8 x i16> %ld, i32 5
  %s.e2 = extractelement <8 x i16> %ld, i32 6
  %s.e3 = extractelement <8 x i16> %ld, i32 7
  %s.i0 = insertelement <4 x i16> poison, i16 %s.e0, i32 0
  %s.i1 = insertelement <4 x i16> %s.i0, i16 %s.e1, i32 1
  %s.i2 = insertelement <4 x i16> %s.i1, i16 %s.e2, i32 2
  %s.i3 = insertelement <4 x i16> %s.i2, i16 %s.e3, i32 3
  %t.e0 = extractelement <8 x half> %v, i32 0
  %t.e1 = extractelement <8 x half> %v, i32 1
  %t.e2 = extractelement <8 x half> %v, i32 2
  %t.e3 = extractelement <8 x half> %v, i32 3
  %t.i0 = insertelement <4 x half> poison, half %t.e0, i32 0
  %t.i1 = insertelement <4 x half> %t.i0, half %t.e1, i32 1
  %t.i2 = insertelement <4 x half> %t.i1, half %t.e2, i32 2
  %t.i3 = insertelement <4 x half> %t.i2, half %t.e3, i32 3
  %a = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 16, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %da = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %da, ptr addrspace(1) %dst, align 32
  br label %next
next:
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %b = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 32, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %db = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %b, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %db, ptr addrspace(1) %dst, align 32
  %p1_s_i3 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <4 x i16> %s.i3, ptr addrspace(1) %p1_s_i3, align 16
  %p2_t_i3 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <4 x half> %t.i3, ptr addrspace(1) %p2_t_i3, align 16
  %p3_v = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 192
  store <8 x half> %v, ptr addrspace(1) %p3_v, align 16
  br label %exit
exit:
  ret void
}

; %ld and its no-op bitcast %c are live-in to %next and live out of it. Nothing
; is released in %next, and %c shares the registers of %ld, so it is not counted
; again: %next ends at the pressure %entry ends with.
;
; CHECK-LABEL: Function livein_cast_and_source_liveout
; CHECK: Original schedule: entry
; CHECK: ([[END:[0-9]+]], 0 {{.*}} OG:{{.*}} br label %next
; CHECK: Original schedule: next
; CHECK: ([[END]], 0 {{.*}} OG:{{.*}} br label %exit

define spir_kernel void @livein_cast_and_source_liveout(ptr addrspace(1) %A, ptr addrspace(1) %dst) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %c = bitcast <8 x i16> %ld to <8 x half>
  %ma0 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 8, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %da0 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %ma0, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %da0, ptr addrspace(1) %dst, align 32
  br label %next
next:
  %base2 = ptrtoint ptr addrspace(1) %A to i64
  %ma1 = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base2, i32 127, i32 1023, i32 127, i32 0, i32 24, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %da1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %ma1, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %da1, ptr addrspace(1) %dst, align 32
  br label %exit
exit:
  %sp1 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store <8 x i16> %ld, ptr addrspace(1) %sp1, align 16
  %sp2 = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 192
  store <8 x half> %c, ptr addrspace(1) %sp2, align 16
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()
declare <16 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v16i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
