; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys, dg2-supported

; Need to backport some SPIRV-LLVM-Translator patches first
; XFAIL: *

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_kernel_attributes -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; CHECK-LLVM: define spir_kernel void @test_1{{.*}} ![[#NOWAITFORDONEWRITE:]]
; CHECK-LLVM: define spir_kernel void @test_2{{.*}} ![[#WAITFORDONEWRITE:]]
; CHECK-LLVM: ![[#NOWAITFORDONEWRITE:]] = !{!"csr"}
; CHECK-LLVM: ![[#WAITFORDONEWRITE:]] = !{!"csr", !"wait_for_done_write"}

; ModuleID = 'test.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

; Function Attrs: nounwind
define spir_kernel void @test_1() #0 !ip_interface !0
{
entry:
  ret void
}

; Function Attrs: nounwind
define spir_kernel void @test_2() #0 !ip_interface !1
{
entry:
  ret void
}

attributes #0 = { nounwind }

!0 = !{!"csr"}
!1 = !{!"csr", !"wait_for_done_write"}
