;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Regression test: per-kernel "num-thread-per-eu 0" annotation must be honoured
; by the SIMD32-vs-SIMD16 selection heuristic on XE2 (BMG).
;
; REQUIRES: regkeys, bmg-supported, llvm-14-plus
;
; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device bmg -options "-igc_opts 'EnableOpaquePointersBackend=1,AllowSIMD16DropForXE2Plus=1,VISAOptions=-asmToConsole'" | FileCheck %s
;
; Kernel must compile at SIMD32 (not be dropped to SIMD16).
; CHECK: math.exp (32|M0)

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

; Per-kernel auto large-GRF mode annotation -- the key item under test.
@gVar = private unnamed_addr constant [20 x i8] c"num-thread-per-eu 0\00", section "llvm.metadata"
@llvm.global.annotations = appending global [1 x { ptr, ptr, ptr, i32, ptr }] [{ ptr, ptr, ptr, i32, ptr } { ptr @_ZTS13KernelGRFAuto, ptr @gVar, ptr undef, i32 undef, ptr undef }], section "llvm.metadata"

; Function Attrs: nounwind
define spir_kernel void @_ZTS13KernelGRFAuto(ptr addrspace(1) align 4 %0, i32 %1) #0
    !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5
    !kernel_arg_type_qual !6 !kernel_arg_base_type !5 !kernel_arg_name !6 {
  %3 = alloca [16 x float], align 4
  %4 = alloca [16 x float], align 4
  %5 = alloca [16 x float], align 4
  %6 = alloca [16 x float], align 4
  %7 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0) #4
  %8 = insertelement <3 x i64> undef, i64 %7, i32 0
  %9 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 1) #4
  %10 = insertelement <3 x i64> %8, i64 %9, i32 1
  %11 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 2) #4
  %12 = insertelement <3 x i64> %10, i64 %11, i32 2
  %13 = extractelement <3 x i64> %12, i32 0
  %14 = select i1 true, i64 %13, i64 0
  %15 = icmp ult i64 %14, 2147483648
  call void @llvm.assume(i1 %15)
  %16 = sext i32 %1 to i64
  %17 = icmp ult i64 %14, %16
  br i1 %17, label %18, label %134

18:
  %19 = getelementptr inbounds float, ptr addrspace(1) %0, i64 %14
  %20 = load float, ptr addrspace(1) %19, align 4
  %21 = bitcast ptr %3 to ptr
  call void @llvm.lifetime.start.p0(i64 64, ptr %21)
  %22 = bitcast ptr %4 to ptr
  call void @llvm.lifetime.start.p0(i64 64, ptr %22)
  %23 = bitcast ptr %5 to ptr
  call void @llvm.lifetime.start.p0(i64 64, ptr %23)
  %24 = bitcast ptr %6 to ptr
  call void @llvm.lifetime.start.p0(i64 64, ptr %24)
  br label %25

25:
  %26 = phi i32 [ 0, %18 ], [ %29, %28 ]
  %27 = icmp ult i32 %26, 16
  br i1 %27, label %28, label %.preheader3

.preheader3:
  br label %38

28:
  %29 = add nuw nsw i32 %26, 1
  %30 = uitofp i32 %29 to float
  %31 = fmul reassoc nsz arcp contract float %20, %30
  %32 = uitofp i32 %26 to float
  %33 = fadd reassoc nsz arcp contract float %20, %32
  %34 = call reassoc nsz arcp contract spir_func float @_Z15__spirv_ocl_sinf(float %33) #0
  %35 = fadd reassoc nsz arcp contract float %31, %34
  %36 = zext i32 %26 to i64
  %37 = getelementptr inbounds [16 x float], ptr %3, i64 0, i64 %36
  store float %35, ptr %37, align 4
  br label %25

38:
  %39 = phi i32 [ %54, %41 ], [ 0, %.preheader3 ]
  %40 = icmp ult i32 %39, 16
  br i1 %40, label %41, label %.preheader2

.preheader2:
  br label %55

41:
  %42 = zext i32 %39 to i64
  %43 = getelementptr inbounds [16 x float], ptr %3, i64 0, i64 %42
  %44 = load float, ptr %43, align 4
  %45 = call reassoc nsz arcp contract spir_func float @_Z15__spirv_ocl_cosf(float %44) #0
  %46 = sub nuw nsw i32 15, %39
  %47 = zext i32 %46 to i64
  %48 = getelementptr inbounds [16 x float], ptr %3, i64 0, i64 %47
  %49 = load float, ptr %48, align 4
  %50 = fmul reassoc nsz arcp contract float %49, 0x3F847AE140000000
  %51 = call reassoc nsz arcp contract spir_func float @_Z15__spirv_ocl_expf(float %50) #0
  %52 = fmul reassoc nsz arcp contract float %45, %51
  %53 = getelementptr inbounds [16 x float], ptr %4, i64 0, i64 %42
  store float %52, ptr %53, align 4
  %54 = add nuw nsw i32 %39, 1
  br label %38

55:
  %56 = phi i32 [ %65, %58 ], [ 0, %.preheader2 ]
  %57 = icmp ult i32 %56, 16
  br i1 %57, label %58, label %.preheader1

.preheader1:
  br label %78

58:
  %59 = zext i32 %56 to i64
  %60 = getelementptr inbounds [16 x float], ptr %3, i64 0, i64 %59
  %61 = load float, ptr %60, align 4
  %62 = getelementptr inbounds [16 x float], ptr %4, i64 0, i64 %59
  %63 = load float, ptr %62, align 4
  %64 = fmul reassoc nsz arcp contract float %61, %63
  %65 = add nuw nsw i32 %56, 1
  %66 = and i32 %65, 15
  %67 = zext i32 %66 to i64
  %68 = getelementptr inbounds [16 x float], ptr %3, i64 0, i64 %67
  %69 = load float, ptr %68, align 4
  %70 = add nuw nsw i32 %56, 3
  %71 = and i32 %70, 15
  %72 = zext i32 %71 to i64
  %73 = getelementptr inbounds [16 x float], ptr %4, i64 0, i64 %72
  %74 = load float, ptr %73, align 4
  %75 = fmul reassoc nsz arcp contract float %69, %74
  %76 = fadd reassoc nsz arcp contract float %64, %75
  %77 = getelementptr inbounds [16 x float], ptr %5, i64 0, i64 %59
  store float %76, ptr %77, align 4
  br label %55

78:
  %79 = phi i32 [ %109, %81 ], [ 0, %.preheader1 ]
  %80 = icmp ult i32 %79, 16
  br i1 %80, label %81, label %.preheader

.preheader:
  br label %110

81:
  %82 = zext i32 %79 to i64
  %83 = getelementptr inbounds [16 x float], ptr %5, i64 0, i64 %82
  %84 = load float, ptr %83, align 4
  %85 = call reassoc nsz arcp contract spir_func float @_Z15__spirv_ocl_sinf(float %84) #0
  %86 = add nuw nsw i32 %79, 7
  %87 = and i32 %86, 15
  %88 = zext i32 %87 to i64
  %89 = getelementptr inbounds [16 x float], ptr %5, i64 0, i64 %88
  %90 = load float, ptr %89, align 4
  %91 = call reassoc nsz arcp contract spir_func float @_Z15__spirv_ocl_cosf(float %90) #0
  %92 = fadd reassoc nsz arcp contract float %85, %91
  %93 = getelementptr inbounds [16 x float], ptr %3, i64 0, i64 %82
  %94 = load float, ptr %93, align 4
  %95 = add nuw nsw i32 %79, 5
  %96 = and i32 %95, 15
  %97 = zext i32 %96 to i64
  %98 = getelementptr inbounds [16 x float], ptr %4, i64 0, i64 %97
  %99 = load float, ptr %98, align 4
  %100 = fmul reassoc nsz arcp contract float %94, %99
  %101 = fadd reassoc nsz arcp contract float %92, %100
  %102 = add nuw nsw i32 %79, 11
  %103 = and i32 %102, 15
  %104 = zext i32 %103 to i64
  %105 = getelementptr inbounds [16 x float], ptr %5, i64 0, i64 %104
  %106 = load float, ptr %105, align 4
  %107 = fadd reassoc nsz arcp contract float %101, %106
  %108 = getelementptr inbounds [16 x float], ptr %6, i64 0, i64 %82
  store float %107, ptr %108, align 4
  %109 = add nuw nsw i32 %79, 1
  br label %78

110:
  %111 = phi float [ %132, %119 ], [ 0.000000e+00, %.preheader ]
  %112 = phi i32 [ %133, %119 ], [ 0, %.preheader ]
  %113 = icmp ult i32 %112, 16
  br i1 %113, label %119, label %114

114:
  store float %111, ptr addrspace(1) %19, align 4
  %115 = bitcast ptr %6 to ptr
  call void @llvm.lifetime.end.p0(i64 64, ptr %115)
  %116 = bitcast ptr %5 to ptr
  call void @llvm.lifetime.end.p0(i64 64, ptr %116)
  %117 = bitcast ptr %4 to ptr
  call void @llvm.lifetime.end.p0(i64 64, ptr %117)
  %118 = bitcast ptr %3 to ptr
  call void @llvm.lifetime.end.p0(i64 64, ptr %118)
  br label %134

119:
  %120 = zext i32 %112 to i64
  %121 = getelementptr inbounds [16 x float], ptr %3, i64 0, i64 %120
  %122 = load float, ptr %121, align 4
  %123 = getelementptr inbounds [16 x float], ptr %4, i64 0, i64 %120
  %124 = load float, ptr %123, align 4
  %125 = fadd reassoc nsz arcp contract float %122, %124
  %126 = getelementptr inbounds [16 x float], ptr %5, i64 0, i64 %120
  %127 = load float, ptr %126, align 4
  %128 = fadd reassoc nsz arcp contract float %125, %127
  %129 = getelementptr inbounds [16 x float], ptr %6, i64 0, i64 %120
  %130 = load float, ptr %129, align 4
  %131 = fadd reassoc nsz arcp contract float %128, %130
  %132 = fadd reassoc nsz arcp contract float %111, %131
  %133 = add nuw nsw i32 %112, 1
  br label %110

134:
  ret void
}

declare void @llvm.assume(i1 noundef) #1
declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture) #2
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture) #2
declare spir_func float @_Z15__spirv_ocl_sinf(float) #3
declare spir_func float @_Z15__spirv_ocl_cosf(float) #3
declare spir_func float @_Z15__spirv_ocl_expf(float) #3
declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32) #4

attributes #0 = { nounwind }
attributes #1 = { nocallback nofree nosync nounwind willreturn }
attributes #2 = { nocallback nofree nosync nounwind willreturn }
attributes #3 = { nounwind }
attributes #4 = { nounwind willreturn }

!3 = !{i32 1, i32 0}
!4 = !{!"none", !"none"}
!5 = !{!"float*", !"uint"}
!6 = !{!"", !""}
