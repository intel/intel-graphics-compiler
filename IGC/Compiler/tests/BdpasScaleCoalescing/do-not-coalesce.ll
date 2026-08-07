;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-16-plus
; RUN: igc_opt --opaque-pointers --regkey EnableFP4Dpas=1 --regkey EnableBdpasScaleCoalescing=1 --igc-bdpas-scale-coalescing -platformCri -S < %s | FileCheck %s

; Reconstruction is transactional. Unsupported or incomplete groups keep their
; original scale operands and intrinsic. Except for the uniform-scale case, all
; scale operands come from per-lane loads so each case reaches its intended
; rejection after its function-level gates pass.

; CHECK-LABEL: define spir_kernel void @incomplete_rectangle(
; CHECK-NOT: bdpas.scale
; CHECK-COUNT-3: call <8 x float> @llvm.genx.GenISA.sub.group.bdpas{{.*}}i32 13, i32 13
; CHECK: ret void
define spir_kernel void @incomplete_rectangle(<8 x i16> %a, <8 x i32> %b,
                                               ptr addrspace(1) %a0.ptr, ptr addrspace(1) %a1.ptr,
                                               ptr addrspace(1) %b0.ptr, ptr addrspace(1) %b1.ptr) !intel_reqd_sub_group_size !7 {
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i64
  %a0.lane = getelementptr i16, ptr addrspace(1) %a0.ptr, i64 %lane
  %a1.lane = getelementptr i16, ptr addrspace(1) %a1.ptr, i64 %lane
  %b0.lane = getelementptr i16, ptr addrspace(1) %b0.ptr, i64 %lane
  %b1.lane = getelementptr i16, ptr addrspace(1) %b1.ptr, i64 %lane
  %a0.raw = load i16, ptr addrspace(1) %a0.lane, align 2
  %a1.raw = load i16, ptr addrspace(1) %a1.lane, align 2
  %b0.raw = load i16, ptr addrspace(1) %b0.lane, align 2
  %b1.raw = load i16, ptr addrspace(1) %b1.lane, align 2
  %a0 = bitcast i16 %a0.raw to <2 x i8>
  %a1 = bitcast i16 %a1.raw to <2 x i8>
  %b0 = bitcast i16 %b0.raw to <2 x i8>
  %b1 = bitcast i16 %b1.raw to <2 x i8>
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a0, <2 x i8> %b0, i32 13, i32 13
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a0, <2 x i8> %b1, i32 13, i32 13
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a1, <2 x i8> %b0, i32 13, i32 13
  )
  ret void
}

; CHECK-LABEL: define spir_kernel void @simd32(
; CHECK-NOT: bdpas.scale
; CHECK-COUNT-4: call <8 x float> @llvm.genx.GenISA.sub.group.bdpas{{.*}}i32 13, i32 13
; CHECK: ret void
define spir_kernel void @simd32(<8 x i16> %a, <8 x i32> %b,
                                ptr addrspace(1) %a0.ptr, ptr addrspace(1) %a1.ptr,
                                ptr addrspace(1) %b0.ptr, ptr addrspace(1) %b1.ptr) !intel_reqd_sub_group_size !8 {
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i64
  %a0.lane = getelementptr i16, ptr addrspace(1) %a0.ptr, i64 %lane
  %a1.lane = getelementptr i16, ptr addrspace(1) %a1.ptr, i64 %lane
  %b0.lane = getelementptr i16, ptr addrspace(1) %b0.ptr, i64 %lane
  %b1.lane = getelementptr i16, ptr addrspace(1) %b1.ptr, i64 %lane
  %a0.raw = load i16, ptr addrspace(1) %a0.lane, align 2
  %a1.raw = load i16, ptr addrspace(1) %a1.lane, align 2
  %b0.raw = load i16, ptr addrspace(1) %b0.lane, align 2
  %b1.raw = load i16, ptr addrspace(1) %b1.lane, align 2
  %a0 = bitcast i16 %a0.raw to <2 x i8>
  %a1 = bitcast i16 %a1.raw to <2 x i8>
  %b0 = bitcast i16 %b0.raw to <2 x i8>
  %b1 = bitcast i16 %b1.raw to <2 x i8>
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a0, <2 x i8> %b0, i32 13, i32 13
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a0, <2 x i8> %b1, i32 13, i32 13
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a1, <2 x i8> %b0, i32 13, i32 13
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a1, <2 x i8> %b1, i32 13, i32 13
  )
  ret void
}

; CHECK-LABEL: define spir_kernel void @attached_only_simd16(
; CHECK-NOT: bdpas.scale
; CHECK-COUNT-4: call <8 x float> @llvm.genx.GenISA.sub.group.bdpas{{.*}}i32 13, i32 13
; CHECK: ret void
define spir_kernel void @attached_only_simd16(<8 x i16> %a, <8 x i32> %b,
                                              ptr addrspace(1) %a0.ptr, ptr addrspace(1) %a1.ptr,
                                              ptr addrspace(1) %b0.ptr, ptr addrspace(1) %b1.ptr) !intel_reqd_sub_group_size !7 {
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i64
  %a0.lane = getelementptr i16, ptr addrspace(1) %a0.ptr, i64 %lane
  %a1.lane = getelementptr i16, ptr addrspace(1) %a1.ptr, i64 %lane
  %b0.lane = getelementptr i16, ptr addrspace(1) %b0.ptr, i64 %lane
  %b1.lane = getelementptr i16, ptr addrspace(1) %b1.ptr, i64 %lane
  %a0.raw = load i16, ptr addrspace(1) %a0.lane, align 2
  %a1.raw = load i16, ptr addrspace(1) %a1.lane, align 2
  %b0.raw = load i16, ptr addrspace(1) %b0.lane, align 2
  %b1.raw = load i16, ptr addrspace(1) %b1.lane, align 2
  %a0 = bitcast i16 %a0.raw to <2 x i8>
  %a1 = bitcast i16 %a1.raw to <2 x i8>
  %b0 = bitcast i16 %b0.raw to <2 x i8>
  %b1 = bitcast i16 %b1.raw to <2 x i8>
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a0, <2 x i8> %b0, i32 13, i32 13
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a0, <2 x i8> %b1, i32 13, i32 13
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a1, <2 x i8> %b0, i32 13, i32 13
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a1, <2 x i8> %b1, i32 13, i32 13
  )
  ret void
}

; CHECK-LABEL: define spir_kernel void @bf8(
; CHECK-NOT: bdpas.scale
; CHECK-COUNT-4: call <8 x float> @llvm.genx.GenISA.sub.group.bdpas{{.*}}i32 7, i32 7
; CHECK: ret void
define spir_kernel void @bf8(<8 x i16> %a, <8 x i32> %b,
                             ptr addrspace(1) %a0.ptr, ptr addrspace(1) %a1.ptr,
                             ptr addrspace(1) %b0.ptr, ptr addrspace(1) %b1.ptr) !intel_reqd_sub_group_size !7 {
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i64
  %a0.lane = getelementptr i16, ptr addrspace(1) %a0.ptr, i64 %lane
  %a1.lane = getelementptr i16, ptr addrspace(1) %a1.ptr, i64 %lane
  %b0.lane = getelementptr i16, ptr addrspace(1) %b0.ptr, i64 %lane
  %b1.lane = getelementptr i16, ptr addrspace(1) %b1.ptr, i64 %lane
  %a0.raw = load i16, ptr addrspace(1) %a0.lane, align 2
  %a1.raw = load i16, ptr addrspace(1) %a1.lane, align 2
  %b0.raw = load i16, ptr addrspace(1) %b0.lane, align 2
  %b1.raw = load i16, ptr addrspace(1) %b1.lane, align 2
  %a0 = bitcast i16 %a0.raw to <2 x i8>
  %a1 = bitcast i16 %a1.raw to <2 x i8>
  %b0 = bitcast i16 %b0.raw to <2 x i8>
  %b1 = bitcast i16 %b1.raw to <2 x i8>
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a0, <2 x i8> %b0, i32 7, i32 7
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a0, <2 x i8> %b1, i32 7, i32 7
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a1, <2 x i8> %b0, i32 7, i32 7
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a1, <2 x i8> %b1, i32 7, i32 7
  )
  ret void
}

; CHECK-LABEL: define spir_kernel void @already_packed(
; CHECK-NOT: bdpas.scale
; CHECK-COUNT-4: call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed{{.*}}i32 0, i32 0)
; CHECK: ret void
define spir_kernel void @already_packed(<8 x i16> %a, <8 x i32> %b,
                                        ptr addrspace(1) %a0.ptr, ptr addrspace(1) %a1.ptr,
                                        ptr addrspace(1) %b0.ptr, ptr addrspace(1) %b1.ptr) !intel_reqd_sub_group_size !7 {
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i64
  %a0.lane = getelementptr <4 x i8>, ptr addrspace(1) %a0.ptr, i64 %lane
  %a1.lane = getelementptr <4 x i8>, ptr addrspace(1) %a1.ptr, i64 %lane
  %b0.lane = getelementptr <4 x i8>, ptr addrspace(1) %b0.ptr, i64 %lane
  %b1.lane = getelementptr <4 x i8>, ptr addrspace(1) %b1.ptr, i64 %lane
  %a0 = load <4 x i8>, ptr addrspace(1) %a0.lane, align 4
  %a1 = load <4 x i8>, ptr addrspace(1) %a1.lane, align 4
  %b0 = load <4 x i8>, ptr addrspace(1) %b0.lane, align 4
  %b1 = load <4 x i8>, ptr addrspace(1) %b1.lane, align 4
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <4 x i8> %a0, <4 x i8> %b0, i32 13, i32 13,
      i32 0, i32 0)
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <4 x i8> %a0, <4 x i8> %b1, i32 13, i32 13,
      i32 0, i32 0)
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <4 x i8> %a1, <4 x i8> %b0, i32 13, i32 13,
      i32 0, i32 0)
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <4 x i8> %a1, <4 x i8> %b1, i32 13, i32 13,
      i32 0, i32 0)
  ret void
}

; CHECK-LABEL: define spir_kernel void @late_definition(
; CHECK-NOT: bdpas.scale
; CHECK-COUNT-4: call <8 x float> @llvm.genx.GenISA.sub.group.bdpas{{.*}}i32 13, i32 13
; CHECK: ret void
define spir_kernel void @late_definition(<8 x i16> %a, <8 x i32> %b,
                                         ptr addrspace(1) %a0.ptr, ptr addrspace(1) %a1.ptr,
                                         ptr addrspace(1) %b0.ptr, ptr addrspace(1) %b1.ptr) !intel_reqd_sub_group_size !7 {
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i64
  %a0.lane = getelementptr i16, ptr addrspace(1) %a0.ptr, i64 %lane
  %b0.lane = getelementptr i16, ptr addrspace(1) %b0.ptr, i64 %lane
  %b1.lane = getelementptr i16, ptr addrspace(1) %b1.ptr, i64 %lane
  %a0.raw = load i16, ptr addrspace(1) %a0.lane, align 2
  %b0.raw = load i16, ptr addrspace(1) %b0.lane, align 2
  %b1.raw = load i16, ptr addrspace(1) %b1.lane, align 2
  %a0 = bitcast i16 %a0.raw to <2 x i8>
  %b0 = bitcast i16 %b0.raw to <2 x i8>
  %b1 = bitcast i16 %b1.raw to <2 x i8>
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a0, <2 x i8> %b0, i32 13, i32 13
  )
  %a1.lane = getelementptr i16, ptr addrspace(1) %a1.ptr, i64 %lane
  %a1.raw = load i16, ptr addrspace(1) %a1.lane, align 2
  %a1 = bitcast i16 %a1.raw to <2 x i8>
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a0, <2 x i8> %b1, i32 13, i32 13
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a1, <2 x i8> %b0, i32 13, i32 13
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a1, <2 x i8> %b1, i32 13, i32 13
  )
  ret void
}

; CHECK-LABEL: define spir_kernel void @conflicting_simd(
; CHECK-NOT: bdpas.scale
; CHECK-COUNT-4: call <8 x float> @llvm.genx.GenISA.sub.group.bdpas{{.*}}i32 13, i32 13
; CHECK: ret void
define spir_kernel void @conflicting_simd(<8 x i16> %a, <8 x i32> %b,
                                          ptr addrspace(1) %a0.ptr, ptr addrspace(1) %a1.ptr,
                                          ptr addrspace(1) %b0.ptr, ptr addrspace(1) %b1.ptr) !intel_reqd_sub_group_size !7 {
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i64
  %a0.lane = getelementptr i16, ptr addrspace(1) %a0.ptr, i64 %lane
  %a1.lane = getelementptr i16, ptr addrspace(1) %a1.ptr, i64 %lane
  %b0.lane = getelementptr i16, ptr addrspace(1) %b0.ptr, i64 %lane
  %b1.lane = getelementptr i16, ptr addrspace(1) %b1.ptr, i64 %lane
  %a0.raw = load i16, ptr addrspace(1) %a0.lane, align 2
  %a1.raw = load i16, ptr addrspace(1) %a1.lane, align 2
  %b0.raw = load i16, ptr addrspace(1) %b0.lane, align 2
  %b1.raw = load i16, ptr addrspace(1) %b1.lane, align 2
  %a0 = bitcast i16 %a0.raw to <2 x i8>
  %a1 = bitcast i16 %a1.raw to <2 x i8>
  %b0 = bitcast i16 %b0.raw to <2 x i8>
  %b1 = bitcast i16 %b1.raw to <2 x i8>
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a0, <2 x i8> %b0, i32 13, i32 13
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a0, <2 x i8> %b1, i32 13, i32 13
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a1, <2 x i8> %b0, i32 13, i32 13
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a1, <2 x i8> %b1, i32 13, i32 13
  )
  ret void
}

; CHECK-LABEL: define internal spir_func void @simd16_callee(
; CHECK-NOT: bdpas.scale
; CHECK-COUNT-4: call <8 x float> @llvm.genx.GenISA.sub.group.bdpas{{.*}}i32 13, i32 13
; CHECK: ret void
define internal spir_func void @simd16_callee(<8 x i16> %a, <8 x i32> %b,
                                              ptr addrspace(1) %a0.ptr, ptr addrspace(1) %a1.ptr,
                                              ptr addrspace(1) %b0.ptr, ptr addrspace(1) %b1.ptr) !intel_reqd_sub_group_size !7 {
  %lane16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane = zext i16 %lane16 to i64
  %a0.lane = getelementptr i16, ptr addrspace(1) %a0.ptr, i64 %lane
  %a1.lane = getelementptr i16, ptr addrspace(1) %a1.ptr, i64 %lane
  %b0.lane = getelementptr i16, ptr addrspace(1) %b0.ptr, i64 %lane
  %b1.lane = getelementptr i16, ptr addrspace(1) %b1.ptr, i64 %lane
  %a0.raw = load i16, ptr addrspace(1) %a0.lane, align 2
  %a1.raw = load i16, ptr addrspace(1) %a1.lane, align 2
  %b0.raw = load i16, ptr addrspace(1) %b0.lane, align 2
  %b1.raw = load i16, ptr addrspace(1) %b1.lane, align 2
  %a0 = bitcast i16 %a0.raw to <2 x i8>
  %a1 = bitcast i16 %a1.raw to <2 x i8>
  %b0 = bitcast i16 %b0.raw to <2 x i8>
  %b1 = bitcast i16 %b1.raw to <2 x i8>
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a0, <2 x i8> %b0, i32 13, i32 13
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a0, <2 x i8> %b1, i32 13, i32 13
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a1, <2 x i8> %b0, i32 13, i32 13
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> %a1, <2 x i8> %b1, i32 13, i32 13
  )
  ret void
}

; CHECK-LABEL: define spir_kernel void @uniform_scales(
; CHECK-NOT: bdpas.scale
; CHECK-COUNT-4: call <8 x float> @llvm.genx.GenISA.sub.group.bdpas{{.*}}i32 13, i32 13
; CHECK: ret void
define spir_kernel void @uniform_scales(<8 x i16> %a, <8 x i32> %b) !intel_reqd_sub_group_size !7 {
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> <i8 1, i8 2>, <2 x i8> <i8 5, i8 6>, i32 13, i32 13
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> <i8 1, i8 2>, <2 x i8> <i8 7, i8 8>, i32 13, i32 13
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> <i8 3, i8 4>, <2 x i8> <i8 5, i8 6>, i32 13, i32 13
  )
  call <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
      <8 x float> zeroinitializer, <8 x i16> %a, <8 x i32> %b,
      <2 x i8> <i8 3, i8 4>, <2 x i8> <i8 7, i8 8>, i32 13, i32 13
  )
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v2i8.v2i8(
    <8 x float>, <8 x i16>, <8 x i32>,
    <2 x i8>, <2 x i8>, i32, i32
)
declare <8 x float> @llvm.genx.GenISA.sub.group.bdpas.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
    <8 x float>, <8 x i16>, <8 x i32>,
    <4 x i8>, <4 x i8>, i32, i32
)
declare <8 x float> @llvm.genx.GenISA.sub.group.bdpas.packed.v8f32.v8f32.v8i16.v8i32.v4i8.v4i8(
    <8 x float>, <8 x i16>, <8 x i32>,
    <4 x i8>, <4 x i8>, i32, i32,
    i32, i32)
declare i16 @llvm.genx.GenISA.simdLaneId()

!igc.functions = !{!0, !3, !4, !5, !6, !9, !16, !17}
!0 = !{ptr @incomplete_rectangle, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @simd32, !1}
!4 = !{ptr @bf8, !1}
!5 = !{ptr @already_packed, !1}
!6 = !{ptr @late_definition, !1}
!7 = !{i32 16}
!8 = !{i32 32}
!9 = !{ptr @conflicting_simd, !10}
!10 = !{!2}
!11 = !{!"requiredSubGroupSize", i32 16}
!12 = !{!"requiredSubGroupSize", i32 32}
!13 = !{!"FuncMDMap[0]", ptr @incomplete_rectangle}
!14 = !{!"FuncMDValue[0]", !11}
!15 = !{!"FuncMDMap[1]", ptr @simd32}
!18 = !{!"FuncMDValue[1]", !12}
!19 = !{!"FuncMDMap[2]", ptr @bf8}
!20 = !{!"FuncMDValue[2]", !11}
!21 = !{!"FuncMDMap[3]", ptr @already_packed}
!22 = !{!"FuncMDValue[3]", !11}
!23 = !{!"FuncMDMap[4]", ptr @late_definition}
!24 = !{!"FuncMDValue[4]", !11}
!25 = !{!"FuncMDMap[5]", ptr @conflicting_simd}
!26 = !{!"FuncMDValue[5]", !12}
!27 = !{!"FuncMDMap[6]", ptr @uniform_scales}
!28 = !{!"FuncMDValue[6]", !11}
!29 = !{!"FuncMD", !13, !14, !15, !18, !19, !20, !21, !22, !23, !24, !25, !26, !27, !28}
!30 = !{!"ModuleMD", !29}
!16 = !{ptr @uniform_scales, !1}
!17 = !{ptr @attached_only_simd16, !1}
; attached_only_simd16 deliberately has no FuncMD entry: attached LLVM metadata
; is not the pass's dispatch-width authority. A FuncMD entry would also make
; simd16_callee an entry function, so the callee is deliberately omitted.
!IGCMetadata = !{!30}
