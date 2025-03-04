;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -CMABILegacy -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -CMABILegacy -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

; RUN: %opt_new_pm_typed -passes=CMABI -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_new_pm_opaque -passes=CMABI -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

; CHECK: [[GV:@[^ ]+]] = internal global <32 x i8> undef #0
; CHECK: [[G:@[^ ]+]] = internal global <32 x i8> undef #1
@gv = internal global <32 x i8> undef #0
@g = internal global <32 x i8> undef #1

; CHECK: define internal spir_func <32 x i8> @bar(<32 x i8> [[ARG:%[^ ]+]], <32 x i8> %data) {
define internal spir_func void @bar(<32 x i8>* %arg, <32 x i8> %data) {
; CHECK: [[ALLOCA:%[^ ]+]] = alloca <32 x i8>, align 32
; CHECK-TYPED-PTRS: store <32 x i8> [[ARG]], <32 x i8>* [[ALLOCA]], align 32
; CHECK-TYPED-PTRS: store <32 x i8> %data, <32 x i8>* [[ALLOCA]], align 1
; CHECK-TYPED-PTRS: [[LOAD:%[^ ]+]] = load <32 x i8>, <32 x i8>* [[ALLOCA]], align 32
; CHECK-OPAQUE-PTRS: store <32 x i8> [[ARG]], ptr [[ALLOCA]], align 32
; CHECK-OPAQUE-PTRS: store <32 x i8> %data, ptr [[ALLOCA]], align 1
; CHECK-OPAQUE-PTRS: [[LOAD:%[^ ]+]] = load <32 x i8>, ptr [[ALLOCA]], align 32
; CHECK: ret <32 x i8> [[LOAD]]

  store <32 x i8> %data, <32 x i8>* %arg, align 1
  ret void
}

; CHECK-LABEL: @test_genx_volatile
define void @test_genx_volatile(<32 x i8> %arg) {
; CHECK-TYPED-PTRS: [[LOAD:%[^ ]+]] = call <32 x i8> @llvm.genx.vload.v32i8.p0v32i8(<32 x i8>* [[GV]])
; CHECK-OPAQUE-PTRS: [[LOAD:%[^ ]+]] = call <32 x i8> @llvm.genx.vload.v32i8.p0(ptr [[GV]])
; CHECK: [[RET:%[^ ]+]] = call spir_func <32 x i8> @bar(<32 x i8> [[LOAD]], <32 x i8> %arg)
; CHECK-TYPED-PTRS: call void @llvm.genx.vstore.v32i8.p0v32i8(<32 x i8> [[RET]], <32 x i8>* [[GV]])
; CHECK-OPAQUE-PTRS: call void @llvm.genx.vstore.v32i8.p0(<32 x i8> [[RET]], ptr [[GV]])

  call spir_func void @bar(<32 x i8>* @gv, <32 x i8> %arg)
  ret void
}

; CHECK-LABEL: @test_genx_none_volatile
define void @test_genx_none_volatile(<32 x i8> %arg) {
; CHECK: [[ALLOCA:%[^ ]+]] = alloca <32 x i8>, align 1
; CHECK-TYPED-PTRS: [[LOAD:%[^ ]+]] = load <32 x i8>, <32 x i8>* [[ALLOCA]], align 32
; CHECK-OPAQUE-PTRS: [[LOAD:%[^ ]+]] = load <32 x i8>, ptr [[ALLOCA]], align 32
; CHECK: [[RET:%[^ ]+]] = call spir_func <32 x i8> @bar(<32 x i8> [[LOAD]], <32 x i8> %arg)
; CHECK-TYPED-PTRS: store <32 x i8> [[RET]], <32 x i8>* [[ALLOCA]], align 32
; CHECK-OPAQUE-PTRS: store <32 x i8> [[RET]], ptr [[ALLOCA]], align 32

  call spir_func void @bar(<32 x i8>* @g, <32 x i8> %arg)
  ret void
}


attributes #0 = { "genx_volatile" }
attributes #1 = { "VCGlobalVariable" }
