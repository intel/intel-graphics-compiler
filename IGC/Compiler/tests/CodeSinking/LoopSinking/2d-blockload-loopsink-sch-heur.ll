;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-14-plus
; RUN: igc_opt --opaque-pointers --regkey CodeSinkingLoadSchedulingInstr=3 --regkey CodeSinking2dLoadSchedulingInstr=2 --regkey LoopSinkMinSave=1 --regkey LoopSinkEnable2dBlockReads=1 --regkey ForceLoopSink=1 --regkey CodeLoopSinkingMinSize=10 --regkey LoopSinkDisableRollback=1 --basic-aa --igc-code-loop-sinking -S %s 2>&1 | FileCheck %s

define void @sink1(ptr addrspace(1) %in0, ptr addrspace(1) noalias %out0, i32 %count, i32 %offsetIn0, <8 x i32> %r0) {
; CHECK-LABEL: @sink1(

; Check that early load placing heuristic respects 2d load and ordinary loads options (CodeSinking2dLoadSchedulingInstr and CodeSinkingLoadSchedulingInstr respectively)

; CHECK:       loop:
; CHECK:         [[X1:%.*]] = add i32 [[INDEX:.*]], 1
; CHECK:         [[X2:%.*]] = add i32 [[INDEX]], 2
; CHECK:         [[SINK_LOAD:%.*]] = load i32, ptr addrspace(1) [[IN0:%.*]], align 64
; CHECK:         [[X3:%.*]] = add i32 [[INDEX]], 3
; CHECK:         [[X4:%.*]] = add i32 [[INDEX]], 4
; CHECK:         [[X5:%.*]] = add i32 [[INDEX]], 5

; CHECK:       loop2:
; CHECK:         [[Y1:%.*]] = add i32 [[INDEX]], 1
; CHECK:         [[Y2:%.*]] = add i32 [[INDEX]], 2
; CHECK:         [[Y3:%.*]] = add i32 [[INDEX]], 3
; CHECK:         [[SINK_LOAD2D:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 [[SINK_BASE_ADDR:.*]], i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
; CHECK:         [[Y4:%.*]] = add i32 [[INDEX]], 4
; CHECK:         [[Y5:%.*]] = add i32 [[INDEX]], 5

; CHECK:         ret void
;

entry_preheader:
  %bc = bitcast <8 x i32> %r0 to <32 x i8>
  %ee_thread = extractelement <32 x i8> %bc, i32 8
  ; %zext = zext i8 %ee_thread to i32

  %base_addr = ptrtoint ptr addrspace(1) %in0 to i64

  %load2d = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %base_addr, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
  %load = load i32, ptr addrspace(1) %in0, align 64

  br label %loop

loop:                                             ; preds = %loop2, %entry_preheader
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop2 ]

  %addr = getelementptr i32, ptr addrspace(1) %out0, i32 8

  %x1 = add i32 %index, 1
  %x2 = add i32 %index, 2
  %x3 = add i32 %index, 3
  %x4 = add i32 %index, 4
  %x5 = add i32 %index, 5

  store i32 %load, ptr addrspace(1) %addr, align 64

  br label %loop2

loop2:                                           ; preds = %loop
  %addr2 = getelementptr <32 x i16>, ptr addrspace(1) %out0, i32 %index

  %y1 = add i32 %index, 1
  %y2 = add i32 %index, 2
  %y3 = add i32 %index, 3
  %y4 = add i32 %index, 4
  %y5 = add i32 %index, 5

  store <32 x i16> %load2d, <32 x i16> addrspace(1)* %addr2, align 64

  %cmptmp = icmp ult i32 %index, %count
  %inc = add i32 %index, 1
  br i1 %cmptmp, label %loop, label %afterloop

afterloop:                                        ; preds = %loop2
  ret void
}

declare <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32) #0


attributes #0 = { argmemonly nounwind readonly willreturn }

!igc.functions = !{}


