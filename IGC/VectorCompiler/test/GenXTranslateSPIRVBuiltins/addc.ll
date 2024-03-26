;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %opt %use_old_pass_manager% -GenXTranslateSPIRVBuiltins \
; RUN: -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 \
; RUN: -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S < %s | FileCheck %s
; ------------------------------------------------
; GenXTranslateSPIRVBuiltins
; ------------------------------------------------
; This test checks that GenXTranslateSPIRVBuiltins translates addc

%struct_pair = type { <16 x i32>, <16 x i32> }

; CHECK: [[ADDC:%[^ ]*]] = call { <16 x i32>, <16 x i32> } @llvm.genx.addc.v16i32.v16i32(<16 x i32> {{.*}}, <16 x i32> {{.*}})
; CHECK: [[ADDC_C:%[^ ]*]] = extractvalue { <16 x i32>, <16 x i32> } [[ADDC]], 0
; CHECK: [[ADDC_RES:%[^ ]*]] = extractvalue { <16 x i32>, <16 x i32> } [[ADDC]], 1
; CHECK: [[PTR_CAST:%[^ ]*]] = bitcast %struct_pair{{.*}} to <16 x i32>
; CHECK: [[PTR_RES:%[^ ]*]] = getelementptr <16 x i32>, <16 x i32>{{.*}} [[PTR_CAST]], i32 0
; CHECK: [[PTR_C:%[^ ]*]] = getelementptr <16 x i32>, <16 x i32>{{.*}} [[PTR_CAST]], i32 1
; CHECK: store <16 x i32> [[ADDC_RES]], <16 x i32>{{.*}} [[PTR_RES]], align 64
; CHECK: store <16 x i32> [[ADDC_C]], <16 x i32>{{.*}} [[PTR_C]], align 64

; Function Attrs: nounwind
define dllexport spir_kernel void @test() #0 {
entry:
  %alloca1 = alloca %struct_pair, align 64
  %alloca2 = addrspacecast %struct_pair* %alloca1 to %struct_pair addrspace(4)*
  %first = add <16 x i32> zeroinitializer, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %second = add <16 x i32> zeroinitializer, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  call spir_func void @_Z17__spirv_IAddCarryDv16_iS_(%struct_pair addrspace(4)* sret(%struct_pair) %alloca2, <16 x i32> %first, <16 x i32> %second)
  ret void
}
declare spir_func void @_Z17__spirv_IAddCarryDv16_iS_(%struct_pair addrspace(4)* sret(%struct_pair), <16 x i32>, <16 x i32>)

attributes #0 = { nounwind "CMGenxMain" "oclrt"="1" }

