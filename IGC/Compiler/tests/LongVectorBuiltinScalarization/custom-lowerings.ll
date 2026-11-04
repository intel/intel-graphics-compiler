;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-long-vector-builtin-scalarization -S < %s | FileCheck %s

; Builtins whose vector form isn't the scalar form applied to each element get
; a dedicated lowering, shown here at widths up to 16.

target triple = "spir64-unknown-unknown"

%structtype = type { <5 x i32>, <5 x i32> }

; Same expression as BiF's dot for widths 2-4:
; fma(x0, y0, fma(x1, y1, fma(x2, y2, fma(x3, y3, x4 * y4)))).
; CHECK-LABEL: define spir_func float @dot(
; CHECK:         [[X4:%.*]] = extractelement <5 x float> %x, i32 4
; CHECK:         [[Y4:%.*]] = extractelement <5 x float> %y, i32 4
; CHECK:         [[M:%.*]] = fmul float [[X4]], [[Y4]]
; CHECK:         [[X3:%.*]] = extractelement <5 x float> %x, i32 3
; CHECK:         [[Y3:%.*]] = extractelement <5 x float> %y, i32 3
; CHECK:         [[A3:%.*]] = call spir_func float @_Z15__spirv_ocl_fmafff(float [[X3]], float [[Y3]], float [[M]])
; CHECK-COUNT-2: call spir_func float @_Z15__spirv_ocl_fmafff(
; CHECK:         [[X0:%.*]] = extractelement <5 x float> %x, i32 0
; CHECK:         [[Y0:%.*]] = extractelement <5 x float> %y, i32 0
; CHECK:         [[A0:%.*]] = call spir_func float @_Z15__spirv_ocl_fmafff(float [[X0]], float [[Y0]], float
; CHECK-NOT:     __spirv_Dot
; CHECK:         ret float [[A0]]
define spir_func float @dot(<5 x float> %x, <5 x float> %y) {
  %r = call spir_func float @_Z11__spirv_DotDv5_fS_(<5 x float> %x, <5 x float> %y)
  ret float %r
}

; Like BiF, the lanes are reduced bitwise and tested once.
; CHECK-LABEL: define spir_func i1 @any(
; CHECK:         [[E0:%.*]] = extractelement <5 x i8> %x, i32 0
; CHECK:         [[E1:%.*]] = extractelement <5 x i8> %x, i32 1
; CHECK:         [[A1:%.*]] = or i8 [[E0]], [[E1]]
; CHECK-COUNT-3: or i8
; CHECK:         [[R:%.*]] = icmp ne i8 {{%.*}}, 0
; CHECK:         ret i1 [[R]]
; CHECK-NOT:     call
define spir_func i1 @any(<5 x i8> %x) {
  %r = call spir_func i1 @_Z11__spirv_AnyDv5_c(<5 x i8> %x)
  ret i1 %r
}

; CHECK-LABEL: define spir_func i1 @all(
; CHECK-COUNT-4: and i8
; CHECK:         [[R:%.*]] = icmp ne i8 {{%.*}}, 0
; CHECK:         ret i1 [[R]]
; CHECK-NOT:     call
define spir_func i1 @all(<5 x i8> %x) {
  %r = call spir_func i1 @_Z11__spirv_AllDv5_c(<5 x i8> %x)
  ret i1 %r
}

; CHECK-LABEL: define spir_func i1 @all1(
; CHECK:         [[E0:%.*]] = extractelement <1 x i8> %x, i32 0
; CHECK:         [[R:%.*]] = icmp ne i8 [[E0]], 0
; CHECK:         ret i1 [[R]]
define spir_func i1 @all1(<1 x i8> %x) {
  %r = call spir_func i1 @_Z11__spirv_AllDv1_c(<1 x i8> %x)
  ret i1 %r
}

; OpenCL.std select on vectors tests the MSB of c.
; CHECK-LABEL: define spir_func <5 x float> @select(
; CHECK:         [[MSB:%.*]] = icmp slt <5 x i32> %c, zeroinitializer
; CHECK:         [[R:%.*]] = select <5 x i1> [[MSB]], <5 x float> %b, <5 x float> %a
; CHECK:         ret <5 x float> [[R]]
define spir_func <5 x float> @select(<5 x float> %a, <5 x float> %b, <5 x i32> %c) {
  %r = call spir_func <5 x float> @_Z18__spirv_ocl_selectDv5_fS_Dv5_i(<5 x float> %a, <5 x float> %b, <5 x i32> %c)
  ret <5 x float> %r
}

; shuffle considers the low ilogb(2 * 5 - 1) = 3 mask bits; indices 5-7 give 0.
; CHECK-LABEL: define spir_func <3 x float> @shuffle(
; CHECK:         [[M0:%.*]] = extractelement <3 x i32> %m, i32 0
; CHECK:         [[I0:%.*]] = and i32 [[M0]], 7
; CHECK:         [[S0:%.*]] = urem i32 [[I0]], 5
; CHECK:         [[X0:%.*]] = extractelement <5 x float> %x, i32 [[S0]]
; CHECK:         [[IN:%.*]] = icmp ult i32 [[I0]], 5
; CHECK:         [[R0:%.*]] = select i1 [[IN]], float [[X0]], float 0.000000e+00
; CHECK:         insertelement <3 x float> poison, float [[R0]], i32 0
; CHECK-COUNT-2: and i32 {{%.*}}, 7
; CHECK-NOT:     call
define spir_func <3 x float> @shuffle(<5 x float> %x, <3 x i32> %m) {
  %r = call spir_func <3 x float> @_Z19__spirv_ocl_shuffleDv5_fDv3_j(<5 x float> %x, <3 x i32> %m)
  ret <3 x float> %r
}

; shuffle2 considers one more bit: indices 0-4 are x, 5-9 are y, 10-15 give 0.
; CHECK-LABEL: define spir_func <3 x float> @shuffle2(
; CHECK:         [[M0:%.*]] = extractelement <3 x i32> %m, i32 0
; CHECK:         [[I0:%.*]] = and i32 [[M0]], 15
; CHECK:         [[S0:%.*]] = urem i32 [[I0]], 5
; CHECK:         [[X0:%.*]] = extractelement <5 x float> %x, i32 [[S0]]
; CHECK:         [[Y0:%.*]] = extractelement <5 x float> %y, i32 [[S0]]
; CHECK:         [[INX:%.*]] = icmp ult i32 [[I0]], 5
; CHECK:         [[XY0:%.*]] = select i1 [[INX]], float [[X0]], float [[Y0]]
; CHECK:         [[IN:%.*]] = icmp ult i32 [[I0]], 10
; CHECK:         [[R0:%.*]] = select i1 [[IN]], float [[XY0]], float 0.000000e+00
; CHECK:         insertelement <3 x float> poison, float [[R0]], i32 0
; CHECK-NOT:     call
define spir_func <3 x float> @shuffle2(<5 x float> %x, <5 x float> %y, <3 x i32> %m) {
  %r = call spir_func <3 x float> @_Z20__spirv_ocl_shuffle2Dv5_fS_Dv3_j(<5 x float> %x, <5 x float> %y, <3 x i32> %m)
  ret <3 x float> %r
}

; With a power-of-2 source every index the mask bits can form is in range.
; CHECK-LABEL: define spir_func <5 x float> @shuffle2_pow2_source(
; CHECK:         [[M0:%.*]] = extractelement <5 x i32> %m, i32 0
; CHECK:         [[I0:%.*]] = and i32 [[M0]], 7
; CHECK:         [[S0:%.*]] = urem i32 [[I0]], 4
; CHECK:         [[X0:%.*]] = extractelement <4 x float> %x, i32 [[S0]]
; CHECK:         [[Y0:%.*]] = extractelement <4 x float> %y, i32 [[S0]]
; CHECK:         [[INX:%.*]] = icmp ult i32 [[I0]], 4
; CHECK:         [[R0:%.*]] = select i1 [[INX]], float [[X0]], float [[Y0]]
; CHECK:         insertelement <5 x float> poison, float [[R0]], i32 0
; CHECK-NOT:     0.000000e+00
; CHECK-NOT:     call
define spir_func <5 x float> @shuffle2_pow2_source(<4 x float> %x, <4 x float> %y, <5 x i32> %m) {
  %r = call spir_func <5 x float> @_Z20__spirv_ocl_shuffle2Dv4_fS_Dv5_j(<4 x float> %x, <4 x float> %y, <5 x i32> %m)
  ret <5 x float> %r
}

; CHECK-LABEL: define spir_func <4 x float> @shuffle_wide_source(
; CHECK:         [[M0:%.*]] = extractelement <4 x i32> %m, i32 0
; CHECK:         [[I0:%.*]] = and i32 [[M0]], 511
; CHECK:         [[S0:%.*]] = urem i32 [[I0]], 300
; CHECK:         [[E0:%.*]] = extractelement <300 x float> %x, i32 [[S0]]
; CHECK:         [[IN:%.*]] = icmp ult i32 [[I0]], 300
; CHECK:         [[R0:%.*]] = select i1 [[IN]], float [[E0]], float 0.000000e+00
; CHECK:         insertelement <4 x float> poison, float [[R0]], i32 0
define spir_func <4 x float> @shuffle_wide_source(<300 x float> %x, <4 x i32> %m) {
  %r = call spir_func <4 x float> @_Z19__spirv_ocl_shuffleDv300_fDv4_j(<300 x float> %x, <4 x i32> %m)
  ret <4 x float> %r
}

; CHECK-LABEL: define spir_func <5 x float> @vloadn(
; CHECK:         [[IDX:%.*]] = mul i64 %o, 5
; CHECK:         [[P:%.*]] = getelementptr inbounds float, ptr addrspace(1) %p, i64 [[IDX]]
; CHECK:         [[V:%.*]] = load <5 x float>, ptr addrspace(1) [[P]], align 4
; CHECK:         ret <5 x float> [[V]]
define spir_func <5 x float> @vloadn(i64 %o, ptr addrspace(1) %p) {
  %r = call spir_func <5 x float> @_Z26__spirv_ocl_vloadn_Rfloat5mPU3AS1Kfi(i64 %o, ptr addrspace(1) %p, i32 5)
  ret <5 x float> %r
}

; CHECK-LABEL: define spir_func <5 x bfloat> @vloadn_bfloat(
; CHECK:         [[IDX:%.*]] = mul i64 %o, 5
; CHECK:         [[P:%.*]] = getelementptr inbounds bfloat, ptr addrspace(1) %p, i64 [[IDX]]
; CHECK:         [[V:%.*]] = load <5 x bfloat>, ptr addrspace(1) [[P]], align 2
; CHECK:         ret <5 x bfloat> [[V]]
define spir_func <5 x bfloat> @vloadn_bfloat(i64 %o, ptr addrspace(1) %p) {
  %r = call spir_func <5 x bfloat> @_Z26__spirv_ocl_vloadn_RDF16b5mPU3AS1KDF16bi(i64 %o, ptr addrspace(1) %p, i32 5)
  ret <5 x bfloat> %r
}

; CHECK-LABEL: define spir_func void @vstoren(
; CHECK:         [[IDX:%.*]] = mul i64 %o, 5
; CHECK:         [[P:%.*]] = getelementptr inbounds float, ptr addrspace(1) %p, i64 [[IDX]]
; CHECK:         store <5 x float> %x, ptr addrspace(1) [[P]], align 4
define spir_func void @vstoren(<5 x float> %x, i64 %o, ptr addrspace(1) %p) {
  call spir_func void @_Z19__spirv_ocl_vstorenDv5_fmPU3AS1f(<5 x float> %x, i64 %o, ptr addrspace(1) %p)
  ret void
}

; Converting half to float is exact, so vload_halfn is a plain load.
; CHECK-LABEL: define spir_func <5 x float> @vload_halfn(
; CHECK:         [[IDX:%.*]] = mul i64 %o, 5
; CHECK:         [[P:%.*]] = getelementptr inbounds half, ptr addrspace(1) %p, i64 [[IDX]]
; CHECK:         [[H:%.*]] = load <5 x half>, ptr addrspace(1) [[P]], align 2
; CHECK:         [[F:%.*]] = fpext <5 x half> [[H]] to <5 x float>
; CHECK:         ret <5 x float> [[F]]
define spir_func <5 x float> @vload_halfn(i64 %o, ptr addrspace(1) %p) {
  %r = call spir_func <5 x float> @_Z31__spirv_ocl_vload_halfn_Rfloat5mPU3AS1KDhi(i64 %o, ptr addrspace(1) %p, i32 5)
  ret <5 x float> %r
}

; The aligned variants address p + offset * 8 for 5 elements, like n = 3 uses 4.
; CHECK-LABEL: define spir_func <5 x float> @vloada_halfn(
; CHECK:         [[IDX:%.*]] = mul i64 %o, 8
; CHECK:         [[P:%.*]] = getelementptr inbounds half, ptr addrspace(1) %p, i64 [[IDX]]
; CHECK:         [[H:%.*]] = load <5 x half>, ptr addrspace(1) [[P]], align 16
; CHECK:         fpext <5 x half> [[H]] to <5 x float>
define spir_func <5 x float> @vloada_halfn(i64 %o, ptr addrspace(1) %p) {
  %r = call spir_func <5 x float> @_Z32__spirv_ocl_vloada_halfn_Rfloat5mPU3AS1KDhi(i64 %o, ptr addrspace(1) %p, i32 5)
  ret <5 x float> %r
}

; As BiF: each element goes through the scalar conversion of the rounding mode
; (1 = RTZ), then one store of the half vector at the padded stride.
; CHECK-LABEL: define spir_func void @vstorea_halfn_r(
; CHECK:         [[IDX:%.*]] = mul i64 %o, 8
; CHECK:         [[P:%.*]] = getelementptr inbounds half, ptr addrspace(1) %p, i64 [[IDX]]
; CHECK:         [[X0:%.*]] = extractelement <5 x float> %x, i32 0
; CHECK:         [[H0:%.*]] = call spir_func half @_Z26__spirv_FConvert_Rhalf_rtzf(float [[X0]])
; CHECK:         [[V0:%.*]] = insertelement <5 x half> poison, half [[H0]], i32 0
; CHECK-COUNT-4: call spir_func half @_Z26__spirv_FConvert_Rhalf_rtzf(
; CHECK:         store <5 x half> {{%.*}}, ptr addrspace(1) [[P]], align 16
; CHECK-NOT:     call
define spir_func void @vstorea_halfn_r(<5 x float> %x, i64 %o, ptr addrspace(1) %p) {
  call spir_func void @_Z27__spirv_ocl_vstorea_halfn_rDv5_fmPU3AS1Dhi(<5 x float> %x, i64 %o, ptr addrspace(1) %p, i32 1)
  ret void
}

; CHECK-LABEL: define spir_func void @vstore_halfn_r(
; CHECK:         [[IDX:%.*]] = mul i64 %o, 5
; CHECK:         [[P:%.*]] = getelementptr inbounds half, ptr addrspace(1) %p, i64 [[IDX]]
; CHECK-COUNT-5: call spir_func half @_Z26__spirv_FConvert_Rhalf_rtpd(
; CHECK:         store <5 x half> {{%.*}}, ptr addrspace(1) [[P]], align 2
define spir_func void @vstore_halfn_r(<5 x double> %x, i64 %o, ptr addrspace(1) %p) {
  call spir_func void @_Z26__spirv_ocl_vstore_halfn_rDv5_dmPU3AS1Dhi(<5 x double> %x, i64 %o, ptr addrspace(1) %p, i32 2)
  ret void
}

; A float5 occupies 8 floats in memory.
; CHECK-LABEL: define spir_func void @prefetch(
; CHECK:         [[N:%.*]] = mul i64 %n, 8
; CHECK:         call spir_func void @_Z20__spirv_ocl_prefetchPU3AS1Kfm(ptr addrspace(1) %p, i64 [[N]])
define spir_func void @prefetch(ptr addrspace(1) %p, i64 %n) {
  call spir_func void @_Z20__spirv_ocl_prefetchPU3AS1KDv5_fm(ptr addrspace(1) %p, i64 %n)
  ret void
}

; A contiguous copy of n vectors is a copy of 8 * n scalars. A strided one
; copies each vector separately, joining the event of an initial empty copy.
; Stride 0 copies contiguously, as BiF does.
; CHECK-LABEL: define spir_func ptr @async_copy(
; CHECK:         [[N:%.*]] = mul i64 %n, 8
; CHECK:         [[E1:%.*]] = call spir_func ptr @_Z22__spirv_GroupAsyncCopyiPU3AS3fPU3AS1KfmmP13__spirv_Event(i32 2, ptr addrspace(3) %dst, ptr addrspace(1) %src, i64 [[N]], i64 1, ptr null)
; CHECK:         [[N0:%.*]] = mul i64 %n, 8
; CHECK:         [[E0:%.*]] = call spir_func ptr @_Z22__spirv_GroupAsyncCopyiPU3AS3fPU3AS1KfmmP13__spirv_Event(i32 2, ptr addrspace(3) %dst, ptr addrspace(1) %src, i64 [[N0]], i64 0, ptr [[E1]])
; CHECK:         [[Z:%.*]] = icmp eq i64 %stride, 0
; CHECK:         [[ST:%.*]] = select i1 [[Z]], i64 1, i64 %stride
; CHECK:         [[E2:%.*]] = call spir_func ptr @_Z22__spirv_GroupAsyncCopyiPU3AS3fPU3AS1KfmmP13__spirv_Event(i32 2, ptr addrspace(3) %dst, ptr addrspace(1) %src, i64 0, i64 1, ptr [[E0]])
; CHECK:         [[NZ:%.*]] = icmp ne i64 %n, 0
; CHECK:         br i1 [[NZ]], label %[[LOOP:async.copy.loop]], label %[[TAIL:async.copy.tail]]
; CHECK:       [[LOOP]]:
; CHECK:         [[I:%.*]] = phi i64
; CHECK:         [[S:%.*]] = mul i64 [[I]], [[ST]]
; CHECK:         [[DI:%.*]] = mul i64 [[I]], 8
; CHECK:         [[D:%.*]] = getelementptr inbounds float, ptr addrspace(3) %dst, i64 [[DI]]
; CHECK:         [[SI:%.*]] = mul i64 [[S]], 8
; CHECK:         [[SP:%.*]] = getelementptr inbounds float, ptr addrspace(1) %src, i64 [[SI]]
; CHECK:         call spir_func ptr @_Z22__spirv_GroupAsyncCopyiPU3AS3fPU3AS1KfmmP13__spirv_Event(i32 2, ptr addrspace(3) [[D]], ptr addrspace(1) [[SP]], i64 8, i64 1, ptr [[E2]])
; CHECK:       [[TAIL]]:
; CHECK:         ret ptr [[E2]]
define spir_func ptr @async_copy(ptr addrspace(3) %dst, ptr addrspace(1) %src, i64 %n, i64 %stride) {
  %e1 = call spir_func ptr @_Z22__spirv_GroupAsyncCopyiPU3AS3Dv5_fPU3AS1KS_mmP13__spirv_Event(i32 2, ptr addrspace(3) %dst, ptr addrspace(1) %src, i64 %n, i64 1, ptr null)
  %e0 = call spir_func ptr @_Z22__spirv_GroupAsyncCopyiPU3AS3Dv5_fPU3AS1KS_mmP13__spirv_Event(i32 2, ptr addrspace(3) %dst, ptr addrspace(1) %src, i64 %n, i64 0, ptr %e1)
  %e2 = call spir_func ptr @_Z22__spirv_GroupAsyncCopyiPU3AS3Dv5_fPU3AS1KS_mmP13__spirv_Event(i32 2, ptr addrspace(3) %dst, ptr addrspace(1) %src, i64 %n, i64 %stride, ptr %e0)
  ret ptr %e2
}

; [US]MulExtended return {low, high} through sret: the low half is a vector mul,
; the high half the scalarized [us]_mul_hi.
; CHECK-LABEL: define spir_func void @umul_extended(
; CHECK-COUNT-5: call spir_func i32 @_Z20__spirv_ocl_u_mul_hijj(i32
; CHECK:         [[HI:%.*]] = insertelement <5 x i32> {{.*}}, i32 4
; CHECK:         [[P0:%.*]] = getelementptr inbounds{{( nuw)?}} %structtype, ptr %r, i32 0, i32 0
; CHECK:         [[LO:%.*]] = mul <5 x i32> %x, %y
; CHECK:         store <5 x i32> [[LO]], ptr [[P0]]
; CHECK:         [[P1:%.*]] = getelementptr inbounds{{( nuw)?}} %structtype, ptr %r, i32 0, i32 1
; CHECK:         store <5 x i32> [[HI]], ptr [[P1]]
; CHECK-NOT:     call {{.*}}UMulExtended
define spir_func void @umul_extended(ptr %r, <5 x i32> %x, <5 x i32> %y) {
  call spir_func void @_Z20__spirv_UMulExtendedDv5_jS_(ptr sret(%structtype) %r, <5 x i32> %x, <5 x i32> %y)
  ret void
}

declare spir_func float @_Z11__spirv_DotDv5_fS_(<5 x float>, <5 x float>)
declare spir_func i1 @_Z11__spirv_AnyDv5_c(<5 x i8>)
declare spir_func <5 x float> @_Z18__spirv_ocl_selectDv5_fS_Dv5_i(<5 x float>, <5 x float>, <5 x i32>)
declare spir_func <3 x float> @_Z20__spirv_ocl_shuffle2Dv5_fS_Dv3_j(<5 x float>, <5 x float>, <3 x i32>)
declare spir_func <5 x float> @_Z20__spirv_ocl_shuffle2Dv4_fS_Dv5_j(<4 x float>, <4 x float>, <5 x i32>)
declare spir_func <5 x float> @_Z26__spirv_ocl_vloadn_Rfloat5mPU3AS1Kfi(i64, ptr addrspace(1), i32)
declare spir_func <5 x bfloat> @_Z26__spirv_ocl_vloadn_RDF16b5mPU3AS1KDF16bi(i64, ptr addrspace(1), i32)
declare spir_func void @_Z19__spirv_ocl_vstorenDv5_fmPU3AS1f(<5 x float>, i64, ptr addrspace(1))
declare spir_func void @_Z20__spirv_ocl_prefetchPU3AS1KDv5_fm(ptr addrspace(1), i64)
declare spir_func ptr @_Z22__spirv_GroupAsyncCopyiPU3AS3Dv5_fPU3AS1KS_mmP13__spirv_Event(i32, ptr addrspace(3), ptr addrspace(1), i64, i64, ptr)
declare spir_func i1 @_Z11__spirv_AllDv5_c(<5 x i8>)
declare spir_func i1 @_Z11__spirv_AllDv1_c(<1 x i8>)
declare spir_func <3 x float> @_Z19__spirv_ocl_shuffleDv5_fDv3_j(<5 x float>, <3 x i32>)
declare spir_func <5 x float> @_Z31__spirv_ocl_vload_halfn_Rfloat5mPU3AS1KDhi(i64, ptr addrspace(1), i32)
declare spir_func <5 x float> @_Z32__spirv_ocl_vloada_halfn_Rfloat5mPU3AS1KDhi(i64, ptr addrspace(1), i32)
declare spir_func void @_Z27__spirv_ocl_vstorea_halfn_rDv5_fmPU3AS1Dhi(<5 x float>, i64, ptr addrspace(1), i32)
declare spir_func void @_Z26__spirv_ocl_vstore_halfn_rDv5_dmPU3AS1Dhi(<5 x double>, i64, ptr addrspace(1), i32)
declare spir_func <4 x float> @_Z19__spirv_ocl_shuffleDv300_fDv4_j(<300 x float>, <4 x i32>)
declare spir_func void @_Z20__spirv_UMulExtendedDv5_jS_(ptr sret(%structtype), <5 x i32>, <5 x i32>)

; CHECK-NOT: declare {{.*}}Dv5_fS_(
