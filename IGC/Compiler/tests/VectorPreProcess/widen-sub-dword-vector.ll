;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-vectorpreprocess -S %s -o - | FileCheck %s

target triple = "igil_32_GEN9"

;
; Positive cases: loads that should be widened to i32 elements
;

; CHECK-LABEL: @test_v6half_load
; CHECK: load <3 x i32>, ptr addrspace(3)
; CHECK: bitcast <3 x i32> %{{.*}} to <6 x half>
define void @test_v6half_load(ptr addrspace(3) %p) {
  %v = load <6 x half>, ptr addrspace(3) %p, align 4
  call void @use_v6half(<6 x half> %v)
  ret void
}

; CHECK-LABEL: @test_v6i16_load
; CHECK: load <3 x i32>, ptr addrspace(3)
; CHECK: bitcast <3 x i32> %{{.*}} to <6 x i16>
define void @test_v6i16_load(ptr addrspace(3) %p) {
  %v = load <6 x i16>, ptr addrspace(3) %p, align 4
  call void @use_v6i16(<6 x i16> %v)
  ret void
}

; CHECK-LABEL: @test_v12i8_load
; CHECK: load <3 x i32>, ptr addrspace(3)
; CHECK: bitcast <3 x i32> %{{.*}} to <12 x i8>
define void @test_v12i8_load(ptr addrspace(3) %p) {
  %v = load <12 x i8>, ptr addrspace(3) %p, align 4
  call void @use_v12i8(<12 x i8> %v)
  ret void
}

; Metadata should be preserved on the widened load.
; CHECK-LABEL: @test_v6half_metadata
; CHECK: load <3 x i32>, ptr addrspace(3) %{{.*}}, align 4, !tbaa [[TBAA:![0-9]+]], !lsc.cache.ctrl [[LSC:![0-9]+]]
define void @test_v6half_metadata(ptr addrspace(3) %p) {
  %v = load <6 x half>, ptr addrspace(3) %p, align 4, !tbaa !0, !lsc.cache.ctrl !3
  call void @use_v6half(<6 x half> %v)
  ret void
}

; LdRawIntrinsic should also be widened.
; CHECK-LABEL: @test_v6half_ldraw
; CHECK: call <3 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v3i32
; CHECK: bitcast <3 x i32> %{{.*}} to <6 x half>
define void @test_v6half_ldraw(ptr addrspace(2490368) %buf, i32 %off) {
  %v = call <6 x half> @llvm.genx.GenISA.ldrawvector.indexed.v6f16(ptr addrspace(2490368) %buf, i32 %off, i32 4, i1 false)
  call void @use_v6half(<6 x half> %v)
  ret void
}

; PredicatedLoad should also be widened (zeroinitializer merge).
; CHECK-LABEL: @test_v6half_predload
; CHECK: call <3 x i32> @llvm.genx.GenISA.PredicatedLoad.v3i32
; CHECK: bitcast <3 x i32> %{{.*}} to <6 x half>
define void @test_v6half_predload(ptr addrspace(1) %p) {
  %v = call <6 x half> @llvm.genx.GenISA.PredicatedLoad.v6f16(ptr addrspace(1) %p, i64 4, i1 true, <6 x half> zeroinitializer)
  call void @use_v6half(<6 x half> %v)
  ret void
}

; PredicatedLoad with a non-constant merge value: the merge must be bitcast
; to <3 x i32> before being passed to the widened intrinsic.
; CHECK-LABEL: @test_v6half_predload_merge
; CHECK: [[BC_MERGE:%.*]] = bitcast <6 x half> %merge to <3 x i32>
; CHECK: call <3 x i32> @llvm.genx.GenISA.PredicatedLoad.v3i32.{{.*}}({{.*}}, i64 4, i1 true, <3 x i32> [[BC_MERGE]])
; CHECK: bitcast <3 x i32> %{{.*}} to <6 x half>
define void @test_v6half_predload_merge(ptr addrspace(1) %p, <6 x half> %merge) {
  %v = call <6 x half> @llvm.genx.GenISA.PredicatedLoad.v6f16(ptr addrspace(1) %p, i64 4, i1 true, <6 x half> %merge)
  call void @use_v6half(<6 x half> %v)
  ret void
}

; Load is widened but store is NOT widened.
; CHECK-LABEL: @test_load_store_chain
; CHECK: load <3 x i32>, ptr addrspace(3)
; CHECK: bitcast <3 x i32> %{{.*}} to <6 x half>
; CHECK-NOT: store <3 x i32>
define void @test_load_store_chain(ptr addrspace(3) %p, ptr addrspace(3) %q) {
  %v = load <6 x half>, ptr addrspace(3) %p, align 4
  store <6 x half> %v, ptr addrspace(3) %q, align 4
  ret void
}

;
; Negative cases: loads/stores that must NOT be widened
;

; Power-of-2 element count: no widening.
; CHECK-LABEL: @neg_v4half_load
; CHECK: load <4 x half>, ptr addrspace(3)
; CHECK-NOT: load <2 x i32>
define void @neg_v4half_load(ptr addrspace(3) %p) {
  %v = load <4 x half>, ptr addrspace(3) %p, align 4
  call void @use_v4half(<4 x half> %v)
  ret void
}

; 6 bytes, not a multiple of 4: no widening.
; CHECK-LABEL: @neg_v3half_load
; CHECK-NOT: load {{.*}} x i32>
define void @neg_v3half_load(ptr addrspace(3) %p) {
  %v = load <3 x half>, ptr addrspace(3) %p, align 4
  call void @use_v3half(<3 x half> %v)
  ret void
}

; Alignment < 4: no widening.
; CHECK-LABEL: @neg_v6half_align2
; CHECK-NOT: load <3 x i32>
define void @neg_v6half_align2(ptr addrspace(3) %p) {
  %v = load <6 x half>, ptr addrspace(3) %p, align 2
  call void @use_v6half(<6 x half> %v)
  ret void
}

; Stores are never widened.
; CHECK-LABEL: @neg_store_v6half
; CHECK-NOT: store <3 x i32>
define void @neg_store_v6half(ptr addrspace(3) %p, <6 x half> %v) {
  store <6 x half> %v, ptr addrspace(3) %p, align 4
  ret void
}

; LdRaw with alignment < 4: no widening.
; CHECK-LABEL: @neg_v6half_ldraw_align2
; CHECK-NOT: ldrawvector.indexed.v3i32
define void @neg_v6half_ldraw_align2(ptr addrspace(2490368) %buf, i32 %off) {
  %v = call <6 x half> @llvm.genx.GenISA.ldrawvector.indexed.v6f16(ptr addrspace(2490368) %buf, i32 %off, i32 2, i1 false)
  call void @use_v6half(<6 x half> %v)
  ret void
}

; 6 bytes (6 x i8), not a multiple of 4: no widening.
; CHECK-LABEL: @neg_v6i8_load
; CHECK-NOT: load {{.*}} x i32>
define void @neg_v6i8_load(ptr addrspace(3) %p) {
  %v = load <6 x i8>, ptr addrspace(3) %p, align 4
  call void @use_v6i8(<6 x i8> %v)
  ret void
}

; Declarations
declare void @use_v6half(<6 x half>)
declare void @use_v6i16(<6 x i16>)
declare void @use_v12i8(<12 x i8>)
declare void @use_v4half(<4 x half>)
declare void @use_v3half(<3 x half>)
declare void @use_v6i8(<6 x i8>)
declare <6 x half> @llvm.genx.GenISA.ldrawvector.indexed.v6f16(ptr addrspace(2490368), i32, i32, i1)
declare <6 x half> @llvm.genx.GenISA.PredicatedLoad.v6f16(ptr addrspace(1), i64, i1, <6 x half>)

!0 = !{!1, !1, i64 0}
!1 = !{!"omnipotent char", !2, i64 0}
!2 = !{!"Simple C/C++ TBAA"}
!3 = !{i32 9}
