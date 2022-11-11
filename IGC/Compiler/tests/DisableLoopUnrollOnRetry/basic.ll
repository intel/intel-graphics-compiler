;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -enable-debugify -igc-disable-loop-unroll -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; DisableLoopUnrollOnRetry
; ------------------------------------------------

; Test checks that unroll md is updated

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_loopunroll(i32 %a, i32 %b, i32* %c) {
; CHECK-LABEL: @test_loopunroll(
; CHECK:    br i1 {{.*}}, !llvm.loop [[LOOP_MD:![0-9]*]]
;
entry:
  %0 = add i32 %a, %b
  br label %bb1

bb1:                                              ; preds = %bb1, %entry
  %1 = phi i32 [ %a, %entry ], [ %3, %bb1 ]
  %2 = icmp sgt i32 %1, 13
  %3 = add i32 %1, 13
  br i1 %2, label %bb1, label %end

end:                                              ; preds = %bb1
  store i32 %1, i32* %c, align 4
  ret void
}

; CHECK-DAG: [[LOOP_MD]] = distinct !{[[LOOP_MD]], [[UNROLL_MD:![0-9]*]]}
; CHECK-DAG: [[UNROLL_MD]] = !{!"llvm.loop.unroll.disable"}

