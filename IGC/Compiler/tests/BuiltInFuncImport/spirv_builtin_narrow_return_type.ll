;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; A BuiltIn variable that the SPIR-V spec types as a vector of size_t may legally be declared
; narrower than the target's size_t, e.g. "OpTypeVector %uint 3" under Physical64. The translator
; then emits calls returning i32, while BiFModule defines __spirv_BuiltInLocalInvocationId returning
; i64. Because Itanium mangling does not encode the return type, the definition silently replaces
; the declaration when BiF is linked in and leaves call sites disagreeing with their callee.
;
; Such a call reads as indirect (CallBase::getCalledFunction() type-checks and returns null), which
; keeps the builtin from being inlined and keeps BuiltinCallGraphAnalysis from propagating the
; implicit arguments it needs up to the kernel. Check that BIImport instead rebuilds the call
; against the linked-in definition and truncates the result back to the declared width.

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-builtin-import -disable-verify -S < %s | FileCheck %s

; CHECK-LABEL: define spir_kernel void @test_scalar
; CHECK: [[ID:%.*]] = call{{.*}} i64 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 0)
; CHECK: [[TR:%.*]] = trunc i64 [[ID]] to i32
; CHECK: store i32 [[TR]]

define spir_kernel void @test_scalar(ptr addrspace(1) %out) {
entry:
  %id = call spir_func i32 @_Z32__spirv_BuiltInLocalInvocationIdi(i32 0)
  store i32 %id, ptr addrspace(1) %out, align 4
  ret void
}

; The shape an OpTypeVector %uint 3 BuiltIn actually translates to. Every element must be truncated
; before it is inserted, otherwise an i64 ends up in a <3 x i32> and the vector is later legalized
; into six i32 lanes and bitcast back down to three, silently dropping component 2.
; CHECK-LABEL: define spir_kernel void @test_vector
; CHECK: [[X:%.*]] = call{{.*}} i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
; CHECK: [[XT:%.*]] = trunc i64 [[X]] to i32
; CHECK: insertelement <3 x i32> undef, i32 [[XT]], i32 0
; CHECK: [[Y:%.*]] = call{{.*}} i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 1)
; CHECK: [[YT:%.*]] = trunc i64 [[Y]] to i32
; CHECK: insertelement <3 x i32> %{{.*}}, i32 [[YT]], i32 1
; CHECK: [[Z:%.*]] = call{{.*}} i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 2)
; CHECK: [[ZT:%.*]] = trunc i64 [[Z]] to i32
; CHECK: insertelement <3 x i32> %{{.*}}, i32 [[ZT]], i32 2

define spir_kernel void @test_vector(ptr addrspace(1) %out) {
entry:
  %x = call spir_func i32 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %v0 = insertelement <3 x i32> undef, i32 %x, i32 0
  %y = call spir_func i32 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 1)
  %v1 = insertelement <3 x i32> %v0, i32 %y, i32 1
  %z = call spir_func i32 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 2)
  %v2 = insertelement <3 x i32> %v1, i32 %z, i32 2
  store <3 x i32> %v2, ptr addrspace(1) %out, align 16
  ret void
}

; No call site may be left returning the narrow type.
; CHECK-NOT: call{{.*}} i32 @_Z32__spirv_BuiltInLocalInvocationIdi
; CHECK-NOT: call{{.*}} i32 @_Z33__spirv_BuiltInGlobalInvocationIdi

declare spir_func i32 @_Z32__spirv_BuiltInLocalInvocationIdi(i32)
declare spir_func i32 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)
