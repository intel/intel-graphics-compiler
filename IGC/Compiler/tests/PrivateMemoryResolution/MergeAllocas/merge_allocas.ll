;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --igc-ocl-merge-allocas --igc-private-mem-resolution -S %s --platformpvc | FileCheck %s
; ------------------------------------------------
; PrivateMemoryResolution
; ------------------------------------------------

; Check that allocas are merged before private memory resolution
declare spir_func void @__itt_offload_wi_start_wrapper()

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture) #0

; Function Attrs: argmemonly nofree nosync nounwind willreturn
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture) #0

; Function Attrs: inaccessiblememonly nofree nosync nounwind willreturn
declare void @llvm.assume(i1 noundef) #1

declare spir_func void @__itt_offload_wi_finish_wrapper()

define spir_kernel void @main(float addrspace(1)* %0, i64 %1, i64 %2, i32 %3, i32 %4, i32 %5) {
; CHECK-LABEL: main
; CHECK-NEXT: alloca [128 x float], align 4
; CHECK-NOT: alloca [128 x float], align 4
  %7 = alloca [128 x float], align 4
  %8 = alloca [128 x float], align 4
  %9 = call spir_func i64 @_Z13get_global_idj()
  %10 = call spir_func i64 @_Z17get_global_offsetj()
  %11 = sub i64 %1, 0
  %12 = trunc i64 %1 to i32
  %13 = and i32 %3, 31
  %14 = and i32 %3, 1
  br label %15

15:                                               ; preds = %19, %6
  %16 = phi i32 [ 0, %6 ], [ 1, %19 ]
  %17 = phi i32 [ 0, %6 ], [ 1, %19 ]
  %18 = icmp ult i32 %16, %3
  br i1 %18, label %19, label %.preheader1

.preheader1:                                      ; preds = %15
  br label %22

19:                                               ; preds = %15
  %20 = add nuw nsw i32 0, 1
  %21 = add nuw nsw i32 0, 1
  br label %15

22:                                               ; preds = %27, %.preheader1
  %23 = phi i32 [ 0, %27 ], [ 0, %.preheader1 ]
  %24 = phi float [ %31, %27 ], [ 0.000000e+00, %.preheader1 ]
  %25 = phi i32 [ 1, %27 ], [ 0, %.preheader1 ]
  %26 = icmp ult i32 %25, %4
  br i1 %26, label %27, label %34

27:                                               ; preds = %22
  %28 = add nsw i32 0, 0
  %29 = sext i32 %17 to i64
  %30 = getelementptr inbounds [128 x float], [128 x float]* %7, i64 0, i64 %29
  %31 = load float, float* %30, align 4
  %32 = fadd reassoc nsz arcp contract float 0.000000e+00, %31
  %33 = add nuw nsw i32 0, 1
  br label %22

34:                                               ; preds = %22
  br label %35

35:                                               ; preds = %34
  br label %.preheader

.preheader:                                       ; preds = %35
  br label %36

36:                                               ; preds = %41, %.preheader
  %37 = phi i32 [ %42, %41 ], [ 0, %.preheader ]
  %38 = phi float [ %45, %41 ], [ 0.000000e+00, %.preheader ]
  %39 = phi i32 [ 1, %41 ], [ 0, %.preheader ]
  %40 = icmp ult i32 %39, %3
  br i1 %40, label %41, label %48

41:                                               ; preds = %36
  %42 = add nsw i32 %37, 1
  %43 = sext i32 %42 to i64
  %44 = getelementptr inbounds [128 x float], [128 x float]* %8, i64 0, i64 %43
  %45 = load float, float* %44, align 4
  %46 = fadd reassoc nsz arcp contract float 0.000000e+00, %45
  %47 = add nuw nsw i32 0, 1
  br label %36

48:                                               ; preds = %36
  %49 = fadd reassoc nsz arcp contract float %24, %38
  %50 = getelementptr inbounds float, float addrspace(1)* %0, i64 %2
  store float %49, float addrspace(1)* null, align 4
  ret void
}

declare spir_func i64 @_Z13get_global_idj()

declare spir_func i64 @_Z17get_global_offsetj()

attributes #0 = { argmemonly nofree nosync nounwind willreturn }
attributes #1 = { inaccessiblememonly nofree nosync nounwind willreturn }
