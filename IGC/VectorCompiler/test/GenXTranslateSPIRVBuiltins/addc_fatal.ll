;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: not %opt %use_old_pass_manager% -GenXTranslateSPIRVBuiltins \
; RUN: -vc-spirv-builtins-bif-path=%VC_SPIRV_OCL_BIF% -march=genx64 \
; RUN: -mtriple=spir64-unknown-unknown -mcpu=XeHPC -S %s 2>&1 | FileCheck %s
; ------------------------------------------------
; GenXTranslateSPIRVBuiltins
; ------------------------------------------------

%str_p = type { <16 x i8>, <16 x i8> }

; CHECK: LLVM ERROR: GenXTranslateSPIRV {{.*}} @_Z17__spirv_IAddCarryDv16_iS_{{.*}} only 32/64-bit addc/subb supported

; Function Attrs: nounwind
define dllexport spir_kernel void @test() #0 {
entry:
  %alloca1 = alloca %str_p, align 64
  %alloca2 = addrspacecast %str_p* %alloca1 to %str_p addrspace(4)*
  %first = add <16 x i8> zeroinitializer, <i8 0, i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7, i8 8, i8 9, i8 10, i8 11, i8 12, i8 13, i8 14, i8 15>
  %second = add <16 x i8> zeroinitializer, <i8 0, i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7, i8 8, i8 9, i8 10, i8 11, i8 12, i8 13, i8 14, i8 15>
  call spir_func void @_Z17__spirv_IAddCarryDv16_iS_(%str_p addrspace(4)* sret(%str_p) %alloca2, <16 x i8> %first, <16 x i8> %second)
  ret void
}
declare spir_func void @_Z17__spirv_IAddCarryDv16_iS_(%str_p addrspace(4)* sret(%str_p), <16 x i8>, <16 x i8>)

attributes #0 = { nounwind "CMGenxMain" "oclrt"="1" }

