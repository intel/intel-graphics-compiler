;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; With a stack call the per-HW-thread stride is PrivateMemoryPerFG * simdSize and must
; keep the kernel's `alloca align 64` aligned. With the size pinned to 2020 at SIMD8 the
; unfixed stride is 0x3f20 (32 mod 64); rounded up it is 2048 * 8 = 0x4000.
;
; REQUIRES: regkeys, dg2-supported, llvm-16-plus
;
; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device dg2 \
; RUN:   -options "-igc_opts 'DumpVISAASMToConsole=1,ForcePerThreadPrivateMemorySize=2020,ForceOCLSIMDWidth=8'" \
; RUN:   | FileCheck %s

; CHECK-LABEL: .kernel "test_stackcall_private_alignment"
;
; COM: The mul immediate is the per-HW-thread stride.
; CHECK:      mul (M1_NM, 1) [[OFF:V[0-9]+]](0,0){{.*}} 0x4000:ud
; CHECK-NEXT: add (M1_NM, 1) {{.*}} [[OFF]](0,0)

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

%struct.aligned64 = type { i64, i64, i64, i64, i64, i64, i64, i64 }

define spir_func void @callee(ptr addrspace(1) %out, ptr %s) #1 {
entry:
  %p = getelementptr inbounds i8, ptr %s, i64 40
  %v = load i64, ptr %p, align 8
  store i64 %v, ptr addrspace(1) %out, align 8
  ret void
}

define spir_kernel void @test_stackcall_private_alignment(ptr addrspace(1) %out, i64 %a) #0 !kernel_arg_addr_space !0 !kernel_arg_access_qual !1 !kernel_arg_type !2 !kernel_arg_base_type !2 !kernel_arg_type_qual !3 {
entry:
  %buf = alloca %struct.aligned64, align 64
  %p5 = getelementptr inbounds i8, ptr %buf, i64 40
  store i64 %a, ptr %p5, align 8
  call spir_func void @callee(ptr addrspace(1) %out, ptr %buf) #1
  ret void
}

attributes #0 = { convergent nounwind }
attributes #1 = { convergent noinline nounwind optnone "visaStackCall" }

!0 = !{i32 1, i32 0}
!1 = !{!"none", !"none"}
!2 = !{!"long*", !"long"}
!3 = !{!"", !""}
