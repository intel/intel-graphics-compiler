;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --opaque-pointers -igc-add-implicit-args -S < %s | FileCheck %s

; Test that in bindless advanced mode AddImplicitArgs does not add a bindless offset
; for pointer arguments listed in FuncMD pulledArgs. A pulled pointer is read from
; cross-thread memory, so it cannot be promoted to bindless.
; k_ref has the same arguments without pulledArgs, so every pointer gets a bindless offset.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; %p and %c are pulled, only %out gets a bindless offset
define spir_kernel void @k_pulled(ptr addrspace(1) %out, ptr addrspace(1) %p, ptr addrspace(2) %c) {
  %p_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 1)
  %p.val = load ptr addrspace(1), ptr addrspace(5) %p_pulled, align 8
  %c_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 2)
  %c.val = load ptr addrspace(2), ptr addrspace(5) %c_pulled, align 8
  %v = load i32, ptr addrspace(2) %c.val, align 4
  store i32 %v, ptr addrspace(1) %out, align 4
  store i32 %v, ptr addrspace(1) %p.val, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @k_pulled(ptr addrspace(1) %out, ptr addrspace(1) %p, ptr addrspace(2) %c, i32 %bindlessOffset) {

; Same arguments without pulledArgs: %out, %p and %c get a bindless offset
define spir_kernel void @k_ref(ptr addrspace(1) %out, ptr addrspace(1) %p, ptr addrspace(2) %c) {
  %v = load i32, ptr addrspace(2) %c, align 4
  store i32 %v, ptr addrspace(1) %out, align 4
  store i32 %v, ptr addrspace(1) %p, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @k_ref(ptr addrspace(1) %out, ptr addrspace(1) %p, ptr addrspace(2) %c, i32 %bindlessOffset, i32 %bindlessOffset1, i32 %bindlessOffset2) {

declare ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32)

!igc.functions = !{!0, !3}
!IGCMetadata = !{!10}

!0 = !{ptr @k_pulled, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @k_ref, !1}
!10 = !{!"ModuleMD", !11, !16}
!11 = !{!"compOpt", !12, !13}
!12 = !{!"UseBindlessMode", i1 true}
!13 = !{!"UseLegacyBindlessMode", i1 false}
!16 = !{!"FuncMD", !17, !18, !19, !20}
!17 = distinct !{!"FuncMDMap[0]", ptr @k_pulled}
!18 = !{!"FuncMDValue[0]", !21, !22}
!19 = distinct !{!"FuncMDMap[1]", ptr @k_ref}
!20 = !{!"FuncMDValue[1]", !21}
!21 = !{!"functionType", !"KernelFunction"}
!22 = !{!"pulledArgs", !23, !24}
!23 = !{!"pulledArgsVec[0]", i32 1}
!24 = !{!"pulledArgsVec[1]", i32 2}
