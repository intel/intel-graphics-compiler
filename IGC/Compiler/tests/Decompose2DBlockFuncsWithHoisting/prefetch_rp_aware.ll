;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -decompose-2d-block-funcs-with-hoisting --regkey=AllowPrefetchDecomposeWithHoisting=-1 -dce -platformbmg -S < %s 2>&1 | FileCheck %s
; RUN: igc_opt --opaque-pointers -decompose-2d-block-funcs-with-hoisting --regkey=AllowPrefetchDecomposeWithHoisting=-1 --regkey=TotalGRFNum=256 -dce -platformbmg -S < %s 2>&1 | FileCheck %s --check-prefix=HOIST
; RUN: igc_opt --opaque-pointers -decompose-2d-block-funcs-with-hoisting --regkey=AllowPrefetchDecomposeWithHoisting=1 -dce -platformbmg -S < %s 2>&1 | FileCheck %s --check-prefix=HOIST

declare void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <16 x i32>)
declare i16 @llvm.genx.GenISA.simdLaneId()

; High register pressure blocks the standalone prefetch payload. The payload
; shared with the read/write is still hoisted, as it would be without
; prefetch decomposition. A larger budget or forced mode hoists both.
; CHECK-LABEL: define spir_kernel void @test_high_pressure
; CHECK-LABEL: entry:
; CHECK: [[SHARED:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(
; CHECK-NOT: LSC2DBlockCreateAddrPayload
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[SHARED]]
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetch{{(.p0)?}}(
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[SHARED]]
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v16i32(ptr [[SHARED]]
; CHECK-NOT: LSC2DBlockCreateAddrPayload
; CHECK: ret void
;
; HOIST-LABEL: define spir_kernel void @test_high_pressure
; HOIST-LABEL: entry:
; HOIST: [[SHARED:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(
; HOIST: [[STANDALONE:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(
; HOIST-NOT: LSC2DBlockCreateAddrPayload
; HOIST-LABEL: body:
; HOIST: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[SHARED]]
; HOIST: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[STANDALONE]]
; HOIST: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(ptr [[SHARED]]
; HOIST: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v16i32(ptr [[SHARED]]

define spir_kernel void @test_high_pressure(i32 %N, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, ptr addrspace(1) %out) {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lanef = uitofp i16 %lane to float
  %ins = insertelement <64 x float> undef, float %lanef, i32 0
  %init = shufflevector <64 x float> %ins, <64 x float> undef, <64 x i32> zeroinitializer
  br label %body

body:
  %acc = phi <64 x float> [ %init, %entry ], [ %acc.next, %body ]
  %phi_x = phi i32 [ %offsetX, %entry ], [ %phi_x_next, %body ]
  %i = phi i32 [ 0, %entry ], [ %i.next, %body ]
  %prefetch_x = add i32 %phi_x, 64
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %prefetch_x, i32 %offsetY, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %prefetch_x, i32 64, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 4)
  %ld = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %offsetY, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %phi_x, i32 %offsetY, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <16 x i32> %ld)
  %ldf = bitcast <16 x i32> %ld to <16 x float>
  %ldw = shufflevector <16 x float> %ldf, <16 x float> undef, <64 x i32> zeroinitializer
  %acc.next = fadd <64 x float> %acc, %ldw
  %phi_x_next = add i32 %phi_x, 32
  %i.next = add i32 %i, 1
  %cond = icmp slt i32 %i.next, %N
  br i1 %cond, label %body, label %exit

exit:
  store <64 x float> %acc.next, ptr addrspace(1) %out
  ret void
}

; High inner-loop pressure blocks the outer payload.
; The low-pressure sibling payload is hoisted.
; CHECK-LABEL: define spir_kernel void @test_per_payload
; CHECK-NOT: LSC2DBlockCreateAddrPayload
; CHECK-LABEL: outer:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetch{{(.p0)?}}(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %outer_x, i32 %offsetY, i32 16, i32 32, i32 8, i32 1, i1 false, i1 false, i32 4)
; CHECK-NOT: LSC2DBlockCreateAddrPayload
; CHECK-LABEL: sibling.ph:
; CHECK: [[SIBLING:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(
; CHECK-NOT: LSC2DBlockCreateAddrPayload
; CHECK-LABEL: sibling:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[SIBLING]], i32 5, i32 %sib_x, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[SIBLING]]
; CHECK-NOT: LSC2DBlockCreateAddrPayload
; CHECK: ret void
;
; HOIST-LABEL: define spir_kernel void @test_per_payload
; HOIST-LABEL: outer:
; HOIST: call void @llvm.genx.GenISA.LSC2DBlockPrefetch{{.*}}(
; HOIST-LABEL: sibling.ph:
; HOIST: [[LOW:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(
; HOIST-LABEL: sibling:
; HOIST: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[LOW]]

define spir_kernel void @test_per_payload(i32 %N, i32 %M, i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %offsetX, i32 %offsetY, ptr addrspace(1) %out) #0 {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lanef = uitofp i16 %lane to float
  %ins = insertelement <128 x float> undef, float %lanef, i32 0
  %init = shufflevector <128 x float> %ins, <128 x float> undef, <128 x i32> zeroinitializer
  br label %outer

outer:
  %outer_x = phi i32 [ %offsetX, %entry ], [ %outer_x_next, %outer.latch ]
  %j = phi i32 [ 0, %entry ], [ %j.next, %outer.latch ]
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %outer_x, i32 %offsetY, i32 16, i32 32, i32 8, i32 1, i1 false, i1 false, i32 4)
  br label %inner

inner:
  %acc = phi <128 x float> [ %init, %outer ], [ %acc.next, %inner ]
  %i = phi i32 [ 0, %outer ], [ %i.next, %inner ]
  %acc.next = fadd <128 x float> %acc, %acc
  %i.next = add i32 %i, 1
  %cond = icmp slt i32 %i.next, %N
  br i1 %cond, label %inner, label %outer.latch

outer.latch:
  %gep = getelementptr <128 x float>, ptr addrspace(1) %out, i32 %j
  store <128 x float> %acc.next, ptr addrspace(1) %gep
  %outer_x_next = add i32 %outer_x, 32
  %j.next = add i32 %j, 1
  %outer.cond = icmp slt i32 %j.next, %M
  br i1 %outer.cond, label %outer, label %sibling.ph

sibling.ph:
  br label %sibling

sibling:
  %sib_x = phi i32 [ %offsetX, %sibling.ph ], [ %sib_x_next, %sibling ]
  %k = phi i32 [ 0, %sibling.ph ], [ %k.next, %sibling ]
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 %width, i32 %height, i32 %pitch, i32 %sib_x, i32 %offsetY, i32 16, i32 32, i32 8, i32 1, i1 false, i1 false, i32 4)
  %sib_x_next = add i32 %sib_x, 32
  %k.next = add i32 %k, 1
  %sib.cond = icmp slt i32 %k.next, %N
  br i1 %sib.cond, label %sibling, label %exit

exit:
  ret void
}

; At SIMD16, a 118-GRF accumulator leaves room for two payloads.
; All four read/write payloads are hoisted and use up that room, so the
; earlier prefetch payload no longer fits.
; CHECK-LABEL: define spir_kernel void @test_independent_read_write_payloads
; CHECK-LABEL: entry:
; CHECK-COUNT-4: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(
; CHECK-NOT: LSC2DBlockCreateAddrPayload
; CHECK-LABEL: body:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetch{{(.p0)?}}(
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v16i32(
; CHECK: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v16i32(
; CHECK-NOT: LSC2DBlockCreateAddrPayload
; CHECK: ret void
;
; HOIST-LABEL: define spir_kernel void @test_independent_read_write_payloads
; HOIST-LABEL: entry:
; HOIST-COUNT-5: call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(
; HOIST-NOT: LSC2DBlockCreateAddrPayload
; HOIST-LABEL: body:
; HOIST: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(
; HOIST: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(
; HOIST: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v16i32(
; HOIST: call <16 x i32> @llvm.genx.GenISA.LSC2DBlockReadAddrPayload.v16i32.p0(
; HOIST: call void @llvm.genx.GenISA.LSC2DBlockWriteAddrPayload.p0.v16i32(
; HOIST: ret void

define spir_kernel void @test_independent_read_write_payloads(i32 %N, i64 %off, i32 %offsetX, ptr addrspace(1) %out) {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lanef = uitofp i16 %lane to float
  %ins = insertelement <118 x float> undef, float %lanef, i32 0
  %init = shufflevector <118 x float> %ins, <118 x float> undef, <118 x i32> zeroinitializer
  br label %body

body:
  %acc = phi <118 x float> [ %init, %entry ], [ %acc.next, %body ]
  %phi_x = phi i32 [ %offsetX, %entry ], [ %phi_x_next, %body ]
  %i = phi i32 [ 0, %entry ], [ %i.next, %body ]
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 1024, i32 1024, i32 1024, i32 %phi_x, i32 32, i32 32, i32 16, i32 1, i32 1, i1 false, i1 false, i32 0)
  %r0 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 1024, i32 1024, i32 1024, i32 %phi_x, i32 0, i32 32, i32 16, i32 1, i32 1, i1 false, i1 false, i32 0)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 1024, i32 1024, i32 1024, i32 %phi_x, i32 1, i32 32, i32 16, i32 1, i32 1, i1 false, i1 false, i32 0, <16 x i32> zeroinitializer)
  %r1 = call <16 x i32> @llvm.genx.GenISA.LSC2DBlockRead.p0(i64 %off, i32 1024, i32 1024, i32 1024, i32 %phi_x, i32 2, i32 32, i32 16, i32 1, i32 1, i1 false, i1 false, i32 0)
  call void @llvm.genx.GenISA.LSC2DBlockWrite.p0(i64 %off, i32 1024, i32 1024, i32 1024, i32 %phi_x, i32 3, i32 32, i32 16, i32 1, i32 1, i1 false, i1 false, i32 0, <16 x i32> zeroinitializer)
  %acc.next = fadd <118 x float> %acc, %acc
  %phi_x_next = add i32 %phi_x, 32
  %i.next = add i32 %i, 1
  %cond = icmp slt i32 %i.next, %N
  br i1 %cond, label %body, label %exit

exit:
  store <118 x float> %acc.next, ptr addrspace(1) %out
  ret void
}

; A 115-GRF accumulator leaves room for one payload.
; The inner payload is checked first, resulting in no room for the outer payload.
; CHECK-LABEL: define spir_kernel void @test_nested_depth_order
; CHECK-LABEL: entry:
; CHECK-NOT: LSC2DBlockCreateAddrPayload
; CHECK-LABEL: outer:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetch{{(.p0)?}}(i64 %off, i32 1024, i32 1024, i32 1024, i32 %outer_x,
; CHECK: [[INNER:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(
; CHECK-NOT: LSC2DBlockCreateAddrPayload
; CHECK-LABEL: inner:
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockSetAddrPayloadField.p0.i32(ptr [[INNER]], i32 5, i32 %inner_x, i1 false)
; CHECK: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[INNER]]
; CHECK-NOT: LSC2DBlockCreateAddrPayload
; CHECK: ret void
;
; HOIST-LABEL: define spir_kernel void @test_nested_depth_order
; HOIST-LABEL: entry:
; HOIST: [[OUTER:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(
; HOIST-LABEL: outer:
; HOIST: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[OUTER]]
; HOIST: [[INNER:%.+]] = call ptr @llvm.genx.GenISA.LSC2DBlockCreateAddrPayload.p0(
; HOIST-LABEL: inner:
; HOIST: call void @llvm.genx.GenISA.LSC2DBlockPrefetchAddrPayload.p0(ptr [[INNER]]

define spir_kernel void @test_nested_depth_order(i32 %N, i32 %M, i64 %off, i32 %offsetX, ptr addrspace(1) %out) {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lanef = uitofp i16 %lane to float
  %ins = insertelement <115 x float> undef, float %lanef, i32 0
  %init = shufflevector <115 x float> %ins, <115 x float> undef, <115 x i32> zeroinitializer
  br label %outer

outer:
  %acc.outer = phi <115 x float> [ %init, %entry ], [ %acc.next, %outer.latch ]
  %outer_x = phi i32 [ %offsetX, %entry ], [ %outer_x_next, %outer.latch ]
  %j = phi i32 [ 0, %entry ], [ %j.next, %outer.latch ]
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 1024, i32 1024, i32 1024, i32 %outer_x, i32 0, i32 32, i32 16, i32 1, i32 1, i1 false, i1 false, i32 4)
  br label %inner

inner:
  %acc = phi <115 x float> [ %acc.outer, %outer ], [ %acc.next, %inner ]
  %inner_x = phi i32 [ %offsetX, %outer ], [ %inner_x_next, %inner ]
  %i = phi i32 [ 0, %outer ], [ %i.next, %inner ]
  call void @llvm.genx.GenISA.LSC2DBlockPrefetch.p0(i64 %off, i32 1024, i32 1024, i32 1024, i32 %inner_x, i32 1, i32 32, i32 16, i32 1, i32 1, i1 false, i1 false, i32 4)
  %acc.next = fadd <115 x float> %acc, %acc
  %inner_x_next = add i32 %inner_x, 32
  %i.next = add i32 %i, 1
  %cond = icmp slt i32 %i.next, %N
  br i1 %cond, label %inner, label %outer.latch

outer.latch:
  %outer_x_next = add i32 %outer_x, 32
  %j.next = add i32 %j, 1
  %outer.cond = icmp slt i32 %j.next, %M
  br i1 %outer.cond, label %outer, label %exit

exit:
  store <115 x float> %acc.next, ptr addrspace(1) %out
  ret void
}

attributes #0 = { "num-grf-per-thread"="0" }
