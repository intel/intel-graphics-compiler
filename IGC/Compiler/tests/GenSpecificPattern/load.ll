;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-gen-specific-pattern -S < %s | FileCheck %s
; ------------------------------------------------
; GenSpecificPattern: Load pattern transformation
; ------------------------------------------------

define void @test_load(<2 x i32> addrspace(1)* %src) {
; CHECK-LABEL: @test_load(
; CHECK:    [[TMP1:%.*]] = bitcast <2 x i32> addrspace(1)* [[SRC:%.*]] to <2 x i32> addrspace(1)* addrspace(1)*
; CHECK:    [[TMP2:%.*]] = load <2 x i32> addrspace(1)*, <2 x i32> addrspace(1)* addrspace(1)* [[TMP1]], align 64
; CHECK:    [[TMP7:%.*]] = load <2 x i32>, <2 x i32> addrspace(1)* [[TMP2]], align 64
; CHECK:    call void @use.2i32(<2 x i32> [[TMP7]])
; CHECK-NEXT:    ret void
;
  %1 = load <2 x i32>, <2 x i32> addrspace(1)* %src, align 64
  %2 = extractelement <2 x i32> %1, i32 0
  %3 = extractelement <2 x i32> %1, i32 1
  %4 = call i64 addrspace(1)* addrspace(1)* @llvm.genx.GenISA.pair.to.ptr.p1p1i64(i32 %2, i32 %3)
  %5 = bitcast i64 addrspace(1)* addrspace(1)* %4 to i64 addrspace(1)*
  %6 = bitcast i64 addrspace(1)* %5 to <2 x i32> addrspace(1)*
  %7 = load <2 x i32>, <2 x i32> addrspace(1)* %6, align 64
  call void @use.2i32(<2 x i32> %7)
  ret void
}

declare i64 addrspace(1)* addrspace(1)* @llvm.genx.GenISA.pair.to.ptr.p1p1i64(i32, i32)
declare void @use.2i32(<2 x i32>)
