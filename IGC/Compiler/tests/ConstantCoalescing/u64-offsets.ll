;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers --igc-constant-coalescing -dce -S < %s | FileCheck %s

; This test verifies that two loads with an offset difference of 4, where
; one offset is greater than UINT32_MAX and the other is less than UINT32_MAX,
; are merged into a single vector load <2 x i32>.

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-f80:128:128-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a:64:64-f80:128:128-n8:16:32:64"

define void @test_merge_add(ptr addrspace(1) %src) {
; CHECK-LABEL: define void @test_merge_add(
; CHECK:         [[CHUNKPTR:%.*]] = inttoptr i64 %ptr1 to ptr addrspace(2)
; CHECK-NEXT:    [[LOAD:%.*]] = load <2 x i32>, ptr addrspace(2) [[CHUNKPTR]], align 4
; CHECK-NEXT:    [[EXT0:%.*]] = extractelement <2 x i32> [[LOAD]], i32 0
; CHECK-NEXT:    [[EXT1:%.*]] = extractelement <2 x i32> [[LOAD]], i32 1
; CHECK:         call void @use.i32(i32 [[EXT0]])
; CHECK-NEXT:    call void @use.i32(i32 [[EXT1]])
;
  %base_ptr = ptrtoint ptr addrspace(1) %src to i64
  %ptr1 = add i64 %base_ptr, 4294967292                  ; 0x0fffffffc
  %ptr2 = add i64 %base_ptr, 4294967296                  ; 0x100000000
  %ptr1_int = inttoptr i64 %ptr1 to ptr addrspace(2)
  %load1 = load i32, ptr addrspace(2) %ptr1_int, align 4
  %ptr2_int = inttoptr i64 %ptr2 to ptr addrspace(2)
  %load2 = load i32, ptr addrspace(2) %ptr2_int, align 4
  call void @use.i32(i32 %load1)
  call void @use.i32(i32 %load2)
  ret void
}

declare void @use.i32(i32)

!igc.functions = !{!0}

!0 = !{ptr @test_merge_add, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc"}
