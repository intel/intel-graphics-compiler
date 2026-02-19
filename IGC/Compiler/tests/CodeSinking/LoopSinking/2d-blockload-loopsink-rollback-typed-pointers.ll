;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-14-plus
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --regkey LoopSinkMinSave=1 --regkey LoopSinkEnable2dBlockReads=1 --regkey ForceLoopSink=1 --regkey CodeLoopSinkingMinSize=10 --regkey LoopSinkForceRollback=1 --regkey CodeSinkingLoadSchedulingInstr=10 --basic-aa --igc-code-loop-sinking -S %s 2>&1 | FileCheck %s

declare i8* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i8(i64, i32, i32, i32, i32, i32, i32, i32, i32) #0

declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8*, i32, i32, i1) #0

declare <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8*, i32, i32, i32, i32, i32, i32, i1, i1, i32) #0

; Sink all 2d block reads tp the loop, then rollback
; Check that the instructions are in their initial places

define void @sink1(half addrspace(1)* %in0, <32 x i16> addrspace(1)* noalias %out0, i32 %count, i32 %offsetIn0, <8 x i32> %r0, i32 %n) {
; CHECK-LABEL: @sink1(
; CHECK:       entry_preheader:
; CHECK:         [[BASE_ADDR:%.*]] = ptrtoint half addrspace(1)* [[IN0:%.*]] to i64
; CHECK:         [[BLOCK2D_ADDRPAYLOAD:%.*]] = call i8* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i8(i64 [[BASE_ADDR]], i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 2)

; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 5, i32 5, i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 6, i32 6, i1 false)
; CHECK:         [[SINK_LOAD:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

; CHECK:         add
; CHECK:         add
; CHECK:         add
; CHECK:         mul
; CHECK:         mul
; CHECK:         mul

; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 5, i32 12, i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 6, i32 13, i1 false)
; CHECK:         [[LOAD2:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

; CHECK:         br label [[LOOP:%.*]]

; CHECK:       loop:


entry_preheader:
  %bc = bitcast <8 x i32> %r0 to <32 x i8>
  %ee_thread = extractelement <32 x i8> %bc, i32 8
  ; %zext = zext i8 %ee_thread to i32

  %base_addr = ptrtoint half addrspace(1)* %in0 to i64

  %Block2D_AddrPayload = call i8* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i8(i64 %base_addr, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 2)

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 5, i32 5, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 6, i32 6, i1 false)
  %load = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

  %x = add i32 %n, 5
  %y = add i32 %n, 6
  %z = add i32 %n, 12
  %x1 = mul i32 %n, %x
  %y1 = mul i32 %x1, %y
  %z1 = mul i32 %y1, %z

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 5, i32 12, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 6, i32 13, i1 false)
  %load2 = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

  br label %loop

loop:                                             ; preds = %loop, %entry_preheader
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]

  %addr = getelementptr <32 x i16>, <32 x i16> addrspace(1)* %out0, i32 %index
  store <32 x i16> %load, <32 x i16> addrspace(1)* %addr, align 64

  %index2 = add i32 %index, 1

  %addr2 = getelementptr <32 x i16>, <32 x i16> addrspace(1)* %out0, i32 %index2
  store <32 x i16> %load2, <32 x i16> addrspace(1)* %addr2, align 64

  %cmptmp = icmp ult i32 %index, %count
  %inc = add i32 %index, 1
  br i1 %cmptmp, label %loop, label %afterloop


afterloop:                                        ; preds = %loop2
  ret void
}


attributes #0 = { argmemonly nounwind readonly willreturn }

!igc.functions = !{}
