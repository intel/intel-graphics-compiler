;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unkonwn-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck %s
; RUN: opt %use_old_pass_manager% -enable-debugify -GenXLoadStoreLowering -march=genx64 -mcpu=XeHPC -mtriple=spir64-unkonwn-unknown -enable-ldst-lowering=true -mattr=+ocl_runtime -S < %s 2>&1 | FileCheck --check-prefix=CHECK-LSC %s
;
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS
; CHECK-LSC-NOT: WARNING
; CHECK-LSC: CheckModuleDebugify: PASS

target datalayout = "e-p:64:64-i64:64-n8:16:32"

; CHECK-LABEL: foo
; CHECK: %[[GATHER:.*]] = call <1 x i64> @llvm.genx.svm.gather.v1i64.v1i1.v1i64(<1 x i1> <i1 true>, i32 0, <1 x i64> undef, <1 x i64> undef)
; CHECK: %[[INTPTR:.*]] = inttoptr <1 x i64> %[[GATHER]] to <1 x float (i8 addrspace(1)*, float)*>
; CHECK: bitcast <1 x float (i8 addrspace(1)*, float)*> %[[INTPTR]] to float (i8 addrspace(1)*, float)*
; CHECK-LSC: %[[GATHER:.*]] = call <1 x i64> @llvm.genx.lsc.load.merge.stateless.v1i64.v1i1.i64(<1 x i1> <i1 true>, i8 0, i8 0, i8 0, i16 1, i32 0, i8 4, i8 1, i8 1, i8 0, i64 undef, i32 0, <1 x i64> undef)
; CHECK-LSC: %[[INTPTR:.*]] = inttoptr <1 x i64> %[[GATHER]] to <1 x float (i8 addrspace(1)*, float)*>
; CHECK-LSC: bitcast <1 x float (i8 addrspace(1)*, float)*> %[[INTPTR]] to float (i8 addrspace(1)*, float)*

define void @foo() {
  %1 = load float (i8 addrspace(1)*, float)*, float (i8 addrspace(1)*, float)* addrspace(1)* undef, align 8
  ret void
}
