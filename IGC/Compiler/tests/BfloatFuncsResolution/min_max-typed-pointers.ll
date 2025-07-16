;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-14-plus
;
; RUN: igc_opt %s -S -o - --igc-bfloat-funcs-resolution | FileCheck %s

; This test verifies if bfloat OCL validation functions are lowered correctly.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: nounwind
declare spir_func zeroext i16 @_Z18__builtin_bf16_mintt(i16 zeroext, i16 zeroext) #0

; Function Attrs: nounwind
declare spir_func <2 x i16> @_Z18__builtin_bf16_minDv2_tS_(<2 x i16>, <2 x i16>) #0

; Function Attrs: nounwind
declare spir_func <4 x i16> @_Z18__builtin_bf16_minDv4_tS_(<4 x i16>, <4 x i16>) #0

; Function Attrs: nounwind
declare spir_func <8 x i16> @_Z18__builtin_bf16_minDv8_tS_(<8 x i16>, <8 x i16>) #0

; Function Attrs: nounwind
declare spir_func <16 x i16> @_Z18__builtin_bf16_minDv16_tS_(<16 x i16>, <16 x i16>) #0

; Function Attrs: nounwind
declare spir_func zeroext i16 @_Z18__builtin_bf16_maxtt(i16 zeroext, i16 zeroext) #0

; Function Attrs: nounwind
declare spir_func <2 x i16> @_Z18__builtin_bf16_maxDv2_tS_(<2 x i16>, <2 x i16>) #0

; Function Attrs: nounwind
declare spir_func <4 x i16> @_Z18__builtin_bf16_maxDv4_tS_(<4 x i16>, <4 x i16>) #0

; Function Attrs: nounwind
declare spir_func <8 x i16> @_Z18__builtin_bf16_maxDv8_tS_(<8 x i16>, <8 x i16>) #0

; Function Attrs: nounwind
declare spir_func <16 x i16> @_Z18__builtin_bf16_maxDv16_tS_(<16 x i16>, <16 x i16>) #0

; Function Attrs: convergent nounwind
; CHECK: define spir_kernel void @test_min
define spir_kernel void @test_min(i16 addrspace(1)* %out1, i16 zeroext %v1_1, i16 zeroext %v2_1, <2 x i16> addrspace(1)* %out2, <2 x i16> %v1_2, <2 x i16> %v2_2, <4 x i16> addrspace(1)* %out4, <4 x i16> %v1_4, <4 x i16> %v2_4, <8 x i16> addrspace(1)* %out8, <8 x i16> %v1_8, <8 x i16> %v2_8, <16 x i16> addrspace(1)* %out16, <16 x i16> %v1_16, <16 x i16> %v2_16) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[COND1:.*]] = fcmp olt bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[SELECT1RES:.*]] = select i1 %[[COND1]], bfloat %[[SRC0BF]], bfloat %[[SRC1BF]]
; CHECK: %[[COND2:.*]] = fcmp uno bfloat %[[SRC0BF]], %[[SRC0BF]]
; CHECK: %[[SELECT2RES:.*]] = select i1 %[[COND2]], bfloat %[[SRC1BF]], bfloat %[[SRC0BF]]
; CHECK: %[[COND3:.*]] = fcmp ord bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[SELECT3RES:.*]] = select i1 %[[COND3]], bfloat %[[SELECT1RES]], bfloat %[[SELECT2RES]]
; CHECK: %{{.*}} = bitcast bfloat %[[SELECT3RES]] to i16
  %call = call spir_func zeroext i16 @_Z18__builtin_bf16_mintt(i16 zeroext %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds i16, i16 addrspace(1)* %out1, i64 0
  store i16 %call, i16 addrspace(1)* %arrayidx, align 2

; CHECK: %[[SRC0BF:.*]] = bitcast <2 x i16> %v1_2 to <2 x bfloat>
; CHECK: %[[SRC1BF:.*]] = bitcast <2 x i16> %v2_2 to <2 x bfloat>
; CHECK: %[[COND1:.*]] = fcmp olt <2 x bfloat> %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[SELECT1RES:.*]] = select <2 x i1> %[[COND1]], <2 x bfloat> %[[SRC0BF]], <2 x bfloat> %[[SRC1BF]]
; CHECK: %[[COND2:.*]] = fcmp uno <2 x bfloat> %[[SRC0BF]], %[[SRC0BF]]
; CHECK: %[[SELECT2RES:.*]] = select <2 x i1> %[[COND2]], <2 x bfloat> %[[SRC1BF]], <2 x bfloat> %[[SRC0BF]]
; CHECK: %[[COND3:.*]] = fcmp ord <2 x bfloat> %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[SELECT3RES:.*]] = select <2 x i1> %[[COND3]], <2 x bfloat> %[[SELECT1RES]], <2 x bfloat> %[[SELECT2RES]]
; CHECK: %{{.*}} = bitcast <2 x bfloat> %[[SELECT3RES]] to <2 x i16>
  %call1 = call spir_func <2 x i16> @_Z18__builtin_bf16_minDv2_tS_(<2 x i16> %v1_2, <2 x i16> %v2_2) #2
  %arrayidx2 = getelementptr inbounds <2 x i16>, <2 x i16> addrspace(1)* %out2, i64 1
  store <2 x i16> %call1, <2 x i16> addrspace(1)* %arrayidx2, align 4

; CHECK: %[[SRC0BF:.*]] = bitcast <4 x i16> %v1_4 to <4 x bfloat>
; CHECK: %[[SRC1BF:.*]] = bitcast <4 x i16> %v2_4 to <4 x bfloat>
; CHECK: %[[COND1:.*]] = fcmp olt <4 x bfloat> %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[SELECT1RES:.*]] = select <4 x i1> %[[COND1]], <4 x bfloat> %[[SRC0BF]], <4 x bfloat> %[[SRC1BF]]
; CHECK: %[[COND2:.*]] = fcmp uno <4 x bfloat> %[[SRC0BF]], %[[SRC0BF]]
; CHECK: %[[SELECT2RES:.*]] = select <4 x i1> %[[COND2]], <4 x bfloat> %[[SRC1BF]], <4 x bfloat> %[[SRC0BF]]
; CHECK: %[[COND3:.*]] = fcmp ord <4 x bfloat> %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[SELECT3RES:.*]] = select <4 x i1> %[[COND3]], <4 x bfloat> %[[SELECT1RES]], <4 x bfloat> %[[SELECT2RES]]
; CHECK: %{{.*}} = bitcast <4 x bfloat> %[[SELECT3RES]] to <4 x i16>
  %call3 = call spir_func <4 x i16> @_Z18__builtin_bf16_minDv4_tS_(<4 x i16> %v1_4, <4 x i16> %v2_4) #2
  %arrayidx4 = getelementptr inbounds <4 x i16>, <4 x i16> addrspace(1)* %out4, i64 2
  store <4 x i16> %call3, <4 x i16> addrspace(1)* %arrayidx4, align 8

; CHECK: %[[SRC0BF:.*]] = bitcast <8 x i16> %v1_8 to <8 x bfloat>
; CHECK: %[[SRC1BF:.*]] = bitcast <8 x i16> %v2_8 to <8 x bfloat>
; CHECK: %[[COND1:.*]] = fcmp olt <8 x bfloat> %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[SELECT1RES:.*]] = select <8 x i1> %[[COND1]], <8 x bfloat> %[[SRC0BF]], <8 x bfloat> %[[SRC1BF]]
; CHECK: %[[COND2:.*]] = fcmp uno <8 x bfloat> %[[SRC0BF]], %[[SRC0BF]]
; CHECK: %[[SELECT2RES:.*]] = select <8 x i1> %[[COND2]], <8 x bfloat> %[[SRC1BF]], <8 x bfloat> %[[SRC0BF]]
; CHECK: %[[COND3:.*]] = fcmp ord <8 x bfloat> %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[SELECT3RES:.*]] = select <8 x i1> %[[COND3]], <8 x bfloat> %[[SELECT1RES]], <8 x bfloat> %[[SELECT2RES]]
; CHECK: %{{.*}} = bitcast <8 x bfloat> %[[SELECT3RES]] to <8 x i16>
  %call5 = call spir_func <8 x i16> @_Z18__builtin_bf16_minDv8_tS_(<8 x i16> %v1_8, <8 x i16> %v2_8) #2
  %arrayidx6 = getelementptr inbounds <8 x i16>, <8 x i16> addrspace(1)* %out8, i64 3
  store <8 x i16> %call5, <8 x i16> addrspace(1)* %arrayidx6, align 16

; CHECK: %[[SRC0BF:.*]] = bitcast <16 x i16> %v1_16 to <16 x bfloat>
; CHECK: %[[SRC1BF:.*]] = bitcast <16 x i16> %v2_16 to <16 x bfloat>
; CHECK: %[[COND1:.*]] = fcmp olt <16 x bfloat> %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[SELECT1RES:.*]] = select <16 x i1> %[[COND1]], <16 x bfloat> %[[SRC0BF]], <16 x bfloat> %[[SRC1BF]]
; CHECK: %[[COND2:.*]] = fcmp uno <16 x bfloat> %[[SRC0BF]], %[[SRC0BF]]
; CHECK: %[[SELECT2RES:.*]] = select <16 x i1> %[[COND2]], <16 x bfloat> %[[SRC1BF]], <16 x bfloat> %[[SRC0BF]]
; CHECK: %[[COND3:.*]] = fcmp ord <16 x bfloat> %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[SELECT3RES:.*]] = select <16 x i1> %[[COND3]], <16 x bfloat> %[[SELECT1RES]], <16 x bfloat> %[[SELECT2RES]]
; CHECK: %{{.*}} = bitcast <16 x bfloat> %[[SELECT3RES]] to <16 x i16>
  %call7 = call spir_func <16 x i16> @_Z18__builtin_bf16_minDv16_tS_(<16 x i16> %v1_16, <16 x i16> %v2_16) #2
  %arrayidx8 = getelementptr inbounds <16 x i16>, <16 x i16> addrspace(1)* %out16, i64 4
  store <16 x i16> %call7, <16 x i16> addrspace(1)* %arrayidx8, align 32
  ret void
}

; Function Attrs: convergent nounwind
; CHECK: define spir_kernel void @test_max
define spir_kernel void @test_max(i16 addrspace(1)* %out1, i16 zeroext %v1_1, i16 zeroext %v2_1, <2 x i16> addrspace(1)* %out2, <2 x i16> %v1_2, <2 x i16> %v2_2, <4 x i16> addrspace(1)* %out4, <4 x i16> %v1_4, <4 x i16> %v2_4, <8 x i16> addrspace(1)* %out8, <8 x i16> %v1_8, <8 x i16> %v2_8, <16 x i16> addrspace(1)* %out16, <16 x i16> %v1_16, <16 x i16> %v2_16) #1 {
entry:
; CHECK: %[[SRC0BF:.*]] = bitcast i16 %v1_1 to bfloat
; CHECK: %[[SRC1BF:.*]] = bitcast i16 %v2_1 to bfloat
; CHECK: %[[COND1:.*]] = fcmp ogt bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[SELECT1RES:.*]] = select i1 %[[COND1]], bfloat %[[SRC0BF]], bfloat %[[SRC1BF]]
; CHECK: %[[COND2:.*]] = fcmp uno bfloat %[[SRC0BF]], %[[SRC0BF]]
; CHECK: %[[SELECT2RES:.*]] = select i1 %[[COND2]], bfloat %[[SRC1BF]], bfloat %[[SRC0BF]]
; CHECK: %[[COND3:.*]] = fcmp ord bfloat %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[SELECT3RES:.*]] = select i1 %[[COND3]], bfloat %[[SELECT1RES]], bfloat %[[SELECT2RES]]
; CHECK: %{{.*}} = bitcast bfloat %[[SELECT3RES]] to i16
  %call = call spir_func zeroext i16 @_Z18__builtin_bf16_maxtt(i16 zeroext %v1_1, i16 zeroext %v2_1) #2
  %arrayidx = getelementptr inbounds i16, i16 addrspace(1)* %out1, i64 0
  store i16 %call, i16 addrspace(1)* %arrayidx, align 2

; CHECK: %[[SRC0BF:.*]] = bitcast <2 x i16> %v1_2 to <2 x bfloat>
; CHECK: %[[SRC1BF:.*]] = bitcast <2 x i16> %v2_2 to <2 x bfloat>
; CHECK: %[[COND1:.*]] = fcmp ogt <2 x bfloat> %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[SELECT1RES:.*]] = select <2 x i1> %[[COND1]], <2 x bfloat> %[[SRC0BF]], <2 x bfloat> %[[SRC1BF]]
; CHECK: %[[COND2:.*]] = fcmp uno <2 x bfloat> %[[SRC0BF]], %[[SRC0BF]]
; CHECK: %[[SELECT2RES:.*]] = select <2 x i1> %[[COND2]], <2 x bfloat> %[[SRC1BF]], <2 x bfloat> %[[SRC0BF]]
; CHECK: %[[COND3:.*]] = fcmp ord <2 x bfloat> %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[SELECT3RES:.*]] = select <2 x i1> %[[COND3]], <2 x bfloat> %[[SELECT1RES]], <2 x bfloat> %[[SELECT2RES]]
; CHECK: %{{.*}} = bitcast <2 x bfloat> %[[SELECT3RES]] to <2 x i16>
  %call1 = call spir_func <2 x i16> @_Z18__builtin_bf16_maxDv2_tS_(<2 x i16> %v1_2, <2 x i16> %v2_2) #2
  %arrayidx2 = getelementptr inbounds <2 x i16>, <2 x i16> addrspace(1)* %out2, i64 1
  store <2 x i16> %call1, <2 x i16> addrspace(1)* %arrayidx2, align 4

; CHECK: %[[SRC0BF:.*]] = bitcast <4 x i16> %v1_4 to <4 x bfloat>
; CHECK: %[[SRC1BF:.*]] = bitcast <4 x i16> %v2_4 to <4 x bfloat>
; CHECK: %[[COND1:.*]] = fcmp ogt <4 x bfloat> %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[SELECT1RES:.*]] = select <4 x i1> %[[COND1]], <4 x bfloat> %[[SRC0BF]], <4 x bfloat> %[[SRC1BF]]
; CHECK: %[[COND2:.*]] = fcmp uno <4 x bfloat> %[[SRC0BF]], %[[SRC0BF]]
; CHECK: %[[SELECT2RES:.*]] = select <4 x i1> %[[COND2]], <4 x bfloat> %[[SRC1BF]], <4 x bfloat> %[[SRC0BF]]
; CHECK: %[[COND3:.*]] = fcmp ord <4 x bfloat> %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[SELECT3RES:.*]] = select <4 x i1> %[[COND3]], <4 x bfloat> %[[SELECT1RES]], <4 x bfloat> %[[SELECT2RES]]
; CHECK: %{{.*}} = bitcast <4 x bfloat> %[[SELECT3RES]] to <4 x i16>
  %call3 = call spir_func <4 x i16> @_Z18__builtin_bf16_maxDv4_tS_(<4 x i16> %v1_4, <4 x i16> %v2_4) #2
  %arrayidx4 = getelementptr inbounds <4 x i16>, <4 x i16> addrspace(1)* %out4, i64 2
  store <4 x i16> %call3, <4 x i16> addrspace(1)* %arrayidx4, align 8

; CHECK: %[[SRC0BF:.*]] = bitcast <8 x i16> %v1_8 to <8 x bfloat>
; CHECK: %[[SRC1BF:.*]] = bitcast <8 x i16> %v2_8 to <8 x bfloat>
; CHECK: %[[COND1:.*]] = fcmp ogt <8 x bfloat> %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[SELECT1RES:.*]] = select <8 x i1> %[[COND1]], <8 x bfloat> %[[SRC0BF]], <8 x bfloat> %[[SRC1BF]]
; CHECK: %[[COND2:.*]] = fcmp uno <8 x bfloat> %[[SRC0BF]], %[[SRC0BF]]
; CHECK: %[[SELECT2RES:.*]] = select <8 x i1> %[[COND2]], <8 x bfloat> %[[SRC1BF]], <8 x bfloat> %[[SRC0BF]]
; CHECK: %[[COND3:.*]] = fcmp ord <8 x bfloat> %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[SELECT3RES:.*]] = select <8 x i1> %[[COND3]], <8 x bfloat> %[[SELECT1RES]], <8 x bfloat> %[[SELECT2RES]]
; CHECK: %{{.*}} = bitcast <8 x bfloat> %[[SELECT3RES]] to <8 x i16>
  %call5 = call spir_func <8 x i16> @_Z18__builtin_bf16_maxDv8_tS_(<8 x i16> %v1_8, <8 x i16> %v2_8) #2
  %arrayidx6 = getelementptr inbounds <8 x i16>, <8 x i16> addrspace(1)* %out8, i64 3
  store <8 x i16> %call5, <8 x i16> addrspace(1)* %arrayidx6, align 16

; CHECK: %[[SRC0BF:.*]] = bitcast <16 x i16> %v1_16 to <16 x bfloat>
; CHECK: %[[SRC1BF:.*]] = bitcast <16 x i16> %v2_16 to <16 x bfloat>
; CHECK: %[[COND1:.*]] = fcmp ogt <16 x bfloat> %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[SELECT1RES:.*]] = select <16 x i1> %[[COND1]], <16 x bfloat> %[[SRC0BF]], <16 x bfloat> %[[SRC1BF]]
; CHECK: %[[COND2:.*]] = fcmp uno <16 x bfloat> %[[SRC0BF]], %[[SRC0BF]]
; CHECK: %[[SELECT2RES:.*]] = select <16 x i1> %[[COND2]], <16 x bfloat> %[[SRC1BF]], <16 x bfloat> %[[SRC0BF]]
; CHECK: %[[COND3:.*]] = fcmp ord <16 x bfloat> %[[SRC0BF]], %[[SRC1BF]]
; CHECK: %[[SELECT3RES:.*]] = select <16 x i1> %[[COND3]], <16 x bfloat> %[[SELECT1RES]], <16 x bfloat> %[[SELECT2RES]]
; CHECK: %{{.*}} = bitcast <16 x bfloat> %[[SELECT3RES]] to <16 x i16>
  %call7 = call spir_func <16 x i16> @_Z18__builtin_bf16_maxDv16_tS_(<16 x i16> %v1_16, <16 x i16> %v2_16) #2
  %arrayidx8 = getelementptr inbounds <16 x i16>, <16 x i16> addrspace(1)* %out16, i64 4
  store <16 x i16> %call7, <16 x i16> addrspace(1)* %arrayidx8, align 32
  ret void
}

attributes #0 = { nounwind "referenced-indirectly" "visaStackCall" }
attributes #1 = { convergent nounwind }
attributes #2 = { nounwind }
