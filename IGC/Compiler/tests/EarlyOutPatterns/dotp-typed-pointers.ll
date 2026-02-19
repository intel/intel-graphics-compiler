;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-early-out-patterns-pass -S -inputcs < %s | FileCheck %s
; ------------------------------------------------
; EarlyOutPatterns
; ------------------------------------------------

define spir_kernel void @test_earlyout(i32 %a, i32 %b, i32 %c, float* %d) {
; CHECK-LABEL: @test_earlyout(
; CHECK:  entry:
; CHECK:    [[BT1:%.*]] = uitofp i32 [[A:%.*]] to float
; CHECK:    [[FMUL1:%.*]] = fmul float [[BT1]], [[BT1]]
; CHECK:    [[BT2:%.*]] = uitofp i32 [[B:%.*]] to float
; CHECK:    [[FMUL2:%.*]] = fmul float [[BT2]], [[BT2]]
; CHECK:    [[BT3:%.*]] = uitofp i32 [[C:%.*]] to float
; CHECK:    [[FMUL3:%.*]] = fmul float [[BT3]], [[BT3]]
; CHECK:    [[FADD1:%.*]] = fadd float [[FMUL2]], [[FMUL3]]
; CHECK:    [[TMP0:%.*]] = fadd float [[FMUL1]], [[FADD1]]
; CHECK:    [[TMP1:%.*]] = fcmp fast ogt float [[TMP0]], 0.000000e+00
; CHECK:    [[TMP2:%.*]] = icmp eq i1 [[TMP1]], false
; CHECK:    br i1 [[TMP2]], label %[[EO_IF:[A-z0-9_]*]], label %[[EO_ELSE:[A-z0-9_]*]]
; CHECK;  [[EO_ELSE]]:
; CHECK:    [[TMP3:%.*]] = sext i1 [[TMP1]] to i32
; CHECK:    [[TMP4:%.*]] = and i32 [[A]], [[TMP3]]
; CHECK:    [[TMP5:%.*]] = and i32 [[B]], [[TMP3]]
; CHECK:    [[TMP6:%.*]] = and i32 [[C]], [[TMP3]]
; CHECK:    [[TMP7:%.*]] = bitcast i32 [[TMP4]] to float
; CHECK:    [[TMP8:%.*]] = bitcast i32 [[TMP5]] to float
; CHECK:    [[TMP9:%.*]] = bitcast i32 [[TMP6]] to float
; CHECK:    [[TMP10:%.*]] = fmul fast float [[BT3]], [[TMP7]]
; CHECK:    [[TMP11:%.*]] = fmul fast float [[BT2]], [[TMP8]]
; CHECK:    [[TMP12:%.*]] = fmul fast float [[BT1]], [[TMP9]]
; CHECK:    [[TMP13:%.*]] = call fast float @llvm.maxnum.f32(float [[TMP10]], float 0.000000e+00)
; CHECK:    [[TMP14:%.*]] = call fast float @llvm.maxnum.f32(float [[TMP11]], float 0.000000e+00)
; CHECK:    [[TMP15:%.*]] = call fast float @llvm.maxnum.f32(float [[TMP12]], float 0.000000e+00)
; CHECK:    br label %[[EO_ENDIF:[A-z0-9_]*]]
; CHECK;  [[EO_IF]]:
; CHECK:    [[TMP16:%.*]] = sext i1 [[TMP1]] to i32
; CHECK:    [[TMP17:%.*]] = and i32 [[A]], 0
; CHECK:    [[TMP18:%.*]] = and i32 [[B]], 0
; CHECK:    [[TMP19:%.*]] = and i32 [[C]], 0
; CHECK:    [[TMP20:%.*]] = bitcast i32 0 to float
; CHECK:    [[TMP21:%.*]] = bitcast i32 0 to float
; CHECK:    [[TMP22:%.*]] = bitcast i32 0 to float
; CHECK:    [[TMP23:%.*]] = fmul fast float [[BT3]], 0.000000e+00
; CHECK:    [[TMP24:%.*]] = fmul fast float [[BT2]], 0.000000e+00
; CHECK:    [[TMP25:%.*]] = fmul fast float [[BT1]], 0.000000e+00
; CHECK:    [[TMP26:%.*]] = call fast float @llvm.maxnum.f32(float 0.000000e+00, float 0.000000e+00)
; CHECK:    [[TMP27:%.*]] = call fast float @llvm.maxnum.f32(float 0.000000e+00, float 0.000000e+00)
; CHECK:    [[TMP28:%.*]] = call fast float @llvm.maxnum.f32(float 0.000000e+00, float 0.000000e+00)
; CHECK:    br label %[[EO_ENDIF]]
; CHECK:  [[EO_ENDIF]]:
; CHECK:    [[TMP29:%.*]] = phi float [ [[TMP15]], %[[EO_ELSE]] ], [ 0.000000e+00, %[[EO_IF]] ]
; CHECK:    [[TMP30:%.*]] = phi float [ [[TMP14]], %[[EO_ELSE]] ], [ 0.000000e+00, %[[EO_IF]] ]
; CHECK:    [[TMP31:%.*]] = phi float [ [[TMP13]], %[[EO_ELSE]] ], [ 0.000000e+00, %[[EO_IF]] ]
; CHECK:    [[TMP32:%.*]] = fadd float [[TMP31]], [[TMP30]]
; CHECK:    [[TMP33:%.*]] = fadd float [[TMP32]], [[TMP29]]
; CHECK:    store float [[TMP33]], float* [[D:%.*]], align 4
; CHECK:    ret void
;
entry:
  %bt1 = uitofp i32 %a to float
  %bt2 = uitofp i32 %b to float
  %bt3 = uitofp i32 %c to float
  %fmul1 = fmul float %bt1, %bt1
  %fmul2 = fmul float %bt2, %bt2
  %fmul3 = fmul float %bt3, %bt3
  %fadd1 = fadd float %fmul2, %fmul3
  %0 = fadd float %fmul1, %fadd1
  %1 = fcmp fast ogt float %0, 0.000000e+00
  %2 = sext i1 %1 to i32
  %3 = and i32 %a, %2
  %4 = and i32 %b, %2
  %5 = and i32 %c, %2
  %6 = bitcast i32 %3 to float
  %7 = bitcast i32 %4 to float
  %8 = bitcast i32 %5 to float
  %9 = fmul fast float %bt3, %6
  %10 = fmul fast float %bt2, %7
  %11 = fmul fast float %bt1, %8
  %12 = call fast float @llvm.maxnum.f32(float %9, float 0.000000e+00)
  %13 = call fast float @llvm.maxnum.f32(float %10, float 0.000000e+00)
  %14 = call fast float @llvm.maxnum.f32(float %11, float 0.000000e+00)
  %15 = fadd float %12, %13
  %16 = fadd float %15, %14
  store float %16, float* %d, align 4
  ret void
}

declare float @llvm.maxnum.f32(float, float)
