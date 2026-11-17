;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --platformbmg -regkey PullKernelArgs=1 --igc-pull-kernel-args -igc-serialize-metadata -S < %s 2>&1 | FileCheck %s

; Test that a pulled byval struct argument is replaced with a private copy
; (alloca) filled by a memcpy from in cross-thread memory (ADDRESS_SPACE_THREAD_ARG)

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%struct.Mid = type { [2040 x i8] }
%struct.S = type { float, [6 x i32], float }
%struct.C = type { [3 x i8] }
%struct.D = type { double, i32 }

; %m (2040) and %n (4) are pushed, %s and %out are pulled
define spir_kernel void @k_copy(ptr byval(%struct.Mid) %m, i32 %n, ptr byval(%struct.S) %s, ptr addrspace(1) %out) {
entry:
  %pm = getelementptr inbounds i8, ptr %m, i32 1
  %vm = load i8, ptr %pm, align 1
  %pa = getelementptr inbounds %struct.S, ptr %s, i32 0, i32 0
  %a = load float, ptr %pa, align 4
  %pb = getelementptr inbounds %struct.S, ptr %s, i32 0, i32 1, i32 %n
  store i32 %n, ptr %pb, align 4
  %b = load i32, ptr %pb, align 4
  %bf = sitofp i32 %b to float
  %r = fadd float %a, %bf
  store float %r, ptr addrspace(1) %out, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @k_copy(
; CHECK-NEXT:  entry:
; CHECK:       %out_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 3)
; CHECK:       %[[OUT:.+]] = load ptr addrspace(1), ptr addrspace(5) %out_pulled, align 8, !invariant.load
; CHECK:       %s_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 2)
; CHECK:       %s_copy = alloca %struct.S, align 4
; CHECK:       call void @llvm.memcpy.p0.p5.i64(ptr align 4 %s_copy, ptr addrspace(5) align 4 %s_pulled, i64 32, i1 false)
; CHECK-NOT:   getKernelArgPtr
; CHECK:       %pm = getelementptr inbounds i8, ptr %m, i32 1
; CHECK:       %vm = load i8, ptr %pm, align 1
; CHECK:       %pa = getelementptr inbounds %struct.S, ptr %s_copy, i32 0, i32 0
; CHECK:       %a = load float, ptr %pa, align 4
; CHECK:       %pb = getelementptr inbounds %struct.S, ptr %s_copy, i32 0, i32 1, i32 %n
; CHECK:       store i32 %n, ptr %pb, align 4
; CHECK:       %b = load i32, ptr %pb, align 4
; CHECK:       store float %r, ptr addrspace(1) %[[OUT]], align 4


; The pulled struct is passed to a function, so the function gets a pointer to the copy
define spir_func i32 @use(ptr %p) {
  %pb = getelementptr inbounds %struct.S, ptr %p, i32 0, i32 1, i32 3
  %b = load i32, ptr %pb, align 4
  ret i32 %b
}

define spir_kernel void @k_call(ptr byval(%struct.Mid) %m, i32 %n, ptr byval(%struct.S) %s, ptr addrspace(1) %out) {
  %vm = load i8, ptr %m, align 1
  %b = call spir_func i32 @use(ptr %s)
  store i32 %b, ptr addrspace(1) %out, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @k_call(
; CHECK:       %out_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 3)
; CHECK:       %[[OUT:.+]] = load ptr addrspace(1), ptr addrspace(5) %out_pulled, align 8, !invariant.load
; CHECK:       %s_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 2)
; CHECK:       %s_copy = alloca %struct.S, align 4
; CHECK:       call void @llvm.memcpy.p0.p5.i64(ptr align 4 %s_copy, ptr addrspace(5) align 4 %s_pulled, i64 32, i1 false)
; CHECK-NOT:   getKernelArgPtr
; CHECK:       %vm = load i8, ptr %m, align 1
; CHECK:       %b = call spir_func i32 @use(ptr %s_copy)
; CHECK:       store i32 %b, ptr addrspace(1) %[[OUT]], align 4


; Alignment of the copy and of its slot in cross-thread memory.
; The copy (alloca) keeps the byval alignment: %a -> 64, %c -> 1, %d -> 8.
; The slot alignment is clamped to [4, 32]: %a 64 -> 32, %c 1 -> 4, %d stays 8.
define spir_kernel void @k_align(ptr byval(%struct.Mid) %m, i32 %n, ptr byval(%struct.S) align 64 %a, ptr byval(%struct.C) %c, ptr byval(%struct.D) %d, ptr addrspace(1) %out) {
  %vm = load i8, ptr %m, align 1
  %va = load i32, ptr %a, align 4
  %vc = load i8, ptr %c, align 1
  %vd = load double, ptr %d, align 8
  %vcz = zext i8 %vc to i32
  %vdi = fptosi double %vd to i32
  %r0 = add i32 %va, %vcz
  %r = add i32 %r0, %vdi
  store i32 %r, ptr addrspace(1) %out, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @k_align(
; CHECK:       %out_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 5)
; CHECK:       %[[OUT:.+]] = load ptr addrspace(1), ptr addrspace(5) %out_pulled, align 8, !invariant.load
; CHECK:       %d_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 4)
; CHECK:       %d_copy = alloca %struct.D, align 8
; CHECK:       call void @llvm.memcpy.p0.p5.i64(ptr align 8 %d_copy, ptr addrspace(5) align 8 %d_pulled, i64 16, i1 false)
; CHECK:       %c_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 3)
; CHECK:       %c_copy = alloca %struct.C, align 1
; CHECK:       call void @llvm.memcpy.p0.p5.i64(ptr align 1 %c_copy, ptr addrspace(5) align 4 %c_pulled, i64 3, i1 false)
; CHECK:       %a_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 2)
; CHECK:       %a_copy = alloca %struct.S, align 64
; CHECK:       call void @llvm.memcpy.p0.p5.i64(ptr align 64 %a_copy, ptr addrspace(5) align 32 %a_pulled, i64 32, i1 false)
; CHECK-NOT:   getKernelArgPtr
; CHECK:       %vm = load i8, ptr %m, align 1
; CHECK:       %va = load i32, ptr %a_copy, align 4
; CHECK:       %vc = load i8, ptr %c_copy, align 1
; CHECK:       %vd = load double, ptr %d_copy, align 8
; CHECK:       %vcz = zext i8 %vc to i32
; CHECK:       %vdi = fptosi double %vd to i32
; CHECK:       %r0 = add i32 %va, %vcz
; CHECK:       %r = add i32 %r0, %vdi
; CHECK:       store i32 %r, ptr addrspace(1) %[[OUT]], align 4


; CHECK:      !{!"FuncMDMap[0]", ptr @k_copy}
; CHECK:      !{!"pulledArgs", [[A0:![0-9]+]], [[A1:![0-9]+]]}
; CHECK-NEXT: [[A0]] = !{!"pulledArgsVec[0]", i32 2}
; CHECK-NEXT: [[A1]] = !{!"pulledArgsVec[1]", i32 3}

!igc.functions = !{!0, !3, !4}
!IGCMetadata = !{!10}

!0 = !{ptr @k_copy, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @k_call, !1}
!4 = !{ptr @k_align, !1}
!10 = !{!"ModuleMD", !11}
!11 = !{!"FuncMD", !12, !13, !14, !15, !16, !17}
!12 = distinct !{!"FuncMDMap[0]", ptr @k_copy}
!13 = !{!"FuncMDValue[0]", !20}
!14 = distinct !{!"FuncMDMap[1]", ptr @k_call}
!15 = !{!"FuncMDValue[1]", !20}
!16 = distinct !{!"FuncMDMap[2]", ptr @k_align}
!17 = !{!"FuncMDValue[2]", !20}
!20 = !{!"functionType", !"KernelFunction"}
