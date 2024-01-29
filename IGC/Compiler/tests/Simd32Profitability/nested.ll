;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt --regkey PrintToConsole --regkey PSSIMD32HeuristicFP16 --enable-profitability-print --simd32-profit -igc-serialize-metadata --inputps -S < %s 2>&1 | FileCheck %s
; RUN: igc_opt --regkey PrintToConsole --regkey PSSIMD32HeuristicLoopAndDiscard --enable-profitability-print --simd32-profit -igc-serialize-metadata --inputps -S < %s 2>&1 | FileCheck %s
;
; RUN: igc_opt --regkey PrintToConsole --regkey OCLSIMD16SelectionMask=1 --enable-profitability-print --simd32-profit -igc-serialize-metadata --inputocl -S < %s 2>&1 | FileCheck %s
; RUN: igc_opt --regkey PrintToConsole --regkey OCLSIMD16SelectionMask=2 --enable-profitability-print --simd32-profit -igc-serialize-metadata --inputocl -S < %s 2>&1 | FileCheck %s
; RUN: igc_opt --regkey PrintToConsole --regkey OCLSIMD16SelectionMask=4 --enable-profitability-print --simd32-profit -igc-serialize-metadata --inputocl -S < %s 2>&1 | FileCheck %s
;
; RUN: igc_opt --regkey PrintToConsole --enable-profitability-print --simd32-profit -igc-serialize-metadata --inputocl --platformglk -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; Simd32ProfitabilityAnalysis
; ------------------------------------------------

; CHECK: {{.*}}isSimd16Profitable: 1{{.*}}
; CHECK-NEXT: {{.*}}isSimd32Profitable: 0{{.*}}

declare void @use_int(i32)
declare i16 @llvm.genx.GenISA.getLocalID.X()

define void @test_func(i32 %a, i32 addrspace(1)* %ptr) {
entry:
  %local_id = call i16 @llvm.genx.GenISA.getLocalID.X()
  %i32id = zext i16 %local_id to i32
  br label %loop

loop_header:
  %ccc = icmp sge i32 %i32id, 4
  br i1 %ccc, label %loop, label %loop_end

loop:
  %indvar = phi i32 [ 0, %entry ], [ %nextindvar, %loop_header ]

  %ld = load i32, i32 addrspace(1)* %ptr
  %st = add i32 %ld, %i32id
  store i32 %st, i32 addrspace(1)* %ptr

  %nextindvar = add i32 %indvar, 1
  call void @use_int(i32 %nextindvar)

  %cc = icmp sle i32 %i32id, 4
  br i1 %cc, label %loop2, label %loop_foot

loop2:
  %indvar2 = phi i32 [ %nextindvar, %loop ], [ %nextindvar2, %loop2 ]
  %nextindvar2 = sub i32 %indvar2, 1

  call void @use_int(i32 %nextindvar2)

  %cond2 = icmp sle i32 %nextindvar2, 4
  br i1 %cond2, label %loop2, label %loop_foot

loop_foot:
  %id = phi i32 [ 0, %loop ], [ %i32id, %loop2 ]
  %cond = icmp uge i32 %nextindvar, %id
  br i1 %cond, label %loop_header, label %loop_end

loop_end:
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!4}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (i32, i32 addrspace(1)*)* @test_func}
!3 = !{!"FuncMDValue[0]"}
!4 = !{void (i32, i32 addrspace(1)*)* @test_func, !5}
!5 = !{!6}
!6 = !{!"function_type", i32 0}
