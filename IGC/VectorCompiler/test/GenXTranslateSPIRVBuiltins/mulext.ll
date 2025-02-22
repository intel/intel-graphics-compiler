;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt_legacy_typed %use_old_pass_manager% -GenXTranslateSPIRVBuiltins \
; RUN: -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 \
; RUN: -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_legacy_opaque %use_old_pass_manager% -GenXTranslateSPIRVBuiltins \
; RUN: -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 \
; RUN: -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS
; RUN: %opt_new_pm_typed -passes=GenXTranslateSPIRVBuiltins \
; RUN: -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 \
; RUN: -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-TYPED-PTRS
; RUN: %opt_new_pm_opaque -passes=GenXTranslateSPIRVBuiltins \
; RUN: -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 \
; RUN: -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s --check-prefixes=CHECK,CHECK-OPAQUE-PTRS
; ------------------------------------------------
; GenXTranslateSPIRVBuiltins
; ------------------------------------------------
; This test checks that GenXTranslateSPIRVBuiltins translates addc

%struct_pair = type { <16 x i32>, <16 x i32> }

; CHECK: [[UIMAD:%[^ ]*]] = call { <16 x i32>, <16 x i32> } @llvm.genx.uimad.v16i32.v16i32(<16 x i32> {{.*}}, <16 x i32> {{.*}})
; CHECK: [[LO:%[^ ]*]] = extractvalue { <16 x i32>, <16 x i32> } [[UIMAD]], 0
; CHECK: [[HI:%[^ ]*]] = extractvalue { <16 x i32>, <16 x i32> } [[UIMAD]], 1
; CHECK-TYPED-PTRS: [[PTR_CAST:%[^ ]*]] = bitcast %struct_pair addrspace(4)* {{.*}} to <16 x i32> addrspace(4)*
; CHECK-TYPED-PTRS: [[PTR_LO:%[^ ]*]] = getelementptr <16 x i32>, <16 x i32> addrspace(4)* [[PTR_CAST]], i32 0
; CHECK-TYPED-PTRS: [[PTR_HI:%[^ ]*]] = getelementptr <16 x i32>, <16 x i32> addrspace(4)* [[PTR_CAST]], i32 1
; CHECK-TYPED-PTRS: store <16 x i32> [[LO]], <16 x i32> addrspace(4)* [[PTR_LO]], align 64
; CHECK-TYPED-PTRS: store <16 x i32> [[HI]], <16 x i32> addrspace(4)* [[PTR_HI]], align 64
; CHECK-OPAQUE-PTRS: [[PTR_LO:%[^ ]*]] = getelementptr <16 x i32>, ptr addrspace(4) {{.*}}, i32 0
; CHECK-OPAQUE-PTRS: [[PTR_HI:%[^ ]*]] = getelementptr <16 x i32>, ptr addrspace(4) {{.*}}, i32 1
; CHECK-OPAQUE-PTRS: store <16 x i32> [[LO]], ptr addrspace(4) [[PTR_LO]], align 64
; CHECK-OPAQUE-PTRS: store <16 x i32> [[HI]], ptr addrspace(4) [[PTR_HI]], align 64

; Function Attrs: nounwind
define dllexport spir_kernel void @test_umul() #0 {
entry:
  %alloca1 = alloca %struct_pair, align 64
  %alloca2 = addrspacecast %struct_pair* %alloca1 to %struct_pair addrspace(4)*
  %first = add <16 x i32> zeroinitializer, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %second = add <16 x i32> zeroinitializer, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  call spir_func void @_Z20__spirv_UMulExtendedDv16_iS_(%struct_pair addrspace(4)* sret(%struct_pair) %alloca2, <16 x i32> %first, <16 x i32> %second)
  ret void
}
declare spir_func void @_Z20__spirv_UMulExtendedDv16_iS_(%struct_pair addrspace(4)* sret(%struct_pair), <16 x i32>, <16 x i32>)

; CHECK: [[SIMAD:%[^ ]*]] = call { <16 x i32>, <16 x i32> } @llvm.genx.simad.v16i32.v16i32(<16 x i32> {{.*}}, <16 x i32> {{.*}})
; CHECK: [[LO:%[^ ]*]] = extractvalue { <16 x i32>, <16 x i32> } [[SIMAD]], 0
; CHECK: [[HI:%[^ ]*]] = extractvalue { <16 x i32>, <16 x i32> } [[SIMAD]], 1
; CHECK-TYPED-PTRS: [[PTR_CAST:%[^ ]*]] = bitcast %struct_pair addrspace(4)* {{.*}} to <16 x i32> addrspace(4)*
; CHECK-TYPED-PTRS: [[PTR_LO:%[^ ]*]] = getelementptr <16 x i32>, <16 x i32> addrspace(4)* [[PTR_CAST]], i32 0
; CHECK-TYPED-PTRS: [[PTR_HI:%[^ ]*]] = getelementptr <16 x i32>, <16 x i32> addrspace(4)* [[PTR_CAST]], i32 1
; CHECK-TYPED-PTRS: store <16 x i32> [[LO]], <16 x i32> addrspace(4)* [[PTR_LO]], align 64
; CHECK-TYPED-PTRS: store <16 x i32> [[HI]], <16 x i32> addrspace(4)* [[PTR_HI]], align 64
; CHECK-OPAQUE-PTRS: [[PTR_LO:%[^ ]*]] = getelementptr <16 x i32>, ptr addrspace(4) {{.*}}, i32 0
; CHECK-OPAQUE-PTRS: [[PTR_HI:%[^ ]*]] = getelementptr <16 x i32>, ptr addrspace(4) {{.*}}, i32 1
; CHECK-OPAQUE-PTRS: store <16 x i32> [[LO]], ptr addrspace(4) [[PTR_LO]], align 64
; CHECK-OPAQUE-PTRS: store <16 x i32> [[HI]], ptr addrspace(4) [[PTR_HI]], align 64

; Function Attrs: nounwind
define dllexport spir_kernel void @test_smul() #0 {
entry:
  %alloca1 = alloca %struct_pair, align 64
  %alloca2 = addrspacecast %struct_pair* %alloca1 to %struct_pair addrspace(4)*
  %first = add <16 x i32> zeroinitializer, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %second = add <16 x i32> zeroinitializer, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  call spir_func void @_Z20__spirv_SMulExtendedDv16_iS_(%struct_pair addrspace(4)* sret(%struct_pair) %alloca2, <16 x i32> %first, <16 x i32> %second)
  ret void
}
declare spir_func void @_Z20__spirv_SMulExtendedDv16_iS_(%struct_pair addrspace(4)* sret(%struct_pair), <16 x i32>, <16 x i32>)

attributes #0 = { nounwind "CMGenxMain" "oclrt"="1" }

