;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; This test won't work with LLVM 16+ due to different representation of OpenCL builtin types (pointers to opaque structs vs TargetExtTy).
; It would work on LLVM 16 with typed pointers forced (in CMake), but not on a default build of IGC with LLVM 16.
; REQUIRES: llvm-spirv, regkeys, dg2-supported, llvm-15-or-older

; LLVM with typed pointers/default pointer typing:
; RUN: llvm-as %s -o %t.bc
; RUN: llvm-spirv %t.bc --spirv-ext=+SPV_INTEL_bindless_images -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device dg2 -options " -igc_opts 'PrintToConsole=1 PrintAfter=igc-spir-metadata-translation'" 2>&1 | FileCheck %s

; CHECK: !{!"spvINTELBindlessImages", i1 true}

target datalayout = "e-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-n8:16:32:64"
target triple = "spir64-unknown-unknown"

%spirv.SampledImage._void_1_0_0_0_0_0_0 = type opaque

define spir_func void @foo(i64 %in) {
  %img = call spir_func %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* @_Z90__spirv_ConvertHandleToSampledImageINTEL_RPU3AS140__spirv_SampledImage__void_1_0_0_0_0_0_0m(i64 %in)
  ret void
}

declare spir_func %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* @_Z90__spirv_ConvertHandleToSampledImageINTEL_RPU3AS140__spirv_SampledImage__void_1_0_0_0_0_0_0m(i64)

!opencl.spir.version = !{!0}
!spirv.Source = !{!1}
!llvm.ident = !{!2}

!0 = !{i32 1, i32 2}
!1 = !{i32 4, i32 100000}
!2 = !{!"clang version 14.0.0"}
