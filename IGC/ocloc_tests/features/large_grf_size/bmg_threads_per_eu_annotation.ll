;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Without VRT, a per-kernel "num-thread-per-eu" annotation selects one of the two
; GRF modes, 128 GRF with 8 threads or 256 GRF with 4 threads, and it overrides
; the module option. No mode has 2 threads, so 2 threads get the next larger
; count, 4.
;
; REQUIRES: regkeys, bmg-supported
;
; RUN: llvm-as %OPAQUE_PTR_FLAG% %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device bmg \
; RUN: -options "-cl-intel-reqd-eu-thread-count 8 -igc_opts 'EnableOpaquePointersBackend=1,VISAOptions=-asmToConsole'" \
; RUN: | FileCheck %s --implicit-check-not HWThreadNumberPerEU
;
; CHECK: //.kernel threads4
; CHECK: //.thread_config {{[.]*}}numGRF=256{{[.]*}}
; CHECK: //.kernel threads2
; CHECK: //.thread_config {{[.]*}}numGRF=256{{[.]*}}
; CHECK: //.kernel module_threads
; CHECK: //.thread_config {{[.]*}}numGRF=128{{[.]*}}

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

@threads4.annotation = private unnamed_addr constant [20 x i8] c"num-thread-per-eu 4\00", section "llvm.metadata"
@threads2.annotation = private unnamed_addr constant [20 x i8] c"num-thread-per-eu 2\00", section "llvm.metadata"
@llvm.global.annotations = appending global [2 x { ptr, ptr, ptr, i32, ptr }] [
  { ptr, ptr, ptr, i32, ptr } { ptr @threads4, ptr @threads4.annotation, ptr undef, i32 undef, ptr undef },
  { ptr, ptr, ptr, i32, ptr } { ptr @threads2, ptr @threads2.annotation, ptr undef, i32 undef, ptr undef }], section "llvm.metadata"

define spir_kernel void @threads4(ptr addrspace(1) %p) !kernel_arg_addr_space !0 !kernel_arg_access_qual !1
    !kernel_arg_type !2 !kernel_arg_type_qual !3 !kernel_arg_base_type !2 !kernel_arg_name !3 {
  store i32 4, ptr addrspace(1) %p, align 4
  ret void
}

define spir_kernel void @threads2(ptr addrspace(1) %p) !kernel_arg_addr_space !0 !kernel_arg_access_qual !1
    !kernel_arg_type !2 !kernel_arg_type_qual !3 !kernel_arg_base_type !2 !kernel_arg_name !3 {
  store i32 2, ptr addrspace(1) %p, align 4
  ret void
}

define spir_kernel void @module_threads(ptr addrspace(1) %p) !kernel_arg_addr_space !0 !kernel_arg_access_qual !1
    !kernel_arg_type !2 !kernel_arg_type_qual !3 !kernel_arg_base_type !2 !kernel_arg_name !3 {
  store i32 0, ptr addrspace(1) %p, align 4
  ret void
}

!0 = !{i32 1}
!1 = !{!"none"}
!2 = !{!"int*"}
!3 = !{!""}
