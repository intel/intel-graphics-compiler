;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; COM: ;;;;;;;;;; RUNNERS ;;;;;;;;;;

; RUN: %opt %use_old_pass_manager% -march=genx64 -mtriple=spir64-unknown-unknown -mcpu=Xe3P \
; RUN: -GenXModule -GenXNumberingWrapper -GenXLiveRangesWrapper -GenXCisaBuilderPass -GenXFinalizer \
; RUN: -disable-verify -finalizer-opts="-dumpcommonisa -isaasmToConsole" < %s | FileCheck %s

; CHECK: .kernel_attr NumGRF=32
define dllexport spir_kernel void @test_32(i32 %arg) #0 {
  ret void
}

; CHECK: .kernel_attr NumGRF=64
define dllexport spir_kernel void @test_64(i32 %arg) #0 {
  ret void
}

; CHECK: .kernel_attr NumGRF=96
define dllexport spir_kernel void @test_96(i32 %arg) #0 {
  ret void
}

; CHECK: .kernel_attr NumGRF=128
define dllexport spir_kernel void @test_128(i32 %arg) #0 {
  ret void
}

; CHECK: .kernel_attr NumGRF=160
define dllexport spir_kernel void @test_160(i32 %arg) #0 {
  ret void
}

; CHECK: .kernel_attr NumGRF=192
define dllexport spir_kernel void @test_192(i32 %arg) #0 {
  ret void
}

; CHECK: .kernel_attr NumGRF=256
define dllexport spir_kernel void @test_256(i32 %arg) #0 {
  ret void
}

; CHECK: .kernel_attr NumGRF=512
define dllexport spir_kernel void @test_512(i32 %arg) #0 {
  ret void
}

; CHECK: .kernel_attr NumGRF=0
define dllexport spir_kernel void @test_auto(i32 %arg) #0 {
  ret void
}

; CHECK: .kernel_attr NumGRF=128
define dllexport spir_kernel void @test_default(i32 %arg) #0 {
  ret void
}

; CHECK: .kernel_attr NumGRF=128
define dllexport spir_kernel void @test_invalid(i32 %arg) #0 {
  ret void
}

attributes #0 = { "CMGenxMain" "VC.Stack.Amount"="0" "target-cpu"="XeHPC" }

!genx.kernels = !{!2, !5, !8, !11, !14, !17, !20, !23, !26, !29, !31}
!genx.kernel.internal = !{!3, !6, !9, !12, !15, !18, !21, !24, !27, !30, !32}
!spirv.ExecutionMode = !{!4, !7, !10, !13, !16, !19, !22, !25, !28, !33}

!0 = !{i32 24}
!1 = !{}

!2 = !{void (i32)* @test_32, !"test_32", !0, i32 0, !0, !1, !1, i32 0}
!3 = !{void (i32)* @test_32, null, null, null, null}
!4 = !{void (i32)* @test_32, i32 6461, i32 32}

!5 = !{void (i32)* @test_64, !"test_64", !0, i32 0, !0, !1, !1, i32 0}
!6 = !{void (i32)* @test_64, null, null, null, null}
!7 = !{void (i32)* @test_64, i32 6461, i32 64}

!8 = !{void (i32)* @test_96, !"test_96", !0, i32 0, !0, !1, !1, i32 0}
!9 = !{void (i32)* @test_96, null, null, null, null}
!10 = !{void (i32)* @test_96, i32 6461, i32 96}

!11 = !{void (i32)* @test_128, !"test_128", !0, i32 0, !0, !1, !1, i32 0}
!12 = !{void (i32)* @test_128, null, null, null, null}
!13 = !{void (i32)* @test_128, i32 6461, i32 128}

!14 = !{void (i32)* @test_160, !"test_160", !0, i32 0, !0, !1, !1, i32 0}
!15 = !{void (i32)* @test_160, null, null, null, null}
!16 = !{void (i32)* @test_160, i32 6461, i32 160}

!17 = !{void (i32)* @test_192, !"test_192", !0, i32 0, !0, !1, !1, i32 0}
!18 = !{void (i32)* @test_192, null, null, null, null}
!19 = !{void (i32)* @test_192, i32 6461, i32 192}

!20 = !{void (i32)* @test_256, !"test_256", !0, i32 0, !0, !1, !1, i32 0}
!21 = !{void (i32)* @test_256, null, null, null, null}
!22 = !{void (i32)* @test_256, i32 6461, i32 256}

!23 = !{void (i32)* @test_512, !"test_512", !0, i32 0, !0, !1, !1, i32 0}
!24 = !{void (i32)* @test_512, null, null, null, null}
!25 = !{void (i32)* @test_512, i32 6461, i32 512}

!26 = !{void (i32)* @test_auto, !"test_auto", !0, i32 0, !0, !1, !1, i32 0}
!27 = !{void (i32)* @test_auto, null, null, null, null}
!28 = !{void (i32)* @test_auto, i32 6463, !"AutoINTEL"}

!29 = !{void (i32)* @test_default, !"test_default", !0, i32 0, !0, !1, !1, i32 0}
!30 = !{void (i32)* @test_default, null, null, null, null}

!31 = !{void (i32)* @test_invalid, !"test_invalid", !0, i32 0, !0, !1, !1, i32 0}
!32 = !{void (i32)* @test_invalid, null, null, null, null}
!33 = !{void (i32)* @test_invalid, i32 6461, i32 200}
