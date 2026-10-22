;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Payload fields whose loop-invariant value already exists before the loop must
; reuse that value. SCEV folds the offset out of "(base + off) & 63" because the
; offset has zero low bits, and SCEVExpander does not reuse %width for the folded
; expression because %width may be more poisonous. Expanding the expression from
; its leaves would recompute the width from %base and keep %base live across the
; loop. A value is reused only if it is no more poisonous or undefined than the
; value of any message sharing the payload.
;
; RUN: igc_opt --opaque-pointers -decompose-2d-block-funcs-with-hoisting -dce -platformbmg -S < %s 2>&1 | FileCheck %s

declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)

; CHECK-LABEL: define spir_kernel void @test_reuse_width_and_x
; CHECK: entry:
; CHECK-NOT: trunc i64 %base
; CHECK: %width = add nuw nsw i32 %low, 191
; CHECK-NEXT: %x = lshr i32 %low, 1
; CHECK-NOT: trunc i64 %base
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %addr, i32 %width, i32 1023, i32 191, i32 %x, i32 %y,
; CHECK: body:
; CHECK: call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0

define spir_kernel void @test_reuse_width_and_x(i32 %N, i64 %base, i32 %tile, i32 %y) {
entry:
  %scaled = mul i32 %tile, 262144
  %ext = sext i32 %scaled to i64
  %off = shl nsw i64 %ext, 1
  %sum = add i64 %base, %off
  %addr = and i64 %sum, -64
  %lo = trunc i64 %sum to i32
  %low = and i32 %lo, 63
  %width = add nuw nsw i32 %low, 191
  %x = lshr i32 %low, 1
  br label %body

body:
  %i = phi i32 [ %N, %entry ], [ %i.next, %body ]
  %0 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %addr, i32 %width, i32 1023, i32 191, i32 %x, i32 %y, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %i.next = sub i32 %i, 1
  %done = icmp eq i32 %i.next, 0
  br i1 %done, label %exit, label %body

exit:
  ret void
}

; Messages on exclusive paths share one payload. Both use %width, so it is reused.
;
; CHECK-LABEL: define spir_kernel void @test_reuse_shared_width
; CHECK: entry:
; CHECK-NOT: trunc i64 %base
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 4096, i32 %width, i32 1023, i32 255,

define spir_kernel void @test_reuse_shared_width(i64 %base, i64 %tile, i1 %cond, ptr addrspace(1) %out) {
entry:
  %off = shl nsw i64 %tile, 6
  %sum = add i64 %base, %off
  %lo = trunc i64 %sum to i32
  %low = and i32 %lo, 63
  %width = add nuw nsw i32 %low, 191
  br label %header

header:
  %i = phi i32 [ 0, %entry ], [ %i.next, %latch ]
  br i1 %cond, label %then, label %else

then:
  %r0 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 4096, i32 %width, i32 1023, i32 255, i32 0, i32 0, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  store <8 x i32> %r0, ptr addrspace(1) %out
  br label %latch

else:
  %r1 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 4096, i32 %width, i32 1023, i32 255, i32 0, i32 0, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  store <8 x i32> %r1, ptr addrspace(1) %out
  br label %latch

latch:
  %i.next = add i32 %i, 1
  %done = icmp eq i32 %i.next, 4
  br i1 %done, label %exit, label %header

exit:
  ret void
}

; Messages on exclusive paths share one payload. %undef_x and %x have the same
; non-constant SCEV, but %undef_x may be undef, so only %x is reused for X.
;
; CHECK-LABEL: define spir_kernel void @test_no_reuse_undef_x
; CHECK-NOT: CreateAddrPayload.p0(i64 4096, i32 255, i32 1023, i32 255, i32 %undef_x,
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 4096, i32 255, i32 1023, i32 255, i32 %x,

define spir_kernel void @test_no_reuse_undef_x(i32 noundef %x, i1 %cond, ptr addrspace(1) %out) {
entry:
  %any = sub i32 undef, undef
  %undef_x = add i32 %x, %any
  br label %header

header:
  %i = phi i32 [ 0, %entry ], [ %i.next, %latch ]
  br i1 %cond, label %then, label %else

then:
  %r0 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 4096, i32 255, i32 1023, i32 255, i32 %undef_x, i32 0, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  store <8 x i32> %r0, ptr addrspace(1) %out
  br label %latch

else:
  %r1 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 4096, i32 255, i32 1023, i32 255, i32 %x, i32 0, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  store <8 x i32> %r1, ptr addrspace(1) %out
  br label %latch

latch:
  %i.next = add i32 %i, 1
  %done = icmp eq i32 %i.next, 4
  br i1 %done, label %exit, label %header

exit:
  ret void
}

; Messages that share one payload through an immediate X offset. The payload
; takes the X value of the first message, which is reused even though it may be
; undef, because the X value of the other message is that value plus a constant.
;
; %x1 is not guaranteed to be defined, but %x2 = %x1 + 16.
;
; CHECK-LABEL: define spir_kernel void @test_reuse_x_with_add_imm_offset
; CHECK: entry:
; CHECK-NOT: trunc {{.*}}i64 %base
; CHECK: [[PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %addr, i32 255, i32 1023, i32 255, i32 %x1, i32 0, i32 8, i32 16, i32 1)
; CHECK: body:
; CHECK: call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr [[PAYLOAD]], i32 0, i32 0,
; CHECK: call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr [[PAYLOAD]], i32 16, i32 0,

define spir_kernel void @test_reuse_x_with_add_imm_offset(i32 %N, i64 %base, i32 %tile) {
entry:
  %scaled = mul i32 %tile, 262144
  %ext = sext i32 %scaled to i64
  %off = shl nsw i64 %ext, 1
  %sum = add i64 %base, %off
  %addr = and i64 %sum, -64
  %lo = trunc i64 %sum to i32
  %low = and i32 %lo, 63
  %x1 = lshr i32 %low, 1
  %x2 = add i32 %x1, 16
  br label %body

body:
  %i = phi i32 [ %N, %entry ], [ %i.next, %body ]
  %r0 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %addr, i32 255, i32 1023, i32 255, i32 %x1, i32 0, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %r1 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %addr, i32 255, i32 1023, i32 255, i32 %x2, i32 0, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %i.next = sub i32 %i, 1
  %done = icmp eq i32 %i.next, 0
  br i1 %done, label %exit, label %body

exit:
  ret void
}

; Same with a negative offset: %x2 = %x1 - 16.
;
; CHECK-LABEL: define spir_kernel void @test_reuse_x_with_sub_imm_offset
; CHECK: entry:
; CHECK-NOT: trunc {{.*}}i64 %base
; CHECK: [[PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %addr, i32 255, i32 1023, i32 255, i32 %x1, i32 0, i32 8, i32 16, i32 1)
; CHECK: body:
; CHECK: call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr [[PAYLOAD]], i32 0, i32 0,
; CHECK: call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr [[PAYLOAD]], i32 -16, i32 0,

define spir_kernel void @test_reuse_x_with_sub_imm_offset(i32 %N, i64 %base, i32 %tile) {
entry:
  %scaled = mul i32 %tile, 262144
  %ext = sext i32 %scaled to i64
  %off = shl nsw i64 %ext, 1
  %sum = add i64 %base, %off
  %addr = and i64 %sum, -64
  %lo = trunc i64 %sum to i32
  %low = and i32 %lo, 63
  %x1 = lshr i32 %low, 1
  %x2 = sub i32 %x1, 16
  br label %body

body:
  %i = phi i32 [ %N, %entry ], [ %i.next, %body ]
  %r0 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %addr, i32 255, i32 1023, i32 255, i32 %x1, i32 0, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %r1 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %addr, i32 255, i32 1023, i32 255, i32 %x2, i32 0, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  %i.next = sub i32 %i, 1
  %done = icmp eq i32 %i.next, 0
  br i1 %done, label %exit, label %body

exit:
  ret void
}
