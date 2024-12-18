;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -CMABILegacy -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -CMABILegacy -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

; RUN: %opt_new_pm_typed -passes=CMABI -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_new_pm_opaque -passes=CMABI -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS

target datalayout = "e-p:64:64-i64:64-n8:16:32"

@global.int = internal global i32 0, align 4
; CHECK: @global.int
; CHECK-NOT: @global.int

; Function Attrs: nounwind readnone
declare <8 x i32> @llvm.genx.wrregioni.v8i32.i32.i16.i1(<8 x i32>, i32, i32, i32, i32, i16, i32, i1)

; Function Attrs: nounwind readnone
declare <4 x float> @llvm.genx.wrregionf.v4f32.f32.i16.i1(<4 x float>, float, i32, i32, i32, i16, i32, i1)

; Function Attrs: nounwind readnone
declare <10 x float> @llvm.genx.wrregionf.v10f32.v4f32.i16.i1(<10 x float>, <4 x float>, i32, i32, i32, i16, i32, i1) #2

; Function Attrs: nounwind
declare void @llvm.genx.oword.st.v8i32(i32, i32, <8 x i32>)

; Function Attrs: noinline nounwind
define internal spir_func i32 @foo(<8 x i32>* %foo.int.vec.ref.ptr, float %foo.value, <4 x float>* %foo.flt.vec.ref.ptr) {
; CHECK: define internal spir_func { i32, <8 x i32>, <4 x float>, i32 } @foo(<8 x i32> %[[FOO_INT_VEC_IN:[^ ]+]], float %foo.value, <4 x float> %[[FOO_FLT_VEC_IN:[^ ]+]], i32 %global.int.in) {
; CHECK: %global.int.local = alloca i32
; CHECK-TYPED-PTRS: store i32 %global.int.in, i32* %global.int.local
; CHECK-OPAQUE-PTRS: store i32 %global.int.in, ptr %global.int.local
; CHECK: %[[FOO_INT_VEC_ALLOCA:[^ ]+]] = alloca <8 x i32>
; CHECK-TYPED-PTRS: store <8 x i32> %[[FOO_INT_VEC_IN]], <8 x i32>* %[[FOO_INT_VEC_ALLOCA]]
; CHECK-OPAQUE-PTRS: store <8 x i32> %[[FOO_INT_VEC_IN]], ptr %[[FOO_INT_VEC_ALLOCA]]
; CHECK: %[[FOO_FLT_VEC_ALLOCA:[^ ]+]] = alloca <4 x float>
; CHECK-TYPED-PTRS: store <4 x float> %[[FOO_FLT_VEC_IN]], <4 x float>* %[[FOO_FLT_VEC_ALLOCA]]
; CHECK-OPAQUE-PTRS: store <4 x float> %[[FOO_FLT_VEC_IN]], ptr %[[FOO_FLT_VEC_ALLOCA]]

  %global.int.load = load i32, i32* @global.int, align 4
; CHECK-TYPED-PTRS: %global.int.load = load i32, i32* %global.int.local, align 4
; CHECK-OPAQUE-PTRS: %global.int.load = load i32, ptr %global.int.local, align 4
; COM: the line should be the same
  %global.int.inc = add nsw i32 %global.int.load, 1
; CHECK: %global.int.inc = add nsw i32 %global.int.load, 1
  store i32 %global.int.inc, i32* @global.int, align 4
; CHECK-TYPED-PTRS: store i32 %global.int.inc, i32* %global.int.local, align 4
; CHECK-OPAQUE-PTRS: store i32 %global.int.inc, ptr %global.int.local, align 4

  %foo.int.vec.ref = load <8 x i32>, <8 x i32>* %foo.int.vec.ref.ptr
; CHECK-TYPED-PTRS: %foo.int.vec.ref = load <8 x i32>, <8 x i32>* %[[FOO_INT_VEC_ALLOCA]]
; CHECK-OPAQUE-PTRS: %foo.int.vec.ref = load <8 x i32>, ptr %[[FOO_INT_VEC_ALLOCA]]
; COM: the line should be the same
  %foo.int.vec.ref.new = call <8 x i32> @llvm.genx.wrregioni.v8i32.i32.i16.i1(<8 x i32> %foo.int.vec.ref, i32 %global.int.inc, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
; CHECK: %foo.int.vec.ref.new = call <8 x i32> @llvm.genx.wrregioni.v8i32.i32.i16.i1(<8 x i32> %foo.int.vec.ref, i32 %global.int.inc, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
  store <8 x i32> %foo.int.vec.ref.new, <8 x i32>* %foo.int.vec.ref.ptr
; CHECK-TYPED-PTRS: store <8 x i32> %foo.int.vec.ref.new, <8 x i32>* %[[FOO_INT_VEC_ALLOCA]]
; CHECK-OPAQUE-PTRS: store <8 x i32> %foo.int.vec.ref.new, ptr %[[FOO_INT_VEC_ALLOCA]]

  %foo.flt.vec.ref = load <4 x float>, <4 x float>* %foo.flt.vec.ref.ptr
; CHECK-TYPED-PTRS: %foo.flt.vec.ref = load <4 x float>, <4 x float>* %[[FOO_FLT_VEC_ALLOCA]]
; CHECK-OPAQUE-PTRS: %foo.flt.vec.ref = load <4 x float>, ptr %[[FOO_FLT_VEC_ALLOCA]]
; COM: the line should be the same
  %foo.flt.vec.ref.new = call <4 x float> @llvm.genx.wrregionf.v4f32.f32.i16.i1(<4 x float> %foo.flt.vec.ref, float %foo.value, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
; CHECK: %foo.flt.vec.ref.new = call <4 x float> @llvm.genx.wrregionf.v4f32.f32.i16.i1(<4 x float> %foo.flt.vec.ref, float %foo.value, i32 0, i32 1, i32 0, i16 0, i32 undef, i1 true)
  store <4 x float> %foo.flt.vec.ref.new, <4 x float>* %foo.flt.vec.ref.ptr
; CHECK-TYPED-PTRS: store <4 x float> %foo.flt.vec.ref.new, <4 x float>* %[[FOO_FLT_VEC_ALLOCA]]
; CHECK-OPAQUE-PTRS: store <4 x float> %foo.flt.vec.ref.new, ptr %[[FOO_FLT_VEC_ALLOCA]]

  %foo.ret = fptosi float %foo.value to i32
; CHECK: %foo.ret = fptosi float %foo.value to i32
; CHECK: %[[INSERT_ORIG_RET:[^ ]+]] = insertvalue { i32, <8 x i32>, <4 x float>, i32 } undef, i32 %foo.ret, 0
; CHECK-TYPED-PTRS: %[[FOO_INT_VEC_OUT:[^ ]+]] = load <8 x i32>, <8 x i32>* %[[FOO_INT_VEC_ALLOCA]]
; CHECK-OPAQUE-PTRS: %[[FOO_INT_VEC_OUT:[^ ]+]] = load <8 x i32>, ptr %[[FOO_INT_VEC_ALLOCA]]
; CHECK: %[[INSERT_INT_VEC:[^ ]+]] = insertvalue { i32, <8 x i32>, <4 x float>, i32 } %[[INSERT_ORIG_RET]], <8 x i32> %[[FOO_INT_VEC_OUT]], 1
; CHECK-TYPED-PTRS: %[[FOO_FLT_VEC_OUT:[^ ]+]] = load <4 x float>, <4 x float>* %[[FOO_FLT_VEC_ALLOCA]]
; CHECK-OPAQUE-PTRS: %[[FOO_FLT_VEC_OUT:[^ ]+]] = load <4 x float>, ptr %[[FOO_FLT_VEC_ALLOCA]]
; CHECK: %[[INSERT_FLT_VEC:[^ ]+]] = insertvalue { i32, <8 x i32>, <4 x float>, i32 } %[[INSERT_INT_VEC]], <4 x float> %[[FOO_FLT_VEC_OUT]], 2
; CHECK-TYPED-PTRS: %[[GLOBAL_INT_OUT:[^ ]+]] = load i32, i32* %global.int.local
; CHECK-OPAQUE-PTRS: %[[GLOBAL_INT_OUT:[^ ]+]] = load i32, ptr %global.int.local
; CHECK: %[[INSERT_GLOBAL_INT:[^ ]+]] = insertvalue { i32, <8 x i32>, <4 x float>, i32 } %[[INSERT_FLT_VEC]], i32 %[[GLOBAL_INT_OUT]], 3
  ret i32 %foo.ret
; CHECK: ret { i32, <8 x i32>, <4 x float>, i32 } %[[INSERT_GLOBAL_INT]]
}

; Function Attrs: noinline nounwind
define dllexport void @kernel(float %kernel.value) {
; CHECK: %global.int.local = alloca i32, align 4
; CHECK-TYPED-PTRS: store i32 0, i32* %global.int.local
; CHECK-OPAQUE-PTRS: store i32 0, ptr %global.int.local
  %kernel.int.vec.ref.ptr = alloca <8 x i32>, align 32
  %kernel.flt.vec.ref.ptr = alloca <4 x float>, align 16

; CHECK-TYPED-PTRS: %kernel.int.vec.ref.ptr.val = load <8 x i32>, <8 x i32>* %kernel.int.vec.ref.ptr
; CHECK-TYPED-PTRS: %kernel.flt.vec.ref.ptr.val = load <4 x float>, <4 x float>* %kernel.flt.vec.ref.ptr
; CHECK-TYPED-PTRS: %global.int.val = load i32, i32* %global.int.local
; CHECK-OPAQUE-PTRS: %kernel.int.vec.ref.ptr.val = load <8 x i32>, ptr %kernel.int.vec.ref.ptr
; CHECK-OPAQUE-PTRS: %kernel.flt.vec.ref.ptr.val = load <4 x float>, ptr %kernel.flt.vec.ref.ptr
; CHECK-OPAQUE-PTRS: %global.int.val = load i32, ptr %global.int.local

  %foo.res = call spir_func i32 @foo(<8 x i32>* nonnull %kernel.int.vec.ref.ptr, float %kernel.value, <4 x float>* nonnull %kernel.flt.vec.ref.ptr)
; CHECK: %foo.res = call spir_func { i32, <8 x i32>, <4 x float>, i32 } @foo(<8 x i32> %kernel.int.vec.ref.ptr.val, float %kernel.value, <4 x float> %kernel.flt.vec.ref.ptr.val, i32 %global.int.val)

; CHECK: %ret = extractvalue { i32, <8 x i32>, <4 x float>, i32 } %foo.res, 0
; CHECK: %[[KERN_INT_VEC_RET:[^ ]+]] = extractvalue { i32, <8 x i32>, <4 x float>, i32 } %foo.res, 1
; CHECK-TYPED-PTRS: store <8 x i32> %[[KERN_INT_VEC_RET]], <8 x i32>* %kernel.int.vec.ref.ptr
; CHECK-OPAQUE-PTRS: store <8 x i32> %[[KERN_INT_VEC_RET]], ptr %kernel.int.vec.ref.ptr
; CHECK: %[[KERN_FLT_VEC_RET:[^ ]+]] = extractvalue { i32, <8 x i32>, <4 x float>, i32 } %foo.res, 2
; CHECK-TYPED-PTRS: store <4 x float> %[[KERN_FLT_VEC_RET]], <4 x float>* %kernel.flt.vec.ref.ptr
; CHECK-OPAQUE-PTRS: store <4 x float> %[[KERN_FLT_VEC_RET]], ptr %kernel.flt.vec.ref.ptr
; CHECK: %[[KERN_GLOBAL_INT_RET:[^ ]+]] = extractvalue { i32, <8 x i32>, <4 x float>, i32 } %foo.res, 3
; CHECK-TYPED-PTRS: store i32 %[[KERN_GLOBAL_INT_RET]], i32* %global.int.local
; CHECK-OPAQUE-PTRS: store i32 %[[KERN_GLOBAL_INT_RET]], ptr %global.int.local

  %kernel.flt.vec.ref.changed = load <4 x float>, <4 x float>* %kernel.flt.vec.ref.ptr
; COM: the line should be the same
; CHECK-TYPED-PTRS: %kernel.flt.vec.ref.changed = load <4 x float>, <4 x float>* %kernel.flt.vec.ref.ptr
; CHECK-OPAQUE-PTRS: %kernel.flt.vec.ref.changed = load <4 x float>, ptr %kernel.flt.vec.ref.ptr
  %kernel.flt.vec.ref.wr = call <10 x float> @llvm.genx.wrregionf.v10f32.v4f32.i16.i1(<10 x float> undef, <4 x float> %kernel.flt.vec.ref.changed, i32 0, i32 4, i32 2, i16 4, i32 undef, i1 true)
; COM: the line should be the same
; CHECK: %kernel.flt.vec.ref.wr = call <10 x float> @llvm.genx.wrregionf.v10f32.v4f32.i16.i1(<10 x float> undef, <4 x float> %kernel.flt.vec.ref.changed, i32 0, i32 4, i32 2, i16 4, i32 undef, i1 true)
  %kernel.int.vec.ref.changed = load <8 x i32>, <8 x i32>* %kernel.int.vec.ref.ptr, align 32
; COM: the line should be the same
; CHECK-TYPED-PTRS: %kernel.int.vec.ref.changed = load <8 x i32>, <8 x i32>* %kernel.int.vec.ref.ptr, align 32
; CHECK-OPAQUE-PTRS: %kernel.int.vec.ref.changed = load <8 x i32>, ptr %kernel.int.vec.ref.ptr, align 32
  %kernel.int.vec.ref.new = call <8 x i32> @llvm.genx.wrregioni.v8i32.i32.i16.i1(<8 x i32> %kernel.int.vec.ref.changed, i32 %foo.res, i32 0, i32 1, i32 0, i16 4, i32 undef, i1 true)
; COM: the line should be the same
; CHECK: %kernel.int.vec.ref.new = call <8 x i32> @llvm.genx.wrregioni.v8i32.i32.i16.i1(<8 x i32> %kernel.int.vec.ref.changed, i32 %ret, i32 0, i32 1, i32 0, i16 4, i32 undef, i1 true)
  call void @llvm.genx.oword.st.v8i32(i32 0, i32 0, <8 x i32> %kernel.int.vec.ref.new)
  ret void
}

!genx.kernels = !{!0}
!0 = !{void (float)* @kernel}
