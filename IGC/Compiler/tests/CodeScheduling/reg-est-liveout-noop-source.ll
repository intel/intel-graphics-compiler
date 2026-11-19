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
; A no-op value that lives out of its block uses the registers of its source,
; so the source stays live to the end of the block, also after its last use in
; the block. The block that the value lives into starts at the pressure the
; block before it ends with: every argument that lives into the first block also
; lives into the second, and the first instruction of the second block does not
; change the pressure.

; The no-op cast %p lives out, and its source %addr is last used by %q.
;
; CHECK-LABEL: Function liveout_cast_source_dies
; CHECK: Original schedule: entry
; CHECK: ([[END:[0-9]+]], 0 {{.*}} OG:{{.*}} br label %next
; CHECK: Original schedule: next
; CHECK: ([[END]], 0 {{.*}} OG:{{.*}} store i64 %base

define spir_kernel void @liveout_cast_source_dies(ptr addrspace(1) %dst, i64 %base, <8 x i16> %a, <8 x i32> %b) {
entry:
  %d0 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  %addr = add i64 %base, 64
  %p = inttoptr i64 %addr to ptr addrspace(1)
  %q = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 %addr
  store <8 x float> %d0, ptr addrspace(1) %q, align 32
  br label %next

next:
  store i64 %base, ptr addrspace(1) %dst, align 8
  %d1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %d1, ptr addrspace(1) %p, align 32
  ret void
}

; The pair of %p lives out, its extracts are in %next, and %p is last used by
; the store in %entry. Codegen reads the halves from %p at the extracts.
;
; CHECK-LABEL: Function liveout_pair_pointer_dies
; CHECK: Original schedule: entry
; CHECK: ([[END:[0-9]+]], 0 {{.*}} OG:{{.*}} br label %next
; CHECK: Original schedule: next
; CHECK: ([[END]], 0 {{.*}} OG:{{.*}} store i64 %base

define spir_kernel void @liveout_pair_pointer_dies(ptr addrspace(1) %dst, i64 %base, <8 x i16> %a, <8 x i32> %b) {
entry:
  %d0 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  %addr = add i64 %base, 64
  %p = inttoptr i64 %addr to ptr addrspace(1)
  %pair = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p1(ptr addrspace(1) %p)
  store <8 x float> %d0, ptr addrspace(1) %p, align 32
  br label %next

next:
  store i64 %base, ptr addrspace(1) %dst, align 8
  %d1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %d1, ptr addrspace(1) %dst, align 32
  %lo = extractvalue { i32, i32 } %pair, 0
  %hi = extractvalue { i32, i32 } %pair, 1
  %s = add i32 %lo, %hi
  store i32 %s, ptr addrspace(1) %dst, align 4
  ret void
}

; %v is made of four scalars, and the no-op cast %c1 of the scalar %s1 lives
; out. %s1 stays live, so it does not hang in the group of %v, and the other
; scalars are released when the last of them dies.
;
; CHECK-LABEL: Function liveout_cast_of_vector_scalar
; CHECK: Original schedule: next
; CHECK: ([[END:[0-9]+]], 0 {{.*}} OG:{{.*}} br label %exit
; CHECK: Original schedule: exit
; CHECK: ([[END]], 0 {{.*}} OG:{{.*}} store i32 0

define spir_kernel void @liveout_cast_of_vector_scalar(ptr addrspace(1) %A, ptr addrspace(1) %dst, float %p, float %q, float %r, float %s) {
entry:
  %base = ptrtoint ptr addrspace(1) %A to i64
  %pa = fadd float %p, 1.0
  %qa = fadd float %q, 1.0
  %ra = fadd float %r, 1.0
  %sa = fadd float %s, 1.0
  br label %next

next:
  %ld = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %d = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> %ld, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %d, ptr addrspace(1) %dst, align 32
  %s0 = fadd float %pa, 1.0
  %s1 = fadd float %qa, 1.0
  %s2 = fadd float %ra, 1.0
  %s3 = fadd float %sa, 1.0
  %c1 = bitcast float %s1 to i32
  %v.0 = insertelement <4 x float> undef, float %s0, i32 0
  %v.1 = insertelement <4 x float> %v.0, float %s1, i32 1
  %v.2 = insertelement <4 x float> %v.1, float %s2, i32 2
  %v = insertelement <4 x float> %v.2, float %s3, i32 3
  %pv = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 64
  store <4 x float> %v, ptr addrspace(1) %pv, align 16
  %t = fadd float %s0, %s2
  %t2 = fadd float %t, %s3
  %pr = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 128
  store float %t2, ptr addrspace(1) %pr, align 4
  br label %exit

exit:
  store i32 0, ptr addrspace(1) %dst, align 4
  %d1 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 1, i32 1, i32 1, i32 1, i1 false)
  store <8 x float> %d1, ptr addrspace(1) %dst, align 32
  %pe = getelementptr inbounds i8, ptr addrspace(1) %dst, i64 256
  store i32 %c1, ptr addrspace(1) %pe, align 4
  ret void
}

declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p1(ptr addrspace(1))
