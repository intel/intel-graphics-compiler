;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-scalar-arg-as-pointer-analysis -igc-serialize-metadata -S %s | FileCheck %s
;
; CHECK:     !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 0}
; CHECK:     !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 1}
; CHECK-NOT: !{!"m_OpenCLArgScalarAsPointersSet{{[[][0-9][]]}}", i32 2}

define spir_kernel void @test(i64 %a, i64 %b, i64 %c) #0 {
entry:
  %0 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %a, i32 8191, i32 7, i32 8191, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 %b, i32 8191, i32 7, i32 8191, i32 0, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, <8 x i32> %0)
  ret void
}

declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1) #1
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, <8 x i32>) #1

attributes #1 = { nounwind }

!igc.functions = !{!0}

!0 = !{void (i64, i64, i64)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
