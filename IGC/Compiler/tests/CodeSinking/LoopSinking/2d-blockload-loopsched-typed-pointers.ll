;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-14-plus
; RUN: igc_opt --regkey LoopSinkMinSave=0 --regkey LoopSinkEnable2dBlockReads=1 --regkey LoopSinkCoarserLoadsRescheduling=1 --regkey LoopSinkEnableLoadsRescheduling=1 --regkey ForceLoopSink=1 --regkey CodeLoopSinkingMinSize=10 --regkey LoopSinkDisableRollback=1 --regkey CodeSinkingLoadSchedulingInstr=0 %enable-basic-aa% --igc-code-loop-sinking -S %s 2>&1 | FileCheck %s

declare i8* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i8(i64, i32, i32, i32, i32, i32, i32, i32, i32) #0

declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8*, i32, i32, i1) #0

declare <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8*, i32, i32, i32, i32, i32, i32, i1, i1, i32) #0

; We cannot reorder load2 and load3, but we can more them after the first store together
; And also move the first store closer to its uses separately

define void @sink1(half addrspace(1)* %in0, <32 x i16> addrspace(1)* noalias %out0, i32 %count, i32 %offsetIn0, <8 x i32> %r0) {
; CHECK-LABEL: @sink1(
; CHECK:       entry_preheader:

; CHECK:         [[BLOCK2D_ADDRPAYLOAD:%.*]] = call i8* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i8(i64 [[BASE_ADDR:%.*]], i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 2)

; CHECK:       loop:

; CHECK:         mul
; CHECK:         mul
; CHECK:         mul

; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 5, i32 1, i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 6, i32 1, i1 false)
; CHECK:         [[SINK_LOAD:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

; CHECK:         store <32 x i16> [[SINK_LOAD]], <32 x i16> addrspace(1)* [[ADDR:%.*]], align 64

; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 2, i32 80, i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 5, i32 3, i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 6, i32 3, i1 false)
; CHECK:         [[SINK_LOAD3:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 5, i32 2, i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 6, i32 2, i1 false)
; CHECK:         [[SINK_LOAD2:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

; CHECK:         store <32 x i16> [[SINK_LOAD2]], <32 x i16> addrspace(1)* [[ADDR2:%.*]], align 64
; CHECK:         store <32 x i16> [[SINK_LOAD3]], <32 x i16> addrspace(1)* [[ADDR3:%.*]], align 64

; CHECK:         br



entry_preheader:
  %bc = bitcast <8 x i32> %r0 to <32 x i8>
  %ee_thread = extractelement <32 x i8> %bc, i32 8

  %base_addr = ptrtoint half addrspace(1)* %in0 to i64

  %Block2D_AddrPayload = call i8* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i8(i64 %base_addr, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 2)

  br label %loop

loop:                                             ; preds = %loop, %entry_preheader
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 5, i32 1, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 6, i32 1, i1 false)
  %load = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

  %x = add i32 %index, 5
  %y = add i32 %index, 6
  %z = add i32 %index, 12
  %x1 = mul i32 %index, %x

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 2, i32 80, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 5, i32 3, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 6, i32 3, i1 false)
  %load3 = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 5, i32 2, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 6, i32 2, i1 false)
  %load2 = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

  %y1 = mul i32 %x1, %y
  %z1 = mul i32 %y1, %z


  %addr = getelementptr <32 x i16>, <32 x i16> addrspace(1)* %out0, i32 %index
  store <32 x i16> %load, <32 x i16> addrspace(1)* %addr, align 64

  %addr2 = getelementptr <32 x i16>, <32 x i16> addrspace(1)* %out0, i32 %x1
  store <32 x i16> %load2, <32 x i16> addrspace(1)* %addr2, align 64

  %addr3 = getelementptr <32 x i16>, <32 x i16> addrspace(1)* %out0, i32 %y1
  store <32 x i16> %load3, <32 x i16> addrspace(1)* %addr3, align 64

  %cmptmp = icmp ult i32 %index, %count
  %inc = add i32 %index, 1
  br i1 %cmptmp, label %loop, label %afterloop

afterloop:                                        ; preds = %loop
  ret void
}


attributes #0 = { argmemonly nounwind readonly willreturn }

!igc.functions = !{}
