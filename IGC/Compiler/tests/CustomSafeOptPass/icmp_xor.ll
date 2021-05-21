;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2020-2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-custom-safe-opt -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define void @sample_test(i32 %x, i32 %y, i1 %cond, float addrspace(1)* nocapture %res) nounwind {
entry:
  %cmp = icmp slt i32 %x, %y
  %cond.not = xor i1 %cond, true
  %and.cond = and i1 %cmp, %cond.not
  br i1 %and.cond, label %bb1, label %bb2
bb1:
  store float 0.0, float addrspace(1)* %res
  br label %bb3
bb2:
  store float 1.0, float addrspace(1)* %res
  br label %bb3
bb3:
  ret void
}


; CHECK:         [[CONDNEW:%[a-zA-Z0-9]+]] = icmp sge i32 %x, %y
; CHECK-NOT:     and
; CHECK:         [[ORRES:%[a-zA-Z0-9]+]] = or i1 %cond, [[CONDNEW]]
; CHECK:         br i1 [[ORRES:%[a-zA-Z0-9]+]], label %bb2, label %bb1
; CHECK-NOT:     br i1 {{.*}}, label %bb1, label %bb2

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"compOpt", !2}
!2 = !{!"FastRelaxedMath", i1 true}
