;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, shader-types
; RUN: igc_opt -platformbmg -igc-emit-visa %s -inputcs -simd-mode 16 -regkey DumpVISAASMToConsole=1 | FileCheck %s

; Boolean struct members cannot alias the struct storage, as booleans live
; in flag registers. They are stored as a 0/1 byte instead.

target datalayout = "e-p:32:32:32-p1:64:64:64-p2:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a0:0:32-n8:16:32-S32"

@ThreadGroupSize_X = constant i32 16
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

define void @entry(<8 x i32> %r0, i32 %a, i32 %b, i32* %result) {
  ; CHECK-LABEL: .kernel

  ; verify the boolean struct member is a byte alias and not a bool alias
  ; CHECK:       .decl [[BYTES:StructV_[0-9]+v]] v_type=G type=ub num_elts=128 align=wordx32 alias=<StructV, 0>
  ; CHECK-NOT:   type=bool {{.*}}alias=<StructV

  ; CHECK:       cmp.lt (M1, 16) [[FLAG:P[0-9]+]]
  %lid16 = call i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32 17)
  %lid = zext i16 %lid16 to i32
  %flag = icmp ult i32 %lid, %b

  ; verify inserting a flag stores 0/1 bytes
  ; CHECK:       ([[FLAG]]) sel (M1, 16) [[BYTES]](1,0)<1> 0x1:ub 0x0:ub
  %partial = insertvalue { i32, i1 } poison, i32 %lid, 0
  %pair = insertvalue { i32, i1 } %partial, i1 %flag, 1

  ; verify the constant true member is initialized as a byte holding 1
  ; CHECK:       setp (M1_NM, 16) [[TRUEFLAG:P[0-9]+]] 0xffffffff:ud
  ; CHECK:       ([[TRUEFLAG]]) sel (M1, 16) [[CONSTBYTES:StructV_[0-9]+v]](1,0)<1> 0x1:ub 0x0:ub
  %const = insertvalue { i32, i1 } { i32 7, i1 true }, i32 %lid, 0

  ; verify struct select copies the boolean bytes
  ; CHECK:       ([[FLAG]]) mov (M1, 16) [[SELBYTES:StructV_[0-9]+v]](1,0)<1> [[BYTES]](1,0)<1;1,0>
  ; CHECK:       (![[FLAG]]) mov (M1, 16) [[SELBYTES]](1,0)<1> [[CONSTBYTES]](1,0)<1;1,0>
  %sel = select i1 %flag, { i32, i1 } %pair, { i32, i1 } %const

  ; verify extracting the boolean compares the byte against 0
  ; CHECK:       cmp.ne (M1, 16) [[OVERFLOW:P[0-9]+]] [[SELBYTES]](1,0)<1;1,0> 0x0:ub
  ; CHECK:       ([[OVERFLOW]]) sel
  %value = extractvalue { i32, i1 } %sel, 0
  %overflow = extractvalue { i32, i1 } %sel, 1
  %res = select i1 %overflow, i32 %value, i32 0
  store i32 %res, i32* %result, align 4
  ret void
}

declare i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32)

!igc.functions = !{!1}
!1 = !{void (<8 x i32>, i32, i32, i32*)* @entry, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
