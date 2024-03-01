;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys, llvm-14-plus
;
; RUN: igc_opt %s -S -o - --igc-bfloat-funcs-resolution | FileCheck %s

; This test verifies if bfloat OCL validation functions are lowered correctly.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: nounwind
declare spir_func i32 @_Z22__builtin_bf16_isequaltt(i16 zeroext, i16 zeroext) #0

; Function Attrs: nounwind
declare spir_func <2 x i32> @_Z22__builtin_bf16_isequalDv2_tS_(<2 x i16>, <2 x i16>) #0

; Function Attrs: nounwind
declare spir_func <4 x i32> @_Z22__builtin_bf16_isequalDv4_tS_(<4 x i16>, <4 x i16>) #0

; Function Attrs: nounwind
declare spir_func <8 x i32> @_Z22__builtin_bf16_isequalDv8_tS_(<8 x i16>, <8 x i16>) #0

; Function Attrs: nounwind
declare spir_func <16 x i32> @_Z22__builtin_bf16_isequalDv16_tS_(<16 x i16>, <16 x i16>) #0

; Function Attrs: convergent nounwind
; CHECK: define spir_kernel void @test_isequal
define spir_kernel void @test_isequal(i32 addrspace(1)* %out1, i16 zeroext %v1_1, i16 zeroext %v2_1, <2 x i32> addrspace(1)* %out2, <2 x i16> %v1_2, <2 x i16> %v2_2, <4 x i32> addrspace(1)* %out4, <4 x i16> %v1_4, <4 x i16> %v2_4, <8 x i32> addrspace(1)* %out8, <8 x i16> %v1_8, <8 x i16> %v2_8, <16 x i32> addrspace(1)* %out16, <16 x i16> %v1_16, <16 x i16> %v2_16) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[FCMPRES:.*]] = fcmp oeq bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = zext i1 %[[FCMPRES]] to i32
  %call = call spir_func i32 @_Z22__builtin_bf16_isequaltt(i16 zeroext %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out1, i64 0
  store i32 %call, i32 addrspace(1)* %arrayidx, align 4

; CHECK: %[[SRC0BF:.*]] = bitcast <2 x i16> %v1_2 to <2 x bfloat>
; CHECK: %[[SRC1BF:.*]] = bitcast <2 x i16> %v2_2 to <2 x bfloat>
; CHECK: %[[FCMPRES:.*]] = fcmp oeq <2 x bfloat> %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = zext <2 x i1> %[[FCMPRES]] to <2 x i32>
  %call1 = call spir_func <2 x i32> @_Z22__builtin_bf16_isequalDv2_tS_(<2 x i16> %v1_2, <2 x i16> %v2_2) #2
  %arrayidx2 = getelementptr inbounds <2 x i32>, <2 x i32> addrspace(1)* %out2, i64 1
  store <2 x i32> %call1, <2 x i32> addrspace(1)* %arrayidx2, align 8

; CHECK: %[[SRC0BF:.*]] = bitcast <4 x i16> %v1_4 to <4 x bfloat>
; CHECK: %[[SRC1BF:.*]] = bitcast <4 x i16> %v2_4 to <4 x bfloat>
; CHECK: %[[FCMPRES:.*]] = fcmp oeq <4 x bfloat> %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = zext <4 x i1> %[[FCMPRES]] to <4 x i32>
  %call3 = call spir_func <4 x i32> @_Z22__builtin_bf16_isequalDv4_tS_(<4 x i16> %v1_4, <4 x i16> %v2_4) #2
  %arrayidx4 = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %out4, i64 2
  store <4 x i32> %call3, <4 x i32> addrspace(1)* %arrayidx4, align 16

; CHECK: %[[SRC0BF:.*]] = bitcast <8 x i16> %v1_8 to <8 x bfloat>
; CHECK: %[[SRC1BF:.*]] = bitcast <8 x i16> %v2_8 to <8 x bfloat>
; CHECK: %[[FCMPRES:.*]] = fcmp oeq <8 x bfloat> %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = zext <8 x i1> %[[FCMPRES]] to <8 x i32>
  %call5 = call spir_func <8 x i32> @_Z22__builtin_bf16_isequalDv8_tS_(<8 x i16> %v1_8, <8 x i16> %v2_8) #2
  %arrayidx6 = getelementptr inbounds <8 x i32>, <8 x i32> addrspace(1)* %out8, i64 3
  store <8 x i32> %call5, <8 x i32> addrspace(1)* %arrayidx6, align 32

; CHECK: %[[SRC0BF:.*]] = bitcast <16 x i16> %v1_16 to <16 x bfloat>
; CHECK: %[[SRC1BF:.*]] = bitcast <16 x i16> %v2_16 to <16 x bfloat>
; CHECK: %[[FCMPRES:.*]] = fcmp oeq <16 x bfloat> %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = zext <16 x i1> %[[FCMPRES]] to <16 x i32>
  %call7 = call spir_func <16 x i32> @_Z22__builtin_bf16_isequalDv16_tS_(<16 x i16> %v1_16, <16 x i16> %v2_16) #2
  %arrayidx8 = getelementptr inbounds <16 x i32>, <16 x i32> addrspace(1)* %out16, i64 4
  store <16 x i32> %call7, <16 x i32> addrspace(1)* %arrayidx8, align 64
  ret void
}


; Function Attrs: nounwind
declare spir_func i32 @_Z24__builtin_bf16_isgreatertt(i16 zeroext, i16 zeroext) #0

; Function Attrs: convergent nounwind
; CHECK: define spir_kernel void @test_isgreater
define spir_kernel void @test_isgreater(i32 addrspace(1)* %out1, i16 zeroext %v1_1, i16 zeroext %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[FCMPRES:.*]] = fcmp ogt bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = zext i1 %[[FCMPRES]] to i32
  %call = call spir_func i32 @_Z24__builtin_bf16_isgreatertt(i16 zeroext %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out1, i64 0
  store i32 %call, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: nounwind
declare spir_func i32 @_Z21__builtin_bf16_islesstt(i16 zeroext, i16 zeroext) #0

; Function Attrs: convergent nounwind
; CHECK: define spir_kernel void @test_isless
define spir_kernel void @test_isless(i32 addrspace(1)* %out1, i16 zeroext %v1_1, i16 zeroext %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[FCMPRES:.*]] = fcmp olt bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = zext i1 %[[FCMPRES]] to i32
  %call = call spir_func i32 @_Z21__builtin_bf16_islesstt(i16 zeroext %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out1, i64 0
  store i32 %call, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: nounwind
declare spir_func i32 @_Z25__builtin_bf16_isnotequaltt(i16 zeroext, i16 zeroext) #0

; Function Attrs: convergent nounwind
; CHECK: define spir_kernel void @test_isnotequal
define spir_kernel void @test_isnotequal(i32 addrspace(1)* %out1, i16 zeroext %v1_1, i16 zeroext %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[FCMPRES:.*]] = fcmp one bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = zext i1 %[[FCMPRES]] to i32
  %call = call spir_func i32 @_Z25__builtin_bf16_isnotequaltt(i16 zeroext %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out1, i64 0
  store i32 %call, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: nounwind
declare spir_func i32 @_Z26__builtin_bf16_islessequaltt(i16 zeroext, i16 zeroext) #0

; Function Attrs: convergent nounwind
; CHECK: define spir_kernel void @test_islessequal
define spir_kernel void @test_islessequal(i32 addrspace(1)* %out1, i16 zeroext %v1_1, i16 zeroext %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[FCMPRES:.*]] = fcmp ole bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = zext i1 %[[FCMPRES]] to i32
  %call = call spir_func i32 @_Z26__builtin_bf16_islessequaltt(i16 zeroext %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out1, i64 0
  store i32 %call, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: nounwind
declare spir_func i32 @_Z29__builtin_bf16_isgreaterequaltt(i16 zeroext, i16 zeroext) #0

; Function Attrs: convergent nounwind
; CHECK: define spir_kernel void @test_isgreaterequal
define spir_kernel void @test_isgreaterequal(i32 addrspace(1)* %out1, i16 zeroext %v1_1, i16 zeroext %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[FCMPRES:.*]] = fcmp oge bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = zext i1 %[[FCMPRES]] to i32
  %call = call spir_func i32 @_Z29__builtin_bf16_isgreaterequaltt(i16 zeroext %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out1, i64 0
  store i32 %call, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

declare spir_func i32 @_Z26__builtin_bf16_isunorderedtt(i16 zeroext, i16 zeroext) #0
; Function Attrs: convergent nounwind
; CHECK: define spir_kernel void @test_isunordered
define spir_kernel void @test_isunordered(i32 addrspace(1)* %out1, i16 zeroext %v1_1, i16 zeroext %v2_1) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[FCMPRES:.*]] = fcmp uno bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %{{.*}} = zext i1 %[[FCMPRES]] to i32
  %call = call spir_func i32 @_Z26__builtin_bf16_isunorderedtt(i16 zeroext %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %out1, i64 0
  store i32 %call, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

attributes #0 = { nounwind "referenced-indirectly" "visaStackCall" }
attributes #1 = { convergent nounwind }
attributes #2 = { nounwind }