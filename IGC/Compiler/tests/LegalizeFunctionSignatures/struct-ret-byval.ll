;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers --igc-legalize-function-signatures -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LegalizeFunctionSignatures
; ------------------------------------------------

%struct = type {float, float}


; CHECK: define spir_func %struct @func()
define spir_func %struct @func() {
  ; CHECK: [[TMP1:%.*]] = alloca %struct, align 8, addrspace(4)
  ; CHECK: store %struct { float {{[0-9].[0-9]+e\+[0-9]+}}, float {{[0-9].[0-9]+e\+[0-9]+}} }, ptr addrspace(4) [[TMP1]]
  ; CHECK: [[TMP2:%.*]] = load %struct, ptr addrspace(4) [[TMP1]], align 8
  ; CHECK: [[TMP3:%.*]] = getelementptr inbounds %struct, ptr addrspace(4) [[TMP1]], i32 0, i32 0
  ; CHECK: [[TMP4:%.*]] = load float, ptr addrspace(4) [[TMP3]], align 4
  ; CHECK: [[TMP5:%.*]] = insertvalue %struct undef, float [[TMP4]], 0
  ; CHECK: [[TMP6:%.*]] = getelementptr inbounds %struct, ptr addrspace(4) [[TMP1]], i32 0, i32 1
  ; CHECK: [[TMP7:%.*]] = load float, ptr addrspace(4) [[TMP6]], align 4
  ; CHECK: [[TMP8:%.*]] = insertvalue %struct [[TMP5]], float [[TMP7]], 1
  ; CHECK: ret %struct [[TMP8]]
  %1 = alloca %struct, align 8, addrspace(4)
  store %struct {float 1.0, float 2.0}, ptr addrspace(4) %1, align 8
  %2 = load %struct, ptr addrspace(4) %1, align 8
  ret %struct %2
}
