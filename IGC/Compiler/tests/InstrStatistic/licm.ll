;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt --regkey PrintToConsole --serialize-igc-metadata --igc-inst-stat-licm --igc-inst-stat-th 4 --licm --enable-instrstat-print -S < %s 2>&1 | FileCheck %s --check-prefix=EXCEEDED
; RUN: igc_opt --regkey PrintToConsole --serialize-igc-metadata --igc-inst-stat-licm --igc-inst-stat-th 5 --licm --enable-instrstat-print -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; InstrStatistic
; ------------------------------------------------

; Test checks statistics gathered by InstrStatistic pass about llvm::LICMPass run

; EXCEEDED: {{.*}}BEGIN: 11{{.*}}
; EXCEEDED-NEXT: {{.*}}END: 6{{.*}}
; EXCEEDED-NEXT: {{.*}}EXCEED_THRESHOLD: 1{{.*}}

; CHECK: {{.*}}BEGIN: 11{{.*}}
; CHECK-NEXT: {{.*}}END: 6{{.*}}
; CHECK-NEXT: {{.*}}EXCEED_THRESHOLD: 0{{.*}}

declare void @use_int(i32)

define void @test_func(i32 %a, i32 addrspace(1)* %ptr) {
entry:
  %id = add i32 %a, 1
  br label %loop

loop:
  %indvar = phi i32 [ %id, %entry ], [ %nextindvar, %loop ]

  %cmp = icmp sle i32 %indvar, 0
  %sel = select i1 %cmp, i32 10, i32 20
  %nextindvar = sub i32 %sel, %indvar

  ; series of loads to be removed by LICM
  %xx1 = load i32, i32 addrspace(1)* %ptr
  %xx2 = load i32, i32 addrspace(1)* %ptr
  %xx3 = load i32, i32 addrspace(1)* %ptr
  %xx4 = load i32, i32 addrspace(1)* %ptr
  %xx5 = load i32, i32 addrspace(1)* %ptr

  %cond = icmp ult i32 %nextindvar, %id
  br i1 %cond, label %loop, label %loop_end

loop_end:
  call void @use_int(i32 %nextindvar)
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
