;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -cmkernelargoffset -cmkernelargoffset-cmrt=false -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; CHECK: [[STRUCT:%struct.state]]
%struct.state = type { i8, [3 x i32], float }

; COM: Find the kernel definition
; CHECK: define dllexport [[FTYPE:void]] @[[FNAME:foo]]
; COM: Explicit arguments. The first argument must be byval svmptr_t
; CHECK-SAME: [[STRUCT]]* byval(%struct.state) %_arg_
; CHECK-SAME: [[EXPARG2:i32 addrspace\(1\)\*]] %_arg_1
; COM: Private base
; CHECK-SAME: [[PRIVBASE:i64]]
; COM: Implicit linearization of %_arg_
; CHECK-SAME: [[IMPLIN1:i8]] [[LIN1NAME:%__arg_lin__arg_.0]]
; CHECK-SAME: [[IMPLIN2:i32]] %__arg_lin__arg_.4
; CHECK-SAME: [[IMPLIN3:i32]] %__arg_lin__arg_.8
; CHECK-SAME: [[IMPLIN4:i32]] %__arg_lin__arg_.12
; CHECK-SAME: [[IMPLIN5:float]] %__arg_lin__arg_.16

define dllexport void @foo(%struct.state* byval(%struct.state) %_arg_, i32 addrspace(1)* %_arg_1, i64 %privBase, i8 %__arg_lin__arg_.0, i32 %__arg_lin__arg_.4, i32 %__arg_lin__arg_.8, i32 %__arg_lin__arg_.12, float %__arg_lin__arg_.16) #0 {
entry:
; COM: Check linearization transformation.
; COM: [[STRUCT]] allocation.
; CHECK: [[ALLOCA:%_arg_.linearization]] = alloca [[STRUCT]]
; CHECK: [[I8P:%_arg_.linearization.i8]] = bitcast [[STRUCT]]* [[ALLOCA]] to i8*
;
; COM: The first linearization arg has i8 type, so it must be stored as-is in the right position
; CHECK: [[LIN1POS:%0]] = getelementptr i8, i8* [[I8P]], i32 [[LIN1OFF:0]]
; CHECK: store [[IMPLIN1]] [[LIN1NAME]], [[IMPLIN1]]* [[LIN1POS]]
;
; COM: All the next linearization arguments
; COM: The check-count-4 itselt is confusing, but it checks the following sequence (additionally checking types correctness):
;   %1 = getelementptr i8, i8* %_arg_.linearization.i8, i32 4
;   %2 = bitcast i8* %1 to i32*
;   store i32 %__arg_lin__arg_.4, i32* %2
; CHECK-COUNT-4: [[POS:%[0-9]+]] = getelementptr {{.+}}[[I8P]]{{.+[[:space:]]+}}[[CAST:%[0-9]+]] = bitcast i8* [[POS]] to [[LINTY:[[:alnum:]]+]]*{{[[:space:]]+}}store [[LINTY]]{{.+}} [[LINTY]]* [[CAST]]
;
; COM: Uses of the linearized %_arg_ must be replaced with ALLOCA
; CHECK: %.sroa.0.0..sroa_idx = getelementptr inbounds %struct.state, %struct.state* [[ALLOCA]], i64 0, i32 0
; CHECK: %.sroa.44.0..sroa_idx = getelementptr inbounds %struct.state, %struct.state* [[ALLOCA]], i64 0, i32 1, i64 1
; CHECK: %.sroa.57.0..sroa_idx = getelementptr inbounds %struct.state, %struct.state* [[ALLOCA]], i64 0, i32 2
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
!genx.kernel.internal = !{!9}
; CHECK: !genx.kernel.internal = !{[[INTERNAL:![0-9]+]]}

!0 = !{i32 0, i32 100000}
!1 = !{i32 1, i32 2}
!2 = !{i32 1, i32 0}
!3 = !{}
!4 = !{i16 6, i16 14}
!5 = !{void (%struct.state*, i32 addrspace(1)*, i64, i8, i32, i32, i32, float)* @foo, !"foo", !6, i32 0, i32 0, !7, !8, i32 0}
!6 = !{i32 112, i32 0, i32 96, i32 104, i32 104, i32 104, i32 104, i32 104}
!7 = !{i32 0, i32 0}
!8 = !{!"svmptr_t", !"svmptr_t"}
!9 = !{void (%struct.state*, i32 addrspace(1)*, i64, i8, i32, i32, i32, float)* @foo, null, null, !10, null}
; COM: Check OffsetInArgs and ArgIndexes
; CHECK: [[INTERNAL]] = !{[[FTYPE]] ([[STRUCT]]*, [[EXPARG2]], [[PRIVBASE]], [[IMPLIN1]], [[IMPLIN2]], [[IMPLIN3]], [[IMPLIN4]], [[IMPLIN5]])* @[[FNAME]], [[OFFSETINARGS:![0-9]+]], [[ARGINDEXES:![0-9]+]], [[LINMD:![0-9]+]], null}
; CHECK: [[OFFSETINARGS]] = !{i32 0, i32 0, i32 0, i32 0, i32 4, i32 8, i32 12, i32 16}
; CHECK: [[ARGINDEXES]] = !{i32 0, i32 1, i32 2, i32 0, i32 0, i32 0, i32 0, i32 0}
!10 = !{!11}
!11 = !{i32 0, !12}
!12 = !{!13, !14, !15, !16, !17}
!13 = !{i32 3, i32 0}
!14 = !{i32 4, i32 4}
!15 = !{i32 5, i32 8}
!16 = !{i32 6, i32 12}
!17 = !{i32 7, i32 16}
; COM: Also check that linearization MD is still here
; CHECK: [[LINMD]] = !{[[ARG1LINMD:![0-9]+]]}
; CHECK: [[ARG1LINMD]] = !{i32 0, [[IMPLIN:![0-9]+]]}
; CHECK: [[IMPLIN]] = !{[[IMPLINMD1:![0-9]+]], [[IMPLINMD2:![0-9]+]], [[IMPLINMD3:![0-9]+]], [[IMPLINMD4:![0-9]+]], [[IMPLINMD5:![0-9]+]]}
; CHECK: [[IMPLINMD1]] = !{i32 3, i32 [[LIN1OFF]]}
; CHECK: [[IMPLINMD2]] = !{i32 4, i32 4}
; CHECK: [[IMPLINMD3]] = !{i32 5, i32 8}
; CHECK: [[IMPLINMD4]] = !{i32 6, i32 12}
; CHECK: [[IMPLINMD5]] = !{i32 7, i32 16}
!18 = !{i32 1}

