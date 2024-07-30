;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt --regkey PrintToConsole --regkey PSSIMD32HeuristicFP16 --enable-profitability-print --simd32-profit -igc-serialize-metadata --inputps -S < %s 2>&1 | FileCheck %s --check-prefix=ONLY16
; RUN: igc_opt --regkey PrintToConsole --regkey PSSIMD32HeuristicLoopAndDiscard --enable-profitability-print --simd32-profit -igc-serialize-metadata --inputps -S < %s 2>&1 | FileCheck %s --check-prefix=ONLY16
;
; RUN: igc_opt --regkey PrintToConsole --regkey OCLSIMD16SelectionMask=1 --enable-profitability-print --simd32-profit -igc-serialize-metadata --inputocl -S < %s 2>&1 | FileCheck %s --check-prefix=BOTH
; RUN: igc_opt --regkey PrintToConsole --regkey OCLSIMD16SelectionMask=2 --enable-profitability-print --simd32-profit -igc-serialize-metadata --inputocl -S < %s 2>&1 | FileCheck %s --check-prefix=BOTH
; RUN: igc_opt --regkey PrintToConsole --regkey OCLSIMD16SelectionMask=4 --enable-profitability-print --simd32-profit -igc-serialize-metadata --inputocl -S < %s 2>&1 | FileCheck %s --check-prefix=BOTH
;
; RUN: igc_opt --regkey PrintToConsole --enable-profitability-print --simd32-profit -igc-serialize-metadata --inputocl --platformglk -S < %s 2>&1 | FileCheck %s --check-prefix=BOTH
; ------------------------------------------------
; Simd32ProfitabilityAnalysis
; ------------------------------------------------

; ONLY16: {{.*}}isSimd16Profitable: 1{{.*}}
; ONLY16-NEXT: {{.*}}isSimd32Profitable: 0{{.*}}

; BOTH: {{.*}}isSimd16Profitable: 1{{.*}}
; BOTH-NEXT: {{.*}}isSimd32Profitable: 1{{.*}}

declare void @use_int(i32)
declare i16 @llvm.genx.GenISA.getLocalID.X()

define void @test_func(i32 %a, i32 addrspace(1)* %ptr) {
entry:
  %local_id = call i16 @llvm.genx.GenISA.getLocalID.X()
  %id = zext i16 %local_id to i32
  br label %ifbb0

ifbb0:
  %LC = phi i32 [%x, %bb], [0, %entry]
  %cond2 = icmp slt i32 %LC, 0
  br i1 %cond2, label %bb0, label %somebb

bb0:
  %x0 = sub i32 0, %LC
  br label %bb

somebb:
  %M = shl i32 %id, 1
  %x1 = sub i32 %M, %LC
  br label %bb

bb:
  %x = phi i32 [%x0, %bb0], [%x1, %somebb]
  %cond = icmp ugt i32 %x, %id
  br i1 %cond, label %ifbb0, label %exit

exit:
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
