;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify -igc-loop-hoist-constant -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LoopHoistConstant
; ------------------------------------------------

; Test checks that loop body is splited to if-then-else block,
; with minnum uses in original body replaced by it's operands,
; and uses outside of body replaced by corresponding phi's
; Debugify used to check that debug intrinsics don't affect pass

define void @test_loop(float %a, float %lsize, float* %b) {

; Prehead is not modified
;
; CHECK: @test_loop(
; CHECK:  prehead:
; CHECK:    [[TMP0:%.*]] = load float, float* %b
; CHECK:    br label %[[BODY:[A-z0-9.]*]]

prehead:
  %0 = load float, float* %b
  br label %body

; Loop body is split to if-then-else
;
; CHECK:  [[BODY]]:
; CHECK:    [[TMP1:%.*]] = phi float [ [[TMP0]], %[[PREHEAD:[A-z0-9.]*]] ], [ [[TMP2:%.*]], %[[BODY_END_HOIST:[A-z0-9.]*]] ]
; CHECK:    [[TMP2]] = fmul float [[TMP1]], %a
; CHECK:    [[TMP3:%.*]] = fcmp ult float [[TMP2]], [[LSIZE:%.*]]
; CHECK:    br i1 [[TMP3]], label %[[BODY_IF_HOIST:[A-z0-9.]*]], label %[[BODY_ELSE_HOIST:[A-z0-9.]*]]
; CHECK:  [[BODY_IF_HOIST]]:
; CHECK:    [[TMP4:%.*]] = call float @llvm.minnum.f32(float [[TMP2]], float [[LSIZE]])
; CHECK:    [[TMP5:%.*]] = fsub float [[TMP2]], [[TMP1]]
; CHECK:    [[TMP6:%.*]] = fdiv float [[TMP2]], [[TMP1]]
; CHECK:    [[TMP7:%.*]] = fcmp ult float [[TMP2]], [[LSIZE]]
; CHECK:    br label %[[BODY_END_HOIST]]
; CHECK:  [[BODY_ELSE_HOIST]]:
; CHECK:    [[TMP8:%.*]] = call float @llvm.minnum.f32(float [[TMP2]], float [[LSIZE]])
; CHECK:    [[TMP9:%.*]] = fsub float [[LSIZE]], [[TMP1]]
; CHECK:    [[TMP10:%.*]] = fdiv float [[LSIZE]], [[TMP1]]
; CHECK:    [[TMP11:%.*]] = fcmp ult float [[TMP2]], [[LSIZE]]
; CHECK:    br label %[[BODY_END_HOIST]]
; CHECK:  [[BODY_END_HOIST]]:
; CHECK:    [[TMP12:%.*]] = phi float [ [[TMP6]], %[[BODY_IF_HOIST]] ], [ [[TMP10]], %[[BODY_ELSE_HOIST]] ]
; CHECK:    [[TMP13:%.*]] = phi i1 [ [[TMP7]], %[[BODY_IF_HOIST]] ], [ [[TMP11]], %[[BODY_ELSE_HOIST]] ]
; CHECK:    br i1 [[TMP13]], label %[[BODY]], label %[[END:[A-z0-9.]*]]

body:
  %1 = phi float [%0, %prehead], [%2, %body]
  %2 = fmul float %1, %a
  %3 = call float @llvm.minnum.f32(float %2, float %lsize)
  %4 = fsub float %3, %1
  %5 = fdiv float %3, %1
  %6 = fcmp ult float %2, %lsize
  br i1 %6, label %body, label %end

; End block is not modified
;
; CHECK:  [[END]]:
; CHECK:    store float [[TMP12]], float* %b
; CHECK:    ret void

 end:
  store float %5, float* %b
  ret void
}
declare float @llvm.minnum.f32(float, float)
