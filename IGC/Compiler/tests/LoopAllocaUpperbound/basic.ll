;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-loop-alloca-upperbound -S %s | FileCheck %s
; --------------------------------------------------------------------------
; IGC Loop Alloca Upperbound : testing Loop Alloca Upperbound transformation
; --------------------------------------------------------------------------
; This pass pattern match 1-BB loops with non-constant loop count
; and memory access to alloca that is constant size array.
;
; Then if all alloca accesses are in bound we could create a new
; loop count with constant upper bound of alloca array size.
;
; Loop is split into head/ifcond/continue blocks, where the ifcond block is entered
; by initial loop condition.
;

%struct.Color = type { float, float, float }

define spir_kernel void @test_kernel(i32 %arg2, i8 %ib8, %struct.Color addrspace(1)* align 4 %arg3, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, float addrspace(1)* %out) #0 {
; CHECK:       loop_init:
; CHECK-NEXT:    [[I36_COND_PHI:%.*]] = phi i1 [ true, [[LOOP_INIT_LR_PH:%.*]] ], [ [[I36:%.*]], [[LOOP_INIT_CONT:%.*]] ]
; CHECK-NEXT:    [[I36_NEWIND_PHI:%.*]] = phi i32 [ 0, [[LOOP_INIT_LR_PH]] ], [ [[DOTNEWIND_NEXT3:%.*]], [[LOOP_INIT_CONT]] ]
; CHECK-NEXT:    [[I359:%.*]] = phi i32 [ 0, [[LOOP_INIT_LR_PH]] ], [ [[I45:%.*]], [[LOOP_INIT_CONT]] ]
; CHECK-NEXT:    br i1 [[I36_COND_PHI]], label [[LOOP_INIT_IF_COND:%.*]], label [[LOOP_INIT_CONT]]
; CHECK:       loop_init.if.cond:
; CHECK:         br label [[LOOP_INIT_CONT]]
; CHECK:       loop_init.cont:
; CHECK-NEXT:    [[I45]] = add nuw nsw i32 [[I359]], 1
; CHECK-NEXT:    [[I36]] = icmp slt i32 [[I45]], [[ARG2:%.*]]
; CHECK-NEXT:    [[DOTNEWIND_NEXT3]] = add i32 [[I36_NEWIND_PHI]], 1
; CHECK-NEXT:    [[DOTNEWCMP4:%.*]] = icmp slt i32 [[DOTNEWIND_NEXT3]], 8
; CHECK-NEXT:    br i1 [[DOTNEWCMP4]], label [[LOOP_INIT:%.*]], label [[BB34_BB46_PREHEADER_CRIT_EDGE:%.*]]
;
; CHECK:       loop_load:
; CHECK-NEXT:    [[I53_COND_PHI:%.*]] = phi i1 [ true, [[LOOP_LOAD_LR_PH:%.*]] ], [ [[I53:%.*]], [[LOOP_LOAD_CONT:%.*]] ]
; CHECK-NEXT:    [[I53_NEWIND_PHI:%.*]] = phi i32 [ 0, [[LOOP_LOAD_LR_PH]] ], [ [[DOTNEWIND_NEXT1:%.*]], [[LOOP_LOAD_CONT]] ]
; CHECK-NEXT:    [[I525:%.*]] = phi i32 [ 0, [[LOOP_LOAD_LR_PH]] ], [ [[I78:%.*]], [[LOOP_LOAD_CONT]] ]
; CHECK-NEXT:    br i1 [[I53_COND_PHI]], label [[LOOP_LOAD_IF_COND:%.*]], label [[LOOP_LOAD_CONT]]
; CHECK:       loop_load.if.cond:
; CHECK:         br label [[LOOP_LOAD_CONT]]
; CHECK:       loop_load.cont:
; CHECK-NEXT:    [[I78]] = add nuw nsw i32 [[I525]], 1
; CHECK-NEXT:    [[I53]] = icmp slt i32 [[I78]], [[ARG2]]
; CHECK-NEXT:    [[DOTNEWIND_NEXT1]] = add i32 [[I53_NEWIND_PHI]], 1
; CHECK-NEXT:    [[DOTNEWCMP2:%.*]] = icmp slt i32 [[DOTNEWIND_NEXT1]], 8
; CHECK-NEXT:    br i1 [[DOTNEWCMP2]], label [[LOOP_LOAD:%.*]], label [[BB51_BB46_LOOPEXIT_CRIT_EDGE:%.*]]
;
; CHECK:       loop_calc:
; CHECK-NEXT:    [[I84_COND_PHI:%.*]] = phi i1 [ true, [[LOOP_CALC_LR_PH:%.*]] ], [ [[I84:%.*]], [[LOOP_CALC_CONT:%.*]] ]
; CHECK-NEXT:    [[I84_NEWIND_PHI:%.*]] = phi i32 [ 0, [[LOOP_CALC_LR_PH]] ], [ [[DOTNEWIND_NEXT:%.*]], [[LOOP_CALC_CONT]] ]
; CHECK-NEXT:    [[I833:%.*]] = phi i32 [ 0, [[LOOP_CALC_LR_PH]] ], [ [[I107:%.*]], [[LOOP_CALC_CONT]] ]
; CHECK-NEXT:    [[I802:%.*]] = phi float [ 0.000000e+00, [[LOOP_CALC_LR_PH]] ], [ [[I802_CONT_PHI:%.*]], [[LOOP_CALC_CONT]] ]
; CHECK-NEXT:    br i1 [[I84_COND_PHI]], label [[LOOP_CALC_IF_COND:%.*]], label [[LOOP_CALC_CONT]]
; CHECK:       loop_calc.if.cond:
; CHECK:         br label [[LOOP_CALC_CONT]]
; CHECK:       loop_calc.cont:
; CHECK-NEXT:    [[I802_CONT_PHI]] = phi float [ [[I802]], [[LOOP_CALC:%.*]] ], [ [[I104:%.*]], [[LOOP_CALC_IF_COND]] ]
; CHECK-NEXT:    [[I107]] = add nuw nsw i32 [[I833]], 1
; CHECK-NEXT:    [[I84]] = icmp slt i32 [[I107]], [[ARG2]]
; CHECK-NEXT:    [[DOTNEWIND_NEXT]] = add i32 [[I84_NEWIND_PHI]], 1
; CHECK-NEXT:    [[DOTNEWCMP:%.*]] = icmp slt i32 [[DOTNEWIND_NEXT]], 8
; CHECK-NEXT:    br i1 [[DOTNEWCMP]], label [[LOOP_CALC]], label [[BB79_BB108_CRIT_EDGE:%.*]]
;
bb:
  %i = alloca [8 x %struct.Color], align 4
  %0 = and i8 %ib8, 1
  %.not = icmp eq i8 %0, 0
  %i368 = icmp sgt i32 %arg2, 0
  br i1 %i368, label %loop_init.lr.ph, label %bb46.preheader

loop_init.lr.ph:                                  ; preds = %bb
  br label %loop_init

bb34.bb46.preheader_crit_edge:                    ; preds = %loop_init
  br label %bb46.preheader

bb46.preheader:                                   ; preds = %bb34.bb46.preheader_crit_edge, %bb
  br i1 %.not, label %bb79.preheader, label %bb51.preheader.lr.ph

bb51.preheader.lr.ph:                             ; preds = %bb46.preheader
  %i534 = icmp sgt i32 %arg2, 0
  br label %bb51.preheader

loop_init:                                        ; preds = %loop_init.lr.ph, %loop_init
  %i359 = phi i32 [ 0, %loop_init.lr.ph ], [ %i45, %loop_init ]
  %i38 = zext i32 %i359 to i64
  %i40 = getelementptr inbounds [8 x %struct.Color], [8 x %struct.Color]* %i, i64 0, i64 %i38, i32 0
  store float 0.000000e+00, float* %i40, align 4
  %i31 = getelementptr inbounds float, float* %i40, i64 1
  store float 0.000000e+00, float* %i31, align 4
  %i32 = getelementptr inbounds float, float* %i40, i64 2
  store float 0.000000e+00, float* %i32, align 4
  %i45 = add nuw nsw i32 %i359, 1
  %i36 = icmp slt i32 %i45, %arg2
  br i1 %i36, label %loop_init, label %bb34.bb46.preheader_crit_edge

bb51.bb46.loopexit_crit_edge:                     ; preds = %loop_load
  br label %bb46.loopexit

bb46.loopexit:                                    ; preds = %bb51.bb46.loopexit_crit_edge, %bb51.preheader
  br i1 %.not, label %bb46.bb79.preheader_crit_edge, label %bb51.preheader

bb51.preheader:                                   ; preds = %bb51.preheader.lr.ph, %bb46.loopexit
  br i1 %i534, label %loop_load.lr.ph, label %bb46.loopexit

loop_load.lr.ph:                                  ; preds = %bb51.preheader
  br label %loop_load

bb46.bb79.preheader_crit_edge:                    ; preds = %bb46.loopexit
  br label %bb79.preheader

bb79.preheader:                                   ; preds = %bb46.bb79.preheader_crit_edge, %bb46.preheader
  %i841 = icmp sgt i32 %arg2, 0
  br i1 %i841, label %loop_calc.lr.ph, label %bb108

loop_calc.lr.ph:                                  ; preds = %bb79.preheader
  br label %loop_calc

loop_load:                                        ; preds = %loop_load.lr.ph, %loop_load
  %i525 = phi i32 [ 0, %loop_load.lr.ph ], [ %i78, %loop_load ]
  %i59 = zext i32 %i525 to i64
  %i46 = getelementptr inbounds %struct.Color, %struct.Color addrspace(1)* %arg3, i64 %i59, i32 0
  %i47 = load float, float addrspace(1)* %i46, align 4
  %i48 = getelementptr inbounds [8 x %struct.Color], [8 x %struct.Color]* %i, i64 0, i64 %i59, i32 2
  %i49 = load float, float* %i48, align 4
  %i50 = fadd nsz arcp float %i49, %i47
  store float %i50, float* %i48, align 4
  %i78 = add nuw nsw i32 %i525, 1
  %i53 = icmp slt i32 %i78, %arg2
  br i1 %i53, label %loop_load, label %bb51.bb46.loopexit_crit_edge

loop_calc:                                        ; preds = %loop_calc.lr.ph, %loop_calc
  %i833 = phi i32 [ 0, %loop_calc.lr.ph ], [ %i107, %loop_calc ]
  %i802 = phi float [ 0.000000e+00, %loop_calc.lr.ph ], [ %i104, %loop_calc ]
  %i863 = zext i32 %i833 to i64
  %i88 = getelementptr inbounds [8 x %struct.Color], [8 x %struct.Color]* %i, i64 0, i64 %i863, i32 0
  %i89 = load float, float* %i88, align 4
  %i104 = fadd nsz arcp float %i802, %i89
  %i107 = add nuw nsw i32 %i833, 1
  %i84 = icmp slt i32 %i107, %arg2
  br i1 %i84, label %loop_calc, label %bb79.bb108_crit_edge

bb79.bb108_crit_edge:                             ; preds = %loop_calc
  br label %bb108

bb108:                                            ; preds = %bb79.bb108_crit_edge, %bb79.preheader
  %i80.lcssa = phi float [ %i104, %bb79.bb108_crit_edge ], [ 0.000000e+00, %bb79.preheader ]
  store float %i80.lcssa, float addrspace(1)* %out, align 4
  ret void
}

attributes #0 = { convergent nounwind }

!igc.functions = !{!3}

!3 = !{void (i32, i8, %struct.Color addrspace(1)*, <8 x i32>, <8 x i32>, i8*, float addrspace(1)*)* @test_kernel, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8, !9}
!7 = !{i32 0}
!8 = !{i32 1}
!9 = !{i32 13}
