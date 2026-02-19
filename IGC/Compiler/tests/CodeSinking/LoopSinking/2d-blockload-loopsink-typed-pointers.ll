;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-14-plus
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --regkey LoopSinkMinSave=1 --regkey LoopSinkEnable2dBlockReads=1 --regkey ForceLoopSink=1 --regkey CodeLoopSinkingMinSize=10 --regkey LoopSinkDisableRollback=1 --regkey CodeSinkingLoadSchedulingInstr=10 --basic-aa --igc-code-loop-sinking -S %s 2>&1 | FileCheck %s

declare i8* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i8(i64, i32, i32, i32, i32, i32, i32, i32, i32) #0

declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8*, i32, i32, i1) #0

declare void @spoiler(i8*) #0

declare <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8*, i32, i32, i32, i32, i32, i32, i1, i1, i32) #0

declare <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32) #0

; Sink all 2d block reads to the second BB in the loop, because it contains all uses
define void @sink1(half addrspace(1)* %in0, <32 x i16> addrspace(1)* noalias %out0, i32 %count, i32 %offsetIn0, <8 x i32> %r0) {
; CHECK-LABEL: @sink1(
; CHECK:       entry_preheader:
; CHECK:         [[BASE_ADDR:%.*]] = ptrtoint half addrspace(1)* [[IN0:%.*]] to i64
; CHECK:         [[BLOCK2D_ADDRPAYLOAD:%.*]] = call i8* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i8(i64 [[BASE_ADDR]], i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 2)
; CHECK:         br label [[LOOP:%.*]]

; CHECK:       loop:
; CHECK:         mul
; CHECK:         mul
; CHECK:         mul

; CHECK:       loop2:

; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 5, i32 5, i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 6, i32 6, i1 false)
; CHECK:         [[SINK_LOAD:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 5, i32 12, i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 6, i32 13, i1 false)
; CHECK:         [[LOAD2:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

entry_preheader:
  %bc = bitcast <8 x i32> %r0 to <32 x i8>
  %ee_thread = extractelement <32 x i8> %bc, i32 8
  ; %zext = zext i8 %ee_thread to i32

  %base_addr = ptrtoint half addrspace(1)* %in0 to i64

  %Block2D_AddrPayload = call i8* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i8(i64 %base_addr, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 2)

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 5, i32 5, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 6, i32 6, i1 false)
  %load = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 5, i32 12, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 6, i32 13, i1 false)
  %load2 = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

  br label %loop

loop:                                             ; preds = %loop2, %entry_preheader
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop2 ]

  %x = add i32 %index, 5
  %y = add i32 %index, 6
  %z = add i32 %index, 12
  %x1 = mul i32 %index, %x
  %y1 = mul i32 %x1, %y
  %z1 = mul i32 %y1, %z

  br label %loop2

loop2:                                           ; preds = %loop
  %addr = getelementptr <32 x i16>, <32 x i16> addrspace(1)* %out0, i32 %index
  store <32 x i16> %load, <32 x i16> addrspace(1)* %addr, align 64

  %addr2 = getelementptr <32 x i16>, <32 x i16> addrspace(1)* %out0, i32 %x1
  store <32 x i16> %load2, <32 x i16> addrspace(1)* %addr2, align 64

  %cmptmp = icmp ult i32 %index, %count
  %inc = add i32 %index, 1
  br i1 %cmptmp, label %loop, label %afterloop

afterloop:                                        ; preds = %loop2
  ret void
}


; Sink all 2d block reads to the first BB in the loop, because it's the common dominator of the 2 uses
define void @sink2(half addrspace(1)* %in0, <32 x i16> addrspace(1)* noalias %out0, i32 %count, i32 %offsetIn0, <8 x i32> %r0) {
; CHECK-LABEL: @sink2(
; CHECK:       entry_preheader:
; CHECK:         [[BASE_ADDR:%.*]] = ptrtoint half addrspace(1)* [[IN0:%.*]] to i64
; CHECK:         [[BLOCK2D_ADDRPAYLOAD:%.*]] = call i8* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i8(i64 [[BASE_ADDR]], i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 2)
; CHECK:         br label [[LOOP:%.*]]

; CHECK:       loop:

; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 5, i32 5, i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 6, i32 6, i1 false)
; CHECK:         [[SINK_LOAD:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 5, i32 12, i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 6, i32 13, i1 false)
; CHECK:         [[LOAD2:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

; CHECK:         mul
; CHECK:         mul
; CHECK:         mul

; CHECK:       loop2:


entry_preheader:
  %bc = bitcast <8 x i32> %r0 to <32 x i8>
  %ee_thread = extractelement <32 x i8> %bc, i32 8
  ; %zext = zext i8 %ee_thread to i32

  %base_addr = ptrtoint half addrspace(1)* %in0 to i64

  %Block2D_AddrPayload = call i8* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i8(i64 %base_addr, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 2)

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 5, i32 5, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 6, i32 6, i1 false)
  %load = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 5, i32 12, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 6, i32 13, i1 false)
  %load2 = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

  br label %loop

loop:                                             ; preds = %loop2, %entry_preheader
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop2 ]

  %x = add i32 %index, 5
  %y = add i32 %index, 6
  %z = add i32 %index, 12
  %x1 = mul i32 %index, %x

  %addr2 = getelementptr <32 x i16>, <32 x i16> addrspace(1)* %out0, i32 %x1
  store <32 x i16> %load2, <32 x i16> addrspace(1)* %addr2, align 64

  %y1 = mul i32 %x1, %y
  %z1 = mul i32 %y1, %z

  br label %loop2

loop2:                                           ; preds = %loop
  %addr = getelementptr <32 x i16>, <32 x i16> addrspace(1)* %out0, i32 %index
  store <32 x i16> %load, <32 x i16> addrspace(1)* %addr, align 64

  %cmptmp = icmp ult i32 %index, %count
  %inc = add i32 %index, 1
  br i1 %cmptmp, label %loop, label %afterloop

afterloop:                                        ; preds = %loop2
  ret void
}


; Sink all non-payload 2d block reads to the loop
define void @sink3_nonpayload(half addrspace(1)* %in0, <32 x i16> addrspace(1)* noalias %out0, i32 %count, i32 %offsetIn0, <8 x i32> %r0) {
; CHECK-LABEL: @sink3_nonpayload(
; CHECK:       entry_preheader:
; CHECK:         br label [[LOOP:%.*]]

; CHECK:       loop:
; CHECK:         [[BASE_ADDR:%.*]] = ptrtoint half addrspace(1)* [[IN0:%.*]] to i64
; CHECK:         [[SINK_LOAD2:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 [[BASE_ADDR]], i32 127, i32 1023, i32 127, i32 32, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
; CHECK:         br label %loop2

; CHECK:       loop2:
; CHECK:         [[SINK_LOAD:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 [[BASE_ADDR]], i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)


entry_preheader:
  %bc = bitcast <8 x i32> %r0 to <32 x i8>
  %ee_thread = extractelement <32 x i8> %bc, i32 8
  ; %zext = zext i8 %ee_thread to i32

  %base_addr = ptrtoint half addrspace(1)* %in0 to i64

  %load = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %base_addr, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)
  %load2 = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %base_addr, i32 127, i32 1023, i32 127, i32 32, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

  br label %loop

loop:                                             ; preds = %loop2, %entry_preheader
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop2 ]

  %x = add i32 %index, 5
  %y = add i32 %index, 6
  %z = add i32 %index, 12
  %x1 = mul i32 %index, %x

  %addr2 = getelementptr <32 x i16>, <32 x i16> addrspace(1)* %out0, i32 %x1
  store <32 x i16> %load2, <32 x i16> addrspace(1)* %addr2, align 64

  %y1 = mul i32 %x1, %y
  %z1 = mul i32 %y1, %z

  br label %loop2

loop2:                                           ; preds = %loop
  %addr = getelementptr <32 x i16>, <32 x i16> addrspace(1)* %out0, i32 %index
  store <32 x i16> %load, <32 x i16> addrspace(1)* %addr, align 64

  %cmptmp = icmp ult i32 %index, %count
  %inc = add i32 %index, 1
  br i1 %cmptmp, label %loop, label %afterloop

afterloop:                                        ; preds = %loop2
  ret void
}



; Don't sink 2d block reads, because they may alias (no noalias attr on store in the loop)
define void @nosink1(half addrspace(1)* %in0, <32 x i16> addrspace(1)* %out0, i32 %count, i32 %offsetIn0, <8 x i32> %r0) {
; CHECK-LABEL: @nosink1(
; CHECK:       entry_preheader:
; CHECK:         [[BASE_ADDR:%.*]] = ptrtoint half addrspace(1)* [[IN0:%.*]] to i64
; CHECK:         [[BLOCK2D_ADDRPAYLOAD:%.*]] = call i8* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i8(i64 [[BASE_ADDR]], i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 2)

; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 5, i32 5, i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 6, i32 6, i1 false)
; CHECK:         [[SINK_LOAD:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 5, i32 12, i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 6, i32 13, i1 false)
; CHECK:         [[LOAD2:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

; CHECK:         br label [[LOOP:%.*]]

; CHECK:       loop:

; CHECK:         mul
; CHECK:         mul
; CHECK:         mul

; CHECK:       loop2:


entry_preheader:
  %bc = bitcast <8 x i32> %r0 to <32 x i8>
  %ee_thread = extractelement <32 x i8> %bc, i32 8
  ; %zext = zext i8 %ee_thread to i32

  %base_addr = ptrtoint half addrspace(1)* %in0 to i64

  %Block2D_AddrPayload = call i8* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i8(i64 %base_addr, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 2)

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 5, i32 5, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 6, i32 6, i1 false)
  %load = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 5, i32 12, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 6, i32 13, i1 false)
  %load2 = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

  br label %loop

loop:                                             ; preds = %loop2, %entry_preheader
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop2 ]

  %x = add i32 %index, 5
  %y = add i32 %index, 6
  %z = add i32 %index, 12
  %x1 = mul i32 %index, %x
  %y1 = mul i32 %x1, %y
  %z1 = mul i32 %y1, %z

  br label %loop2

loop2:                                           ; preds = %loop
  %addr = getelementptr <32 x i16>, <32 x i16> addrspace(1)* %out0, i32 %index
  store <32 x i16> %load, <32 x i16> addrspace(1)* %addr, align 64

  %addr2 = getelementptr <32 x i16>, <32 x i16> addrspace(1)* %out0, i32 %x1
  store <32 x i16> %load2, <32 x i16> addrspace(1)* %addr2, align 64

  %cmptmp = icmp ult i32 %index, %count
  %inc = add i32 %index, 1
  br i1 %cmptmp, label %loop, label %afterloop

afterloop:                                        ; preds = %loop2
  ret void
}


; Don't sink 2d block reads, because there is an unexpected use of AddrPayload
define void @nosink2(half addrspace(1)* %in0, <32 x i16> addrspace(1)* noalias %out0, i32 %count, i32 %offsetIn0, <8 x i32> %r0) {
; CHECK-LABEL: @nosink2(
; CHECK:       entry_preheader:
; CHECK:         [[BASE_ADDR:%.*]] = ptrtoint half addrspace(1)* [[IN0:%.*]] to i64
; CHECK:         [[BLOCK2D_ADDRPAYLOAD:%.*]] = call i8* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i8(i64 [[BASE_ADDR]], i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 2)

; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 5, i32 5, i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 6, i32 6, i1 false)
; CHECK:         [[SINK_LOAD:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 5, i32 12, i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 6, i32 13, i1 false)
; CHECK:         [[LOAD2:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

; CHECK:         br label [[LOOP:%.*]]

; CHECK:       loop:

; CHECK:         mul
; CHECK:         mul
; CHECK:         mul

; CHECK:       loop2:


entry_preheader:
  %bc = bitcast <8 x i32> %r0 to <32 x i8>
  %ee_thread = extractelement <32 x i8> %bc, i32 8
  ; %zext = zext i8 %ee_thread to i32

  %base_addr = ptrtoint half addrspace(1)* %in0 to i64

  %Block2D_AddrPayload = call i8* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i8(i64 %base_addr, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 2)

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 5, i32 5, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 6, i32 6, i1 false)
  %load = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 5, i32 12, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 6, i32 13, i1 false)
  %load2 = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

  br label %loop

loop:                                             ; preds = %loop2, %entry_preheader
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop2 ]

  %x = add i32 %index, 5
  %y = add i32 %index, 6
  %z = add i32 %index, 12
  %x1 = mul i32 %index, %x
  %y1 = mul i32 %x1, %y
  %z1 = mul i32 %y1, %z

  ; spoiler in the loop
  call void @spoiler(i8* %Block2D_AddrPayload)

  br label %loop2

loop2:                                           ; preds = %loop
  %addr = getelementptr <32 x i16>, <32 x i16> addrspace(1)* %out0, i32 %index
  store <32 x i16> %load, <32 x i16> addrspace(1)* %addr, align 64

  %addr2 = getelementptr <32 x i16>, <32 x i16> addrspace(1)* %out0, i32 %x1
  store <32 x i16> %load2, <32 x i16> addrspace(1)* %addr2, align 64

  %cmptmp = icmp ult i32 %index, %count
  %inc = add i32 %index, 1
  br i1 %cmptmp, label %loop, label %afterloop

afterloop:                                        ; preds = %loop2
  ret void
}



; Don't sink 2d block reads, because there is an unexpected use of AddrPayload
define void @nosink3(half addrspace(1)* %in0, <32 x i16> addrspace(1)* noalias %out0, i32 %count, i32 %offsetIn0, <8 x i32> %r0) {
; CHECK-LABEL: @nosink3(
; CHECK:       entry_preheader:
; CHECK:         [[BASE_ADDR:%.*]] = ptrtoint half addrspace(1)* [[IN0:%.*]] to i64
; CHECK:         [[BLOCK2D_ADDRPAYLOAD:%.*]] = call i8* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i8(i64 [[BASE_ADDR]], i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 2)

; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 5, i32 5, i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 6, i32 6, i1 false)
; CHECK:         [[SINK_LOAD:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 5, i32 12, i1 false)
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 6, i32 13, i1 false)
; CHECK:         [[LOAD2:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* [[BLOCK2D_ADDRPAYLOAD]], i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

; CHECK:         br label [[LOOP:%.*]]

; CHECK:       loop:

; CHECK:         mul
; CHECK:         mul
; CHECK:         mul

; CHECK:       loop2:


entry_preheader:
  %bc = bitcast <8 x i32> %r0 to <32 x i8>
  %ee_thread = extractelement <32 x i8> %bc, i32 8
  ; %zext = zext i8 %ee_thread to i32

  %base_addr = ptrtoint half addrspace(1)* %in0 to i64

  %Block2D_AddrPayload = call i8* @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0i8(i64 %base_addr, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 2)

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 5, i32 5, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 6, i32 6, i1 false)
  %load = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 5, i32 12, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0i8.i32(i8* %Block2D_AddrPayload, i32 6, i32 13, i1 false)
  %load2 = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0i8(i8* %Block2D_AddrPayload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 4)

  ; spoiler in the ph
  call void @spoiler(i8* %Block2D_AddrPayload)

  br label %loop

loop:                                             ; preds = %loop2, %entry_preheader
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop2 ]

  %x = add i32 %index, 5
  %y = add i32 %index, 6
  %z = add i32 %index, 12
  %x1 = mul i32 %index, %x
  %y1 = mul i32 %x1, %y
  %z1 = mul i32 %y1, %z

  br label %loop2

loop2:                                           ; preds = %loop
  %addr = getelementptr <32 x i16>, <32 x i16> addrspace(1)* %out0, i32 %index
  store <32 x i16> %load, <32 x i16> addrspace(1)* %addr, align 64

  %addr2 = getelementptr <32 x i16>, <32 x i16> addrspace(1)* %out0, i32 %x1
  store <32 x i16> %load2, <32 x i16> addrspace(1)* %addr2, align 64

  %cmptmp = icmp ult i32 %index, %count
  %inc = add i32 %index, 1
  br i1 %cmptmp, label %loop, label %afterloop

afterloop:                                        ; preds = %loop2
  ret void
}




attributes #0 = { argmemonly nounwind readonly willreturn }

!igc.functions = !{}
