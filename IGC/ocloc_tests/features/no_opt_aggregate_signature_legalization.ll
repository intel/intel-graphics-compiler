;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, pvc-supported

; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options "-cl-opt-disable -igc_opts 'EnableOpaquePointersBackend=1'" 2>&1 | FileCheck %s

; CHECK: Build succeeded.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-G1"
target triple = "spir64-unknown-unknown"

%aggregate = type { [2 x i32] }

define internal spir_func %aggregate @make_aggregate(i32 %value) noinline {
entry:
  %element0 = insertvalue %aggregate poison, i32 %value, 0, 0
  %element1 = insertvalue %aggregate %element0, i32 2, 0, 1
  ret %aggregate %element1
}

define spir_kernel void @test_kernel(ptr addrspace(1) %out) {
entry:
  %result = call spir_func %aggregate @make_aggregate(i32 1)
  %value = extractvalue %aggregate %result, 0, 0
  store i32 %value, ptr addrspace(1) %out, align 4
  ret void
}

!opencl.compiler.options = !{!0}

!0 = !{!"-cl-opt-disable"}
