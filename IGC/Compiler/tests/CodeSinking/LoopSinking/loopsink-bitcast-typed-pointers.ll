;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --regkey CodeSinkingLoadSchedulingInstr=1 --regkey LoopSinkMinSave=1 --regkey ForceLoadsLoopSink=1 --regkey ForceLoopSink=1 --regkey CodeLoopSinkingMinSize=10 --basic-aa --igc-code-loop-sinking -S %s | FileCheck %s



define void @nosink_bc(<2 x i32> addrspace(3)* %in0, i64 addrspace(3)* %out0, i32 %count, i32 %offsetIn0) {
; CHECK-LABEL: @nosink_bc(
; CHECK:       entry:
; CHECK:         [[IN0_SHIFTED:%.*]] = getelementptr <2 x i32>, <2 x i32> addrspace(3)* [[IN0:%.*]], i32 [[OFFSETIN0:%.*]]
; CHECK:         br label [[ENTRY_PREHEADER:%.*]]
; CHECK:       entry_preheader:
; CHECK:         [[L0:%.*]] = load <2 x i32>, <2 x i32> addrspace(3)* [[IN0_SHIFTED]], align 16

; Bitcast is not sinked, because it has second use in loop
; And the load is not a candidate
;
; If we sinked it, it could lead to a situation when we have more regpressure in the loop:
; 1. Load value is alive
; 2. Bitcast is alive between first and second use

; CHECK:         [[BC:%.*]] = bitcast <2 x i32> [[L0]] to i64
; CHECK:         br label [[LOOP:%.*]]
; CHECK:       loop:
; CHECK:         [[INDEX:%.*]] = phi i32 [ 0, [[ENTRY_PREHEADER]] ], [ [[INC:%.*]], [[LOOP]] ]
; CHECK:         [[X:%.*]] = add i64 [[BC]], 1234
; CHECK:         [[ADD2:%.*]] = add i64 [[BC]], 2
; CHECK:         [[TOSTORE:%.*]] = mul i64 [[X]], [[ADD2]]
; CHECK:         [[OUT0_SHIFTED:%.*]] = getelementptr i64, i64 addrspace(3)* [[OUT0:%.*]], i32 [[INDEX]]
; CHECK:         store i64 [[TOSTORE]], i64 addrspace(3)* [[OUT0_SHIFTED]], align 16
; CHECK:         [[CMPTMP:%.*]] = icmp ult i32 [[INDEX]], [[COUNT:%.*]]
; CHECK:         [[INC]] = add i32 [[INDEX]], 1
; CHECK:         br i1 [[CMPTMP]], label [[LOOP]], label [[AFTERLOOP:%.*]]
; CHECK:       afterloop:
; CHECK:         ret void
;
entry:
  %in0_shifted = getelementptr <2 x i32>, <2 x i32> addrspace(3)* %in0, i32 %offsetIn0
  br label %entry_preheader

entry_preheader:                                  ; preds = %entry
  %l0 = load <2 x i32>, <2 x i32> addrspace(3)* %in0_shifted, align 16
  %bc = bitcast <2 x i32> %l0 to i64
  br label %loop

loop:                                             ; preds = %loop, %entry_preheader
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]
  %x = add i64 %bc, 1234
  %add2 = add i64 %bc, 2
  %toStore = mul i64 %x, %add2
  %out0_shifted = getelementptr i64, i64 addrspace(3)* %out0, i32 %index
  store i64 %toStore, i64 addrspace(3)* %out0_shifted, align 16
  %cmptmp = icmp ult i32 %index, %count
  %inc = add i32 %index, 1
  br i1 %cmptmp, label %loop, label %afterloop

afterloop:                                        ; preds = %loop
  ret void
}


define void @sink_bc(<2 x i32> addrspace(3)* %in0, i64 addrspace(3)* %out0, i32 %count, i32 %offsetIn0) {
; CHECK-LABEL: @sink_bc(
; CHECK:       entry:
; CHECK:         [[IN0_SHIFTED:%.*]] = getelementptr <2 x i32>, <2 x i32> addrspace(3)* [[IN0:%.*]], i32 [[OFFSETIN0:%.*]]
; CHECK:         br label [[ENTRY_PREHEADER:%.*]]
; CHECK:       entry_preheader:
; CHECK:         [[L0:%.*]] = load <2 x i32>, <2 x i32> addrspace(3)* [[IN0_SHIFTED]], align 16
; CHECK:         br label [[LOOP:%.*]]
; CHECK:       loop:
; CHECK:         [[INDEX:%.*]] = phi i32 [ 0, [[ENTRY_PREHEADER]] ], [ [[INC:%.*]], [[LOOP]] ]

; bitcast is sinked, because it has 1 use and load has 1 use
; register pressure is not larger: load is not sunk, but the size is the same

; CHECK:         [[SINK_BC:%.*]] = bitcast <2 x i32> [[L0]] to i64
; CHECK:         [[TOSTORE:%.*]] = add i64 [[SINK_BC]], 1234
; CHECK:         [[OUT0_SHIFTED:%.*]] = getelementptr i64, i64 addrspace(3)* [[OUT0:%.*]], i32 [[INDEX]]
; CHECK:         store i64 [[TOSTORE]], i64 addrspace(3)* [[OUT0_SHIFTED]], align 16
; CHECK:         [[CMPTMP:%.*]] = icmp ult i32 [[INDEX]], [[COUNT:%.*]]
; CHECK:         [[INC]] = add i32 [[INDEX]], 1
; CHECK:         br i1 [[CMPTMP]], label [[LOOP]], label [[AFTERLOOP:%.*]]
; CHECK:       afterloop:
; CHECK:         ret void
;


entry:
  %in0_shifted = getelementptr <2 x i32>, <2 x i32> addrspace(3)* %in0, i32 %offsetIn0
  br label %entry_preheader

entry_preheader:                                  ; preds = %entry
  %l0 = load <2 x i32>, <2 x i32> addrspace(3)* %in0_shifted, align 16
  %bc = bitcast <2 x i32> %l0 to i64
  br label %loop

loop:                                             ; preds = %loop, %entry_preheader
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]

  %toStore = add i64 %bc, 1234
  %out0_shifted = getelementptr i64, i64 addrspace(3)* %out0, i32 %index
  store i64 %toStore, i64 addrspace(3)* %out0_shifted, align 16
  %cmptmp = icmp ult i32 %index, %count
  %inc = add i32 %index, 1
  br i1 %cmptmp, label %loop, label %afterloop

afterloop:                                        ; preds = %loop
  ret void
}


define void @sink_bc2(<2 x i32> addrspace(3)* noalias %in0, i64 addrspace(3)* noalias %out0, i32 %count, i32 %offsetIn0) {
; CHECK-LABEL: @sink_bc2(
; CHECK:       entry:
; CHECK:         [[IN0_SHIFTED:%.*]] = getelementptr <2 x i32>, <2 x i32> addrspace(3)* [[IN0:%.*]], i32 [[OFFSETIN0:%.*]]
; CHECK:         br label [[ENTRY_PREHEADER:%.*]]
; CHECK:       entry_preheader:
; CHECK:         br label [[LOOP:%.*]]
; CHECK:       loop:
; CHECK:         [[INDEX:%.*]] = phi i32 [ 0, [[ENTRY_PREHEADER]] ], [ [[INC:%.*]], [[LOOP]] ]
; CHECK:         [[SINK_L0:%.*]] = load <2 x i32>, <2 x i32> addrspace(3)* [[IN0_SHIFTED]], align 16

; bitcast is sinked and enabled load sinking

; CHECK:         [[SINK_BC:%.*]] = bitcast <2 x i32> [[SINK_L0]] to i64
; CHECK:         [[TOSTORE:%.*]] = add i64 [[SINK_BC]], 1234
; CHECK:         [[OUT0_SHIFTED:%.*]] = getelementptr i64, i64 addrspace(3)* [[OUT0:%.*]], i32 [[INDEX]]
; CHECK:         store i64 [[TOSTORE]], i64 addrspace(3)* [[OUT0_SHIFTED]], align 16
; CHECK:         [[CMPTMP:%.*]] = icmp ult i32 [[INDEX]], [[COUNT:%.*]]
; CHECK:         [[INC]] = add i32 [[INDEX]], 1
; CHECK:         br i1 [[CMPTMP]], label [[LOOP]], label [[AFTERLOOP:%.*]]
; CHECK:       afterloop:
; CHECK:         ret void
;

entry:
  %in0_shifted = getelementptr <2 x i32>, <2 x i32> addrspace(3)* %in0, i32 %offsetIn0
  br label %entry_preheader

entry_preheader:                                  ; preds = %entry
  %l0 = load <2 x i32>, <2 x i32> addrspace(3)* %in0_shifted, align 16
  %bc = bitcast <2 x i32> %l0 to i64
  br label %loop

loop:                                             ; preds = %loop, %entry_preheader
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]

  %toStore = add i64 %bc, 1234
  %out0_shifted = getelementptr i64, i64 addrspace(3)* %out0, i32 %index
  store i64 %toStore, i64 addrspace(3)* %out0_shifted, align 16
  %cmptmp = icmp ult i32 %index, %count
  %inc = add i32 %index, 1
  br i1 %cmptmp, label %loop, label %afterloop

afterloop:                                        ; preds = %loop
  ret void
}


!igc.functions = !{}

