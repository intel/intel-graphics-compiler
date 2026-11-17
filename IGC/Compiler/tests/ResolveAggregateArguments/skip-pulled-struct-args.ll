;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-agg-arg-analysis -igc-add-implicit-args -igc-agg-arg -S < %s | FileCheck %s

; Test that AggregateArguments skips struct arguments listed in FuncMD pulledArgs:
; a pulled struct gets no implicit args and its uses are not rewritten.
; k_ref has the same arguments without pulledArgs, so all structs are split there.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%struct.K = type { i32, float }
%struct.S = type { float, i32 }

; %s and %v are pulled, only %k is split into implicit args
define spir_kernel void @k_pulled(ptr byval(%struct.K) %k, ptr byval(%struct.S) %s, %struct.S %v, ptr addrspace(1) %out) {
  %vk = load i32, ptr %k, align 4
  %ps = getelementptr inbounds %struct.S, ptr %s, i32 0, i32 1
  %vs = load i32, ptr %ps, align 4
  %vv = extractvalue %struct.S %v, 1
  %r0 = add i32 %vk, %vs
  %r = add i32 %r0, %vv
  store i32 %r, ptr addrspace(1) %out, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @k_pulled(ptr byval(%struct.K) %k, ptr byval(%struct.S) %s, %struct.S %v, ptr addrspace(1) %out, i32 %const_reg_dword, float %const_reg_fp32)
; CHECK-NEXT:  %k_alloca = alloca %struct.K, align 8
; CHECK-NEXT:  [[K0:%.*]] = getelementptr i8, ptr %k_alloca, i32 0
; CHECK-NEXT:  store i32 %const_reg_dword, ptr [[K0]], align 4
; CHECK-NEXT:  [[K1:%.*]] = getelementptr i8, ptr %k_alloca, i32 4
; CHECK-NEXT:  store float %const_reg_fp32, ptr [[K1]], align 4
; CHECK-NEXT:  %vk = load i32, ptr %k_alloca, align 4
; CHECK-NEXT:  %ps = getelementptr inbounds %struct.S, ptr %s, i32 0, i32 1
; CHECK-NEXT:  %vs = load i32, ptr %ps, align 4
; CHECK-NEXT:  %vv = extractvalue %struct.S %v, 1
; CHECK-NEXT:  %r0 = add i32 %vk, %vs
; CHECK-NEXT:  %r = add i32 %r0, %vv
; CHECK-NEXT:  store i32 %r, ptr addrspace(1) %out, align 4
; CHECK-NEXT:  ret void


; Same arguments without pulledArgs: all three structs are split
define spir_kernel void @k_ref(ptr byval(%struct.K) %k, ptr byval(%struct.S) %s, %struct.S %v, ptr addrspace(1) %out) {
  %vk = load i32, ptr %k, align 4
  %ps = getelementptr inbounds %struct.S, ptr %s, i32 0, i32 1
  %vs = load i32, ptr %ps, align 4
  %vv = extractvalue %struct.S %v, 1
  %r0 = add i32 %vk, %vs
  %r = add i32 %r0, %vv
  store i32 %r, ptr addrspace(1) %out, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @k_ref(ptr byval(%struct.K) %k, ptr byval(%struct.S) %s, %struct.S %v, ptr addrspace(1) %out, i32 %const_reg_dword, float %const_reg_fp32, float %const_reg_fp321, i32 %const_reg_dword2, float %const_reg_fp323, i32 %const_reg_dword4)
; CHECK:       %k_alloca = alloca %struct.K, align 8
; CHECK:       %s_alloca = alloca %struct.S, align 8
; CHECK:       %v_alloca = alloca %struct.S, align 8
; CHECK:       %vk = load i32, ptr %k_alloca, align 4
; CHECK-NEXT:  %ps = getelementptr inbounds %struct.S, ptr %s_alloca, i32 0, i32 1
; CHECK-NEXT:  %vs = load i32, ptr %ps, align 4
; CHECK-NEXT:  [[VG:%.*]] = getelementptr inbounds %struct.S, ptr %v_alloca, i32 0, i32 1
; CHECK-NEXT:  %vv = load i32, ptr [[VG]], align 4

!igc.functions = !{!0, !3}
!IGCMetadata = !{!10}

!0 = !{ptr @k_pulled, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @k_ref, !1}
!10 = !{!"ModuleMD", !11}
!11 = !{!"FuncMD", !12, !13, !14, !15}
!12 = distinct !{!"FuncMDMap[0]", ptr @k_pulled}
!13 = !{!"FuncMDValue[0]", !20, !21}
!14 = distinct !{!"FuncMDMap[1]", ptr @k_ref}
!15 = !{!"FuncMDValue[1]", !20}
!20 = !{!"functionType", !"KernelFunction"}
!21 = !{!"pulledArgs", !22, !23}
!22 = !{!"pulledArgsVec[0]", i32 1}
!23 = !{!"pulledArgsVec[1]", i32 2}
