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
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options "-igc_opts 'EnableOpaquePointersBackend=1, DisableRecompilation=1 DumpASMToConsole=1''" 2>&1 | FileCheck %s --check-prefixes=CHECK

; LLVM with typed pointers:
; RUN: llvm-as -opaque-pointers=0 %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options "-igc_opts 'DisableRecompilation=1 DumpASMToConsole=1''" 2>&1 | FileCheck %s --check-prefixes=CHECK

; CHECK: (W)     load.ugm.d32x32t.a64 (1|M0)
; CHECK: (W)     load.ugm.d32x32t.a64 (1|M0)
; CHECK: (W)     store.ugm.d32x32t.a64 (1|M0)

define spir_kernel void @test(i32 addrspace(1)* %0, i32 addrspace(1)* %1, i32 addrspace(1)* %2) !reqd_work_group_size !0 !intel_reqd_sub_group_size !1 {
  %4 = call spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32 0)
  %5 = getelementptr inbounds i32, i32 addrspace(1)* %1, i64 %4
  %6 = load i32, i32 addrspace(1)* %5, align 4
  %7 = getelementptr inbounds i32, i32 addrspace(1)* %2, i64 %4
  %8 = load i32, i32 addrspace(1)* %7, align 4
  %9 = add nsw i32 %6, %8
  %10 = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 %4
  store i32 %9, i32 addrspace(1)* %10, align 4
  ret void
}

declare spir_func i64 @_Z33__spirv_BuiltInGlobalInvocationIdi(i32)

attributes #0 = { inaccessiblememonly nofree nosync nounwind willreturn }

!0 = !{i32 32, i32 1, i32 1}
!1 = !{i32 32}
