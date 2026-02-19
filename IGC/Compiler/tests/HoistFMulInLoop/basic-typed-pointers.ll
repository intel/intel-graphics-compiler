;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-hoist-fmul-in-loop-pass -S < %s | FileCheck %s
; ------------------------------------------------
; HoistFMulInLoopPass
; ------------------------------------------------

; Test checks that multiply for this pattern is hoisted:
;
;  From:
;    sum = 0;
;    loop {
;      sum += x * ... * y * loopinvirant;
;    }
;
;  To:
;    sum = 0;
;    loop {
;      sum += x * ... * y;
;    }
;    sum = sum * loopinvirant

define void @test_loop(float* %a, float* %b, float %c) {
; CHECK-LABEL: @test_loop(
; CHECK:  entry:
; CHECK:    [[TMP0:%[A-z0-9]*]] = load float, float* [[A:%[A-z0-9]*]], align 4
; CHECK:    [[TMP1:%[A-z0-9]*]] = load float, float* [[B:%[A-z0-9]*]], align 4
; CHECK:    [[TMP2:%[A-z0-9]*]] = fcmp olt float [[TMP0]], [[TMP1]]
; CHECK:    br i1 [[TMP2]], label %[[END:[A-z0-9.]*]], label %[[FOR_BODY:[A-z0-9.]*]]
; CHECK:  [[FOR_BODY]]:
; CHECK:    [[TMP3:%[A-z0-9]*]] = phi float [ [[TMP7:%[A-z0-9]*]], %[[FOR_BODY]] ], [ 0.000000e+00, [[ENTRY:%[A-z0-9]*]] ]
; CHECK:    [[TMP4:%[A-z0-9]*]] = phi float [ [[TMP5:%[A-z0-9]*]], %[[FOR_BODY]] ], [ [[TMP1]], [[ENTRY]] ]
; CHECK:    [[TMP5]] = fmul float [[TMP4]], [[TMP4]]
; CHECK:    [[TMP6:%[A-z0-9]*]] = fmul float [[TMP5]], [[TMP0]]
; CHECK:    [[TMP7]] = fadd float [[TMP5]], [[TMP3]]
; CHECK:    [[TMP8:%[A-z0-9]*]] = fcmp oeq float [[TMP5]], 1.000000e+00
; CHECK:    br i1 [[TMP8]], label %[[FOR_BODY]], label %[[FOR_END:[A-z0-9.]*]]
; CHECK:  [[FOR_END]]:
; CHECK:    [[HOIST:%[A-z0-9]*]] = fmul float [[TMP7]], [[TMP0]]
; CHECK:    [[HOIST1:%[A-z0-9]*]] = fadd float [[HOIST]], [[TMP0]]
; CHECK:    [[TMP9:%[A-z0-9]*]] = fadd float [[HOIST1]], 1.300000e+01
; CHECK:    br label %[[END]]
; CHECK:  [[END]]:
; CHECK:    [[TMP10:%[A-z0-9]*]] = phi float [ [[TMP0]], [[ENTRY]] ], [ [[TMP9]], %[[FOR_END]] ]
; CHECK:    store float [[TMP10]], float* [[A]], align 4
; CHECK:    ret void
;
entry:
  %0 = load float, float* %a, align 4
  %1 = load float, float* %b, align 4
  %2 = fcmp olt float %0, %1
  br i1 %2, label %end, label %for.body

for.body:
  %3 = phi float [ %7, %for.body ], [ %0, %entry ]
  %4 = phi float [ %5, %for.body ], [ %1, %entry ]
  %5 = fmul float %4, %4
  %6 = fmul float %5, %0
  %7 = fadd float %6, %3
  %8 = fcmp oeq float %5, 1.000000e+00
  br i1 %8, label %for.body, label %for.end

for.end:
  %9 = fadd  float %7, 13.0
  br label %end
end:
  %10 = phi float [ %0, %entry ], [ %9, %for.end ]
  store float %10, float* %a, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
