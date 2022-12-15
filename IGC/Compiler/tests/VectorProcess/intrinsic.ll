;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -enable-debugify --igc-vectorprocess -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; VectorProcess : intrinsics
; ------------------------------------------------

; CHECK-COUNT-1: WARNING
; CHECK-SAME: Missing line 2
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_vectorpro(<2 x i16> addrspace(1)* %src) {
; CHECK-LABEL: @test_vectorpro(
; CHECK:    [[VPTRCAST:%.*]] = bitcast <2 x i16> addrspace(1)* %src to i32 addrspace(1)*
; CHECK:    [[TMP1:%.*]] = call i32 @llvm.genx.GenISA.ldrawvector.indexed.i32.p1i32(i32 addrspace(1)* [[VPTRCAST]], i32 4, i32 2, i1 true)
; CHECK:    [[VPTRCAST1:%.*]] = bitcast <2 x i16> addrspace(1)* %src to i32 addrspace(1)*
; CHECK:    call void @llvm.genx.GenISA.storerawvector.indexed.p1i32.i32(i32 addrspace(1)* [[VPTRCAST1]], i32 0, i32 [[TMP1]], i32 2, i1 true)
; CHECK:    [[TMP2:%.*]] = bitcast <2 x i16> addrspace(1)* %src to i32 addrspace(1)*
; CHECK:    store i32 [[TMP1]], i32 addrspace(1)* [[TMP2]], align 4
; CHECK:    [[TMP3:%.*]] = bitcast i32 [[TMP1]] to float
; CHECK:    [[TMP4:%.*]] = bitcast <2 x i16> addrspace(1)* %src to float addrspace(1)*
; CHECK:    store float [[TMP3]], float addrspace(1)* [[TMP4]], align 4
; CHECK:    ret void
;
  %1 = call <2 x i16> @llvm.genx.GenISA.ldrawvector.indexed.p1v2i16(<2 x i16> addrspace(1)* %src, i32 4, i32 2, i1 true)
  %2 = bitcast <2 x i16> %1 to i32
  call void @llvm.genx.GenISA.storerawvector.indexed.p1v2i16(<2 x i16> addrspace(1)* %src, i32 0, <2 x i16> %1, i32 2, i1 true)
  %3 = bitcast <2 x i16> addrspace(1)* %src to i32 addrspace(1)*
  store i32 %2, i32 addrspace(1)* %3, align 4
  %4 = bitcast <2 x i16> %1 to float
  %5 = bitcast <2 x i16> addrspace(1)* %src to float addrspace(1)*
  store float %4, float addrspace(1)* %5, align 4
  ret void
}

declare <2 x i16> @llvm.genx.GenISA.ldrawvector.indexed.p1v2i16(<2 x i16> addrspace(1)*, i32, i32, i1)
declare void @llvm.genx.GenISA.storerawvector.indexed.p1v2i16(<2 x i16> addrspace(1)*, i32, <2 x i16>, i32, i1)
