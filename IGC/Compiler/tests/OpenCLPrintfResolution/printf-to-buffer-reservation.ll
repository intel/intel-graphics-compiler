;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-16-plus
;
; RUN: igc_opt --opaque-pointers -igc-opencl-printf-resolution -S < %s | FileCheck %s
; ------------------------------------------------
; OpenCLPrintfResolution
; ------------------------------------------------

; __builtin_IB_printf_to_buffer, as used by assert: the buffer size is a
; runtime value and the counter lives apart from the buffer base. A record is
; reserved with a cmpxchg loop only if it fits, checked without i32 overflow,
; and a record that doesn't fit moves the counter to the end of the buffer.

@.str = internal unnamed_addr addrspace(2) constant [3 x i8] c"%d\00", align 1

; CHECK-LABEL: @test(
; CHECK:       reserve.loop:
; CHECK-NEXT:    [[OFFSET:%.*]] = phi i32 [ %size, %entry ], [ [[PREV:%.*]], %reserve.loop ]
; CHECK-NEXT:    [[HAS_ROOM:%.*]] = icmp ult i32 [[OFFSET]], %size
; CHECK-NEXT:    [[ROOM:%.*]] = sub i32 %size, [[OFFSET]]
; CHECK-NEXT:    [[SIZE_FITS:%.*]] = icmp ule i32 16, [[ROOM]]
; CHECK-NEXT:    [[FITS:%.*]] = and i1 [[HAS_ROOM]], [[SIZE_FITS]]
; CHECK-NEXT:    [[END:%.*]] = add i32 [[OFFSET]], 16
; CHECK-NEXT:    [[NEW:%.*]] = select i1 [[FITS]], i32 [[END]], i32 %size
; CHECK-NEXT:    [[PREV]] = call i32 @__builtin_IB_atomic_cmpxchg_global_i32(ptr addrspace(1) %offset, i32 [[OFFSET]], i32 [[NEW]])
; CHECK-NEXT:    [[RESERVED:%.*]] = icmp eq i32 [[PREV]], [[OFFSET]]
; CHECK-NEXT:    br i1 [[RESERVED]], label %reserve.done, label %reserve.loop
; CHECK:       reserve.done:
; CHECK-NEXT:    [[ERROR_SIZE_FITS:%.*]] = icmp ule i32 4, [[ROOM]]
; CHECK-NEXT:    [[ERROR_FITS:%.*]] = and i1 [[HAS_ROOM]], [[ERROR_SIZE_FITS]]
; CHECK-NEXT:    [[OFFSET_EXT:%.*]] = zext i32 [[OFFSET]] to i64
; CHECK-NEXT:    [[BUF:%.*]] = ptrtoint ptr addrspace(1) %buf to i64
; CHECK-NEXT:    [[WRITE_PTR:%.*]] = add i64 [[BUF]], [[OFFSET_EXT]]
; CHECK-NEXT:    br i1 [[FITS]], label %write_offset_true, label %write_offset_false
; CHECK:       write_offset_false:
; CHECK-NEXT:    br i1 [[ERROR_FITS]], label %write_error_string, label %bblockFalseJoin
; CHECK:       write_error_string:
; CHECK-NEXT:    [[ERROR_PTR:%.*]] = inttoptr i64 [[WRITE_PTR]] to ptr addrspace(1)
; CHECK-NEXT:    store i32 -1, ptr addrspace(1) [[ERROR_PTR]]
; CHECK:       bblockJoin:
; CHECK-NEXT:    {{%.*}} = select i1 [[FITS]], i32 0, i32 -1
define spir_kernel void @test(ptr addrspace(1) %buf, ptr addrspace(1) %offset, i32 %size, i32 %val) {
entry:
  %fmt = getelementptr inbounds [3 x i8], ptr addrspace(2) @.str, i64 0, i64 0
  %call = call spir_func i32 (ptr addrspace(1), ptr addrspace(1), i32, ...) @__builtin_IB_printf_to_buffer(ptr addrspace(1) %buf, ptr addrspace(1) %offset, i32 %size, ptr addrspace(2) %fmt, i32 %val)
  ret void
}

declare spir_func i32 @__builtin_IB_printf_to_buffer(ptr addrspace(1), ptr addrspace(1), i32, ...)
