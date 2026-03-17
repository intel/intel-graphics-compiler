;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Regression test for stale DomTree after loop unrolling.
;
; LoopUnrollLegacyPassWrapper runs the new-PM LoopUnrollPass with a private
; FunctionAnalysisManager, which updates its own DomTree but leaves the legacy
; PM's DominatorTreeWrapperPass stale. The stale DomTree persists until SROA,
; causing a crash.

; REQUIRES: regkeys,pvc-supported,llvm-16-plus

; RUN: llvm-as %s -o %t.bc
; RUN: ocloc compile -llvm_input -file %t.bc -device pvc -options " -igc_opts 'Decompose2DBlockFuncsMode=2'" 2>&1 | FileCheck %s

; CHECK: Build succeeded.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_domtree_after_loop_unroll(
    ptr addrspace(1) align 2 %src_a,
    ptr addrspace(1) align 2 %src_b,
    ptr addrspace(1) align 4 %dst_c,
    <8 x i32> %r0, <3 x i32> %globalOffset, <3 x i32> %localSize,
    i16 %localIdX, i16 %localIdY, i16 %localIdZ,
    ptr %privateBase, i32 %bufferOffset,
    i32 %bufferOffset1, i32 %bufferOffset2) {
entry:
  %lsz_x = extractelement <3 x i32> %localSize, i32 0
  %lsz_y = extractelement <3 x i32> %localSize, i32 1
  %lsz_z = extractelement <3 x i32> %localSize, i32 2
  %grp_r0 = extractelement <8 x i32> %r0, i32 1
  %grp_r1 = extractelement <8 x i32> %r0, i32 6
  %tile_a = alloca [4 x [2 x <8 x i16>]], align 8
  %tile_b = alloca [4 x [2 x <8 x i32>]], align 8
  %tile_c = alloca [4 x [4 x <8 x float>]], align 8
  %grp_ext = zext i32 %grp_r1 to i64
  %grp_ext2 = zext i32 %grp_r0 to i64
  %lid_y_ext = zext i16 %localIdY to i64
  %lid_x_ext = zext i16 %localIdX to i64
  call void @llvm.lifetime.start.p0(i64 512, ptr nonnull %tile_c)
  br label %init_outer

init_outer:
  %io = phi i32 [ 0, %entry ], [ %io.next, %init_outer_latch ]
  %io.ext = zext i32 %io to i64
  br label %init_inner

init_inner:
  %ii = phi i32 [ 0, %init_outer ], [ %ii.next, %init_inner ]
  %ii.ext = zext i32 %ii to i64
  %init.gep = getelementptr inbounds [4 x [4 x <8 x float>]], ptr %tile_c, i64 0, i64 %io.ext, i64 %ii.ext
  store <8 x float> zeroinitializer, ptr %init.gep, align 8
  %ii.next = add nuw nsw i32 %ii, 1
  %ii.cmp = icmp ult i32 %ii, 3
  br i1 %ii.cmp, label %init_inner, label %init_outer_latch

init_outer_latch:
  %io.next = add nuw nsw i32 %io, 1
  %io.cmp = icmp ult i32 %io, 3
  br i1 %io.cmp, label %init_outer, label %precompute

precompute:
  %row_off = shl nuw nsw i64 %lid_y_ext, 16
  %col_off = shl nuw nsw i64 %grp_ext, 19
  %row_off2 = shl nuw nsw i64 %grp_ext2, 8
  %col_off2 = shl nuw nsw i64 %lid_x_ext, 6
  %sa_base = getelementptr i16, ptr addrspace(1) %src_a, i64 %col_off
  %sa_off = getelementptr i16, ptr addrspace(1) %sa_base, i64 %row_off
  %sb_base = getelementptr i16, ptr addrspace(1) %src_b, i64 %row_off2
  %sb_off = getelementptr i16, ptr addrspace(1) %sb_base, i64 %col_off2
  br label %k_loop

k_loop:
  %k = phi i32 [ 0, %precompute ], [ %k.next, %k_latch ]
  %k.cmp = icmp ult i32 %k, 64
  br i1 %k.cmp, label %k_body, label %k_exit

k_body:
  call void @llvm.lifetime.start.p0(i64 128, ptr nonnull %tile_a)
  call void @llvm.lifetime.start.p0(i64 256, ptr nonnull %tile_b)
  %k.shl = shl nuw nsw i32 %k, 1
  br label %n_loop

n_loop:
  %n = phi i32 [ 0, %k_body ], [ %n.next, %n_latch ]
  %n.ext = zext i32 %n to i64
  %kn = or i32 %k.shl, %n
  %kn.shl = shl nuw nsw i32 %kn, 4
  %kn.ext = zext i32 %kn.shl to i64
  %a_ptr = getelementptr i16, ptr addrspace(1) %sa_off, i64 %kn.ext
  br label %load_a_loop

load_a_loop:
  %la = phi i32 [ 0, %n_loop ], [ %la.next, %load_a_loop ]
  %la.ext = zext i32 %la to i64
  %la.off = shl nuw nsw i64 %la.ext, 14
  %la.gep = getelementptr i16, ptr addrspace(1) %a_ptr, i64 %la.off
  %la.cast = addrspacecast ptr addrspace(1) %la.gep to ptr addrspace(4)
  %la.int = ptrtoint ptr addrspace(4) %la.cast to i64
  %la.base = and i64 %la.int, -64
  %la.tr = trunc i64 %la.int to i32
  %la.sh = lshr i32 %la.tr, 1
  %la.offx = and i32 %la.sh, 31
  %la.data = call <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64 %la.base, i32 4095, i32 7, i32 4095, i32 %la.offx, i32 0, i32 16, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0)
  %la.dst = getelementptr inbounds [4 x [2 x <8 x i16>]], ptr %tile_a, i64 0, i64 %la.ext, i64 %n.ext
  store <8 x i16> %la.data, ptr %la.dst, align 8
  %la.next = add nuw nsw i32 %la, 1
  %la.cmp = icmp ult i32 %la, 3
  br i1 %la.cmp, label %load_a_loop, label %load_a_done

load_a_done:
  %kn.shl2 = shl nuw nsw i32 %kn, 15
  %kn.ext2 = zext i32 %kn.shl2 to i64
  %b_ptr = getelementptr i16, ptr addrspace(1) %sb_off, i64 %kn.ext2
  br label %load_b_loop

load_b_loop:
  %lb = phi i32 [ 0, %load_a_done ], [ %lb.next, %load_b_loop ]
  %lb.ext = zext i32 %lb to i64
  %lb.off = shl nuw nsw i64 %lb.ext, 4
  %lb.gep = getelementptr i16, ptr addrspace(1) %b_ptr, i64 %lb.off
  %lb.cast = addrspacecast ptr addrspace(1) %lb.gep to ptr addrspace(4)
  %lb.int = ptrtoint ptr addrspace(4) %lb.cast to i64
  %lb.base = and i64 %lb.int, -64
  %lb.tr = trunc i64 %lb.int to i32
  %lb.sh = lshr i32 %lb.tr, 1
  %lb.offx = and i32 %lb.sh, 31
  %lb.data = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %lb.base, i32 4095, i32 15, i32 4095, i32 %lb.offx, i32 0, i32 16, i32 16, i32 16, i32 1, i1 false, i1 true, i32 0)
  %lb.dst = getelementptr inbounds [4 x [2 x <8 x i32>]], ptr %tile_b, i64 0, i64 %lb.ext, i64 %n.ext
  store <8 x i32> %lb.data, ptr %lb.dst, align 8
  %lb.next = add nuw nsw i32 %lb, 1
  %lb.cmp = icmp ult i32 %lb, 3
  br i1 %lb.cmp, label %load_b_loop, label %load_b_done

load_b_done:
  br label %dpas_outer

dpas_outer:
  %do = phi i32 [ 0, %load_b_done ], [ %do.next, %dpas_outer_latch ]
  %do.ext = zext i32 %do to i64
  %do.a.gep = getelementptr inbounds [4 x [2 x <8 x i16>]], ptr %tile_a, i64 0, i64 %do.ext, i64 %n.ext
  %do.a.val = load <8 x i16>, ptr %do.a.gep, align 8
  br label %dpas_inner

dpas_inner:
  %di = phi i32 [ 0, %dpas_outer ], [ %di.next, %dpas_inner ]
  %di.ext = zext i32 %di to i64
  %di.b.gep = getelementptr inbounds [4 x [2 x <8 x i32>]], ptr %tile_b, i64 0, i64 %di.ext, i64 %n.ext
  %di.b.val = load <8 x i32>, ptr %di.b.gep, align 8
  %di.c.gep = getelementptr inbounds [4 x [4 x <8 x float>]], ptr %tile_c, i64 0, i64 %do.ext, i64 %di.ext
  %di.c.val = load <8 x float>, ptr %di.c.gep, align 8
  %dpas.res = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %di.c.val, <8 x i16> %do.a.val, <8 x i32> %di.b.val, i32 11, i32 11, i32 8, i32 8, i1 false)
  store <8 x float> %dpas.res, ptr %di.c.gep, align 8
  %di.next = add nuw nsw i32 %di, 1
  %di.cmp = icmp ult i32 %di, 3
  br i1 %di.cmp, label %dpas_inner, label %dpas_outer_latch

dpas_outer_latch:
  %do.next = add nuw nsw i32 %do, 1
  %do.cmp = icmp ult i32 %do, 3
  br i1 %do.cmp, label %dpas_outer, label %n_latch

n_latch:
  %n.next = add nuw nsw i32 %n, 1
  %n.cmp = icmp eq i32 %n, 0
  br i1 %n.cmp, label %n_loop, label %k_latch

k_latch:
  call void @llvm.lifetime.end.p0(i64 256, ptr nonnull %tile_b)
  call void @llvm.lifetime.end.p0(i64 128, ptr nonnull %tile_a)
  %k.next = add nuw nsw i32 %k, 1
  br label %k_loop

k_exit:
  %dc.base0 = getelementptr inbounds float, ptr addrspace(1) %dst_c, i64 %col_off
  %dc.base1 = getelementptr inbounds float, ptr addrspace(1) %dc.base0, i64 %row_off
  %dc.base2 = getelementptr inbounds float, ptr addrspace(1) %dc.base1, i64 %row_off2
  %dc.base3 = getelementptr inbounds float, ptr addrspace(1) %dc.base2, i64 %col_off2
  br label %store_outer

store_outer:
  %so = phi i32 [ 0, %k_exit ], [ %so.next, %store_outer_latch ]
  %so.ext = zext i32 %so to i64
  %so.off = shl nuw nsw i64 %so.ext, 14
  %so.ptr = getelementptr inbounds float, ptr addrspace(1) %dc.base3, i64 %so.off
  br label %store_inner

store_inner:
  %si = phi i32 [ 0, %store_outer ], [ %si.next, %store_merge ]
  %si.ext = zext i32 %si to i64
  %si.off = shl nuw nsw i64 %si.ext, 4
  %si.ptr = getelementptr inbounds float, ptr addrspace(1) %so.ptr, i64 %si.off
  %si.c.gep = getelementptr inbounds [4 x [4 x <8 x float>]], ptr %tile_c, i64 0, i64 %so.ext, i64 %si.ext
  %si.c.val = load <8 x i32>, ptr %si.c.gep, align 8
  %si.cast = addrspacecast ptr addrspace(1) %si.ptr to ptr addrspace(4)
  %wg.prod = mul i32 %lsz_y, %lsz_x
  %wg.prod2 = mul i32 %wg.prod, %lsz_z
  %wg.mod16 = and i32 %wg.prod2, 15
  %wg.is_aligned = icmp eq i32 %wg.mod16, 0
  br i1 %wg.is_aligned, label %store_direct, label %store_checked

store_direct:
  br label %store_merge

store_checked:
  %lid_z32 = zext i16 %localIdZ to i32
  %tmp1 = mul i32 %lsz_y, %lid_z32
  %lid_y32 = zext i16 %localIdY to i32
  %tmp2 = add i32 %tmp1, %lid_y32
  %tmp3 = mul i32 %tmp2, %lsz_x
  %lid_x32 = zext i16 %localIdX to i32
  %tmp4 = add i32 %tmp3, %lid_x32
  %lane = lshr i32 %tmp4, 4
  %wg.size = mul i32 %lsz_y, %lsz_x
  %wg.size2 = mul i32 %wg.size, %lsz_z
  %wg.rnd = add i32 %wg.size2, 15
  %wg.div = lshr i32 %wg.rnd, 4
  %wg.last = add nsw i32 %wg.div, -1
  %lane.ok = icmp ult i32 %lane, %wg.last
  br i1 %lane.ok, label %store_direct, label %store_merge

store_merge:
  %sm.cast.int = ptrtoint ptr addrspace(4) %si.cast to i64
  %sm.base = and i64 %sm.cast.int, -64
  %sm.tr = trunc i64 %sm.cast.int to i32
  %sm.sh = lshr i32 %sm.tr, 2
  %sm.offx = and i32 %sm.sh, 15
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64 %sm.base, i32 8191, i32 7, i32 8191, i32 %sm.offx, i32 0, i32 32, i32 16, i32 8, i32 1, i1 false, i1 false, i32 0, <8 x i32> %si.c.val)
  %si.next = add nuw nsw i32 %si, 1
  %si.cmp = icmp ult i32 %si, 3
  br i1 %si.cmp, label %store_inner, label %store_outer_latch

store_outer_latch:
  %so.next = add nuw nsw i32 %so, 1
  %so.cmp = icmp ult i32 %so, 3
  br i1 %so.cmp, label %store_outer, label %done

done:
  call void @llvm.lifetime.end.p0(i64 512, ptr nonnull %tile_c)
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)
declare <8 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v8i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <8 x i32>)

declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture)
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture)
declare void @llvm.assume(i1 noundef)
