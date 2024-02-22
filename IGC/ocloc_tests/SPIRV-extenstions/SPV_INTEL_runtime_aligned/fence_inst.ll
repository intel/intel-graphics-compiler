; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, dg2-supported

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_runtime_aligned -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; CHECK-LLVM: define spir_kernel void @fence_test_kernel1{{.*}} #0 {{.*}}
; CHECK-LLVM-NEXT: call spir_func void @_Z21__spirv_MemoryBarrierii(i32 0, i32 2)

; CHECK-LLVM: define spir_kernel void @fence_test_kernel2{{.*}} #0 {{.*}}
; CHECK-LLVM-NEXT: call spir_func void @_Z21__spirv_MemoryBarrierii(i32 0, i32 4)

; CHECK-LLVM: define spir_kernel void @fence_test_kernel3{{.*}} #0 {{.*}}
; CHECK-LLVM-NEXT: call spir_func void @_Z21__spirv_MemoryBarrierii(i32 0, i32 8)

; CHECK-LLVM: define spir_kernel void @fence_test_kernel4{{.*}} #0 {{.*}}
; CHECK-LLVM-NEXT: call spir_func void @_Z21__spirv_MemoryBarrierii(i32 0, i32 16)

; ModuleID = 'fence_inst.bc'
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spir64"

; Function Attrs: noinline nounwind
define spir_kernel void @fence_test_kernel1(i32 addrspace(1)* noalias %s.ascast) {
  fence acquire
  ret void
}

; Function Attrs: noinline nounwind
define spir_kernel void @fence_test_kernel2(i32 addrspace(1)* noalias %s.ascast) {
  fence release
  ret void
}

; Function Attrs: noinline nounwind
define spir_kernel void @fence_test_kernel3(i32 addrspace(1)* noalias %s.ascast) {
  fence acq_rel
  ret void
}

; Function Attrs: noinline nounwind
define spir_kernel void @fence_test_kernel4(i32 addrspace(1)* noalias %s.ascast) {
  fence syncscope("singlethread") seq_cst
  ret void
}

