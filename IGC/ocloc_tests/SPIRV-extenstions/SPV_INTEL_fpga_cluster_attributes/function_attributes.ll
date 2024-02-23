; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, dg2-supported

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_fpga_cluster_attributes -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

target triple = "spir64-unknown-unknown"

; CHECK-LLVM: define spir_func void @test_fpga_stallenable_attr() {{.*}} !stall_enable [[STALL_MD:![0-9]+]]
; CHECK-LLVM: define spir_func void @test_fpga_stallfree_attr() {{.*}}
; CHECK-LLVM: [[STALL_MD]] = !{i32 1}

define spir_func void @test_fpga_stallenable_attr() !stall_enable !0 {
entry:
  ret void
}

define spir_func void @test_fpga_stallfree_attr() !stall_free !1 {
entry:
  ret void
}

!0 = !{ i32 1 }
!1 = !{ i32 1 }
