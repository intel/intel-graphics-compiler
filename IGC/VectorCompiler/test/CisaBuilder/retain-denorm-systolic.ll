;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

target triple = "genx64-unknown-unknown"

; RUN: %opt %use_old_pass_manager% -GenXModule -GenXCategoryWrapper -GenXCisaBuilderPass -GenXFinalizer \
; RUN: -march=genx64 -mtriple=spir64-unknown-unknown -finalizer-opts="-dumpcommonisa -isaasmToConsole" \
; RUN: -mcpu=XeHPC < %s | FileCheck %s --check-prefix=FLUSH

; RUN: %opt %use_old_pass_manager% -GenXModule -GenXCategoryWrapper -GenXCisaBuilderPass -GenXFinalizer \
; RUN: -march=genx64 -mtriple=spir64-unknown-unknown -finalizer-opts="-dumpcommonisa -isaasmToConsole" \
; RUN: -mcpu=Xe2 < %s | FileCheck %s --check-prefix=RETAIN

; FLUSH-NOT: and (M1, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0>
; FLUSH-NOT: or (M1, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0>

; RETAIN: and (M1, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0xfffffb0f:ud
; RETAIN: or (M1, 1) %cr0(0,0)<1> %cr0(0,0)<0;1,0> 0x40000000:ud

define dllexport spir_kernel void @the_test(i32 %0, i32 %1) #0 {
  ret void
}

attributes #0 = { "CMGenxMain" "CMFloatControl"="0" }

!spirv.Source = !{!0}
!opencl.spir.version = !{!1}
!opencl.ocl.version = !{!0}
!opencl.used.extensions = !{!2}
!opencl.used.optional.core.features = !{!2}
!spirv.Generator = !{!3}
!genx.kernels = !{!4}
!genx.kernel.internal = !{!8}

!0 = !{i32 0, i32 0}
!1 = !{i32 1, i32 2}
!2 = !{}
!3 = !{i16 6, i16 14}
!4 = !{void (i32, i32)* @the_test, !"the_test", !5, i32 0, !6, !0, !7, i32 0}
!5 = !{i32 2, i32 2}
!6 = !{i32 64, i32 68}
!7 = !{!"buffer_t", !"buffer_t"}
!8 = !{void (i32, i32)* @the_test, null, null, null, null}
