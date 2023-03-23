;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt --regkey PrintToConsole --serialize-igc-metadata --igc-inst-stat-sroa --igc-inst-stat-th 4 --sroa --enable-instrstat-print -S < %s 2>&1 | FileCheck %s --check-prefix=EXCEEDED
; RUN: igc_opt --regkey PrintToConsole --serialize-igc-metadata --igc-inst-stat-sroa --igc-inst-stat-th 5 --sroa --enable-instrstat-print -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; InstrStatistic
; ------------------------------------------------

; Test checks statistics gathered by InstrStatistic pass about llvm::SROAPass run

; EXCEEDED: {{.*}}BEGIN: 5{{.*}}
; EXCEEDED-NEXT: {{.*}}END: 0{{.*}}
; EXCEEDED-NEXT: {{.*}}EXCEED_THRESHOLD: 1{{.*}}

; CHECK: {{.*}}BEGIN: 5{{.*}}
; CHECK-NEXT: {{.*}}END: 0{{.*}}
; CHECK-NEXT: {{.*}}EXCEED_THRESHOLD: 0{{.*}}

declare void @use_int(i32)

define void @test_func(i32 %a) {
entry:
  %ptr = alloca i32, align 4, addrspace(0)

  ; series of loads/stores to be removed by SROA
  %yy1 = add i32 %a, 1
  store i32 %yy1, i32 addrspace(0)* %ptr

  %xx2 = load i32, i32 addrspace(0)* %ptr
  %yy2 = add i32 %xx2, 1
  store i32 %yy2, i32 addrspace(0)* %ptr

  %xx3 = load i32, i32 addrspace(0)* %ptr
  %yy3 = add i32 %xx3, 1
  store i32 %yy3, i32 addrspace(0)* %ptr

  call void @use_int(i32 %xx3)
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!4}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (i32)* @test_func}
!3 = !{!"FuncMDValue[0]"}
!4 = !{void (i32)* @test_func, !5}
!5 = !{!6}
!6 = !{!"function_type", i32 0}
