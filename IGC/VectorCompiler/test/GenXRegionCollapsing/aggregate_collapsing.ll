;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXRegionCollapsing \
; RUN:     -march=genx64 -mcpu=Gen9 -mtriple=spir64 -S < %s | FileCheck %s

; ModuleID = 'bugpoint-reduced-simplified.bc'
source_filename = "the_file.ll"
target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "genx64-unknown-unknown"

%struct.hoge = type { %struct.bar }
%struct.bar = type { <192 x i8> }

; COM: the check that collapsing succesfully operates on aggregate types

; CHECK-LABEL: @wobble
; CHECK-NEXT: [[TOPTR:%[^ ]+]] = inttoptr <1 x i64> %arg to <1 x %struct.hoge addrspace(4)*>
; CHECK-NEXT: [[RECAST:%[^ ]+]] = bitcast <1 x %struct.hoge addrspace(4)*> [[TOPTR]] to %struct.hoge addrspace(4)*
; CHECK-NEXT: [[BITCAST:%[^ ]+]] = bitcast %struct.hoge addrspace(4)* [[RECAST]] to %struct.bar addrspace(4)*
; CHECK-NEXT: [[RDREGION:%[^ ]+]] = bitcast %struct.bar addrspace(4)* [[BITCAST]] to <1 x %struct.bar addrspace(4)*>
; CHECK-NEXT: [[ICAST:%[^ ]+]] = ptrtoint <1 x %struct.bar addrspace(4)*> [[RDREGION]] to <1 x i64>
; CHECK-NEXT:  call void @llvm.genx.svm.scatter.v1i1.v1i64.v1i64(<1 x i1> <i1 true>, i32 0, <1 x i64> undef, <1 x i64>  [[ICAST]])

define internal spir_func void @wobble(<1 x i64> %arg) #0 {
  %tmp = inttoptr <1 x i64> %arg to <1 x %struct.hoge addrspace(4)*>
  %tmp2 = call %struct.hoge addrspace(4)* @llvm.genx.rdregioni.p4class.a(<1 x %struct.hoge addrspace(4)*> %tmp, i32 0, i32 1, i32 1, i16 0, i32 0)
  %tmp3 = bitcast %struct.hoge addrspace(4)* %tmp2 to %struct.bar addrspace(4)*
  %tmp4 = bitcast %struct.bar addrspace(4)* %tmp3 to <1 x %struct.bar addrspace(4)*>
  %tmp5 = call <1 x %struct.bar addrspace(4)*> @llvm.genx.rdregioni.v1p4class.p(<1 x %struct.bar addrspace(4)*> %tmp4, i32 0, i32 1, i32 0, i16 0, i32 undef)
  %tmp6 = ptrtoint <1 x %struct.bar addrspace(4)*> %tmp5 to <1 x i64>
  %tmp7 = bitcast <1 x i64> %tmp6 to <1 x i64>
  call void @llvm.genx.svm.scatter.v1i1.v1i64.v1i64(<1 x i1> <i1 true>, i32 0, <1 x i64> undef, <1 x i64> %tmp7)
  ret void
}

; Function Attrs: nounwind
declare void @llvm.genx.svm.scatter.v1i1.v1i64.v1i64(<1 x i1>, i32, <1 x i64>, <1 x i64>) #1

; Function Attrs: readnone
declare <1 x %struct.bar addrspace(4)*> @llvm.genx.rdregioni.v1p4class.p(<1 x %struct.bar addrspace(4)*>, i32, i32, i32, i16, i32) #2

; Function Attrs: readnone
declare %struct.hoge addrspace(4)* @llvm.genx.rdregioni.p4class.a(<1 x %struct.hoge addrspace(4)*>, i32, i32, i32, i16, i32) #2

attributes #0 = { "target-cpu"="Gen9" "target-features"="+ocl_runtime" }
attributes #1 = { nounwind "target-cpu"="Gen9" "target-features"="+ocl_runtime" }
attributes #2 = { readnone "target-cpu"="Gen9" "target-features"="+ocl_runtime" }

!genx.kernel.internal = !{!0}

!0 = distinct !{null, !1, !2, !3, !4}
!1 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!2 = !{i32 0, i32 1, i32 2, i32 3, i32 4, i32 5}
!3 = !{}
!4 = !{i32 1, i32 2, i32 3, i32 4, i32 5, i32 6}
