;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -static-gas-resolution | FileCheck %s
;
; Checks that StaticGASResolution removes IB_to_private/IB_to_local builtin calls
; when there is no Private/Local to Generic AS casting

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

define spir_kernel void @test_builtin(i8 addrspace(4)* noundef %p1) {
; CHECK-LABEL: @test_builtin(
; CHECK-NEXT:  bb2:
; CHECK-NOT:     [[CALL1:%.*]] = call spir_func i8 addrspace(3)* @__builtin_IB_to_local(
; CHECK-NEXT:    [[TOBOOL1:%.*]] = icmp eq i8 addrspace(3)* null, null
; CHECK-NEXT:    br label [[BB3:%.*]]
; CHECK:       bb3:
; CHECK-NOT:     [[CALL2:%.*]] = call spir_func i8* @__builtin_IB_to_private(
; CHECK-NEXT:    [[TOBOOL2:%.*]] = icmp eq i8* null, null
; CHECK-NEXT:    ret void
bb2:
  %call.i1 = call spir_func i8 addrspace(3)* @__builtin_IB_to_local(i8 addrspace(4)* noundef %p1)
  %tobool1 = icmp eq i8 addrspace(3)* %call.i1, null
  br label %bb3

bb3:                                      ; preds = %bb2
  %call.i2 = call spir_func i8* @__builtin_IB_to_private(i8 addrspace(4)* noundef %p1)
  %tobool2 = icmp eq i8* %call.i2, null
  ret void
}

define spir_kernel void @test_builtin1(i8* noundef %p0) {
; CHECK-LABEL: @test_builtin1(
; CHECK-NEXT:  bb2:
; CHECK-NEXT:    [[P1:%.*]] = addrspacecast i8* [[P0:%.*]] to i8 addrspace(4)*
; CHECK-NEXT:    [[CALL_I1:%.*]] = call spir_func i8 addrspace(3)* @__builtin_IB_to_local(i8 addrspace(4)* noundef [[P1]])
; CHECK-NEXT:    [[TOBOOL1:%.*]] = icmp eq i8 addrspace(3)* [[CALL_I1]], null
; CHECK-NEXT:    br label [[BB3:%.*]]
; CHECK:       bb3:
; CHECK-NEXT:    [[CALL_I2:%.*]] = call spir_func i8* @__builtin_IB_to_private(i8 addrspace(4)* noundef [[P1]])
; CHECK-NEXT:    [[TOBOOL2:%.*]] = icmp eq i8* [[CALL_I2]], null
; CHECK-NEXT:    ret void
bb2:
  %p1 = addrspacecast i8* %p0 to i8 addrspace(4)*
  %call.i1 = call spir_func i8 addrspace(3)* @__builtin_IB_to_local(i8 addrspace(4)* noundef %p1)
  %tobool1 = icmp eq i8 addrspace(3)* %call.i1, null
  br label %bb3

bb3:                                      ; preds = %bb2
  %call.i2 = call spir_func i8* @__builtin_IB_to_private(i8 addrspace(4)* noundef %p1)
  %tobool2 = icmp eq i8* %call.i2, null
  ret void
}

declare spir_func i8 addrspace(3)* @__builtin_IB_to_local(i8 addrspace(4)*)

declare spir_func i8* @__builtin_IB_to_private(i8 addrspace(4)*)

