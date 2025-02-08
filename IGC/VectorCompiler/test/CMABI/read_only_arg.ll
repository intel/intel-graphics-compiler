;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_typed_ptrs %use_old_pass_manager% -CMABILegacy -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=CHECK
; RUN: %opt_opaque_ptrs %use_old_pass_manager% -CMABILegacy -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=CHECK

; RUN: %opt_new_pm_typed -passes=CMABI -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=CHECK
; RUN: %opt_new_pm_opaque -passes=CMABI -march=genx64 -mcpu=XeHPG -S < %s | FileCheck %s --check-prefixes=CHECK

target datalayout = "e-p:64:64-i64:64-n8:16:32:64"
target triple = "spir64-unknown-unknown"

; check that the test did not abort
%test.struct = type <{ i8 addrspace(4)*, i32, [4 x i8] }>
%struct4 = type { float }

; CHECK-LABEL: test0
define internal spir_func <3 x i32> @test0(%test.struct addrspace(4)* byval(%test.struct) %0) {
; CHECK-NOT: alloca
  ret <3 x i32> zeroinitializer
}

; CHECK-LABEL: test1
define spir_func void @test1(%struct4* byval(%struct4) %s_ptr, float* %f_ptr) #0 {
; CHECK: alloca
; Alloca -> store
  %s = load %struct4, %struct4* %s_ptr
  store %struct4 %s, %struct4* %s_ptr
  ret void
}

; CHECK-LABEL: test2
define spir_func void @test2(%struct4* byval(%struct4) %s_ptr, float* %f_ptr) #0 {
; CHECK: alloca
; Alloca -> cast -> store
  %s = load %struct4, %struct4* %s_ptr
  %cast1 = bitcast %struct4* %s_ptr to i8*
  %cast2 = bitcast i8* %cast1 to i16*
  %cast3 = bitcast i16* %cast2 to i32*
  %cast4 = bitcast i32* %cast3 to %struct4*
  store %struct4 %s, %struct4* %cast4
  ret void
}

; CHECK-LABEL: test3
define spir_func void @test3(%struct4* byval(%struct4) %s_ptr, float* %f_ptr) #0 {
; CHECK: alloca
; Alloca -> GEP -> store
  %s = load %struct4, %struct4* %s_ptr
  %load_ptr = getelementptr %struct4, %struct4* %s_ptr, i32 1, i32 0
  %f = load float, float* %load_ptr
  store float %f, float* %load_ptr
  ret void
}

; CHECK-LABEL: test4
define spir_func void @test4(%struct4* byval(%struct4) %s_ptr, float* %f_ptr) #0 {
; CHECK-NOT: alloca
; No Alloca -> GEP, cast, load
  %load_ptr = getelementptr %struct4, %struct4* %s_ptr, i32 1, i32 0
  %f = load float, float* %load_ptr
  %cast1 = bitcast %struct4* %s_ptr to i8*
  %cast2 = bitcast i8* %cast1 to i16*
  %cast3 = bitcast i16* %cast2 to i32*
  %cast4 = bitcast i32* %cast3 to %struct4*
  %f2 = load %struct4, %struct4* %cast4
  ret void
}

; CHECK-LABEL: kern
define spir_kernel void @kern(float* %RET, float* %aFOO, i64 %privBase) #1 {
  %str4_ptr = alloca %struct4, i32 2
  %f_value = load float, float* %aFOO
  %1 = call spir_func <3 x i32> @test0(%test.struct addrspace(4)* null)
  call spir_func void @test1(%struct4* %str4_ptr, float* %RET)
  call spir_func void @test2(%struct4* %str4_ptr, float* %RET)
  call spir_func void @test3(%struct4* %str4_ptr, float* %RET)
  call spir_func void @test4(%struct4* %str4_ptr, float* %RET)
  ret void
}

attributes #0 = { noinline nounwind "CMStackCall" }
attributes #1 = { noinline nounwind "CMGenxMain" "oclrt"="1" }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}

!0 = !{i32 1, !"genx.useGlobalMem", i32 1}
!1 = !{i32 0, i32 0}
!genx.kernels = !{!2}
!genx.kernel.internal = !{!7}
!2 = !{void (float*, float*, i64)* @kern, !"kern", !3, i32 0, !4, !5, !6, i32 0}
!3 = !{i32 0, i32 0, i32 96}
!4 = !{i32 72, i32 80, i32 64}
!5 = !{i32 0, i32 0}
!6 = !{!"", !""}
!7 = !{void (float*, float*, i64)* @kern, !8, !9, !10, null}
!8 = !{i32 0, i32 0, i32 0}
!9 = !{i32 0, i32 1, i32 2}
!10 = !{}