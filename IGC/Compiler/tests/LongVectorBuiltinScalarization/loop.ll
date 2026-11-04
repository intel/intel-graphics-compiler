;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --opaque-pointers -igc-long-vector-builtin-scalarization -S < %s | FileCheck %s

; Above 16 elements the per-element work is a loop over the element index, in
; the generic path and in the custom lowerings that work per element.

target triple = "spir64-unknown-unknown"

%structtype.0 = type { <17 x i8>, <17 x i8> }

; CHECK-LABEL: define spir_func <17 x float> @loop(
; CHECK:         br label %[[LOOP:long.vector.loop]]
; CHECK:       [[LOOP]]:
; CHECK:         [[LANE:%.*]] = phi i32 [ 0, {{%.*}} ], [ [[NEXT:%.*]], %[[LOOP]] ]
; CHECK:         [[ACC:%.*]] = phi <17 x float> [ poison, {{%.*}} ], [ [[NEWACC:%.*]], %[[LOOP]] ]
; CHECK:         [[XI:%.*]] = extractelement <17 x float> %x, i32 [[LANE]]
; CHECK:         [[RI:%.*]] = call spir_func float @_Z16__spirv_ocl_fmaxff(float [[XI]], float %y)
; CHECK:         [[NEWACC]] = insertelement <17 x float> [[ACC]], float [[RI]], i32 [[LANE]]
; CHECK:         [[NEXT]] = add i32 [[LANE]], 1
; CHECK:         [[CMP:%.*]] = icmp ult i32 [[NEXT]], 17
; CHECK:         br i1 [[CMP]], label %[[LOOP]], label %[[TAIL:long.vector.tail]]
; CHECK:       [[TAIL]]:
; CHECK:         ret <17 x float> [[NEWACC]]
define spir_func <17 x float> @loop(<17 x float> %x, float %y) {
  %r = call spir_func <17 x float> @_Z16__spirv_ocl_fmaxDv17_ff(<17 x float> %x, float %y)
  ret <17 x float> %r
}

; CHECK-LABEL: define spir_func <17 x i8> @loop_bool(
; CHECK:         [[RI:%.*]] = call spir_func i1 @_Z13__spirv_IsNanf(float
; CHECK:         [[E:%.*]] = zext i1 [[RI]] to i8
; CHECK:         insertelement <17 x i8> {{%.*}}, i8 [[E]], i32
define spir_func <17 x i8> @loop_bool(<17 x float> %x) {
  %r = call spir_func <17 x i8> @_Z13__spirv_IsNanDv17_f(<17 x float> %x)
  ret <17 x i8> %r
}

; Reductions loop too. dot keeps the fma order of the unrolled form, from the
; last element down.
; CHECK-LABEL: define spir_func float @dot_loop(
; CHECK:         [[XL:%.*]] = extractelement <17 x float> %x, i32 16
; CHECK:         [[YL:%.*]] = extractelement <17 x float> %y, i32 16
; CHECK:         [[INIT:%.*]] = fmul float [[XL]], [[YL]]
; CHECK:       long.vector.loop:
; CHECK:         [[S:%.*]] = phi i32 [ 0, %{{.*}} ], [ [[NEXT:%.*]], %long.vector.loop ]
; CHECK:         [[ACC:%.*]] = phi float [ [[INIT]], %{{.*}} ], [ [[FMA:%.*]], %long.vector.loop ]
; CHECK:         [[IDX:%.*]] = sub i32 15, [[S]]
; CHECK:         [[XI:%.*]] = extractelement <17 x float> %x, i32 [[IDX]]
; CHECK:         [[YI:%.*]] = extractelement <17 x float> %y, i32 [[IDX]]
; CHECK:         [[FMA]] = call spir_func float @_Z15__spirv_ocl_fmafff(float [[XI]], float [[YI]], float [[ACC]])
; CHECK:         [[NEXT]] = add i32 [[S]], 1
; CHECK:         icmp ult i32 [[NEXT]], 16
; CHECK:       long.vector.tail:
; CHECK:         ret float [[FMA]]
define spir_func float @dot_loop(<17 x float> %x, <17 x float> %y) {
  %r = call spir_func float @_Z11__spirv_DotDv17_fS_(<17 x float> %x, <17 x float> %y)
  ret float %r
}

; CHECK-LABEL: define spir_func i1 @all_loop(
; CHECK:         [[FIRST:%.*]] = extractelement <17 x i8> %x, i32 0
; CHECK:       long.vector.loop:
; CHECK:         [[ACC:%.*]] = phi i8 [ [[FIRST]], %{{.*}} ], [ [[AND:%.*]], %long.vector.loop ]
; CHECK:         [[E:%.*]] = extractelement <17 x i8> %x, i32
; CHECK:         [[AND]] = and i8 [[ACC]], [[E]]
; CHECK:         icmp ult i32 {{%.*}}, 16
; CHECK:       long.vector.tail:
; CHECK:         [[R:%.*]] = icmp ne i8 [[AND]], 0
; CHECK:         ret i1 [[R]]
define spir_func i1 @all_loop(<17 x i8> %x) {
  %r = call spir_func i1 @_Z11__spirv_AllDv17_c(<17 x i8> %x)
  ret i1 %r
}

; The loop stores each element as it's converted, without a result vector.
; Without _r the rounding is RTE.
; CHECK-LABEL: define spir_func void @vstore_halfn_loop(
; CHECK:         [[IDX:%.*]] = mul i64 %o, 17
; CHECK:         [[P:%.*]] = getelementptr inbounds half, ptr addrspace(1) %p, i64 [[IDX]]
; CHECK:       long.vector.loop:
; CHECK:         [[LANE:%.*]] = phi i32
; CHECK:         [[PI:%.*]] = getelementptr inbounds half, ptr addrspace(1) [[P]], i32 [[LANE]]
; CHECK:         [[XI:%.*]] = extractelement <17 x double> %x, i32 [[LANE]]
; CHECK:         [[H:%.*]] = call spir_func half @_Z26__spirv_FConvert_Rhalf_rted(double [[XI]])
; CHECK:         store half [[H]], ptr addrspace(1) [[PI]], align 2
; CHECK-NOT:     store <17 x half>
define spir_func void @vstore_halfn_loop(<17 x double> %x, i64 %o, ptr addrspace(1) %p) {
  call spir_func void @_Z24__spirv_ocl_vstore_halfnDv17_dmPU3AS1Dh(<17 x double> %x, i64 %o, ptr addrspace(1) %p)
  ret void
}

; The loop form stores both halves of each element right away.
; CHECK-LABEL: define spir_func void @smul_extended(
; CHECK:         [[P0:%.*]] = getelementptr inbounds{{( nuw)?}} %structtype.0, ptr %r, i32 0, i32 0
; CHECK:         [[P1:%.*]] = getelementptr inbounds{{( nuw)?}} %structtype.0, ptr %r, i32 0, i32 1
; CHECK:       long.vector.loop:
; CHECK:         [[LANE:%.*]] = phi i32
; CHECK:         [[X:%.*]] = extractelement <17 x i8> %x, i32 [[LANE]]
; CHECK:         [[Y:%.*]] = extractelement <17 x i8> %y, i32 [[LANE]]
; CHECK:         [[LOP:%.*]] = getelementptr inbounds i8, ptr [[P0]], i32 [[LANE]]
; CHECK:         [[LO:%.*]] = mul i8 [[X]], [[Y]]
; CHECK:         store i8 [[LO]], ptr [[LOP]]
; CHECK:         [[HIP:%.*]] = getelementptr inbounds i8, ptr [[P1]], i32 [[LANE]]
; CHECK:         [[HI:%.*]] = call spir_func i8 @_Z20__spirv_ocl_s_mul_hicc(i8 [[X]], i8 [[Y]])
; CHECK:         store i8 [[HI]], ptr [[HIP]]
; CHECK:       long.vector.tail:
; CHECK-NEXT:    ret void
define spir_func void @smul_extended(ptr %r, <17 x i8> %x, <17 x i8> %y) {
  call spir_func void @_Z20__spirv_SMulExtendedDv17_cS_(ptr sret(%structtype.0) %r, <17 x i8> %x, <17 x i8> %y)
  ret void
}

declare spir_func float @_Z11__spirv_DotDv17_fS_(<17 x float>, <17 x float>)
declare spir_func i1 @_Z11__spirv_AllDv17_c(<17 x i8>)
declare spir_func <17 x float> @_Z16__spirv_ocl_fmaxDv17_ff(<17 x float>, float)
declare spir_func <17 x i8> @_Z13__spirv_IsNanDv17_f(<17 x float>)
declare spir_func void @_Z24__spirv_ocl_vstore_halfnDv17_dmPU3AS1Dh(<17 x double>, i64, ptr addrspace(1))
declare spir_func void @_Z20__spirv_SMulExtendedDv17_cS_(ptr sret(%structtype.0), <17 x i8>, <17 x i8>)
