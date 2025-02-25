;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_legacy_typed %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_legacy_opaque %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS
; RUN: %opt_legacy_typed %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK-LSC,CHECK-LSC-TYPED-PTRS
; RUN: %opt_legacy_opaque %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unknown-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK-LSC,CHECK-LSC-OPAQUE-PTRS
;
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS
; CHECK-LSC-NOT: WARNING
; CHECK-LSC: CheckModuleDebugify: PASS

target datalayout = "e-p:64:64-i64:64-n8:16:32"

; CHECK-LABEL: foo
; CHECK: %[[GATHER:.*]] = call <1 x i64> @llvm.genx.svm.gather.v1i64.v1i1.v1i64(<1 x i1> <i1 true>, i32 0, <1 x i64> undef, <1 x i64> undef)
; CHECK-TYPED-PTRS: %[[INTPTR:.*]] = inttoptr <1 x i64> %[[GATHER]] to <1 x float (i8 addrspace(1)*, float)*>
; CHECK-TYPED-PTRS: bitcast <1 x float (i8 addrspace(1)*, float)*> %[[INTPTR]] to float (i8 addrspace(1)*, float)*
; CHECK-OPAQUE-PTRS: %[[INTPTR:.*]] = inttoptr <1 x i64> %[[GATHER]] to <1 x ptr>
; CHECK-OPAQUE-PTRS: bitcast <1 x ptr> %[[INTPTR]] to ptr
; CHECK-LSC: %[[GATHER:.*]] = call <1 x i64> @llvm.vc.internal.lsc.load.ugm.v1i64.v1i1.v2i8.i64(<1 x i1> <i1 true>, i8 3, i8 4, i8 1, <2 x i8> zeroinitializer, i64 0, i64 undef, i16 1, i32 0, <1 x i64> undef)
; CHECK-LSC-TYPED-PTRS: %[[INTPTR:.*]] = inttoptr <1 x i64> %[[GATHER]] to <1 x float (i8 addrspace(1)*, float)*>
; CHECK-LSC-TYPED-PTRS: bitcast <1 x float (i8 addrspace(1)*, float)*> %[[INTPTR]] to float (i8 addrspace(1)*, float)*
; CHECK-LSC-OPAQUE-PTRS: %[[INTPTR:.*]] = inttoptr <1 x i64> %[[GATHER]] to <1 x ptr>
; CHECK-LSC-OPAQUE-PTRS: bitcast <1 x ptr> %[[INTPTR]] to ptr

define void @foo() {
  %1 = load float (i8 addrspace(1)*, float)*, float (i8 addrspace(1)*, float)* addrspace(1)* undef, align 8
  ret void
}
