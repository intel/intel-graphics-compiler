;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXStructSplitter -dce -vc-struct-splitting=1 -march=genx64 -mcpu=XeLP -S < %s | FileCheck %s --check-prefixes=CHECK

%struct1 = type { [6 x %struct2] }
%struct2 = type { [129 x float] }

; CHECK-LABEL: @test1
define dllexport spir_kernel void @test1(float %val, i64 %idx1, i64 %idx2) {
entry:
; CHECK: [[ALLOCA_F:%.*]] = alloca [6 x %struct2]
  %alloca = alloca %struct1
; CHECK-NEXT: [[GEP1_SPLIT:%.*]] = getelementptr [6 x %struct2], ptr [[ALLOCA_F]], i64 0, i64 %idx1
  %gep1 = getelementptr [6 x %struct2], ptr %alloca, i64 0, i64 %idx1
; CHECK-NEXT: [[GEP2:%.*]] = getelementptr [129 x float], ptr [[GEP1_SPLIT]], i64 0, i64 %idx2
  %gep2 = getelementptr [129 x float], ptr %gep1, i64 0, i64 %idx2
; CHECK-NEXT: store float %val, ptr [[GEP2]]
  store float %val, ptr %gep2
; CHECK-NEXT: ret void
  ret void
}

; CHECK-LABEL: @test2
define dllexport spir_kernel void @test2(float %val, i64 %idx) {
entry:
; CHECK: [[ALLOCA_F:%.*]] = alloca [6 x %struct2]
  %alloca = alloca %struct1
; CHECK-NEXT: [[GEP_SPLIT:%.*]] = getelementptr [129 x float], ptr [[ALLOCA_F]], i64 0, i64 %idx
  %gep = getelementptr [129 x float], ptr %alloca, i64 0, i64 %idx
; CHECK-NEXT: store float %val, ptr [[GEP_SPLIT]]
  store float %val, ptr %gep
; CHECK-NEXT: ret void
  ret void
}

