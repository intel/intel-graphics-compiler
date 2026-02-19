;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify -igc-memopt2 -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; MemOpt2
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test(i32 addrspace(1)* %addr) {
; CHECK-LABEL: @test(
; CHECK;  entry:
; CHECK:    [[TMP0:%.*]] = getelementptr i32, i32 addrspace(1)* [[ADDR:%.*]], i32 0
; CHECK:    [[TMP1:%.*]] = load i32, i32 addrspace(1)* [[TMP0]], align 4
; CHECK:    [[TMP2:%.*]] = getelementptr i32, i32 addrspace(1)* [[ADDR]], i32 1
; CHECK:    [[TMP3:%.*]] = load i32, i32 addrspace(1)* [[TMP2]], align 4
; CHECK:    [[TMP4:%.*]] = load i32, i32 addrspace(1)* [[TMP2]], align 4
; CHECK:    [[TMP5:%.*]] = add i32 [[TMP3]], 32
; CHECK:    [[TMP6:%.*]] = add i32 [[TMP4]], 13
; CHECK:    store i32 [[TMP1]], i32 addrspace(1)* [[ADDR]], align 4
; CHECK:    store i32 [[TMP5]], i32 addrspace(1)* [[ADDR]], align 4
; CHECK:    store i32 [[TMP6]], i32 addrspace(1)* [[ADDR]], align 4
; CHECK:    ret void
;
entry:
  %0 = getelementptr i32, i32 addrspace(1)* %addr, i32 0
  %1 = load i32, i32 addrspace(1)* %0, align 4
  %2 = getelementptr i32, i32 addrspace(1)* %addr, i32 1
  %3 = load i32, i32 addrspace(1)* %2, align 4
  %4 = add i32 %3, 32
  %5 = load i32, i32 addrspace(1)* %2, align 4
  %6 = add i32 %5, 13
  store i32 %1, i32 addrspace(1)* %addr, align 4
  store i32 %4, i32 addrspace(1)* %addr, align 4
  store i32 %6, i32 addrspace(1)* %addr, align 4
  ret void
}

!igc.functions = !{!0}

!0 = !{void (i32 addrspace(1)*)* @test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
