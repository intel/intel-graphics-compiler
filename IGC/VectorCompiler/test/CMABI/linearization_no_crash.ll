;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -CMABILegacy -march=genx64 -mcpu=XeHPG -S -disable-output < %s
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -CMABILegacy -march=genx64 -mcpu=XeHPG -S -disable-output < %s

; RUN: %opt_new_pm_typed -passes=CMABI -march=genx64 -mcpu=XeHPG -S -disable-output < %s
; RUN: %opt_new_pm_opaque -passes=CMABI -march=genx64 -mcpu=XeHPG -S -disable-output < %s

; COM: This test must not crash

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

%struct1 = type { %struct2 }
%struct2 = type { float, float }
%struct3 = type { i32, i32 }
%struct4 = type { [2 x i64] }

define dllexport spir_kernel void @test1(i64 %0, i64 %1, i64 %2, i64 %3, i64 %4, %struct1* byval(%struct1) align 4 %5, %struct3 addrspace(4)* align 4 %6, i1 zeroext %7, %struct1* byval(%struct1) align 4 %8, %struct3 addrspace(4)* align 4 %9, i1 zeroext %10, %struct1 addrspace(1)* readonly align 4 %11, %struct1 addrspace(1)* align 4 %12, %struct4* byval(%struct4) align 8 %13, %struct4* byval(%struct4) align 8 %14, <3 x i16> %15, <3 x i32> %16, i64 %17, i64 %18, i64 %19, i64 %20, i64 %21, float %22, float %23, float %24, float %25) {
  ret void
}

define spir_kernel void @test2(i64 %0, i64 %1, i64 %2, i64 %3, i64 %4, %struct1* %5, %struct3 addrspace(4)* %6, i1 %7, %struct1* %8, %struct3 addrspace(4)* %9, i1 %10, %struct1 addrspace(1)* %11, %struct1 addrspace(1)* %12, %struct4* %13, %struct4* %14, <3 x i16> %15, <3 x i32> %16, i64 %17, i64 %18, i64 %19, i64 %20, i64 %21, float %22, float %23, float %24, float %25) {
  ret void
}

!genx.kernels = !{!0, !4}
!genx.kernel.internal = !{!5, !23}

!0 = distinct !{void (i64, i64, i64, i64, i64, %struct1*, %struct3 addrspace(4)*, i1, %struct1*, %struct3 addrspace(4)*, i1, %struct1 addrspace(1)*, %struct1 addrspace(1)*, %struct4*, %struct4*, <3 x i16>, <3 x i32>, i64, i64, i64, i64, i64, float, float, float, float)* @test1, !"test1", !1, i32 0, i32 0, !2, !3, i32 0}
!1 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 112, i32 0, i32 0, i32 112, i32 0, i32 0, i32 0, i32 0, i32 112, i32 112, i32 24, i32 8, i32 96, i32 104, i32 104, i32 104, i32 104, i32 104, i32 104, i32 104, i32 104}
!2 = !{i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0}
!3 = !{!"", !"", !"", !"", !"", !"", !"svmptr_t", !"", !"", !"svmptr_t", !"", !"svmptr_t", !"svmptr_t", !"", !""}
!4 = !{void (i64, i64, i64, i64, i64, %struct1*, %struct3 addrspace(4)*, i1, %struct1*, %struct3 addrspace(4)*, i1, %struct1 addrspace(1)*, %struct1 addrspace(1)*, %struct4*, %struct4*, <3 x i16>, <3 x i32>, i64, i64, i64, i64, i64, float, float, float, float)* @test2, !"test2", !1, i32 0, i32 0, !2, !3, i32 0}
!5 = !{void (i64, i64, i64, i64, i64, %struct1*, %struct3 addrspace(4)*, i1, %struct1*, %struct3 addrspace(4)*, i1, %struct1 addrspace(1)*, %struct1 addrspace(1)*, %struct4*, %struct4*, <3 x i16>, <3 x i32>, i64, i64, i64, i64, i64, float, float, float, float)* @test1, null, null, !6, null}
!6 = !{!7, !11, !15, !19}
!7 = !{i32 5, !8}
!8 = !{!9, !10}
!9 = !{i32 24, i32 0}
!10 = !{i32 25, i32 4}
!11 = !{i32 8, !12}
!12 = !{!13, !14}
!13 = !{i32 22, i32 0}
!14 = !{i32 23, i32 4}
!15 = !{i32 13, !16}
!16 = !{!17, !18}
!17 = !{i32 20, i32 0}
!18 = !{i32 21, i32 8}
!19 = !{i32 14, !20}
!20 = !{!21, !22}
!21 = !{i32 18, i32 0}
!22 = !{i32 19, i32 8}
!23 = !{void (i64, i64, i64, i64, i64, %struct1*, %struct3 addrspace(4)*, i1, %struct1*, %struct3 addrspace(4)*, i1, %struct1 addrspace(1)*, %struct1 addrspace(1)*, %struct4*, %struct4*, <3 x i16>, <3 x i32>, i64, i64, i64, i64, i64, float, float, float, float)* @test2, null, null, !6, null}
