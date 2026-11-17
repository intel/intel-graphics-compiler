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

; Test that a pulled struct passed by value (not byval) is read member by member
; from cross-thread memory (ADDRESS_SPACE_THREAD_ARG):
; - extractvalue of a member becomes a load of that member,
; - other uses get the whole struct built in the entry block from member loads.
; The whole struct is never loaded at once.
; In every kernel %m (2040) and %n (4) are pushed, the struct and %out are pulled.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%struct.Mid = type { [2040 x i8] }
%S = type { float, i32, [2 x float] }
%P = type { i8, i32, i16, double }
%N = type { i32, { float, [3 x i32] } }
%L = type { i32, [1500 x i16] }

; Only members are used: each member is loaded where its extractvalue was
define spir_kernel void @k_members(ptr byval(%struct.Mid) %m, i32 %n, %S %s, ptr addrspace(1) %out) {
entry:
  %vm = load i8, ptr %m, align 1
  %c = icmp sgt i32 %n, 0
  br i1 %c, label %body, label %exit
body:
  %a = extractvalue %S %s, 0
  %t = extractvalue %S %s, 2, 1
  %r = fadd float %a, %t
  store float %r, ptr addrspace(1) %out, align 4
  br label %exit
exit:
  ret void
}

; CHECK-LABEL: define spir_kernel void @k_members(
; CHECK-NEXT:  entry:
; CHECK:       %out_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 3)
; CHECK:       %[[OUT:.+]] = load ptr addrspace(1), ptr addrspace(5) %out_pulled, align 8, !invariant.load
; CHECK:       %s_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 2)
; CHECK-NOT:   getKernelArgPtr
; CHECK-NOT:   getelementptr {{.*}} %s_pulled
; CHECK:       %vm = load i8, ptr %m, align 1
; CHECK-NOT:   getelementptr {{.*}} %s_pulled
; CHECK:       %c = icmp sgt i32 %n, 0
; CHECK:       body:
; CHECK-NEXT:  %[[G0:[0-9]+]] = getelementptr inbounds %S, ptr addrspace(5) %s_pulled, i32 0, i32 0
; CHECK-NEXT:  %a = load float, ptr addrspace(5) %[[G0]], align 4, !invariant.load
; CHECK-NEXT:  %[[G1:[0-9]+]] = getelementptr inbounds %S, ptr addrspace(5) %s_pulled, i32 0, i32 2, i32 1
; CHECK-NEXT:  %t = load float, ptr addrspace(5) %[[G1]], align 4, !invariant.load
; CHECK-NEXT:  %r = fadd float %a, %t
; CHECK-NEXT:  store float %r, ptr addrspace(1) %[[OUT]], align 4
; CHECK-NOT:   extractvalue
; CHECK:       ret void


; The whole struct is used by selects, so it is built once
; in the entry block and both selects use it
define spir_kernel void @k_whole(ptr byval(%struct.Mid) %m, i32 %n, %S %s, ptr addrspace(1) %out) {
entry:
  %vm = load i8, ptr %m, align 1
  %c = icmp sgt i32 %n, 0
  %w1 = select i1 %c, %S %s, %S zeroinitializer
  %w2 = select i1 %c, %S zeroinitializer, %S %s
  %k = extractvalue %S %s, 1
  %x = extractvalue %S %w1, 0
  %y = extractvalue %S %w2, 0
  %kf = sitofp i32 %k to float
  %r0 = fadd float %x, %y
  %r = fadd float %r0, %kf
  store float %r, ptr addrspace(1) %out, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @k_whole(
; CHECK-NEXT:  entry:
; CHECK:       %out_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 3)
; CHECK:       %[[OUT:.+]] = load ptr addrspace(1), ptr addrspace(5) %out_pulled, align 8, !invariant.load
; CHECK:       %s_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 2)
; CHECK-NEXT:  %[[W0G:[0-9]+]] = getelementptr inbounds %S, ptr addrspace(5) %s_pulled, i32 0, i32 0
; CHECK-NEXT:  %[[W0:[0-9]+]] = load float, ptr addrspace(5) %[[W0G]], align 4, !invariant.load
; CHECK-NEXT:  %[[I0:[0-9]+]] = insertvalue %S poison, float %[[W0]], 0
; CHECK-NEXT:  %[[W1G:[0-9]+]] = getelementptr inbounds %S, ptr addrspace(5) %s_pulled, i32 0, i32 1
; CHECK-NEXT:  %[[W1:[0-9]+]] = load i32, ptr addrspace(5) %[[W1G]], align 4, !invariant.load
; CHECK-NEXT:  %[[I1:[0-9]+]] = insertvalue %S %[[I0]], i32 %[[W1]], 1
; CHECK-NEXT:  %[[W2G:[0-9]+]] = getelementptr inbounds %S, ptr addrspace(5) %s_pulled, i32 0, i32 2, i32 0
; CHECK-NEXT:  %[[W2:[0-9]+]] = load float, ptr addrspace(5) %[[W2G]], align 4, !invariant.load
; CHECK-NEXT:  %[[I2:[0-9]+]] = insertvalue %S %[[I1]], float %[[W2]], 2, 0
; CHECK-NEXT:  %[[W3G:[0-9]+]] = getelementptr inbounds %S, ptr addrspace(5) %s_pulled, i32 0, i32 2, i32 1
; CHECK-NEXT:  %[[W3:[0-9]+]] = load float, ptr addrspace(5) %[[W3G]], align 4, !invariant.load
; CHECK-NEXT:  %[[WHOLE:[0-9]+]] = insertvalue %S %[[I2]], float %[[W3]], 2, 1
; CHECK-NOT:   getKernelArgPtr
; CHECK-NOT:   load %S
; CHECK:       %vm = load i8, ptr %m, align 1
; CHECK:       %c = icmp sgt i32 %n, 0
; CHECK:       %w1 = select i1 %c, %S %[[WHOLE]], %S zeroinitializer
; CHECK:       %w2 = select i1 %c, %S zeroinitializer, %S %[[WHOLE]]
; CHECK:       %[[KG:[0-9]+]] = getelementptr inbounds %S, ptr addrspace(5) %s_pulled, i32 0, i32 1
; CHECK-NEXT:  %k = load i32, ptr addrspace(5) %[[KG]], align 4, !invariant.load
; CHECK:       %x = extractvalue %S %w1, 0
; CHECK:       %y = extractvalue %S %w2, 0
; CHECK:       %kf = sitofp i32 %k to float
; CHECK:       %r0 = fadd float %x, %y
; CHECK:       %r = fadd float %r0, %kf
; CHECK:       store float %r, ptr addrspace(1) %[[OUT]], align 4


; Struct with padding: members are at offsets 0, 4, 8 and 16 and the slot is
; aligned to 8, so the member loads are aligned to 8, 4, 8 and 8
define spir_kernel void @k_padding(ptr byval(%struct.Mid) %m, i32 %n, %P %p, ptr addrspace(1) %out) {
  %vm = load i8, ptr %m, align 1
  %a = extractvalue %P %p, 0
  %b = extractvalue %P %p, 1
  %h = extractvalue %P %p, 2
  %d = extractvalue %P %p, 3
  %az = zext i8 %a to i32
  %hz = zext i16 %h to i32
  %di = fptosi double %d to i32
  %r0 = add i32 %az, %b
  %r1 = add i32 %r0, %hz
  %r = add i32 %r1, %di
  store i32 %r, ptr addrspace(1) %out, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @k_padding(
; CHECK:       %out_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 3)
; CHECK:       %[[OUT:.+]] = load ptr addrspace(1), ptr addrspace(5) %out_pulled, align 8, !invariant.load
; CHECK:       %p_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 2)
; CHECK-NOT:   getKernelArgPtr
; CHECK:       %vm = load i8, ptr %m, align 1
; CHECK-NEXT:  %[[PG0:[0-9]+]] = getelementptr inbounds %P, ptr addrspace(5) %p_pulled, i32 0, i32 0
; CHECK-NEXT:  %a = load i8, ptr addrspace(5) %[[PG0]], align 8, !invariant.load
; CHECK-NEXT:  %[[PG1:[0-9]+]] = getelementptr inbounds %P, ptr addrspace(5) %p_pulled, i32 0, i32 1
; CHECK-NEXT:  %b = load i32, ptr addrspace(5) %[[PG1]], align 4, !invariant.load
; CHECK-NEXT:  %[[PG2:[0-9]+]] = getelementptr inbounds %P, ptr addrspace(5) %p_pulled, i32 0, i32 2
; CHECK-NEXT:  %h = load i16, ptr addrspace(5) %[[PG2]], align 8, !invariant.load
; CHECK-NEXT:  %[[PG3:[0-9]+]] = getelementptr inbounds %P, ptr addrspace(5) %p_pulled, i32 0, i32 3
; CHECK-NEXT:  %d = load double, ptr addrspace(5) %[[PG3]], align 8, !invariant.load
; CHECK-NEXT:  %az = zext i8 %a to i32
; CHECK-NEXT:  %hz = zext i16 %h to i32
; CHECK-NEXT:  %di = fptosi double %d to i32
; CHECK-NEXT:  %r0 = add i32 %az, %b
; CHECK-NEXT:  %r1 = add i32 %r0, %hz
; CHECK-NEXT:  %r = add i32 %r1, %di
; CHECK-NEXT:  store i32 %r, ptr addrspace(1) %[[OUT]], align 4


; Nested struct: the float is loaded directly, the inner array is used as
; a whole, so only the array is built from loads
define spir_kernel void @k_nested(ptr byval(%struct.Mid) %m, i32 %n, %N %s, ptr addrspace(1) %out) {
  %vm = load i8, ptr %m, align 1
  %in = extractvalue %N %s, 1
  %f = extractvalue { float, [3 x i32] } %in, 0
  %arr = extractvalue { float, [3 x i32] } %in, 1
  store [3 x i32] %arr, ptr addrspace(1) %out, align 4
  %fi = fptosi float %f to i32
  %o1 = getelementptr inbounds i32, ptr addrspace(1) %out, i64 3
  store i32 %fi, ptr addrspace(1) %o1, align 4
  ret void
}

; CHECK-LABEL: define spir_kernel void @k_nested(
; CHECK:       %out_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 3)
; CHECK:       %[[OUT:.+]] = load ptr addrspace(1), ptr addrspace(5) %out_pulled, align 8, !invariant.load
; CHECK:       %s_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 2)
; CHECK-NEXT:  %[[A0G:[0-9]+]] = getelementptr inbounds %N, ptr addrspace(5) %s_pulled, i32 0, i32 1, i32 1, i32 0
; CHECK-NEXT:  %[[A0:[0-9]+]] = load i32, ptr addrspace(5) %[[A0G]], align 4, !invariant.load
; CHECK-NEXT:  %[[J0:[0-9]+]] = insertvalue [3 x i32] poison, i32 %[[A0]], 0
; CHECK-NEXT:  %[[A1G:[0-9]+]] = getelementptr inbounds %N, ptr addrspace(5) %s_pulled, i32 0, i32 1, i32 1, i32 1
; CHECK-NEXT:  %[[A1:[0-9]+]] = load i32, ptr addrspace(5) %[[A1G]], align 4, !invariant.load
; CHECK-NEXT:  %[[J1:[0-9]+]] = insertvalue [3 x i32] %[[J0]], i32 %[[A1]], 1
; CHECK-NEXT:  %[[A2G:[0-9]+]] = getelementptr inbounds %N, ptr addrspace(5) %s_pulled, i32 0, i32 1, i32 1, i32 2
; CHECK-NEXT:  %[[A2:[0-9]+]] = load i32, ptr addrspace(5) %[[A2G]], align 4, !invariant.load
; CHECK-NEXT:  %[[ARR:[0-9]+]] = insertvalue [3 x i32] %[[J1]], i32 %[[A2]], 2
; CHECK-NOT:   getKernelArgPtr
; CHECK:       %vm = load i8, ptr %m, align 1
; CHECK-NEXT:  %[[FG:[0-9]+]] = getelementptr inbounds %N, ptr addrspace(5) %s_pulled, i32 0, i32 1, i32 0
; CHECK-NEXT:  %f = load float, ptr addrspace(5) %[[FG]], align 4, !invariant.load
; CHECK-NEXT:  store [3 x i32] %[[ARR]], ptr addrspace(1) %[[OUT]], align 4
; CHECK-NEXT:  %fi = fptosi float %f to i32
; CHECK-NEXT:  %o1 = getelementptr inbounds i32, ptr addrspace(1) %[[OUT]], i64 3
; CHECK-NEXT:  store i32 %fi, ptr addrspace(1) %o1, align 4
; CHECK-NOT:   extractvalue


; Array with 1500 elements: only the used element is loaded
define spir_kernel void @k_long(ptr byval(%struct.Mid) %m, i32 %n, %L %s, ptr addrspace(1) %out) {
  %vm = load i8, ptr %m, align 1
  %e = extractvalue %L %s, 1, 1499
  store i16 %e, ptr addrspace(1) %out, align 2
  ret void
}

; CHECK-LABEL: define spir_kernel void @k_long(
; CHECK:       %out_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 3)
; CHECK:       %[[OUT:.+]] = load ptr addrspace(1), ptr addrspace(5) %out_pulled, align 8, !invariant.load
; CHECK:       %s_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 2)
; CHECK-NOT:   getKernelArgPtr
; CHECK:       %vm = load i8, ptr %m, align 1
; CHECK-NEXT:  %[[LG:[0-9]+]] = getelementptr inbounds %L, ptr addrspace(5) %s_pulled, i32 0, i32 1, i32 1499
; CHECK-NEXT:  %e = load i16, ptr addrspace(5) %[[LG]], align 2, !invariant.load
; CHECK-NEXT:  store i16 %e, ptr addrspace(1) %[[OUT]], align 2

!igc.functions = !{!0, !3, !4, !5, !6}
!IGCMetadata = !{!10}

!0 = !{ptr @k_members, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{ptr @k_whole, !1}
!4 = !{ptr @k_padding, !1}
!5 = !{ptr @k_nested, !1}
!6 = !{ptr @k_long, !1}
!10 = !{!"ModuleMD", !11}
!11 = !{!"FuncMD", !12, !13, !14, !15, !16, !17, !18, !19, !21, !22}
!12 = distinct !{!"FuncMDMap[0]", ptr @k_members}
!13 = !{!"FuncMDValue[0]", !20}
!14 = distinct !{!"FuncMDMap[1]", ptr @k_whole}
!15 = !{!"FuncMDValue[1]", !20}
!16 = distinct !{!"FuncMDMap[2]", ptr @k_padding}
!17 = !{!"FuncMDValue[2]", !20}
!18 = distinct !{!"FuncMDMap[3]", ptr @k_nested}
!19 = !{!"FuncMDValue[3]", !20}
!21 = distinct !{!"FuncMDMap[4]", ptr @k_long}
!22 = !{!"FuncMDValue[4]", !20}
!20 = !{!"functionType", !"KernelFunction"}
