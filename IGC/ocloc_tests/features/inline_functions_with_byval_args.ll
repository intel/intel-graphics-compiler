;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, pvc-supported, llvm-14-plus

; LLVM with opaque pointers:
; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options "-igc_opts 'EnableOpaquePointersBackend=1, DisableRecompilation=1 PrintToConsole=1 PrintBefore=igc-image-sampler-resolution''" 2>&1 | FileCheck %s --check-prefixes=CHECK

; LLVM with typed pointers:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options "-igc_opts 'DisableRecompilation=1 PrintToConsole=1 PrintBefore=igc-image-sampler-resolution''" 2>&1 | FileCheck %s --check-prefixes=CHECK

; CHECK-LABEL: @test_kernel(
; CHECK-NOT: call spir_func void @_ZNSt7complexIdEC2ECd
; CHECK: ret void

; This test checks whether functions with byval arguments were inlined during unification passes.
; igc-image-sampler-solve was chosen because it is only used in unify.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

%"class.std::complex" = type { %structtype.4 }
%structtype.4 = type { double, double }

; Function Attrs: nofree nosync nounwind willreturn
declare i8* @llvm.stacksave() #0

; Function Attrs: nofree nosync nounwind willreturn
declare void @llvm.stackrestore(i8*) #0

; Function Attrs: noinline nounwind
define spir_kernel void @test_kernel(%"class.std::complex" addrspace(1)* noalias %0, %"class.std::complex" addrspace(1)* noalias %1, %"class.std::complex" addrspace(1)* %2, i64 %3) #1 !kernel_arg_addr_space !0 !kernel_arg_access_qual !1 !kernel_arg_type !2 !kernel_arg_type_qual !3 !kernel_arg_base_type !2 !kernel_arg_name !4 !spirv.ParameterDecorations !5 {
  %5 = alloca %"class.std::complex", align 8
  %6 = alloca %structtype.4, align 8
  %7 = alloca %structtype.4, align 8
  %8 = alloca %structtype.4, align 8
  %9 = alloca %structtype.4, align 8
  %10 = addrspacecast %"class.std::complex"* %5 to %"class.std::complex" addrspace(4)*
  %11 = getelementptr inbounds %"class.std::complex", %"class.std::complex" addrspace(1)* %0, i64 0, i32 0, i32 0
  %12 = load double, double addrspace(1)* %11, align 8
  %13 = getelementptr inbounds %"class.std::complex", %"class.std::complex" addrspace(1)* %0, i64 0, i32 0, i32 1
  %14 = load double, double addrspace(1)* %13, align 8
  %15 = shl i64 %3, 9
  %16 = and i64 %15, 4294966784
  %17 = icmp eq i64 %16, 0
  br i1 %17, label %50, label %18

18:                                               ; preds = %4
  %19 = addrspacecast %structtype.4* %6 to %structtype.4 addrspace(4)*
  %20 = addrspacecast %structtype.4* %7 to %structtype.4 addrspace(4)*
  %21 = addrspacecast %structtype.4* %8 to %structtype.4 addrspace(4)*
  %22 = addrspacecast %structtype.4* %9 to %structtype.4 addrspace(4)*
  %23 = getelementptr inbounds %"class.std::complex", %"class.std::complex" addrspace(1)* %1, i64 0, i32 0, i32 0
  %24 = load double, double addrspace(1)* %23, align 8
  %25 = getelementptr inbounds %"class.std::complex", %"class.std::complex" addrspace(1)* %1, i64 0, i32 0, i32 1
  %26 = load double, double addrspace(1)* %25, align 8
  %27 = getelementptr inbounds %structtype.4, %structtype.4* %7, i64 0, i32 0
  %28 = getelementptr inbounds %structtype.4, %structtype.4* %7, i64 0, i32 1
  %29 = getelementptr inbounds %structtype.4, %structtype.4* %8, i64 0, i32 0
  %30 = getelementptr inbounds %structtype.4, %structtype.4* %8, i64 0, i32 1
  %31 = getelementptr inbounds %structtype.4, %structtype.4* %6, i64 0, i32 0
  %32 = getelementptr inbounds %structtype.4, %structtype.4* %6, i64 0, i32 1
  %33 = getelementptr inbounds %structtype.4, %structtype.4* %9, i64 0, i32 0
  %34 = getelementptr inbounds %structtype.4, %structtype.4* %9, i64 0, i32 1
  %35 = getelementptr inbounds %"class.std::complex", %"class.std::complex"* %5, i64 0, i32 0, i32 0
  %36 = getelementptr inbounds %"class.std::complex", %"class.std::complex"* %5, i64 0, i32 0, i32 1
  br label %37

37:                                               ; preds = %37, %18
  %38 = phi i64 [ 0, %18 ], [ %48, %37 ]
  %39 = phi double [ %14, %18 ], [ %47, %37 ]
  %40 = phi double [ %12, %18 ], [ %46, %37 ]
  %41 = call i8* @llvm.stacksave()
  store double %24, double* %27, align 8
  store double %26, double* %28, align 8
  store double %40, double* %29, align 8
  store double %39, double* %30, align 8
  %42 = load double, double* %31, align 8
  %43 = load double, double* %32, align 8
  store double %42, double* %33, align 8
  store double %43, double* %34, align 8
  call spir_func void @_ZNSt7complexIdEC2ECd(%"class.std::complex" addrspace(4)* align 8 %10, %structtype.4 addrspace(4)* byval(%structtype.4) align 8 %22) #2
  call void @llvm.stackrestore(i8* %41)
  %44 = load double, double* %35, align 8
  %45 = load double, double* %36, align 8
  %46 = fadd fast double %40, %44
  %47 = fadd fast double %39, %45
  %48 = add nuw nsw i64 %38, 1
  %49 = icmp eq i64 %48, %16
  br i1 %49, label %.loopexit, label %37

.loopexit:                                        ; preds = %37
  br label %50

50:                                               ; preds = %.loopexit, %4
  %51 = phi double [ %12, %4 ], [ %46, %.loopexit ]
  %52 = phi double [ %14, %4 ], [ %47, %.loopexit ]
  %53 = getelementptr %"class.std::complex", %"class.std::complex" addrspace(1)* %2, i64 0, i32 0, i32 0
  store double %51, double addrspace(1)* %53, align 8
  %54 = getelementptr inbounds %"class.std::complex", %"class.std::complex" addrspace(1)* %2, i64 0, i32 0, i32 1
  store double %52, double addrspace(1)* %54, align 8
  ret void
}

; Function Attrs: nounwind
define linkonce_odr spir_func void @_ZNSt7complexIdEC2ECd(%"class.std::complex" addrspace(4)* align 8 %0, %structtype.4 addrspace(4)* byval(%structtype.4) align 8 %1) #2 {
  %3 = getelementptr inbounds %structtype.4, %structtype.4 addrspace(4)* %1, i64 0, i32 0
  %4 = load double, double addrspace(4)* %3, align 8
  %5 = getelementptr inbounds %structtype.4, %structtype.4 addrspace(4)* %1, i64 0, i32 1
  %6 = load double, double addrspace(4)* %5, align 8
  %7 = getelementptr inbounds %"class.std::complex", %"class.std::complex" addrspace(4)* %0, i64 0, i32 0, i32 0
  store double %4, double addrspace(4)* %7, align 8
  %8 = getelementptr inbounds %"class.std::complex", %"class.std::complex" addrspace(4)* %0, i64 0, i32 0, i32 1
  store double %6, double addrspace(4)* %8, align 8
  ret void
}

attributes #0 = { nofree nosync nounwind willreturn }
attributes #1 = { noinline nounwind }
attributes #2 = { nounwind }

!0 = !{i32 1, i32 1, i32 1, i32 0}
!1 = !{!"none", !"none", !"none", !"none"}
!2 = !{!"class.std::complex*", !"class.std::complex*", !"class.std::complex*", !"long"}
!3 = !{!"restrict", !"restrict", !"", !""}
!4 = !{!"", !"", !"", !""}
!5 = !{!6, !6, !8, !8}
!6 = !{!7}
!7 = !{i32 38, i32 4}
!8 = !{}
