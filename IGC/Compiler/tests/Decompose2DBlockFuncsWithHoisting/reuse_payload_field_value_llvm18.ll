;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Payload field reuse cases whose expected output depends on the LLVM 18+
; SCEVExpander, which does not reuse a value that may be more poisonous than the
; expression, or on LLVM 18+ IR syntax ("or disjoint").
;
; REQUIRES: llvm-18-plus
; RUN: igc_opt --opaque-pointers -decompose-2d-block-funcs-with-hoisting -dce -platformbmg -S < %s 2>&1 | FileCheck %s

declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)

; Messages on exclusive paths share one payload but use different widths with the
; same SCEV. %width_off can be poison when %width_base is not, because the shifted
; offset can overflow, and %width_base may differ from %width_off when %base is
; undef, so the pass reuses neither and leaves the width to SCEVExpander. Since
; LLVM 18 the expander reuses only a value that is not more poisonous.
;
; CHECK-LABEL: define spir_kernel void @test_no_reuse_more_poisonous_width
; CHECK-NOT: CreateAddrPayload.p0(i64 4096, i32 %width_off,
; CHECK: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 4096, i32 %width_base, i32 1023, i32 255,

define spir_kernel void @test_no_reuse_more_poisonous_width(i64 %base, i64 %tile, i1 %cond, ptr addrspace(1) %out) {
entry:
  %off = shl nsw i64 %tile, 6
  %sum = add i64 %base, %off
  %lo_off = trunc i64 %sum to i32
  %low_off = and i32 %lo_off, 63
  %width_off = add nuw nsw i32 %low_off, 191
  %lo_base = trunc i64 %base to i32
  %low_base = and i32 %lo_base, 63
  %width_base = add nuw nsw i32 %low_base, 191
  br label %header

header:
  %i = phi i32 [ 0, %entry ], [ %i.next, %latch ]
  br i1 %cond, label %then, label %else

then:
  %r0 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 4096, i32 %width_off, i32 1023, i32 255, i32 0, i32 0, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  store <8 x i32> %r0, ptr addrspace(1) %out
  br label %latch

else:
  %r1 = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 4096, i32 %width_base, i32 1023, i32 255, i32 0, i32 0, i32 32, i32 8, i32 16, i32 1, i1 true, i1 false, i32 0)
  store <8 x i32> %r1, ptr addrspace(1) %out
  br label %latch

latch:
  %i.next = add i32 %i, 1
  %done = icmp eq i32 %i.next, 4
  br i1 %done, label %exit, label %header

exit:
  ret void
}

; %x2 = %h + 16, where %h has the same SCEV as %x1 but is a different value.
; If %low is undef, %x1 and %h may differ, so payload X = %x1 plus 16 is not a
; value the second message could have read. The pass must not reuse %x1 and
; leaves X to SCEVExpander, which rebuilds it from %base.
;
; CHECK-LABEL: define spir_kernel void @test_no_reuse_x_not_offset_of_value
; CHECK: entry:
; CHECK-NOT: CreateAddrPayload.p0(i64 %addr, i32 255, i32 1023, i32 255, i32 %x1,
; CHECK-NOT: CreateAddrPayload.p0(i64 %addr, i32 255, i32 1023, i32 255, i32 %h,
; CHECK: trunc {{.*}}i64 %base
; CHECK: [[PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %addr, i32 255, i32 1023, i32 255, i32 {{%[^,]+}}, i32 0, i32 8, i32 16, i32 1)
; CHECK: body:
; CHECK: call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr [[PAYLOAD]], i32 0, i32 0,
; CHECK: call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr [[PAYLOAD]], i32 16, i32 0,

define spir_kernel void @test_no_reuse_x_not_offset_of_value(i32 %N, i64 %base, i32 %tile) {
entry:
  %scaled = mul i32 %tile, 262144
  %ext = sext i32 %scaled to i64
  %off = shl nsw i64 %ext, 1
  %sum = add i64 %base, %off
  %addr = and i64 %sum, -64
  %lo = trunc i64 %sum to i32
  %low = and i32 %lo, 63
  %x1 = lshr i32 %low, 1
  %h = lshr i32 %low, 1
  %x2 = add i32 %h, 16
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

; %x2 = or disjoint %x1, 16 is %x1 + 16, so %x1 is reused.
;
; CHECK-LABEL: define spir_kernel void @test_reuse_x_with_disjoint_or_imm_offset
; CHECK: entry:
; CHECK-NOT: trunc {{.*}}i64 %base
; CHECK: [[PAYLOAD:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(i64 %addr, i32 255, i32 1023, i32 255, i32 %x1, i32 0, i32 8, i32 16, i32 1)
; CHECK: body:
; CHECK: call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr [[PAYLOAD]], i32 0, i32 0,
; CHECK: call <8 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v8i32.p0(ptr [[PAYLOAD]], i32 16, i32 0,

define spir_kernel void @test_reuse_x_with_disjoint_or_imm_offset(i32 %N, i64 %base, i32 %tile) {
entry:
  %scaled = mul i32 %tile, 262144
  %ext = sext i32 %scaled to i64
  %off = shl nsw i64 %ext, 1
  %sum = add i64 %base, %off
  %addr = and i64 %sum, -64
  %lo = trunc i64 %sum to i32
  %low = and i32 %lo, 63
  %x1 = shl i32 %low, 5
  %x2 = or disjoint i32 %x1, 16
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
