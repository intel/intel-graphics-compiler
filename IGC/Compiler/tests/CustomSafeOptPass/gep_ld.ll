;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-custom-safe-opt -instcombine -S < %s | FileCheck %s
; ------------------------------------------------
; CustomSafeOptPass: GEP + load
; ------------------------------------------------

; Test checks that sequence:
; %gep = getelementptr %struct.quux, %struct.quux addrspace(3)* %base, i64 0, i32 0
; %val = load %struct.quux addrspace(3)*, %struct.quux addrspace(3)* addrspace(3)* %gep
; is converted to ->
; %val = load %struct.quux addrspace(3)*, %struct.quux addrspace(3)* addrspace(3)* %base

%struct.quux = type { %struct.quux addrspace(3)* }

@global = addrspace(3) global [2 x %struct.quux] zeroinitializer, section "localSLM", align 8

; Function Attrs: convergent
define spir_kernel void @eggs(<8 x i32> %arg, <3 x i32> %arg1) #0 {
; CHECK-LABEL: define spir_kernel void @eggs(
; CHECK-SAME: <8 x i32> [[ARG:%.*]], <3 x i32> [[ARG1:%.*]]){{.*}}{
; CHECK:      bb:
; CHECK-NEXT: [[TMP0:%.*]] = load {{.*}} bitcast ([2 x %struct.quux] addrspace(3)* @global to {{.*}}), align 8
; CHECK-NEXT: [[TMP1:%.*]] = load {{.*}}, {{.*}} [[TMP0]], align 8
; CHECK-NEXT: [[TMP2:%.*]] = load {{.*}}, {{.*}} [[TMP1]], align 8
; CHECK-NOT:  getelementptr %struct.quux, %struct.quux addrspace(3)* {{.*}}, i64 0, i32 0
; CHECK-NEXT: [[TMP3:%.*]] = load %struct.quux addrspace(3)*, %struct.quux addrspace(3)* addrspace(3)* [[TMP2]], align 8
; CHECK-NEXT: [[GEP:%.*]] = getelementptr %struct.quux, %struct.quux addrspace(3)* [[TMP3]], i64 0, i32 0
; CHECK-NEXT: store %struct.quux addrspace(3)* [[TMP3]], %struct.quux addrspace(3)* addrspace(3)* [[GEP]], align 8
; CHECK-NEXT: ret void
bb:
  %tmp = load %struct.quux addrspace(3)*, %struct.quux addrspace(3)* addrspace(3)* getelementptr inbounds ([2 x %struct.quux], [2 x %struct.quux] addrspace(3)* @global, i64 0, i64 0, i32 0), align 8
  %tmp3 = getelementptr %struct.quux, %struct.quux addrspace(3)* %tmp, i64 0, i32 0
  %tmp4 = load %struct.quux addrspace(3)*, %struct.quux addrspace(3)* addrspace(3)* %tmp3, align 8
  %tmp5 = getelementptr %struct.quux, %struct.quux addrspace(3)* %tmp4, i64 0, i32 0
  %tmp6 = load %struct.quux addrspace(3)*, %struct.quux addrspace(3)* addrspace(3)* %tmp5, align 8
  %tmp7 = getelementptr %struct.quux, %struct.quux addrspace(3)* %tmp6, i64 0, i32 0
  %tmp8 = load %struct.quux addrspace(3)*, %struct.quux addrspace(3)* addrspace(3)* %tmp7, align 8
  %tmp9 = getelementptr %struct.quux, %struct.quux addrspace(3)* %tmp8, i64 0, i32 0
  store %struct.quux addrspace(3)* %tmp8, %struct.quux addrspace(3)* addrspace(3)* %tmp9, align 8
  ret void
}

