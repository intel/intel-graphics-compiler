;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmimpparam -cmimpparam-cmrt=false -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

%struct.state = type { i8, [3 x i32], float }

; COM: Find the kernel definition
; CHECK: define dllexport spir_kernel [[FTYPE:void]] @[[FNAME:foo]]
; COM: Explicit arguments. The first argument must be byval svmptr_t
; CHECK-SAME: [[EXPARG1:%struct.state\*]] byval(%struct.state) %_arg_
; CHECK-SAME: [[EXPARG2:i32 addrspace\(1\)\*]] %_arg_1
; COM: Private base
; CHECK-SAME: [[PRIVBASE:i64]]
; COM: Implicit linearization of %_arg_
; CHECK-SAME: [[IMPLIN1:i8]] %__arg_lin__arg_.0
; CHECK-SAME: [[IMPLIN2:i32]] %__arg_lin__arg_.4
; CHECK-SAME: [[IMPLIN3:i32]] %__arg_lin__arg_.8
; CHECK-SAME: [[IMPLIN4:i32]] %__arg_lin__arg_.12
; CHECK-SAME: [[IMPLIN5:float]] %__arg_lin__arg_.16

; Function Attrs: nounwind
define dllexport spir_kernel void @foo(%struct.state* byval(%struct.state) %_arg_, i32 addrspace(1)* %_arg_1) #0 !intel_reqd_sub_group_size !8 {
entry:
    %.sroa.0.0..sroa_idx = getelementptr inbounds %struct.state, %struct.state* %_arg_, i64 0, i32 0
    %.sroa.0.0.copyload = load i8, i8* %.sroa.0.0..sroa_idx, align 4
    %.sroa.44.0..sroa_idx = getelementptr inbounds %struct.state, %struct.state* %_arg_, i64 0, i32 1, i64 1
    %.sroa.44.0.copyload = load i32, i32* %.sroa.44.0..sroa_idx, align 4
    %.sroa.57.0..sroa_idx = getelementptr inbounds %struct.state, %struct.state* %_arg_, i64 0, i32 2
    %.sroa.57.0.copyload = load float, float* %.sroa.57.0..sroa_idx, align 4
    %conv.i = sitofp i32 %.sroa.44.0.copyload to float
    %add.i = fadd float %.sroa.57.0.copyload, %conv.i
    %conv3.i = sitofp i8 %.sroa.0.0.copyload to float
    %add4.i = fadd float %add.i, %conv3.i
    %conv5.i = fptosi float %add4.i to i32
    store i32 %conv5.i, i32 addrspace(1)* %_arg_1, align 4
    ret void
}

attributes #0 = { nounwind "CMGenxMain" "oclrt"="1" }

!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!2}
!opencl.used.extensions = !{!3}
!opencl.used.optional.core.features = !{!3}
!spirv.Generator = !{!4}
!genx.kernels = !{!5}
; CHECK: !genx.kernel.internal = !{[[INTERNAL:![0-9]+]]}

!0 = !{i32 0, i32 100000}
!1 = !{i32 1, i32 2}
!2 = !{i32 1, i32 0}
!3 = !{}
!4 = !{i16 6, i16 14}
!5 = !{void (%struct.state*, i32 addrspace(1)*)* @foo, !"foo", !6, i32 0, i32 0, !6, !7, i32 0}
!6 = !{i32 0, i32 0}
!7 = !{!"svmptr_t", !"svmptr_t"}
!8 = !{i32 1}
; CHECK: [[INTERNAL]] = !{[[FTYPE]] ([[EXPARG1]], [[EXPARG2]], [[PRIVBASE]], [[IMPLIN1]], [[IMPLIN2]], [[IMPLIN3]], [[IMPLIN4]], [[IMPLIN5]])* @[[FNAME]], null, null, [[LINMD:![0-9]+]], null}
; CHECK: [[LINMD]] = !{[[ARG1LINMD:![0-9]+]]}
; CHECK: [[ARG1LINMD]] = !{i32 0, [[IMPLIN:![0-9]+]]}
; CHECK: [[IMPLIN]] = !{[[IMPLINMD1:![0-9]+]], [[IMPLINMD2:![0-9]+]], [[IMPLINMD3:![0-9]+]], [[IMPLINMD4:![0-9]+]], [[IMPLINMD5:![0-9]+]]}
; CHECK: [[IMPLINMD1]] = !{i32 3, i32 0}
; CHECK: [[IMPLINMD2]] = !{i32 4, i32 4}
; CHECK: [[IMPLINMD3]] = !{i32 5, i32 8}
; CHECK: [[IMPLINMD4]] = !{i32 6, i32 12}
; CHECK: [[IMPLINMD5]] = !{i32 7, i32 16}
