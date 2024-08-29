;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-gen-specific-pattern -S %s -o %t.ll
; RUN: FileCheck %s --input-file=%t.ll

define spir_kernel void @testkernel(i32 addrspace(1)* %arg_x) {
entry:
  %ptrtoint = ptrtoint i32 addrspace(1)* %arg_x to i64
  %inttoptr = inttoptr i64 %ptrtoint to <4 x i32> addrspace(1)*
  %load = load <4 x i32>, <4 x i32> addrspace(1)* %inttoptr, align 4
  %w = extractelement <4 x i32> %load, i32 0
  %x = extractelement <4 x i32> %load, i32 1
  %y = extractelement <4 x i32> %load, i32 2
  %z = extractelement <4 x i32> %load, i32 3
  %add = add i32 %w, 1
  %cmp = icmp eq i32 %w, -1
  %conv = zext i1 %cmp to i32
  %add.1 = add i32 %x, %conv
  %cmp.1 = icmp ult i32 %add.1, %x
  %conv.2 = zext i1 %cmp.1 to i32
  %add.2 = add i32 %y, %conv.2
  %cmp.2 = icmp ult i32 %add.2, %y
  %conv.3 = zext i1 %cmp.2 to i32
  %add.3 = add i32 %z, %conv.3
  ret void
}

; CHECK-LABEL: @testkernel
; CHECK: [[CALL:%[a-zA-Z0-9]+]] = call <2 x i32> @llvm.genx.GenISA.uaddc.v2i32.i32(i32 %w, i32 1)
; CHECK: [[ZEXT0:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[CALL]], i32 0
; CHECK: [[ZEXT1:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[CALL]], i32 1
; CHECK: [[CALL1:%[a-zA-Z0-9]+]] = call <2 x i32> @llvm.genx.GenISA.uaddc.v2i32.i32(i32 %x, i32 [[ZEXT1]])
; CHECK: [[ZEXT12:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[CALL1]], i32 0
; CHECK: [[ZEXT13:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[CALL1]], i32 1
; CHECK: [[CALL2:%[a-zA-Z0-9]+]] = call <2 x i32> @llvm.genx.GenISA.uaddc.v2i32.i32(i32 %y, i32 [[ZEXT13]])
; CHECK: [[ZEXT14:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[CALL2]], i32 0
; CHECK: [[ZEXT15:%[a-zA-Z0-9]+]] = extractelement <2 x i32> [[CALL2]], i32 1
