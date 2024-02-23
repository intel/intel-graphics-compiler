; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, dg2-supported

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_fpga_invocation_pipelining_attributes -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir"

; Function Attrs: convergent nounwind
define spir_kernel void @k(float %a, float %b, float %c) #0 !kernel_arg_addr_space !4 !kernel_arg_access_qual !5 !kernel_arg_type !6 !kernel_arg_type_qual !7 !kernel_arg_base_type !6 !spirv.Decorations !9 {
entry:
  ret void
}

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!2}
!llvm.ident = !{!3}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 0}
!2 = !{i32 1, i32 2}
!3 = !{!"clang version 14.0.0"}
!4 = !{i32 0, i32 0, i32 0}
!5 = !{!"none", !"none", !"none"}
!6 = !{!"float", !"float", !"float"}
!7 = !{!"", !"", !""}
!8 = !{i32 19}
!9 = !{!10, !11}
!10 = !{i32 5919, i32 1}
!11 = !{i32 5917, i32 2}

; CHECK-LLVM-NOT: define spir_kernel void @k(float %a, float %b, float %c) {{.*}} !spirv.Decorations ![[DecoListId:[0-9]+]] {
; CHECK-LLVM: define spir_kernel void @k(float %a, float %b, float %c) {{.*}} {