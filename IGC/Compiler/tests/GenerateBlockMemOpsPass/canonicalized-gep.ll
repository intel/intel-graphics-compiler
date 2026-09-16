;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-16-plus
; RUN: igc_opt --opaque-pointers %s -S -generate-block-mem-ops -platformpvc | FileCheck %s
;
; LLVM 23 canonicalizes single-index GEP types to equally sized byte arrays.
; Over-align i32 to distinguish allocation stride from the width of a memory access.
target datalayout = "e-p:64:64-i32:64-i64:64-f32:32-f64:64"
target triple = "spir64-unknown-unknown"

%record = type { float, float, float }

define spir_kernel void @test_strides(ptr addrspace(1) %base, i16 %localIdX) {
; CHECK-LABEL: @test_strides(
entry:
  %idx = zext i16 %localIdX to i64

  %p4 = getelementptr [4 x i8], ptr addrspace(1) %base, i64 %idx
  %v4 = load i32, ptr addrspace(1) %p4, align 4
  store i32 %v4, ptr addrspace(1) %p4, align 4
; CHECK: [[V4:%.*]] = call i32 @llvm.genx.GenISA.simdBlockRead.i32.p1(ptr addrspace(1) %p4)
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1.i32(ptr addrspace(1) %p4, i32 [[V4]])

  %p8 = getelementptr [8 x i8], ptr addrspace(1) %base, i64 %idx
  %v8 = load double, ptr addrspace(1) %p8, align 8
  store double %v8, ptr addrspace(1) %p8, align 8
; CHECK: [[V8:%.*]] = call double @llvm.genx.GenISA.simdBlockRead.f64.p1(ptr addrspace(1) %p8)
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1.f64(ptr addrspace(1) %p8, double [[V8]])

  %offset = getelementptr i8, ptr addrspace(1) %p4, i64 16
  %offset.value = load float, ptr addrspace(1) %offset, align 4
  store float %offset.value, ptr addrspace(1) %offset, align 4
; CHECK: [[OFFSET:%.*]] = call float @llvm.genx.GenISA.simdBlockRead.f32.p1(ptr addrspace(1) %offset)
; CHECK: call void @llvm.genx.GenISA.simdBlockWrite.p1.f32(ptr addrspace(1) %offset, float [[OFFSET]])

  %gap = load i32, ptr addrspace(1) %p8, align 4
  store i32 %gap, ptr addrspace(1) %p8, align 4
; CHECK: %gap = load i32, ptr addrspace(1) %p8
; CHECK: store i32 %gap, ptr addrspace(1) %p8

  %record.ptr = getelementptr %record, ptr addrspace(1) %base, i64 %idx
  store float 0.000000e+00, ptr addrspace(1) %record.ptr, align 4
  %field.ptr = getelementptr %record, ptr addrspace(1) %base, i64 %idx, i32 0
  store float 0.000000e+00, ptr addrspace(1) %field.ptr, align 4
; CHECK: store float 0.000000e+00, ptr addrspace(1) %record.ptr
; CHECK: store float 0.000000e+00, ptr addrspace(1) %field.ptr

  %doubled = mul i64 %idx, 2
  %skip.ptr = getelementptr [4 x i8], ptr addrspace(1) %base, i64 %doubled
  %skip = load i32, ptr addrspace(1) %skip.ptr, align 4
  store i32 %skip, ptr addrspace(1) %skip.ptr, align 4
; CHECK: %skip = load i32, ptr addrspace(1) %skip.ptr
; CHECK: store i32 %skip, ptr addrspace(1) %skip.ptr

  %vector.ptr = getelementptr <2 x i32>, ptr addrspace(1) %base, i64 0, i64 %idx
  %vector.value = load double, ptr addrspace(1) %vector.ptr, align 4
; CHECK: %vector.value = load double, ptr addrspace(1) %vector.ptr
; CHECK: ret void
  ret void
}

!igc.functions = !{!0}
!0 = !{ptr @test_strides, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

!IGCMetadata = !{!3}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", ptr @test_strides}
!6 = !{!"FuncMDValue[0]", !7, !11, !15}
!7 = !{!"workGroupWalkOrder", !8, !9, !10}
!8 = !{!"dim0", i32 0}
!9 = !{!"dim1", i32 1}
!10 = !{!"dim2", i32 2}
!11 = !{!"threadGroupSize", !12, !13, !14}
!12 = !{!"dim0", i32 32}
!13 = !{!"dim1", i32 1}
!14 = !{!"dim2", i32 1}
!15 = !{!"implicitArgInfoList", !16}
!16 = !{!"implicitArgInfoListVec[0]", !17}
!17 = !{!"argId", i32 8}
