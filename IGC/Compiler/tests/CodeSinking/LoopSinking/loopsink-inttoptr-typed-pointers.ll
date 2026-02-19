;
; Copyright (C) 2024 Intel Corporation
;
; This software and the related documents are Intel copyrighted materials,
; and your use of them is governed by the express license under which they were
; provided to you ("License"). Unless the License provides otherwise,
; you may not use, modify, copy, publish, distribute, disclose or transmit this
; software or the related documents without Intel's prior written permission.
;
; This software and the related documents are provided as is, with no express or
; implied warranties, other than those that are expressly stated in the License.
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --regkey LoopSinkMinSave=1 --regkey LoopSinkDisableRollback=1 --regkey ForceLoopSink=1 --regkey CodeLoopSinkingMinSize=10 --basic-aa --igc-code-loop-sinking -S %s | FileCheck %s

; check the inttoptr instructions are sinked

define void @foo(i32 %pointer_int, i32 %count, i32 %offset_load, i32 %offset_store) {
; CHECK-LABEL: @foo(
; CHECK:       entry_preheader:
; CHECK:         [[LOAD_PTR:%.*]] = add i32 [[POINTER_INT:%.*]], [[OFFSET_LOAD:%.*]]
; CHECK:         [[STORE_PTR:%.*]] = add i32 [[POINTER_INT]], [[OFFSET_STORE:%.*]]
; CHECK:         br label [[LOOP:%.*]]
; CHECK:       loop:
; CHECK:         [[INDEX:%.*]] = phi i32 [ 0, [[ENTRY_PREHEADER:%.*]] ], [ [[INC:%.*]], [[LOOP]] ]
; CHECK:         [[SINK_LOAD_PTR_CAST:%.*]] = inttoptr i32 [[LOAD_PTR]] to i32 addrspace(3)*
; CHECK:         [[LOAD:%.*]] = load i32, i32 addrspace(3)* [[SINK_LOAD_PTR_CAST]], align 4
; CHECK:         [[SINK_STORE_PTR_CAST:%.*]] = inttoptr i32 [[STORE_PTR]] to i32 addrspace(3)*
; CHECK:         store i32 [[LOAD]], i32 addrspace(3)* [[SINK_STORE_PTR_CAST]], align 4
; CHECK:         [[CMPTMP:%.*]] = icmp ult i32 [[INDEX]], [[COUNT:%.*]]
; CHECK:         [[INC]] = add i32 [[INDEX]], 1
; CHECK:         br i1 [[CMPTMP]], label [[LOOP]], label [[AFTERLOOP:%.*]]
; CHECK:       afterloop:
; CHECK:         ret void
;
entry_preheader:
  %load_ptr = add i32 %pointer_int, %offset_load
  %load_ptr_cast = inttoptr i32 %load_ptr to i32 addrspace(3)*
  %store_ptr = add i32 %pointer_int, %offset_store
  %store_ptr_cast = inttoptr i32 %store_ptr to i32 addrspace(3)*
  br label %loop

loop:                                             ; preds = %loop, %entry_preheader
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]
  %load = load i32, i32 addrspace(3)* %load_ptr_cast, align 4
  store i32 %load, i32 addrspace(3)* %store_ptr_cast, align 4
  %cmptmp = icmp ult i32 %index, %count
  %inc = add i32 %index, 1
  br i1 %cmptmp, label %loop, label %afterloop

afterloop:                                        ; preds = %loop
  ret void
}

!igc.functions = !{}
