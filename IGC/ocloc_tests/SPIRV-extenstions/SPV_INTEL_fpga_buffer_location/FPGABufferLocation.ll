; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, dg2-supported

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_fpga_buffer_location -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; CHECK-LLVM: define spir_kernel void @test{{.*}} !kernel_arg_buffer_location ![[BUFLOC_MD:[0-9]+]] {{.*}}
; CHECK-LLVM: ![[BUFLOC_MD]] = !{i32 1, i32 2, i32 -1, i32 -1, i32 -1}

; ModuleID = 'buffer_location.cl'
source_filename = "buffer_location.cl"
target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir64-unknown-unknown"

; Function Attrs: norecurse nounwind readnone
define spir_kernel void @test(i32 addrspace(1)* %a, float addrspace(1)* %b, i32 addrspace(1)* %c, i32 %d, i32 %e) local_unnamed_addr !kernel_arg_addr_space !3 !kernel_arg_access_qual !4 !kernel_arg_type !5 !kernel_arg_base_type !5 !kernel_arg_buffer_location !6 {
entry:
  ret void
}

!opencl.enable.FP_CONTRACT = !{}
!opencl.ocl.version = !{!0}
!opencl.spir.version = !{!0}
!opencl.used.extensions = !{!1}
!opencl.used.optional.core.features = !{!1}
!opencl.compiler.options = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 2, i32 0}
!1 = !{}
!2 = !{!""}
!3 = !{i32 1, i32 1, i32 1}
!4 = !{!"none", !"none", !"none"}
!5 = !{!"int*", !"float*", !"int*"}
!6 = !{i32 1, i32 2, i32 -1, i32 -1, i32 3}
