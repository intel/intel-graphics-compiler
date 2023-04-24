;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2021 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: opt %use_old_pass_manager% -GenXLowering -march=genx64 -mcpu=Gen9 -mtriple=spir64-unknown-unknown -S < %s | FileCheck %s

target triple = "spir64-unknown-unknown"

declare i32 @llvm.genx.lzd.i32(i32) #0

declare i64 @llvm.genx.lzd.i64(i64) #0

declare <2 x i64> @llvm.genx.lzd.v2i64(<2 x i64>) #0

; CHECK: @lzd32
; CHECK-NEXT: %res = call i32 @llvm.genx.lzd.i32(i32 %op)
; CHECK-NEXT: ret void
define internal spir_func void @lzd32(i32 %op) #0 {
  %res = call i32 @llvm.genx.lzd.i32(i32 %op)
  ret void
}

; CHECK: @lzd64
; CHECK-NEXT: [[IV1:%[^ ]+]] = bitcast i64 %op to <[[CT:2 x i32]]>
; CHECK-NEXT: [[LO_ARG:%[^ ]+]] = call <[[ET:1 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 1, i32 2, i16 0,]]
; CHECK-NEXT: [[HI_ARG:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 1, i32 2, i16 4,]]
; CHECK-NEXT: [[LZD64_LO:%[^ ]+]] = call <[[ET]]> @llvm.genx.lzd.v1i32(<[[ET]]> [[LO_ARG]])
; CHECK-NEXT: [[LZD64_HI:%[^ ]+]] = call <[[ET]]> @llvm.genx.lzd.v1i32(<[[ET]]> [[HI_ARG]])
; CHECK-NEXT: [[FLAG:%[^ ]+]] = icmp eq <[[ET]]> [[HI_ARG]], zeroinitializer
; CHECK-NEXT: [[LORES:%[^ ]+]] = add <[[ET]]> [[LZD64_LO]], <i32 32>
; CHECK-NEXT: [[RES:%[^ ]+]] = select <1 x i1> [[FLAG]], <[[ET]]> [[LORES]], <[[ET]]> [[LZD64_HI]]
; CHECK-NEXT: [[RES64:%[^ ]+]] = zext <1 x i32> [[RES]] to <1 x i64>
; CHECK-NEXT: [[RES_CAST:%[^ ]+]] = bitcast <1 x i64> [[RES64]] to i64
; CHECK-NEXT: ret void
define internal spir_func void @lzd64(i64 %op) #0 {
  %res = call i64 @llvm.genx.lzd.i64(i64 %op)
  ret void
}

; CHECK: @lzd64v2
; CHECK-NEXT: [[IV1:%[^ ]+]] = bitcast <2 x i64> %op to <[[CT:4 x i32]]>
; CHECK-NEXT: [[LO_ARG:%[^ ]+]] = call <[[ET:2 x i32]]> [[rgn:@llvm.genx.rdregioni.[^(]+]](<[[CT]]> [[IV1]], [[low_reg:i32 0, i32 2, i32 2, i16 0,]]
; CHECK-NEXT: [[HI_ARG:%[^ ]+]] = call <[[ET]]> [[rgn]](<[[CT]]> [[IV1]], [[high_reg:i32 0, i32 2, i32 2, i16 4,]]
; CHECK-NEXT: [[LZD64_LO:%[^ ]+]] = call <[[ET]]> @llvm.genx.lzd.v2i32(<[[ET]]> [[LO_ARG]])
; CHECK-NEXT: [[LZD64_HI:%[^ ]+]] = call <[[ET]]> @llvm.genx.lzd.v2i32(<[[ET]]> [[HI_ARG]])
; CHECK-NEXT: [[FLAG:%[^ ]+]] = icmp eq <[[ET]]> [[HI_ARG]], zeroinitializer
; CHECK-NEXT: [[LORES:%[^ ]+]] = add <[[ET]]> [[LZD64_LO]], <i32 32, i32 32>
; CHECK-NEXT: [[RES:%[^ ]+]] = select <2 x i1> [[FLAG]], <[[ET]]> [[LORES]], <[[ET]]> [[LZD64_HI]]
; CHECK-NEXT: [[RES64:%[^ ]+]] = zext <2 x i32> [[RES]] to <2 x i64>
; CHECK-NEXT: ret void
define internal spir_func void @lzd64v2(<2 x i64> %op) #0 {
  %res = call <2 x i64> @llvm.genx.lzd.v2i64(<2 x i64> %op)
  ret void
}
