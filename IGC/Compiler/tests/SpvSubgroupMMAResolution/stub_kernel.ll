;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt --igc-spv-subgroup-mma-resolution --opaque-pointers --platformpvc -S %s 2>&1 | FileCheck %s

; Clang since https://github.com/llvm/llvm-project/commit/642481a4286c9006958274531ee173b347866c50
; wraps a proper kernel into spir_func, and this spir_func is called from the stub kernel
; @__clang_ocl_...; Make sure subgroup-mma-resolution resolves calls in wrapped kernel properly

target triple = "spir64-unknown-unknown"

declare spir_func i16 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv4_isi(i32, i16, <4 x i32>, i16, i32)

define spir_kernel void @test(i16 %a, <4 x i32> %b, i16 %c, ptr addrspace(1) %out) {
  call spir_func void @__clang_ocl_kern_test(i16 %a, <4 x i32> %b, i16 %c, ptr addrspace(1) %out)
  ret void
}

define spir_func void @__clang_ocl_kern_test(i16 %a, <4 x i32> %b, i16 %c, ptr addrspace(1) %out) !intel_reqd_sub_group_size !10 {
; CHECK-LABEL: define spir_func void @__clang_ocl_kern_test
; CHECK: [[RES:%.*]] = call i16 @__builtin_IB_sub_group32n16_fdpas_bf_bf_bf8_bf8_8_2(i16 %c, i16 %a, <4 x i32> %b)
; CHECK-NEXT: store i16 [[RES]], ptr addrspace(1) %out
; CHECK-NOT: __spirv_SubgroupMatrixMultiplyAccumulateINTEL
  %res = call spir_func i16 @_Z45__spirv_SubgroupMatrixMultiplyAccumulateINTELisDv4_isi(i32 32, i16 %a, <4 x i32> %b, i16 %c, i32 196620)
  store i16 %res, ptr addrspace(1) %out, align 2
  ret void
}

!igc.functions = !{!0}
!IGCMetadata = !{!4}

!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!4 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7}
!6 = !{!"FuncMDMap[0]", ptr @test}
!7 = !{!"FuncMDValue[0]", !8, !9}
!8 = !{!"functionType", !"KernelFunction"}
!9 = !{!"requiredSubGroupSize", i32 32}
!10 = !{i32 32}
