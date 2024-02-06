; UNSUPPORTED: system-windows
; REQUIRES: llvm-spirv, regkeys

; Need to backport some SPIRV-LLVM-Translator patches first
; XFAIL: *

; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_unstructured_loop_controls -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device fcs -options " -igc_opts 'ShaderDumpTranslationOnly=1'" 2>&1 | FileCheck %s --check-prefixes=CHECK-LLVM

; ModuleID = 'infinite.cl'
source_filename = "infinite.cl"
target datalayout = "e-p:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "spir"

; Function Attrs: norecurse noreturn nounwind readnone
define spir_kernel void @foo() local_unnamed_addr #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.cond, %entry
  br label %for.cond, !llvm.loop !3
; CHECK-LLVM: define spir_kernel void @foo()
; CHECK-LLVM: br label %for.cond, !llvm.loop ![[MD_1:[0-9]+]]
}

; Function Attrs: norecurse noreturn nounwind readnone
define spir_kernel void @boo() local_unnamed_addr #0 {
entry:
  br label %while.body

while.body:                                       ; preds = %entry, %while.body
  br label %while.body, !llvm.loop !5
; CHECK-LLVM: define spir_kernel void @boo()
; CHECK-LLVM: br label %while.body, !llvm.loop ![[MD_2:[0-9]+]]
}

attributes #0 = { nounwind }

!llvm.module.flags = !{!0}
!opencl.ocl.version = !{!1}
!opencl.spir.version = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 1, i32 2}
!2 = !{!"clang version 9.0.0"}
!3 = distinct !{!3, !4}
!4 = !{!"llvm.loop.max_concurrency.count", i32 2}
!5 = distinct !{!5, !6}
!6 = !{!"llvm.loop.ii.count", i32 2}

; CHECK-LLVM: ![[MD_1]] = distinct !{![[MD_1]], ![[LOOP_MD_1:[0-9]+]]}
; CHECK-LLVM: ![[LOOP_MD_1]] = !{!"llvm.loop.max_concurrency.count", i32 2}
; CHECK-LLVM: ![[MD_2]] = distinct !{![[MD_2]], ![[LOOP_MD_2:[0-9]+]]}
; CHECK-LLVM: ![[LOOP_MD_2]] = !{!"llvm.loop.ii.count", i32 2}
