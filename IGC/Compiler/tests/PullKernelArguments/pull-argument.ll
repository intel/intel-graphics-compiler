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

; Test that checks pulling for kernel arguments (scalar, vector, global and local pointer)
; are replaced with invariant loads from cross thread memory (ADDRESS_SPACE_THREAD_ARG)

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%struct.Mid = type { [2040 x i8] }
%struct.Odd = type { [2047 x i8] }

; %s (2040) and %n (4) fit into 2048 bytes, everything after them is pulled
define spir_kernel void @k_values(ptr byval(%struct.Mid) %s, i32 %n, <4 x float> %v, <8 x double> %d, ptr addrspace(1) %out, ptr addrspace(3) %l) {
entry:
  %c = icmp sgt i32 %n, 0
  br i1 %c, label %body, label %exit
body:
  %p = getelementptr inbounds i8, ptr %s, i64 5
  %b = load i8, ptr %p, align 1
  %x = extractelement <4 x float> %v, i64 0
  %y = extractelement <8 x double> %d, i64 7
  store float %x, ptr addrspace(1) %out, align 4
  store double %y, ptr addrspace(1) %out, align 8
  store i8 %b, ptr addrspace(3) %l, align 1
  br label %exit
exit:
  ret void
}

; CHECK-LABEL: define spir_kernel void @k_values(ptr byval(%struct.Mid) %s, i32 %n, <4 x float> %v, <8 x double> %d, ptr addrspace(1) %out, ptr addrspace(3) %l)
; CHECK-NEXT:  entry:
; CHECK:     %l_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 5)
; CHECK:     %[[L:.+]] = load ptr addrspace(3), ptr addrspace(5) %l_pulled, align 8, !invariant.load [[INV:![0-9]+]]
; CHECK:     %out_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 4)
; CHECK:     %[[OUT:.+]] = load ptr addrspace(1), ptr addrspace(5) %out_pulled, align 8, !invariant.load [[INV]]
; CHECK:     %d_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 3)
; CHECK:     %[[D:.+]] = load <8 x double>, ptr addrspace(5) %d_pulled, align 32, !invariant.load [[INV]]
; CHECK:     %v_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 2)
; CHECK:     %[[V:.+]] = load <4 x float>, ptr addrspace(5) %v_pulled, align 16, !invariant.load [[INV]]
; CHECK-NOT: getKernelArgPtr
; CHECK:     %c = icmp sgt i32 %n, 0
; CHECK:     body:
; CHECK:     %p = getelementptr inbounds i8, ptr %s, i64 5
; CHECK:     %b = load i8, ptr %p, align 1
; CHECK:     %x = extractelement <4 x float> %[[V]], i64 0
; CHECK:     %y = extractelement <8 x double> %[[D]], i64 7
; CHECK:     store float %x, ptr addrspace(1) %[[OUT]], align 4
; CHECK:     store double %y, ptr addrspace(1) %[[OUT]], align 8
; CHECK:     store i8 %b, ptr addrspace(3) %[[L]], align 1


; %s (2047) is pushed, so even the 2-byte %h does not fit and is pulled
define spir_kernel void @k_scalars(ptr byval(%struct.Odd) %s, i16 %h, half %f, i64 %q, ptr addrspace(1) %out) {
  %b = load i8, ptr %s, align 1
  %hz = zext i16 %h to i64
  %fq = fptosi half %f to i64
  %r0 = add i64 %hz, %q
  %r = add i64 %r0, %fq
  store i64 %r, ptr addrspace(1) %out, align 8
  ret void
}

; CHECK-LABEL: define spir_kernel void @k_scalars(ptr byval(%struct.Odd) %s, i16 %h, half %f, i64 %q, ptr addrspace(1) %out)
; CHECK:     %out_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 4)
; CHECK:     %[[OUT:.+]] = load ptr addrspace(1), ptr addrspace(5) %out_pulled, align 8, !invariant.load [[INV]]
; CHECK:     %q_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 3)
; CHECK:     %[[Q:.+]] = load i64, ptr addrspace(5) %q_pulled, align 8, !invariant.load [[INV]]
; CHECK:     %f_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 2)
; CHECK:     %[[F:.+]] = load half, ptr addrspace(5) %f_pulled, align 4, !invariant.load [[INV]]
; CHECK:     %h_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 1)
; CHECK:     %[[H:.+]] = load i16, ptr addrspace(5) %h_pulled, align 4, !invariant.load [[INV]]
; CHECK-NOT: getKernelArgPtr
; CHECK:     %b = load i8, ptr %s, align 1
; CHECK:     %hz = zext i16 %[[H]] to i64
; CHECK:     %fq = fptosi half %[[F]] to i64
; CHECK:     %r0 = add i64 %hz, %[[Q]]
; CHECK:     %r = add i64 %r0, %fq
; CHECK:     store i64 %r, ptr addrspace(1) %[[OUT]], align 8


; CHECK:      declare ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32) [[ATTR:#[0-9]+]]
; CHECK:      attributes [[ATTR]] = { {{nounwind readnone willreturn|nounwind willreturn memory\(none\)}} }

!igc.functions = !{!0, !3}
!IGCMetadata = !{!10}

!0 = !{ptr @k_values, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @k_scalars, !1}
!10 = !{!"ModuleMD", !11}
!11 = !{!"FuncMD", !12, !13, !14, !15}
!12 = distinct !{!"FuncMDMap[0]", ptr @k_values}
!13 = !{!"FuncMDValue[0]", !20}
!14 = distinct !{!"FuncMDMap[1]", ptr @k_scalars}
!15 = !{!"FuncMDValue[1]", !20}
!20 = !{!"functionType", !"KernelFunction"}
