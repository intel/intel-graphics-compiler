;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-stateless-to-stateful-resolution -igc-serialize-metadata -platformdg2 | FileCheck %s

; This test verifies if raytracing implicit global buffer is not promoted to stateful, as only stateless mode is supported in the runtime.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @test_implicit_dg(i32 addrspace(1)* %out, i32 %offset, <8 x i32> %r0, <8 x i32> %payloadHeader, i8 addrspace(1)* %globalPointer, i32 %bufferOffset) #1 {
entry:
  %mul = shl nsw i32 %offset, 4
  %idx.ext = sext i32 %mul to i64
  %add.ptr = getelementptr inbounds i8, i8 addrspace(1)* %globalPointer, i64 %idx.ext
  %0 = bitcast i8 addrspace(1)* %add.ptr to i32 addrspace(1)*
  ; CHECK-NOT: %{{.*}} = inttoptr i32 %{{.*}} to i32 addrspace(131073)*
  ; CHECK-NOT: %{{.*}} = load i32, i32 addrspace(131073)* %2, align 4
  ; CHECK: %{{.*}} = load i32, i32 addrspace(1)* %0, align 4
  %1 = load i32, i32 addrspace(1)* %0, align 4
  store i32 %1, i32 addrspace(1)* %out, align 4
  ret void
}

attributes #0 = { nounwind }

!igc.functions = !{!300}

!300 = !{void (i32 addrspace(1)*, i32, <8 x i32>, <8 x i32>, i8 addrspace(1)*, i32)* @test_implicit_dg, !301}
!301 = !{!302, !303}
!302 = !{!"function_type", i32 0}
!303 = !{!"implicit_arg_desc", !304, !305, !306, !307}
!304 = !{i32 0}
!305 = !{i32 1}
!306 = !{i32 48}
!307 = !{i32 15, !308}
!308 = !{!"explicit_arg_num", i32 0}
