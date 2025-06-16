;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-minimum-valid-address-checking -igc-minimum-valid-address-checking_arg 0x100 -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define spir_kernel void @kernel(ptr addrspace(1) %input) nounwind {
  %1 = addrspacecast ptr addrspace(1) %input to ptr addrspace(4)
  %2 = getelementptr inbounds i32, ptr addrspace(4) %1, i64 2
  %3 = load i32, ptr addrspace(4) %2
  %4 = add i32 %3, 0
  ret void
}

!igc.functions = !{!0}

!0 = !{ptr @kernel, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

; CHECK:          [[ADDRESS:%[0-9]+]] = ptrtoint ptr addrspace(4) [[POINTER:%[0-9]+]] to i64
; CHECK-NEXT:     [[ADDRESS_SPACE_TAG:%[0-9]+]] = lshr i64 [[ADDRESS]], 61
; CHECK-NEXT:     [[IS_ADDRESS_SPACE_TAG_0:%[0-9]+]] = icmp eq i64 [[ADDRESS_SPACE_TAG]], 0
; CHECK-NEXT:     [[IS_ADDRESS_SPACE_TAG_7:%[0-9]+]] = icmp eq i64 [[ADDRESS_SPACE_TAG]], 7
; CHECK-NEXT:     [[IS_GENERIC_GLOBAL:%[0-9]+]] = or i1 [[IS_ADDRESS_SPACE_TAG_0]], [[IS_ADDRESS_SPACE_TAG_7]]
; CHECK-NEXT:     [[CLEARING_TAG_TMP:%[0-9]+]] = shl i64 [[ADDRESS]], 4
; CHECK-NEXT:     [[ADDRESS_WITHOUT_ADDRESS_SPACE_TAG:%[0-9]+]] = ashr i64 [[CLEARING_TAG_TMP]], 4
; CHECK-DAG:      [[IS_ADDRESS_IN_VALID_REGION:%[0-9]+]] = icmp uge i64 [[ADDRESS_WITHOUT_ADDRESS_SPACE_TAG]], {{[0-9]+}}
; CHECK-DAG:      [[IS_NOT_GENERIC_GLOBAL:%[0-9]+]] = xor i1 [[IS_GENERIC_GLOBAL]], true
; CHECK:          [[IS_VALID_ADDRESS:%[0-9]+]] = or i1 [[IS_NOT_GENERIC_GLOBAL]], [[IS_ADDRESS_IN_VALID_REGION]]
; CHECK-NEXT:     br i1 [[IS_VALID_ADDRESS]], label %minimumvalidaddresschecking.valid, label %minimumvalidaddresschecking.invalid

; CHECK:        minimumvalidaddresschecking.valid:
; CHECK-NEXT:     [[VALID:%[0-9]+]] = load i32, ptr addrspace(4) [[POINTER]]
; CHECK-NEXT:     br label %minimumvalidaddresschecking.end

; CHECK:        minimumvalidaddresschecking.invalid:
; CHECK-NEXT:     call spir_func void @__minimumvalidaddresschecking_assert_

; CHECK:        minimumvalidaddresschecking.end:
; CHECK-NEXT:     [[LOADED_VALUE:%[0-9]+]] = phi i32 [ [[VALID]], %minimumvalidaddresschecking.valid ], [ 0, %minimumvalidaddresschecking.invalid ]
; CHECK-NEXT:     {{%[0-9]+}} = add i32 [[LOADED_VALUE]], 0

; CHECK:        declare void @__minimumvalidaddresschecking
