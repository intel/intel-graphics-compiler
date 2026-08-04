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
; Test handling of the alloca incoming value that is stripped back from
; i8 addrspace(4)* to float*. Replaying the post-PHI byte GEP must restore an i8 pointer in the alloca
; address space before creating the GEP.

; CHECK-LABEL: define float @typed_const_gep(
; CHECK-LABEL: local.path:
; CHECK:       [[LOCAL_BYTES:%.*]] = bitcast float* %local.element to i8*
; CHECK-NEXT:  [[LOCAL_OFFSET:%.*]] = getelementptr inbounds i8, i8* [[LOCAL_BYTES]], i64 4
; CHECK-NEXT:  [[LOCAL_TYPED:%.*]] = bitcast i8* [[LOCAL_OFFSET]] to float*
; CHECK-NEXT:  [[LOCAL_LOAD:%.*]] = load float, float* [[LOCAL_TYPED]], align 4
; CHECK-LABEL: external.path:
; CHECK:       [[EXTERNAL_OFFSET:%.*]] = getelementptr inbounds i8, i8 addrspace(4)* %external.bytes, i64 4
; CHECK-NEXT:  [[EXTERNAL_TYPED:%.*]] = bitcast i8 addrspace(4)* [[EXTERNAL_OFFSET]] to float addrspace(4)*
; CHECK-NEXT:  [[EXTERNAL_LOAD:%.*]] = load float, float addrspace(4)* [[EXTERNAL_TYPED]], align 4
; CHECK-LABEL: merge:
; CHECK:       [[VALUE:%.*]] = phi float [ [[LOCAL_LOAD]], %local.path ], [ [[EXTERNAL_LOAD]], %external.path ]
; CHECK-NEXT:  ret float [[VALUE]]
; CHECK-NOT:   phi i8 addrspace(4)*

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-n8:16:32"
target triple = "spir64-unknown-unknown"

define float @typed_const_gep(float addrspace(4)* %external, i1 %condition, i64 %index) {
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
  %offset = getelementptr inbounds i8, i8 addrspace(4)* %pointer, i64 4
  %typed = bitcast i8 addrspace(4)* %offset to float addrspace(4)*
  %value = load float, float addrspace(4)* %typed, align 4
  ret float %value
}
