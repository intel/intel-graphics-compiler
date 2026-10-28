;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-16-plus, regkeys
; RUN: igc_opt -S --opaque-pointers --igc-vect-combine -dce -verify < %s | FileCheck %s

; Edge cases for IGCVectCombine folding a concatenation of two
; JointWaveBroadcasts into one wider JointWaveBroadcast.

; ---------------------------------------------------------------------------
; Folds
; ---------------------------------------------------------------------------

; Non-zero first lane, i32 elements, 2-lane halves: lanes 3..4 ++ 5..6.
; CHECK-LABEL: define spir_kernel void @fold_offset_i32(
; CHECK: %combined_joint_wavebroadcast = call <4 x i32> @llvm.genx.GenISA.JointWaveBroadcast.v4i32.i32.v4i32(i32 %src, <4 x i32> <i32 3, i32 4, i32 5, i32 6>, i32 0)
; CHECK-NOT: shufflevector
; CHECK: add <4 x i32> %combined_joint_wavebroadcast, %other
define spir_kernel void @fold_offset_i32(i32 %src, <4 x i32> %other, ptr addrspace(1) %out) {
entry:
  %low = call <2 x i32> @llvm.genx.GenISA.JointWaveBroadcast.v2i32.i32.v2i32(i32 %src, <2 x i32> <i32 3, i32 4>, i32 0)
  %high = call <2 x i32> @llvm.genx.GenISA.JointWaveBroadcast.v2i32.i32.v2i32(i32 %src, <2 x i32> <i32 5, i32 6>, i32 0)
  %cat = shufflevector <2 x i32> %low, <2 x i32> %high, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %r = add <4 x i32> %cat, %other
  store <4 x i32> %r, ptr addrspace(1) %out
  ret void
}

; The merged range ends exactly at the last lane of the SIMD16 line: 8..11 ++ 12..15.
; CHECK-LABEL: define spir_kernel void @fold_ends_at_last_lane(
; CHECK: call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float %src, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>, i32 0)
; CHECK-NOT: shufflevector
define spir_kernel void @fold_ends_at_last_lane(float %src, <8 x float> %other, ptr addrspace(1) %out) {
entry:
  %low = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 8, i32 9, i32 10, i32 11>, i32 0)
  %high = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 12, i32 13, i32 14, i32 15>, i32 0)
  %cat = shufflevector <4 x float> %low, <4 x float> %high, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %r = fadd <8 x float> %cat, %other
  store <8 x float> %r, ptr addrspace(1) %out
  ret void
}

; The high half is defined first. The merged broadcast must go after the later
; half (%low here), still before the shuffle's users.
; CHECK-LABEL: define spir_kernel void @fold_high_defined_first(
; CHECK: %high = call <4 x float>
; CHECK: %low = call <4 x float>
; CHECK-NEXT: %combined_joint_wavebroadcast = call <8 x float> {{.*}}<8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
; CHECK: fadd <8 x float> %combined_joint_wavebroadcast
define spir_kernel void @fold_high_defined_first(float %src, <8 x float> %other, ptr addrspace(1) %out) {
entry:
  %high = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 4, i32 5, i32 6, i32 7>, i32 0)
  store <4 x float> %high, ptr addrspace(1) %out
  %low = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 0, i32 1, i32 2, i32 3>, i32 0)
  store <4 x float> %low, ptr addrspace(1) %out
  %cat = shufflevector <4 x float> %low, <4 x float> %high, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %r = fadd <8 x float> %cat, %other
  store <8 x float> %r, ptr addrspace(1) %out
  ret void
}

; Halves that have other users stay; only the concatenation is replaced.
; CHECK-LABEL: define spir_kernel void @fold_halves_multi_use(
; CHECK: %low = call <4 x float>
; CHECK: %high = call <4 x float>
; CHECK-NEXT: %combined_joint_wavebroadcast = call <8 x float>
; CHECK: fadd <4 x float> %low, %high
; CHECK: fadd <8 x float> %combined_joint_wavebroadcast, %other
define spir_kernel void @fold_halves_multi_use(float %src, <8 x float> %other, ptr addrspace(1) %out) {
entry:
  %low = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 0, i32 1, i32 2, i32 3>, i32 0)
  %high = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 4, i32 5, i32 6, i32 7>, i32 0)
  %sum = fadd <4 x float> %low, %high
  store <4 x float> %sum, ptr addrspace(1) %out
  %cat = shufflevector <4 x float> %low, <4 x float> %high, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %r = fadd <8 x float> %cat, %other
  store <8 x float> %r, ptr addrspace(1) %out
  ret void
}

; The shuffle lives in a successor block. The merged broadcast is a convergent
; call and stays in the halves' block, right after the later half.
; CHECK-LABEL: define spir_kernel void @fold_shuffle_in_other_block(
; CHECK: entry:
; CHECK-NEXT: %combined_joint_wavebroadcast = call <8 x float>
; CHECK-NEXT: br i1 %c
; CHECK: then:
; CHECK-NOT: shufflevector
; CHECK: fadd <8 x float> %combined_joint_wavebroadcast, %other
define spir_kernel void @fold_shuffle_in_other_block(float %src, <8 x float> %other, i1 %c, ptr addrspace(1) %out) {
entry:
  %low = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 0, i32 1, i32 2, i32 3>, i32 0)
  %high = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 4, i32 5, i32 6, i32 7>, i32 0)
  br i1 %c, label %then, label %exit
then:
  %cat = shufflevector <4 x float> %low, <4 x float> %high, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %r = fadd <8 x float> %cat, %other
  store <8 x float> %r, ptr addrspace(1) %out
  br label %exit
exit:
  ret void
}

; Two levels of concatenation collapse into one 16-lane broadcast.
; CHECK-LABEL: define spir_kernel void @fold_chain(
; CHECK: call <16 x float> @llvm.genx.GenISA.JointWaveBroadcast.v16f32.f32.v16i32(float %src, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>, i32 0)
; CHECK-NOT: shufflevector
; CHECK: ret void
define spir_kernel void @fold_chain(float %src, <16 x float> %other, ptr addrspace(1) %out) {
entry:
  %a = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 0, i32 1, i32 2, i32 3>, i32 0)
  %b = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 4, i32 5, i32 6, i32 7>, i32 0)
  %c = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 8, i32 9, i32 10, i32 11>, i32 0)
  %d = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 12, i32 13, i32 14, i32 15>, i32 0)
  %ab = shufflevector <4 x float> %a, <4 x float> %b, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %cd = shufflevector <4 x float> %c, <4 x float> %d, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %abcd = shufflevector <8 x float> %ab, <8 x float> %cd, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %r = fadd <16 x float> %abcd, %other
  store <16 x float> %r, ptr addrspace(1) %out
  ret void
}

; ---------------------------------------------------------------------------
; No fold
; ---------------------------------------------------------------------------

; Halves in the wrong order: 8..15 ++ 0..7 is not a consecutive range.
; CHECK-LABEL: define spir_kernel void @no_fold_reversed(
; CHECK-NOT: combined_joint_wavebroadcast
; CHECK: shufflevector <8 x float> %high, <8 x float> %low
define spir_kernel void @no_fold_reversed(float %src, <16 x float> %other, ptr addrspace(1) %out) {
entry:
  %high = call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float %src, <8 x i32> <i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>, i32 0)
  %low = call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float %src, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>, i32 0)
  %cat = shufflevector <8 x float> %high, <8 x float> %low, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %r = fadd <16 x float> %cat, %other
  store <16 x float> %r, ptr addrspace(1) %out
  ret void
}

; Different sources.
; CHECK-LABEL: define spir_kernel void @no_fold_different_src(
; CHECK-NOT: combined_joint_wavebroadcast
; CHECK: shufflevector
define spir_kernel void @no_fold_different_src(float %src0, float %src1, <8 x float> %other, ptr addrspace(1) %out) {
entry:
  %low = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src0, <4 x i32> <i32 0, i32 1, i32 2, i32 3>, i32 0)
  %high = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src1, <4 x i32> <i32 4, i32 5, i32 6, i32 7>, i32 0)
  %cat = shufflevector <4 x float> %low, <4 x float> %high, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %r = fadd <8 x float> %cat, %other
  store <8 x float> %r, ptr addrspace(1) %out
  ret void
}

; Different helper lane modes.
; CHECK-LABEL: define spir_kernel void @no_fold_different_helper(
; CHECK-NOT: combined_joint_wavebroadcast
; CHECK: shufflevector
define spir_kernel void @no_fold_different_helper(float %src, <8 x float> %other, ptr addrspace(1) %out) {
entry:
  %low = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 0, i32 1, i32 2, i32 3>, i32 0)
  %high = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 4, i32 5, i32 6, i32 7>, i32 2)
  %cat = shufflevector <4 x float> %low, <4 x float> %high, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %r = fadd <8 x float> %cat, %other
  store <8 x float> %r, ptr addrspace(1) %out
  ret void
}

; Halves in different blocks: a merged broadcast would have to move a
; convergent call across control flow.
; CHECK-LABEL: define spir_kernel void @no_fold_halves_in_different_blocks(
; CHECK-NOT: combined_joint_wavebroadcast
; CHECK: shufflevector
define spir_kernel void @no_fold_halves_in_different_blocks(float %src, <8 x float> %other, i1 %c, ptr addrspace(1) %out) {
entry:
  %low = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 0, i32 1, i32 2, i32 3>, i32 0)
  br i1 %c, label %then, label %exit
then:
  %high = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 4, i32 5, i32 6, i32 7>, i32 0)
  %cat = shufflevector <4 x float> %low, <4 x float> %high, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %r = fadd <8 x float> %cat, %other
  store <8 x float> %r, ptr addrspace(1) %out
  br label %exit
exit:
  ret void
}

; A gap between the halves: 0..3 ++ 5..8.
; CHECK-LABEL: define spir_kernel void @no_fold_gap(
; CHECK-NOT: combined_joint_wavebroadcast
; CHECK: shufflevector
define spir_kernel void @no_fold_gap(float %src, <8 x float> %other, ptr addrspace(1) %out) {
entry:
  %low = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 0, i32 1, i32 2, i32 3>, i32 0)
  %high = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 5, i32 6, i32 7, i32 8>, i32 0)
  %cat = shufflevector <4 x float> %low, <4 x float> %high, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %r = fadd <8 x float> %cat, %other
  store <8 x float> %r, ptr addrspace(1) %out
  ret void
}

; The merged range runs past the SIMD16 line: 6..13 ++ 14..21.
; CHECK-LABEL: define spir_kernel void @no_fold_oob(
; CHECK-NOT: combined_joint_wavebroadcast
; CHECK: shufflevector
define spir_kernel void @no_fold_oob(float %src, <16 x float> %other, ptr addrspace(1) %out) {
entry:
  %low = call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float %src, <8 x i32> <i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13>, i32 0)
  %high = call <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float %src, <8 x i32> <i32 14, i32 15, i32 16, i32 17, i32 18, i32 19, i32 20, i32 21>, i32 0)
  %cat = shufflevector <8 x float> %low, <8 x float> %high, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %r = fadd <16 x float> %cat, %other
  store <16 x float> %r, ptr addrspace(1) %out
  ret void
}

; Not a plain concatenation: a permuted mask, an undef lane, and a partial
; shuffle that keeps only the low half.
; CHECK-LABEL: define spir_kernel void @no_fold_mask(
; CHECK-NOT: combined_joint_wavebroadcast
; CHECK: shufflevector <4 x float> %low, <4 x float> %high, <8 x i32> <i32 1, i32 0
; CHECK: shufflevector <4 x float> %low, <4 x float> %high, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 {{poison|undef}}>
; CHECK: shufflevector <4 x float> %low, <4 x float> %high, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
define spir_kernel void @no_fold_mask(float %src, ptr addrspace(1) %out) {
entry:
  %low = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 0, i32 1, i32 2, i32 3>, i32 0)
  %high = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 4, i32 5, i32 6, i32 7>, i32 0)
  %perm = shufflevector <4 x float> %low, <4 x float> %high, <8 x i32> <i32 1, i32 0, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  store <8 x float> %perm, ptr addrspace(1) %out
  %undef = shufflevector <4 x float> %low, <4 x float> %high, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 poison>
  store <8 x float> %undef, ptr addrspace(1) %out
  %part = shufflevector <4 x float> %low, <4 x float> %high, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  store <4 x float> %part, ptr addrspace(1) %out
  ret void
}

; The same broadcast on both sides.
; CHECK-LABEL: define spir_kernel void @no_fold_same_half_twice(
; CHECK-NOT: combined_joint_wavebroadcast
; CHECK: shufflevector <4 x float> %low, <4 x float> %low
define spir_kernel void @no_fold_same_half_twice(float %src, ptr addrspace(1) %out) {
entry:
  %low = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 0, i32 1, i32 2, i32 3>, i32 0)
  %cat = shufflevector <4 x float> %low, <4 x float> %low, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  store <8 x float> %cat, ptr addrspace(1) %out
  ret void
}

; The pass only runs for SIMD16: neither SIMD32 nor a missing
; requiredSubGroupSize folds.
; CHECK-LABEL: define spir_kernel void @no_fold_simd32(
; CHECK-NOT: combined_joint_wavebroadcast
; CHECK: shufflevector
define spir_kernel void @no_fold_simd32(float %src, ptr addrspace(1) %out) {
entry:
  %low = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 0, i32 1, i32 2, i32 3>, i32 0)
  %high = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 4, i32 5, i32 6, i32 7>, i32 0)
  %cat = shufflevector <4 x float> %low, <4 x float> %high, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  store <8 x float> %cat, ptr addrspace(1) %out
  ret void
}

; CHECK-LABEL: define spir_kernel void @no_fold_no_subgroup_size(
; CHECK-NOT: combined_joint_wavebroadcast
; CHECK: shufflevector
define spir_kernel void @no_fold_no_subgroup_size(float %src, ptr addrspace(1) %out) {
entry:
  %low = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 0, i32 1, i32 2, i32 3>, i32 0)
  %high = call <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float %src, <4 x i32> <i32 4, i32 5, i32 6, i32 7>, i32 0)
  %cat = shufflevector <4 x float> %low, <4 x float> %high, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  store <8 x float> %cat, ptr addrspace(1) %out
  ret void
}

declare <2 x i32> @llvm.genx.GenISA.JointWaveBroadcast.v2i32.i32.v2i32(i32, <2 x i32>, i32)
declare <4 x float> @llvm.genx.GenISA.JointWaveBroadcast.v4f32.f32.v4i32(float, <4 x i32>, i32)
declare <8 x float> @llvm.genx.GenISA.JointWaveBroadcast.v8f32.f32.v8i32(float, <8 x i32>, i32)

!igc.functions = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9, !10, !11, !12, !13, !14, !15}
!0 = !{ptr @fold_offset_i32, !30}
!1 = !{ptr @fold_ends_at_last_lane, !30}
!2 = !{ptr @fold_high_defined_first, !30}
!3 = !{ptr @fold_halves_multi_use, !30}
!4 = !{ptr @fold_shuffle_in_other_block, !30}
!5 = !{ptr @fold_chain, !30}
!6 = !{ptr @no_fold_reversed, !30}
!7 = !{ptr @no_fold_different_src, !30}
!8 = !{ptr @no_fold_different_helper, !30}
!9 = !{ptr @no_fold_halves_in_different_blocks, !30}
!10 = !{ptr @no_fold_gap, !30}
!11 = !{ptr @no_fold_oob, !30}
!12 = !{ptr @no_fold_mask, !30}
!13 = !{ptr @no_fold_same_half_twice, !30}
!14 = !{ptr @no_fold_simd32, !30}
!15 = !{ptr @no_fold_no_subgroup_size, !30}
!30 = !{!31}
!31 = !{!"function_type", i32 0}

!40 = !{!"requiredSubGroupSize", i32 16}
!41 = !{!"requiredSubGroupSize", i32 32}
!50 = !{!"FuncMDMap[0]", ptr @fold_offset_i32}
!51 = !{!"FuncMDValue[0]", !40}
!52 = !{!"FuncMDMap[1]", ptr @fold_ends_at_last_lane}
!53 = !{!"FuncMDValue[1]", !40}
!54 = !{!"FuncMDMap[2]", ptr @fold_high_defined_first}
!55 = !{!"FuncMDValue[2]", !40}
!56 = !{!"FuncMDMap[3]", ptr @fold_halves_multi_use}
!57 = !{!"FuncMDValue[3]", !40}
!58 = !{!"FuncMDMap[4]", ptr @fold_shuffle_in_other_block}
!59 = !{!"FuncMDValue[4]", !40}
!60 = !{!"FuncMDMap[5]", ptr @fold_chain}
!61 = !{!"FuncMDValue[5]", !40}
!62 = !{!"FuncMDMap[6]", ptr @no_fold_reversed}
!63 = !{!"FuncMDValue[6]", !40}
!64 = !{!"FuncMDMap[7]", ptr @no_fold_different_src}
!65 = !{!"FuncMDValue[7]", !40}
!66 = !{!"FuncMDMap[8]", ptr @no_fold_different_helper}
!67 = !{!"FuncMDValue[8]", !40}
!68 = !{!"FuncMDMap[9]", ptr @no_fold_halves_in_different_blocks}
!69 = !{!"FuncMDValue[9]", !40}
!70 = !{!"FuncMDMap[10]", ptr @no_fold_gap}
!71 = !{!"FuncMDValue[10]", !40}
!72 = !{!"FuncMDMap[11]", ptr @no_fold_oob}
!73 = !{!"FuncMDValue[11]", !40}
!74 = !{!"FuncMDMap[12]", ptr @no_fold_mask}
!75 = !{!"FuncMDValue[12]", !40}
!76 = !{!"FuncMDMap[13]", ptr @no_fold_same_half_twice}
!77 = !{!"FuncMDValue[13]", !40}
!78 = !{!"FuncMDMap[14]", ptr @no_fold_simd32}
!79 = !{!"FuncMDValue[14]", !41}
!80 = !{!"FuncMD", !50, !51, !52, !53, !54, !55, !56, !57, !58, !59, !60, !61, !62, !63, !64, !65, !66, !67, !68, !69, !70, !71, !72, !73, !74, !75, !76, !77, !78, !79}
!81 = !{!"ModuleMD", !80}
!IGCMetadata = !{!81}
