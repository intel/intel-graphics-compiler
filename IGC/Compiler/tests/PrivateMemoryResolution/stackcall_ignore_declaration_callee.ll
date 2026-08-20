;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --typed-pointers --GenXCodeGenModule --igc-private-mem-resolution --igc-serialize-metadata -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PrivateMemoryResolution
; ------------------------------------------------
;
; AnalyzeCGPrivateMemUsage must not treat a call to a declaration-only callee
; (no function body) as a genuine stack-call target: such a callee can never
; actually receive arguments through IGC's private-memory-backed stack-call
; ABI, so its formal-parameter size must not be charged against the caller's
; PrivateMemoryPerFG. Only stackcall_callee, a real defined stack-call
; function, may affect the computed value below.
;
; dummy_extern_callee has wide arguments (a <4 x float> and two pointers) so
; that, if its argSize were incorrectly charged, PrivateMemoryPerFG for
; test_kernel would come out larger than the value checked here.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_kernel(<8 x i32> %r0, i8* %privateBase) #1 {
entry:
  %a = alloca i32, align 4
  store i32 1, i32* %a, align 4
  call void @stackcall_callee()
  call void @dummy_extern_callee(<4 x float> zeroinitializer, i8* null, i8* null)
  ret void
}

define void @stackcall_callee() #0 {
  ret void
}

declare void @dummy_extern_callee(<4 x float>, i8*, i8*)

attributes #0 = { nounwind "visaStackCall" }
attributes #1 = { convergent noinline nounwind }

!igc.functions = !{!1, !2}
!IGCMetadata = !{!10}

!1 = !{void (<8 x i32>, i8*)* @test_kernel, !3}
!2 = !{void ()* @stackcall_callee, !5}
!3 = !{!4}
!4 = !{!"function_type", i32 0}
!5 = !{!6}
!6 = !{!"function_type", i32 2}

!10 = !{!"ModuleMD", !11}
!11 = !{!"compOpt", !12}
!12 = !{!"UseScratchSpacePrivateMemory", i1 true}

; CHECK: !"PrivateMemoryPerFG"
; CHECK: !"PrivateMemoryPerFGMap[0]", {{.*}}@test_kernel
; CHECK-NEXT: !"PrivateMemoryPerFGValue[0]", i32 36
