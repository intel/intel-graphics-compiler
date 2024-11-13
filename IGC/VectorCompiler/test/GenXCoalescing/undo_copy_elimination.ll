;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt %use_old_pass_manager% -GenXModule -GenXUnbalingWrapper -GenXNumberingWrapper \
; RUN: -GenXLiveRangesWrapper -GenXCoalescingWrapper -march=genx64 \
; RUN: -mcpu=XeHPC -mtriple=spir64-unknown-unknown -S -disable-output < %s

; Check that undoing copy elimination doesn't cause a crash caused by LR deletion

define spir_kernel void @test() #0 {
 entry:
   %add.i = add nuw i64 0, 0
   %cmp6.not.i.i.not.not.iv32cast = bitcast i64 %add.i to <2 x i32>
   br label %for.body.i.lr.ph

 for.body.i.lr.ph:                                 ; preds = %entry
   br label %for.body.i

 for.body.i:                                       ; preds = %for.body.i.for.body.i_crit_edge, %for.body.i.lr.ph
   %Gen.sroa.0.0.i8 = phi i64 [ %add.i, %for.body.i.lr.ph ], [ %int_emu.select.recast, %for.body.i.for.body.i_crit_edge ]
   %bitcast = bitcast i64 %Gen.sroa.0.0.i8 to <1 x i64>
   %rdregioni = call <16 x i64> @llvm.genx.rdregioni.v16i64.v1i64.i16(<1 x i64> %bitcast, i32 0, i32 0, i32 0, i16 0, i32 0)
   %int_emu.select.partial_join = call <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32> %cmp6.not.i.i.not.not.iv32cast, <1 x i32> zeroinitializer, i32 0, i32 0,   i32 0, i16 0, i32 0, i1 false)
   %int_emu.select.recast = bitcast <2 x i32> %int_emu.select.partial_join to i64
   br i1 false, label %for.body.i.for.body.i_crit_edge, label %for.body.i.exit_crit_edge

 for.body.i.exit_crit_edge:                        ; preds = %for.body.i
   ret void

 for.body.i.for.body.i_crit_edge:                  ; preds = %for.body.i
   br label %for.body.i
 }

 declare <2 x i32> @llvm.genx.wrregioni.v2i32.v1i32.i16.i1(<2 x i32>, <1 x i32>, i32, i32, i32, i16, i32, i1)

 declare <16 x i64> @llvm.genx.rdregioni.v16i64.v1i64.i16(<1 x i64>, i32, i32, i32, i16, i32)

 attributes #0 = { "CMGenxMain" }
