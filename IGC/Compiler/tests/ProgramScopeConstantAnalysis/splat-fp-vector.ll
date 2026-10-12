;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-22-plus
; RUN: igc_opt --opaque-pointers -igc-programscope-constant-analysis -igc-serialize-metadata -S %s | FileCheck %s
; ------------------------------------------------
; ProgramScopeConstantAnalysis
; ------------------------------------------------

@g_var = addrspace(1) global <2 x float> splat (float 1.000000e+00), align 8

define spir_kernel void @test(ptr addrspace(1) %dst) {
entry:
  %0 = load <2 x float>, ptr addrspace(1) @g_var, align 8
  store <2 x float> %0, ptr addrspace(1) %dst, align 8
  ret void
}

!igc.functions = !{!1}
!IGCMetadata = !{!0}

!0 = !{!"ModuleMD"}
!1 = !{ptr @test, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}

; CHECK-DAG: [[INLINE_BUFFERS_GLOBALS:![0-9]*]] = !{!"inlineBuffersVec[2]", {{![0-9]*}}, [[GLOBALS_ALLOCSIZE:![0-9]*]], [[GLOBALS_BUFFER:![0-9]*]]}
; CHECK-DAG: [[GLOBALS_ALLOCSIZE]] = !{!"allocSize", i{{32|64}} 8}

; @g_var == { 1.0f, 1.0f } == 00 00 80 3f 00 00 80 3f
; CHECK-DAG: [[GLOBALS_BUFFER]] = !{!"Buffer", [[B0:![0-9]*]],
; CHECK-DAG: [[B0]] = !{!"BufferVec[0]", i8 0}
; CHECK-DAG: !{!"BufferVec[1]", i8 0}
; CHECK-DAG: !{!"BufferVec[2]", i8 -128}
; CHECK-DAG: !{!"BufferVec[3]", i8 63}
; CHECK-DAG: !{!"BufferVec[4]", i8 0}
; CHECK-DAG: !{!"BufferVec[5]", i8 0}
; CHECK-DAG: !{!"BufferVec[6]", i8 -128}
; CHECK-DAG: !{!"BufferVec[7]", i8 63}
