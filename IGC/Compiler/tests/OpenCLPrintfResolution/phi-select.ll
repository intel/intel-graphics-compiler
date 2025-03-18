;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; This test checks printf with phi input.
; NB: this only works with the default ZEBin format.
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --typed-pointers %s -S -o - -igc-opencl-printf-resolution | FileCheck %s

@.str.1 = internal unnamed_addr addrspace(2) constant [9 x i8] c"string 1\00"
@.str.2 = internal unnamed_addr addrspace(2) constant [9 x i8] c"string 2\00"
@.str.3 = internal unnamed_addr addrspace(2) constant [9 x i8] c"string 3\00"

define spir_kernel void @printf_test(i64 %0, i8 addrspace(1)* %printfBuffer) #0 {
entry:
  %1 = icmp eq i64 %0, 1
  %2 = getelementptr inbounds [9 x i8], [9 x i8] addrspace(2)* @.str.1, i64 0, i64 0
  %3 = getelementptr inbounds [9 x i8], [9 x i8] addrspace(2)* @.str.2, i64 0, i64 0
  switch i64 %0, label %printf_bb [
    i64 0, label %switch_b0
    i64 1, label %switch_b1
    i64 2, label %switch_b1
    i64 3, label %switch_b2
  ]

switch_b0:
  %4 = getelementptr inbounds [9 x i8], [9 x i8] addrspace(2)* @.str.3, i64 0, i64 0
  br label %switch_b2

switch_b1:
  %5 = select i1 %1, i8 addrspace(2)* %2, i8 addrspace(2)* %3
  br label %printf_bb

switch_b2:
  %6 = phi i8 addrspace(2)* [ %4, %switch_b0 ], [ %3, %entry ]
  br label %printf_bb

printf_bb:
  %7 = phi i8 addrspace(2)* [ %5, %switch_b1 ], [ %6, %switch_b2 ], [ %2, %entry ]
  %8 = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %7)
  br label %exit

exit:
  ret void
}

; CHECK-LABEL: define spir_kernel void @printf_test(
; CHECK-SAME:      i64 %[[ENUM_VAL:[A-Za-z0-9]+]], i8 addrspace(1)* %printfBuffer)
; CHECK: entry:
; CHECK-NEXT: %[[CMP:.+]] = icmp eq i64 %[[ENUM_VAL]], 1
; CHECK-NEXT: %[[STR1_GEP:.+]] = getelementptr inbounds [9 x i8], [9 x i8] addrspace(2)* @.str.1, i64 0, i64 0
; CHECK-NEXT: %[[STR2_GEP:.+]] = getelementptr inbounds [9 x i8], [9 x i8] addrspace(2)* @.str.2, i64 0, i64 0
; CHECK-NEXT: switch i64 %[[ENUM_VAL]], label %printf_bb [
; CHECK-NEXT:     i64 0, label %switch_b0
; CHECK-NEXT:     i64 1, label %switch_b1
; CHECK-NEXT:     i64 2, label %switch_b1
; CHECK-NEXT:     i64 3, label %switch_b2
; CHECK-NEXT:   ]
;
; CHECK: switch_b0:
; CHECK-NEXT: %[[STR3_GEP:.+]] = getelementptr inbounds [9 x i8], [9 x i8] addrspace(2)* @.str.3, i64 0, i64 0
; CHECK-NEXT: br label %switch_b2
;
; CHECK: switch_b1:
; CHECK-NEXT: %[[STR1_STR2_SEL:.+]] = select i1 %[[CMP]], [9 x i8] addrspace(2)* @.str.1, [9 x i8] addrspace(2)* @.str.2
; TODO: Clean up old sels/phis directly within the pass & make next line CHECK-NEXT
; CHECK:      br label %printf_bb
;
; CHECK: switch_b2:
; CHECK-NEXT: %[[STR2_STR3_PHI:.+]] = phi [9 x i8] addrspace(2)* [ @.str.3, %switch_b0 ], [ @.str.2, %entry ]
; TODO: Clean up old sels/phis directly within the pass & make next line CHECK-NEXT
; CHECK:      br label %printf_bb
;
; CHECK: printf_bb:
; CHECK-NEXT: %[[JOIN_PHI:.+]] = phi [9 x i8] addrspace(2)* [ %[[STR1_STR2_SEL]], %switch_b1 ], [ %[[STR2_STR3_PHI]], %switch_b2 ], [ @.str.1, %entry ]
; TODO: Clean up old sels/phis directly within the pass & make next line CHECK-NEXT
; CHECK:      %ptrBC = bitcast i8 addrspace(1)* %printfBuffer to i32 addrspace(1)*
; CHECK-NEXT: %write_offset = call i32 @__builtin_IB_atomic_add_global_i32(i32 addrspace(1)* %ptrBC, i32 8)
; CHECK-NEXT: %end_offset = add i32 %write_offset, 8
; CHECK-NEXT: %[[WRITE_OFFSET_EXT:write_offset.*]] = zext i32 %write_offset to i64
; CHECK-NEXT: %buffer_ptr = ptrtoint i8 addrspace(1)* %printfBuffer to i64
; CHECK-NEXT: %[[WRITE_OFFSET_ADD:write_offset.*]] = add i64 %buffer_ptr, %[[WRITE_OFFSET_EXT]]
; CHECK-NEXT: %[[BUF_CHECK:.+]] = icmp ule i32 %end_offset, 4194304
; CHECK-NEXT: br i1 %[[BUF_CHECK]], label %write_offset_true, label %write_offset_false
;
; CHECK: write_offset_true:
; CHECK-NEXT: %write_offset_ptr = inttoptr i64 %[[WRITE_OFFSET_ADD]] to i64 addrspace(1)*
; CHECK-NEXT: %[[PTRCAST:.+]] = ptrtoint [9 x i8] addrspace(2)* %[[JOIN_PHI]] to i64
; CHECK-NEXT: store i64 %[[PTRCAST]], i64 addrspace(1)* %write_offset_ptr, align 4
; TODO: Fix the extra, unused offset increment (add i64 %[[WRITE_OFFSET_ADD]], 8)
;       and make next line CHECK-NEXT
; CHECK: br label %bblockJoin
;
; CHECK: write_offset_false:
; CHECK-NEXT: %[[END_OFFSET_FALSE:end_offset.*]] = add i32 %write_offset, 4
; CHECK-NEXT: %[[BUF_CHECK_ERR:.+]] = icmp ule i32 %[[END_OFFSET_FALSE]], 4194304
; CHECK-NEXT: br i1 %[[BUF_CHECK_ERR]], label %write_error_string, label %bblockFalseJoin
;
; CHECK: write_error_string:
; CHECK-NEXT: %write_offset_ptr5 = inttoptr i64 %[[WRITE_OFFSET_ADD]] to i32 addrspace(1)*
; CHECK-NEXT: store i32 -1, i32 addrspace(1)* %write_offset_ptr5, align 4
; CHECK-NEXT: br label %bblockFalseJoin
;
; CHECK: bblockFalseJoin:
; CHECK-NEXT: br label %bblockJoin
;
; CHECK: bblockJoin:
; CHECK-NEXT: %printf_ret_val = select i1 %[[BUF_CHECK]], i32 0, i32 -1
; CHECK-NEXT: br label %exit
;
; CHECK: exit:
; CHECK-NEXT: ret void

declare spir_func i32 @printf(i8 addrspace(2)*, ...)

declare i32 @__builtin_IB_atomic_add_global_i32(i32 addrspace(1)*, i32)

!igc.functions = !{!0}

!0 = !{void (i64, i8 addrspace(1)*)* @printf_test, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4}
!4 = !{i32 14}
