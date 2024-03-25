;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: ;;;;;;;;;; RUNNERS ;;;;;;;;;;

; RUN: %opt %use_old_pass_manager% -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=XeHPC \
; RUN: -GenXModule -GenXNumberingWrapper -GenXLiveRangesWrapper -GenXCisaBuilderPass -GenXFinalizer \
; RUN: -disable-verify -finalizer-opts="-dumpcommonisa -isaasmToConsole" < %s | FileCheck %s

; CHECK: .kernel_attr NumGRF=256
; CHECK: -autoGRFSelection
define dllexport spir_kernel void @test_256_uint(i32 %arg) #0 {
  ret void
}

; CHECK: .kernel_attr NumGRF=256
; CHECK: -autoGRFSelection
define dllexport spir_kernel void @test_256_node(i32 %arg) #0 {
  ret void
}

; CHECK-NOT: .kernel_attr NumGRF
; CHECK: -autoGRFSelection
define dllexport spir_kernel void @test_auto(i32 %arg) #0 {
  ret void
}

; CHECK: .kernel_attr NumGRF=128
; CHECK: -autoGRFSelection
define dllexport spir_kernel void @test_default(i32 %arg) #0 {
  ret void
}

; CHECK: .kernel_attr NumGRF=128
; CHECK: -autoGRFSelection
define dllexport spir_kernel void @test_invalid(i32 %arg) #0 {
  ret void
}

attributes #0 = { "CMGenxMain" "VC.Stack.Amount"="0" "target-cpu"="XeHPC" }

!genx.kernels = !{!2, !5, !9, !12, !14}
!genx.kernel.internal = !{!3, !6, !10, !13, !15}
!spirv.ExecutionMode = !{!4, !7, !11, !16}

!0 = !{i32 24}
!1 = !{}

!2 = !{void (i32)* @test_256_uint, !"test_256_uint", !0, i32 0, !0, !1, !1, i32 0}
!3 = !{void (i32)* @test_256_uint, null, null, null, null}
!4 = !{void (i32)* @test_256_uint, i32 6461, i32 256}

!5 = !{void (i32)* @test_256_node, !"test_256_node", !0, i32 0, !0, !1, !1, i32 0}
!6 = !{void (i32)* @test_256_node, null, null, null, null}
!7 = !{void (i32)* @test_256_node, i32 6462, !8}
!8 = !{i32 256}

!9 = !{void (i32)* @test_auto, !"test_auto", !0, i32 0, !0, !1, !1, i32 0}
!10 = !{void (i32)* @test_auto, null, null, null, null}
!11 = !{void (i32)* @test_auto, i32 6463, !"AutoINTEL"}

!12 = !{void (i32)* @test_default, !"test_default", !0, i32 0, !0, !1, !1, i32 0}
!13 = !{void (i32)* @test_default, null, null, null, null}

!14 = !{void (i32)* @test_invalid, !"test_invalid", !0, i32 0, !0, !1, !1, i32 0}
!15 = !{void (i32)* @test_invalid, null, null, null, null}
!16 = !{void (i32)* @test_invalid, i32 6461, i32 200}
