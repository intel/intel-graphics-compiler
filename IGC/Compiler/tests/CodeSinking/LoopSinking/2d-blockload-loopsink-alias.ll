;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --regkey LoopSinkMinSave=1 --regkey LoopSinkEnable2dBlockReads=1 --regkey ForceLoopSink=1 --regkey CodeLoopSinkingMinSize=10 --regkey LoopSinkDisableRollback=1 --basic-aa --igc-code-loop-sinking -S %s 2>&1 | FileCheck %s

; 2d block reads and writes address memory through an i64 surface base or an address payload,
; not through a pointer operand. A 2d block read must not be sunk into a loop that contains
; a 2d block write to memory that may alias it.

declare <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32) #0
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v32i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <32 x i16>) #1
declare ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32) #2
declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i64(ptr, i32, i64, i1) #1
declare void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr, i32, i32, i1) #1
declare <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32) #0
declare void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v32i16(ptr, i32, i32, i32, i32, i32, i32, i1, i1, i32, <32 x i16>) #1
declare ptr @llvm.genx.GenISA.LSC2DBlockCopyAddrPayload.p0(ptr) #0
declare void @spoiler(ptr) #3

; Read and write of the same buffer may alias: keep the read in the preheader.
define void @no_sink_2d_write_may_alias(ptr addrspace(1) %in0, ptr addrspace(1) %out0, i32 %count) {
; CHECK-LABEL: @no_sink_2d_write_may_alias(
; CHECK:       entry_preheader:
; CHECK:         call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16
; CHECK:         br label %[[LOOP:.*]]
; CHECK:       [[LOOP]]:
; CHECK-NOT:     call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockWrite.v32i16
entry_preheader:
  %in.int = ptrtoint ptr addrspace(1) %in0 to i64
  %in.base = and i64 %in.int, -64
  %out.base = ptrtoint ptr addrspace(1) %out0 to i64
  %load = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %in.base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 0)
  br label %loop

loop:
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v32i16(i64 %out.base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 0, <32 x i16> %load)
  %inc = add i32 %index, 1
  %continue = icmp ult i32 %inc, %count
  br i1 %continue, label %loop, label %afterloop

afterloop:
  ret void
}

; Same as above, with the write through an address payload (the form the writes have in codegen).
define void @no_sink_2d_payload_write_may_alias(ptr addrspace(1) %in0, ptr addrspace(1) %out0, i32 %count) {
; CHECK-LABEL: @no_sink_2d_payload_write_may_alias(
; CHECK:       entry_preheader:
; CHECK:         call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16
; CHECK:         br label %[[LOOP:.*]]
; CHECK:       [[LOOP]]:
; CHECK-NOT:     call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v32i16
entry_preheader:
  %in.int = ptrtoint ptr addrspace(1) %in0 to i64
  %in.base = and i64 %in.int, -64
  %out.base = ptrtoint ptr addrspace(1) %out0 to i64
  %load = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %in.base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 0)
  %payload = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %out.base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 1)
  br label %loop

loop:
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]
  call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v32i16(ptr %payload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0, <32 x i16> %load)
  %inc = add i32 %index, 1
  %continue = icmp ult i32 %inc, %count
  br i1 %continue, label %loop, label %afterloop

afterloop:
  ret void
}

; The surface base of the read is not computed from a pointer: it may alias the write.
define void @no_sink_unknown_base(i64 %in.addr, ptr addrspace(1) noalias %out0, i32 %count) {
; CHECK-LABEL: @no_sink_unknown_base(
; CHECK:       entry_preheader:
; CHECK:         call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16
; CHECK:         br label %[[LOOP:.*]]
; CHECK:       [[LOOP]]:
; CHECK-NOT:     call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockWrite.v32i16
entry_preheader:
  %in.base = add i64 %in.addr, 64
  %out.int = ptrtoint ptr addrspace(1) %out0 to i64
  %out.base = and i64 %out.int, -64
  %load = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %in.base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 0)
  br label %loop

loop:
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v32i16(i64 %out.base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 0, <32 x i16> %load)
  %inc = add i32 %index, 1
  %continue = icmp ult i32 %inc, %count
  br i1 %continue, label %loop, label %afterloop

afterloop:
  ret void
}

; The base address of the write payload is replaced: it may alias the read.
define void @no_sink_payload_base_set(ptr addrspace(1) %in0, ptr addrspace(1) noalias %out0, i64 %other.base, i32 %count) {
; CHECK-LABEL: @no_sink_payload_base_set(
; CHECK:       entry_preheader:
; CHECK:         call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16
; CHECK:         br label %[[LOOP:.*]]
; CHECK:       [[LOOP]]:
; CHECK-NOT:     call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v32i16
entry_preheader:
  %in.base = ptrtoint ptr addrspace(1) %in0 to i64
  %out.base = ptrtoint ptr addrspace(1) %out0 to i64
  %load = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %in.base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 0)
  %payload = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %out.base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 1)
  br label %loop

loop:
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i64(ptr %payload, i32 1, i64 %other.base, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v32i16(ptr %payload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0, <32 x i16> %load)
  %inc = add i32 %index, 1
  %continue = icmp ult i32 %inc, %count
  br i1 %continue, label %loop, label %afterloop

afterloop:
  ret void
}

; The write goes to a noalias buffer: sinking the read is still allowed.
define void @sink_2d_write_noalias(ptr addrspace(1) %in0, ptr addrspace(1) noalias %out0, i32 %count) {
; CHECK-LABEL: @sink_2d_write_noalias(
; CHECK:       entry_preheader:
; CHECK-NOT:     call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16
; CHECK:         br label %[[LOOP:.*]]
; CHECK:       [[LOOP]]:
; CHECK:         [[LOAD:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v32i16({{.*}}, <32 x i16> [[LOAD]])
entry_preheader:
  %in.int = ptrtoint ptr addrspace(1) %in0 to i64
  %in.base = and i64 %in.int, -64
  %out.base = ptrtoint ptr addrspace(1) %out0 to i64
  %load = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %in.base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 0)
  %payload = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %out.base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 1)
  br label %loop

loop:
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %payload, i32 5, i32 0, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v32i16(ptr %payload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0, <32 x i16> %load)
  %inc = add i32 %index, 1
  %continue = icmp ult i32 %inc, %count
  br i1 %continue, label %loop, label %afterloop

afterloop:
  ret void
}

; Read through an address payload, write to a buffer that may alias it: keep the read in the preheader.
define void @no_sink_2d_payload_read_may_alias(ptr addrspace(1) %in0, ptr addrspace(1) %out0, i32 %count) {
; CHECK-LABEL: @no_sink_2d_payload_read_may_alias(
; CHECK:       entry_preheader:
; CHECK:         call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0
; CHECK:         br label %[[LOOP:.*]]
; CHECK:       [[LOOP]]:
; CHECK-NOT:     call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v32i16
entry_preheader:
  %in.base = ptrtoint ptr addrspace(1) %in0 to i64
  %out.base = ptrtoint ptr addrspace(1) %out0 to i64
  %in.payload = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %in.base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 2)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %in.payload, i32 5, i32 5, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %in.payload, i32 6, i32 6, i1 false)
  %load = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0(ptr %in.payload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 0)
  %out.payload = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %out.base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 1)
  br label %loop

loop:
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]
  call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v32i16(ptr %out.payload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0, <32 x i16> %load)
  %inc = add i32 %index, 1
  %continue = icmp ult i32 %inc, %count
  br i1 %continue, label %loop, label %afterloop

afterloop:
  ret void
}

; Read through an address payload, write to a noalias buffer: sinking the read is still allowed.
define void @sink_2d_payload_read_write_noalias(ptr addrspace(1) %in0, ptr addrspace(1) noalias %out0, i32 %count) {
; CHECK-LABEL: @sink_2d_payload_read_write_noalias(
; CHECK:       entry_preheader:
; CHECK-NOT:     call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0
; CHECK:         br label %[[LOOP:.*]]
; CHECK:       [[LOOP]]:
; CHECK:         [[LOAD:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v32i16({{.*}}, <32 x i16> [[LOAD]])
entry_preheader:
  %in.base = ptrtoint ptr addrspace(1) %in0 to i64
  %out.base = ptrtoint ptr addrspace(1) %out0 to i64
  %in.payload = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %in.base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 2)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %in.payload, i32 5, i32 5, i1 false)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr %in.payload, i32 6, i32 6, i1 false)
  %load = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v32i16.p0(ptr %in.payload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 0)
  %out.payload = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %out.base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 1)
  br label %loop

loop:
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]
  call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v32i16(ptr %out.payload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0, <32 x i16> %load)
  %inc = add i32 %index, 1
  %continue = icmp ult i32 %inc, %count
  br i1 %continue, label %loop, label %afterloop

afterloop:
  ret void
}

; Write through a copy of a payload of a noalias buffer: the copy is traced to the buffer, sinking is still allowed.
define void @sink_payload_copy_noalias(ptr addrspace(1) %in0, ptr addrspace(1) noalias %out0, i32 %count) {
; CHECK-LABEL: @sink_payload_copy_noalias(
; CHECK:       entry_preheader:
; CHECK-NOT:     call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16
; CHECK:         br label %[[LOOP:.*]]
; CHECK:       [[LOOP]]:
; CHECK:         [[LOAD:%.*]] = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v32i16({{.*}}, <32 x i16> [[LOAD]])
entry_preheader:
  %in.base = ptrtoint ptr addrspace(1) %in0 to i64
  %out.base = ptrtoint ptr addrspace(1) %out0 to i64
  %load = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %in.base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 0)
  %payload = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %out.base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 1)
  %payload.copy = call ptr @llvm.genx.GenISA.LSC2DBlockCopyAddrPayload.p0(ptr %payload)
  br label %loop

loop:
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]
  call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v32i16(ptr %payload.copy, i32 0, i32 0, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0, <32 x i16> %load)
  %inc = add i32 %index, 1
  %continue = icmp ult i32 %inc, %count
  br i1 %continue, label %loop, label %afterloop

afterloop:
  ret void
}

; The base address of the copied payload is replaced before the copy: the copy may alias the read.
define void @no_sink_payload_copy_base_set(ptr addrspace(1) %in0, ptr addrspace(1) noalias %out0, i64 %other.base, i32 %count) {
; CHECK-LABEL: @no_sink_payload_copy_base_set(
; CHECK:       entry_preheader:
; CHECK:         call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16
; CHECK:         br label %[[LOOP:.*]]
; CHECK:       [[LOOP]]:
; CHECK-NOT:     call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v32i16
entry_preheader:
  %in.base = ptrtoint ptr addrspace(1) %in0 to i64
  %out.base = ptrtoint ptr addrspace(1) %out0 to i64
  %load = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %in.base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 0)
  %payload = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %out.base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 1)
  call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i64(ptr %payload, i32 1, i64 %other.base, i1 false)
  %payload.copy = call ptr @llvm.genx.GenISA.LSC2DBlockCopyAddrPayload.p0(ptr %payload)
  br label %loop

loop:
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]
  call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v32i16(ptr %payload.copy, i32 0, i32 0, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0, <32 x i16> %load)
  %inc = add i32 %index, 1
  %continue = icmp ult i32 %inc, %count
  br i1 %continue, label %loop, label %afterloop

afterloop:
  ret void
}

; The payload of a noalias buffer has a user that is not a 2d block intrinsic: it may alias the read.
define void @no_sink_payload_unknown_user(ptr addrspace(1) %in0, ptr addrspace(1) noalias %out0, i32 %count) {
; CHECK-LABEL: @no_sink_payload_unknown_user(
; CHECK:       entry_preheader:
; CHECK:         call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16
; CHECK:         br label %[[LOOP:.*]]
; CHECK:       [[LOOP]]:
; CHECK-NOT:     call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16
; CHECK:         call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v32i16
entry_preheader:
  %in.base = ptrtoint ptr addrspace(1) %in0 to i64
  %out.base = ptrtoint ptr addrspace(1) %out0 to i64
  %load = call <32 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v32i16(i64 %in.base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 16, i32 2, i1 false, i1 false, i32 0)
  %payload = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %out.base, i32 127, i32 1023, i32 127, i32 0, i32 0, i32 16, i32 16, i32 1)
  call void @spoiler(ptr %payload)
  br label %loop

loop:
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]
  call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v32i16(ptr %payload, i32 0, i32 0, i32 16, i32 16, i32 16, i32 1, i1 false, i1 false, i32 0, <32 x i16> %load)
  %inc = add i32 %index, 1
  %continue = icmp ult i32 %inc, %count
  br i1 %continue, label %loop, label %afterloop

afterloop:
  ret void
}

attributes #0 = { argmemonly nounwind readonly willreturn }
attributes #1 = { nounwind willreturn }
attributes #2 = { inaccessiblememonly nounwind willreturn writeonly }
attributes #3 = { nounwind readnone willreturn }
