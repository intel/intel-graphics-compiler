;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers --regkey DisableCodeScheduling=0 \
; RUN:          --regkey EnableCodeSchedulingIfNoSpills=1 --regkey CodeSchedulingGreedyRPHigherRPCommit=1 \
; RUN:         --regkey PrintToConsole=1 --regkey DumpCodeScheduling=1 --igc-code-scheduling \
; RUN:         --regkey CodeSchedulingRPThreshold=-512 --regkey CodeSchedulingForceRPOnly=1 \
; RUN:         -S %s 2>&1 | FileCheck %s

; Check that the loads and store follow the remated instructions immediately
; So the remat instructions from the second load are not scheduled before the 1st load

; And the remat instructions from the store are not scheduled before any load
; So the dependency is respected, though the first instruction of store remat
; will be estimated as decreasing the register pressure

; The last pack of the remated instructions should only be scheduled after the barrier

define spir_kernel void @test_remat(ptr addrspace(1) %A, i32 %x) {
; CHECK-LABEL: @test_remat(
; CHECK:       entry:
; CHECK:         br label [[BB1:%.*]]
; CHECK:       bb1:
; ...
; CHECK:         [[REMAT1_5:%.*]] = shl nuw nsw i32 [[REMAT1_4:.*]], 1
; CHECK:         [[CLONED_1:%.*]] = inttoptr i32 [[REMAT1_5]] to ptr addrspace(3)
; CHECK:         [[LOAD_1:%.*]] = load <8 x i16>, ptr addrspace(3) [[CLONED_1]], align 2
; ...
; CHECK:         [[REMAT2_4:%.*]] = shl nuw nsw i32 [[REMAT2_3:.*]], 1
; CHECK:         [[CLONED_2:%.*]] = inttoptr i32 [[REMAT2_4]] to ptr addrspace(3)
; CHECK:         [[LOAD_2:%.*]] = load <8 x i16>, ptr addrspace(3) [[CLONED_2]], align 2

; CHECK:         [[REMAT3_1:%.*]] = or i32 [[X:.*]], 10, !remat !0
; CHECK:         [[REMAT3_2:%.*]] = shl nuw nsw i32 [[REMAT3_1]], 6, !remat !0
; CHECK:         [[REMAT3_3:%.*]] = or i32 [[REMAT3_2]], 16, !remat !0
; CHECK:         [[REMAT3_4:%.*]] = shl nuw nsw i32 [[REMAT3_3]], 1, !remat !0
; CHECK:         [[CLONED_3:%.*]] = inttoptr i32 [[REMAT3_4]] to ptr addrspace(3)
; CHECK:         store <8 x i16> zeroinitializer, ptr addrspace(3) [[CLONED_3]], align 16

; CHECK:         call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i32 0)
; CHECK:         call void @llvm.genx.GenISA.threadgroupbarrier()

; Checking that DPAS goes right after the barrier helps us to ensure the scheduling applied
; CHECK:         [[DPAS2:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef, i32 1, i32 1, i32 1, i32 1, i1 false)

; CHECK:         [[REMAT4_1:%.*]] = or i32 [[X]], 10, !remat !0
; CHECK:         [[REMAT4_2:%.*]] = shl nuw nsw i32 [[REMAT4_1]], 6, !remat !0
; CHECK:         [[REMAT4_3:%.*]] = or i32 [[REMAT4_2]], 16, !remat !0
; CHECK:         [[REMAT4_4:%.*]] = shl nuw nsw i32 [[REMAT4_3]], 1, !remat !0
; CHECK:         [[CLONED4:%.*]] = inttoptr i32 [[REMAT4_4]] to ptr addrspace(3)
; CHECK:         [[LOAD_4:%.*]] = load <1 x i16>, ptr addrspace(3) [[CLONED4]], align 2

; CHECK:         ret void
;


entry:
  br label %bb1

bb1:

  %remat1_1 = or i32 %x, 10, !remat !0
  %remat1_2 = shl nuw nsw i32 %remat1_1, 6, !remat !0
  %remat1_3 = or i32 %remat1_2, 16, !remat !0
  %remat1_4 = or i32 %remat1_3, %remat1_2, !remat !0
  %remat1_5 = shl nuw nsw i32 %remat1_4, 1, !remat !0
  %cloned_1 = inttoptr i32 %remat1_5 to <1 x i16> addrspace(3)*, !remat !0
  %load_1 = load <8 x i16>, <8 x i16> addrspace(3)* %cloned_1, align 2
  %remat2_1 = or i32 %x, 18
  %remat2_2 = shl nuw nsw i32 %remat2_1, 6
  %remat2_3 = or i32 %remat2_2, 16
  %remat2_4 = shl nuw nsw i32 %remat2_3, 1
  %cloned_2 = inttoptr i32 %remat2_4 to <1 x i16> addrspace(3)*
  %load_2 = load <8 x i16>, <8 x i16> addrspace(3)* %cloned_2, align 2

  %remat3_1 = or i32 %x, 10, !remat !0
  %remat3_2 = shl nuw nsw i32 %remat3_1, 6, !remat !0
  %remat3_3 = or i32 %remat3_2, 16, !remat !0
  %remat3_4 = shl nuw nsw i32 %remat3_3, 1, !remat !0
  %cloned_3 = inttoptr i32 %remat3_4 to <1 x i16> addrspace(3)*  ; intentionally no remat metadata
  store <8 x i16> zeroinitializer, <8 x i16> addrspace(3)* %cloned_3

  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i32 0)
  call void @llvm.genx.GenISA.threadgroupbarrier()

  %remat4_1 = or i32 %x, 10, !remat !0
  %remat4_2 = shl nuw nsw i32 %remat4_1, 6, !remat !0
  %remat4_3 = or i32 %remat4_2, 16, !remat !0
  %remat4_4 = shl nuw nsw i32 %remat4_3, 1, !remat !0
  %cloned4 = inttoptr i32 %remat4_4 to <1 x i16> addrspace(3)*
  %load_4 = load <1 x i16>, <1 x i16> addrspace(3)* %cloned4, align 2

  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
  <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef,
  i32 1, i32 1, i32 1, i32 1, i1 false)
  ret void
}


define spir_kernel void @test_remat_select(ptr addrspace(1) %A, i32 %x, i32 %z) {
; CHECK-LABEL: @test_remat_select(
; CHECK:       bb1:
; CHECK:         [[DPAS2:%.*]] = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef, i32 1, i32 1, i32 1, i32 1, i1 false)
; CHECK:         [[REMAT_OR:%.*]] = or i32 [[X:%.*]], 10, !remat !0
; CHECK:         [[REMAT_ICMP:%.*]] = icmp eq i32 [[REMAT_OR]], 15, !remat !0
; CHECK:         [[SEL:%.*]] = select i1 [[REMAT_ICMP]], i32 [[X]], i32 [[Z:%.*]]
; CHECK:         [[ADD:%.*]] = add i32 [[REMAT_OR]], 2000
; CHECK:         ret void
;
entry:
  br label %bb1

bb1:
  %remat_or = or i32 %x, 10, !remat !0
  %remat_icmp = icmp eq i32 %remat_or, 15, !remat !0
  %sel = select i1 %remat_icmp, i32 %x, i32 %z
  %add = add i32 %remat_or, 2000


  %dpas2 = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
  <8 x float> zeroinitializer, <8 x i16> undef, <8 x i32> undef,
  i32 1, i32 1, i32 1, i32 1, i1 false)
  ret void
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(
  <8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

declare void @llvm.genx.GenISA.memoryfence(i1, i1, i1, i1, i1, i1, i1, i1, i32)
declare void @llvm.genx.GenISA.threadgroupbarrier()

!0 = !{!"remat"}
