;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: With efficient 64-bit addressing the indirect call target is a full
; COM: 64-bit address. Otherwise it stays 32-bit.

; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe3P -mattr=+efficient_64b_enabled \
; RUN: -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' \
; RUN: -o /dev/null | FileCheck %s --check-prefix=CHECK-64
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe3P -mattr=+efficient_64b_enabled \
; RUN: -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' \
; RUN: -o /dev/null | FileCheck %s --check-prefix=CHECK-64
; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe3P \
; RUN: -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' \
; RUN: -o /dev/null | FileCheck %s --check-prefix=CHECK-32
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe3P \
; RUN: -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' \
; RUN: -o /dev/null | FileCheck %s --check-prefix=CHECK-32
; RUN: %llc_typed_ptrs %s -march=genx64 -mcpu=Xe2 \
; RUN: -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' \
; RUN: -o /dev/null | FileCheck %s --check-prefix=CHECK-32
; RUN: %llc_opaque_ptrs %s -march=genx64 -mcpu=Xe2 \
; RUN: -vc-skip-ocl-runtime-info -finalizer-opts='-dumpcommonisa -isaasmToConsole' \
; RUN: -o /dev/null | FileCheck %s --check-prefix=CHECK-32

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

declare <16 x i32> @llvm.genx.svm.block.ld.v16i32.i64(i64)
declare void @llvm.genx.svm.block.st.i64.v16i32(i64, <16 x i32>)

define internal spir_func <16 x i32> @add(<16 x i32> %a, <16 x i32> %b) addrspace(9) #0 !FuncArgSize !0 !FuncRetSize !1 {
  %r = add <16 x i32> %a, %b
  ret <16 x i32> %r
}

define internal spir_func <16 x i32> @sub(<16 x i32> %a, <16 x i32> %b) addrspace(9) #0 !FuncArgSize !0 !FuncRetSize !1 {
  %r = sub <16 x i32> %a, %b
  ret <16 x i32> %r
}

; CHECK-64-LABEL: .kernel "sel"
; CHECK-64: .decl [[FP:V[0-9]+]] v_type=G type=uq num_elts=1 align=qword
; CHECK-64-NOT: mov (M1, 1) [[FP]](0,0)<1>
; CHECK-64: ifcall.uniform (M1, 16) [[FP]](0,0)<0;1,0>
; CHECK-32-LABEL: .kernel "sel"
; CHECK-32: .decl [[FP:V[0-9]+]] v_type=G type=ud num_elts=1 align=qword
; CHECK-32: ifcall.uniform (M1, 16) [[FP]](0,0)<0;1,0>
define dllexport void @sel(i64 %in, i64 %out, i32 %c, i64 %privBase) #1 {
  %v = call <16 x i32> @llvm.genx.svm.block.ld.v16i32.i64(i64 %in)
  %cmp = icmp eq i32 %c, 0
  %fp = select i1 %cmp, <16 x i32> (<16 x i32>, <16 x i32>) addrspace(9)* @add, <16 x i32> (<16 x i32>, <16 x i32>) addrspace(9)* @sub
  %r = call spir_func addrspace(9) <16 x i32> %fp(<16 x i32> %v, <16 x i32> %v) #2, !FuncArgSize !0, !FuncRetSize !1
  call void @llvm.genx.svm.block.st.i64.v16i32(i64 %out, <16 x i32> %r)
  ret void
}

; CHECK-64-LABEL: .kernel "phi"
; CHECK-64: .decl [[FP:V[0-9]+]] v_type=G type=uq num_elts=1 align=qword
; CHECK-64-NOT: mov (M1, 1) [[FP]](0,0)<1>
; CHECK-64: ifcall.uniform (M1, 16) [[FP]](0,0)<0;1,0>
; CHECK-32-LABEL: .kernel "phi"
; CHECK-32: .decl [[FP:V[0-9]+]] v_type=G type=ud num_elts=1 align=qword
; CHECK-32: ifcall.uniform (M1, 16) [[FP]](0,0)<0;1,0>
define dllexport void @phi(i64 %in, i64 %out, i32 %c, i64 %privBase) #1 {
entry:
  %v = call <16 x i32> @llvm.genx.svm.block.ld.v16i32.i64(i64 %in)
  %cmp = icmp eq i32 %c, 0
  br i1 %cmp, label %then, label %join
then:
  br label %join
join:
  %fp = phi <16 x i32> (<16 x i32>, <16 x i32>) addrspace(9)* [ @add, %then ], [ @sub, %entry ]
  %r = call spir_func addrspace(9) <16 x i32> %fp(<16 x i32> %v, <16 x i32> %v) #2, !FuncArgSize !0, !FuncRetSize !1
  call void @llvm.genx.svm.block.st.i64.v16i32(i64 %out, <16 x i32> %r)
  ret void
}

; CHECK-64-LABEL: .kernel "arr"
; CHECK-64: .decl [[FP:V[0-9]+]] v_type=G type=uq num_elts=1 align=qword
; CHECK-64: ifcall.uniform (M1, 16) [[FP]](0,0)<0;1,0>
; CHECK-32-LABEL: .kernel "arr"
; CHECK-32: .decl [[FP:V[0-9]+]] v_type=G type=ud num_elts=1 align=qword
; CHECK-32: ifcall.uniform (M1, 16) [[FP]](0,0)<0;1,0>
define dllexport void @arr(i64 %in, i64 %out, i32 %c, i64 %privBase) #1 {
  %a = alloca [1 x <16 x i32> (<16 x i32>, <16 x i32>) addrspace(9)*], align 8
  %p0 = getelementptr inbounds [1 x <16 x i32> (<16 x i32>, <16 x i32>) addrspace(9)*], [1 x <16 x i32> (<16 x i32>, <16 x i32>) addrspace(9)*]* %a, i64 0, i64 0
  store <16 x i32> (<16 x i32>, <16 x i32>) addrspace(9)* @add, <16 x i32> (<16 x i32>, <16 x i32>) addrspace(9)** %p0, align 8
  %v = call <16 x i32> @llvm.genx.svm.block.ld.v16i32.i64(i64 %in)
  %idx = zext i32 %c to i64
  %pi = getelementptr inbounds [1 x <16 x i32> (<16 x i32>, <16 x i32>) addrspace(9)*], [1 x <16 x i32> (<16 x i32>, <16 x i32>) addrspace(9)*]* %a, i64 0, i64 %idx
  %fp = load <16 x i32> (<16 x i32>, <16 x i32>) addrspace(9)*, <16 x i32> (<16 x i32>, <16 x i32>) addrspace(9)** %pi, align 8
  %r = call spir_func addrspace(9) <16 x i32> %fp(<16 x i32> %v, <16 x i32> %v) #2, !FuncArgSize !0, !FuncRetSize !1
  call void @llvm.genx.svm.block.st.i64.v16i32(i64 %out, <16 x i32> %r)
  ret void
}

attributes #0 = { noinline nounwind "CMStackCall" }
attributes #1 = { nounwind "CMGenxMain" "oclrt"="1" }
attributes #2 = { nounwind }

!genx.kernels = !{!10, !11, !12}
!genx.kernel.internal = !{!20, !21, !22}

!0 = !{i32 4}
!1 = !{i32 2}
!2 = !{i32 0, i32 0, i32 0, i32 96}
!3 = !{i32 64, i32 72, i32 80, i32 88}
!4 = !{i32 0, i32 0, i32 0, i32 0}
!5 = !{!"", !"", !"", !""}
!6 = !{i32 0, i32 1, i32 2, i32 3}
!7 = !{}
!10 = !{void (i64, i64, i32, i64)* @sel, !"sel", !2, i32 0, !3, !4, !5, i32 0}
!11 = !{void (i64, i64, i32, i64)* @phi, !"phi", !2, i32 0, !3, !4, !5, i32 0}
!12 = !{void (i64, i64, i32, i64)* @arr, !"arr", !2, i32 0, !3, !4, !5, i32 0}
!20 = !{void (i64, i64, i32, i64)* @sel, !4, !6, !7, !6}
!21 = !{void (i64, i64, i32, i64)* @phi, !4, !6, !7, !6}
!22 = !{void (i64, i64, i32, i64)* @arr, !4, !6, !7, !6}
