;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -enable-debugify -regkey EnableCodeAssumption=2 -igc-stateless-to-stateful-resolution -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; CodeAssumption : addAssumption on phi part
; ------------------------------------------------

; Check that assumption call were added

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_kernel void @test_phiassume(i32* %dst) {
; CHECK-LABEL: @test_phiassume(
; CHECK:       bb1:
; CHECK-NEXT:    [[TMP0:%.*]] = phi i32 [ 0, %entry ], [ [[TMP1:%.*]], %bb1 ]
; CHECK-NEXT:    [[ASSUMECOND:%.*]] = icmp sge i32 [[TMP0]], 0
; CHECK-NEXT:    call void @llvm.assume(i1 [[ASSUMECOND]])
;
entry:
  br label %bb1
bb1:
  %0 = phi i32 [0, %entry], [%1, %bb1]
  %1 = add i32 %0, 1
  %2 = icmp eq i32 %1, 13
  br i1 %2, label %bb1, label %end
end:
  store i32 %1, i32* %dst
  ret void
}

!igc.functions = !{!1}
!1 = !{void (i32*)* @test_phiassume, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
