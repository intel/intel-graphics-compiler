;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-spirv, regkeys, mtl-supported, llvm-16-plus

; RUN: llvm-as -opaque-pointers=1 %s -o %t.bc
; RUN: llvm-spirv -opaque-pointers=1 %t.bc -o %t.spv
; RUN: ocloc compile -spirv_input -file %t.spv -device mtl -options " -igc_opts 'EnableOpaquePointersBackend=1,PrintToConsole=1,PrintAfter=Splitstructurephis'" 2>&1 | FileCheck %s

; This test checks that 2 inner loops with are fully unrolled by 12 and SROA applied to both allocas

; CHECK-LABEL: spir_kernel void @quux(
; CHECK-NOT: = alloca

target triple = "spir64-unknown-unknown"

%struct.widget = type <{ %struct.baz, %struct.snork, i32, i32, i32, i32, [12 x i8] }>
%struct.baz = type { [2 x ptr addrspace(4)] }
%struct.snork = type { i32, [12 x %struct.spam], [12 x [1 x i32]] }
%struct.spam = type { i32, i32, i32 }
%struct.zot = type <{ %struct.baz, i32, %struct.snork, %struct.wombat, %struct.wombat, %struct.wombat, i8, i32, i32, i32, i32, [4 x i8] }>
%struct.wombat = type { i8 }
%struct.wombat.0 = type { i16 }

define spir_kernel void @quux(i1 %arg, ptr %arg1, i1 %arg2, ptr %arg3) {
bb:
  %tmp = alloca %struct.widget, align 16
  %tmp3 = alloca %struct.zot, align 16
  br i1 %arg, label %bb4, label %bb22

bb4:                                              ; preds = %bb4, %bb
  br i1 %arg2, label %bb5, label %bb4

bb5:                                              ; preds = %bb18, %bb4
  br i1 %arg, label %bb7, label %bb6

bb6:                                              ; preds = %bb33, %bb5
  ret void

bb7:                                              ; preds = %bb12, %bb5
  %tmp8 = phi i32 [ %tmp16, %bb12 ], [ 0, %bb5 ]
  %tmp9 = phi i32 [ %tmp17, %bb12 ], [ 0, %bb5 ]
  %tmp10 = icmp ugt i32 %tmp9, 11
  %tmp11 = or i1 %tmp10, %arg
  br i1 %tmp11, label %bb18, label %bb12

bb12:                                             ; preds = %bb7
  %tmp13 = zext i32 %tmp9 to i64
  %tmp14 = getelementptr %struct.zot, ptr %tmp3, i64 0, i32 2, i32 2, i64 %tmp13, i64 0
  %tmp15 = load i32, ptr %tmp14, align 4
  %tmp16 = or i32 %tmp8, %tmp15
  %tmp17 = add i32 %tmp9, 1
  br label %bb7

bb18:                                             ; preds = %bb7
  %tmp19 = zext i32 %tmp8 to i64
  %tmp20 = getelementptr %struct.wombat.0, ptr %arg3, i64 %tmp19, i32 0
  %tmp21 = load i16, ptr %tmp20, align 2
  store i16 %tmp21, ptr %arg1, align 2
  br label %bb5

bb22:                                             ; preds = %bb27, %bb
  %tmp23 = phi i32 [ %tmp31, %bb27 ], [ 0, %bb ]
  %tmp24 = phi i32 [ %tmp32, %bb27 ], [ 0, %bb ]
  %tmp25 = icmp ugt i32 %tmp24, 11
  %tmp26 = or i1 %tmp25, %arg
  br i1 %tmp26, label %bb33, label %bb27

bb27:                                             ; preds = %bb22
  %tmp28 = zext i32 %tmp24 to i64
  %tmp29 = getelementptr %struct.widget, ptr %tmp, i64 0, i32 1, i32 2, i64 %tmp28, i64 0
  %tmp30 = load i32, ptr %tmp29, align 4
  %tmp31 = or i32 %tmp23, %tmp30
  %tmp32 = add i32 %tmp24, 1
  br label %bb22

bb33:                                             ; preds = %bb22
  %tmp34 = zext i32 %tmp23 to i64
  store i64 %tmp34, ptr %arg1, align 8
  br label %bb6
}
