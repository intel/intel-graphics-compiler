;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-gas-resolve | FileCheck %s

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define internal spir_func void @func() {
  %src = alloca double
  %dst0 = alloca { double, double, double }          ; <-- normal struct type
  %dst1 = alloca <{ double, double, double }>        ; <-- packed struct type

  ; CHECK: %[[SRC:.*]] = bitcast double* %src to i8*
  ; CHECK: %[[DST0:.*]] = bitcast { double, double, double }* %dst0 to i8*
  ; CHECK: %[[DST1:.*]] = bitcast <{ double, double, double }>* %dst1 to i8*
  %src_as_generic = addrspacecast double* %src to i8 addrspace(4)*
  %dst0_as_generic = addrspacecast { double, double, double }* %dst0 to i8 addrspace(4)*
  %dst1_as_generic = addrspacecast <{ double, double, double }>* %dst1 to i8 addrspace(4)*

  ; CHECK: call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %[[DST0]], i8* align 8 %[[SRC]], i64 24, i1 false)
  ; CHECK: call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %[[DST1]], i8* align 8 %[[SRC]], i64 24, i1 false)
  call void @llvm.memcpy.p4i8.p4i8.i64(i8 addrspace(4)* align 8 %dst0_as_generic, i8 addrspace(4)* align 8 %src_as_generic, i64 24, i1 false)
  call void @llvm.memcpy.p4i8.p4i8.i64(i8 addrspace(4)* align 8 %dst1_as_generic, i8 addrspace(4)* align 8 %src_as_generic, i64 24, i1 false)

  ret void
}

; CHECK: declare void @llvm.memcpy.p0i8.p0i8.i64(i8* {{(noalias )?}}nocapture writeonly, i8* {{(noalias )?}}nocapture readonly, i64, i1 immarg)
declare void @llvm.memcpy.p4i8.p4i8.i64(i8 addrspace(4)* nocapture writeonly, i8 addrspace(4)* nocapture readonly, i64, i1 immarg)

