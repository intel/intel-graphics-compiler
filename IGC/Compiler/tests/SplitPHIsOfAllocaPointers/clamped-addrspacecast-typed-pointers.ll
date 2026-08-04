;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; RUN: igc_opt --typed-pointers --igc-split-phis-of-alloca-pointers \
; RUN:   --regkey EnablePHIOfAllocaPtrSplit=1 -verify -S %s | FileCheck %s
;
; Test post-merge handling of addrspaces. Clamping the post-PHI addrspacecast destination
; to the alloca address space makes the source and destination address spaces equal. The replay must use a
; bitcast for the remaining pointee-type conversion instead of addrspacecast.

; CHECK-LABEL: define float @typed_addrspacecast(
; CHECK-LABEL: local.path:
; CHECK:       [[LOCAL_BYTES:%.*]] = bitcast float* %local.element to i8*
; CHECK-NOT:   addrspacecast
; CHECK:       [[LOCAL_TYPED:%.*]] = bitcast i8* [[LOCAL_BYTES]] to float*
; CHECK-NEXT:  [[LOCAL_LOAD:%.*]] = load float, float* [[LOCAL_TYPED]], align 4
; CHECK-LABEL: external.path:
; CHECK:       [[EXTERNAL_GLOBAL:%.*]] = addrspacecast i8 addrspace(4)* %external.bytes to i8 addrspace(1)*
; CHECK-NEXT:  [[EXTERNAL_TYPED:%.*]] = bitcast i8 addrspace(1)* [[EXTERNAL_GLOBAL]] to float addrspace(1)*
; CHECK-NEXT:  [[EXTERNAL_LOAD:%.*]] = load float, float addrspace(1)* [[EXTERNAL_TYPED]], align 4
; CHECK-LABEL: merge:
; CHECK:       [[VALUE:%.*]] = phi float [ [[LOCAL_LOAD]], %local.path ], [ [[EXTERNAL_LOAD]], %external.path ]
; CHECK-NEXT:  ret float [[VALUE]]
; CHECK-NOT:   phi i8 addrspace(4)*

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-n8:16:32"
target triple = "spir64-unknown-unknown"

define float @typed_addrspacecast(float addrspace(4)* %external, i1 %condition, i64 %index) {
entry:
  %local = alloca [4 x float], align 4
  br i1 %condition, label %local.path, label %external.path

local.path:
  %local.element = getelementptr inbounds [4 x float], [4 x float]* %local, i64 0, i64 %index
  %local.bytes = bitcast float* %local.element to i8*
  %local.generic = addrspacecast i8* %local.bytes to i8 addrspace(4)*
  br label %merge

external.path:
  %external.bytes = bitcast float addrspace(4)* %external to i8 addrspace(4)*
  br label %merge

merge:
  %pointer = phi i8 addrspace(4)* [ %local.generic, %local.path ], [ %external.bytes, %external.path ]
  %global = addrspacecast i8 addrspace(4)* %pointer to i8 addrspace(1)*
  %typed = bitcast i8 addrspace(1)* %global to float addrspace(1)*
  %value = load float, float addrspace(1)* %typed, align 4
  ret float %value
}
