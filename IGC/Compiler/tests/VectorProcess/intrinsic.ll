;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify --igc-vectorprocess -S < %s 2>&1 | FileCheck %s
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

define void @test_pointers(i32 %bindlessOffset){
; CHECK-LABEL: @test_pointers(
; CHECK: [[PTRCAST0:%.*]] = inttoptr i32 %bindlessOffset to <2 x i32> addrspace(2490368)*
; CHECK: [[LDVAL:%.*]] = call <2 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v2i32.p2490368v2i32(<2 x i32> addrspace(2490368)* [[PTRCAST0]], i32 160, i32 8, i1 false)
; CHECK: [[TMP0:%.*]] = bitcast <2 x i32> [[LDVAL]] to i64
; CHECK: [[TMP1:%.*]] = inttoptr i64 [[TMP0]] to i32 addrspace(1)*
; CHECK: [[PTRCAST1:%.*]] = inttoptr i32 %bindlessOffset to <2 x i32> addrspace(2490368)*
; CHECK: [[TMP2:%.*]] = ptrtoint i32 addrspace(1)* [[TMP1]] to i64
; CHECK: [[TMP3:%.*]] = bitcast i64 %5 to <2 x i32>
; CHECK: call void @llvm.genx.GenISA.storerawvector.indexed.p2490368v2i32.v2i32(<2 x i32> addrspace(2490368)* [[PTRCAST1]], i32 4, <2 x i32> [[TMP3]], i32 1, i1 false)
;
  %1 = inttoptr i32 %bindlessOffset to i32 addrspace(1)* addrspace(2490368)*
  %2 = call i32 addrspace(1)* @llvm.genx.GenISA.ldraw.indexed.p1i32.p2490368p1i32(i32 addrspace(1)* addrspace(2490368)* %1, i32 160, i32 8, i1 false)
  call void @llvm.genx.GenISA.storeraw.indexed.p2490368p1i32.p1i32(i32 addrspace(1)* addrspace(2490368)* %1, i32 4, i32 addrspace(1)* %2, i32 1, i1 false)
  ret void
}

declare i32 addrspace(1)* @llvm.genx.GenISA.ldraw.indexed.p1i32.p2490368p1i32(i32 addrspace(1)* addrspace(2490368)*, i32, i32, i1)
declare void @llvm.genx.GenISA.storeraw.indexed.p2490368p1i32.p1i32(i32 addrspace(1)* addrspace(2490368)*, i32, i32 addrspace(1)*, i32, i1)

define void @test_vector_of_pointers(i32 %bindlessOffset) {
; CHECK-LABEL: @test_vector_of_pointers(
; CHECK: [[PTRCAST0:%.*]] = inttoptr i32 %bindlessOffset to <8 x i32> addrspace(2490368)*
; CHECK: [[LDVAL:%.*]] = call <8 x i32> @llvm.genx.GenISA.ldrawvector.indexed.v8i32.p2490368v8i32(<8 x i32> addrspace(2490368)* [[PTRCAST0]], i32 160, i32 8, i1 false)
; CHECK: [[TMP0:%.*]] = bitcast <8 x i32> [[LDVAL]] to <4 x i64>
; CHECK: [[TMP1:%.*]] = inttoptr <4 x i64> [[TMP0]] to <4 x i32 addrspace(1)*>
; CHECK: [[PTRCAST1:%.*]] = inttoptr i32 %bindlessOffset to <8 x i32> addrspace(2490368)*
; CHECK: [[TMP2:%.*]] = ptrtoint <4 x i32 addrspace(1)*> [[TMP1]] to <4 x i64>
; CHECK: [[TMP4:%.*]] = bitcast <4 x i64> [[TMP2]] to <8 x i32>
; CHECK: call void @llvm.genx.GenISA.storerawvector.indexed.p2490368v8i32.v8i32(<8 x i32> addrspace(2490368)* [[PTRCAST1]], i32 4, <8 x i32> [[TMP4]], i32 1, i1 false)
;
  %1 = inttoptr i32 %bindlessOffset to <4 x i32 addrspace(1)*> addrspace(2490368)*
  %2 = call <4 x i32 addrspace(1)*> @llvm.genx.GenISA.ldrawvector.indexed.v4p1i32.p2490368v4p1i32(<4 x i32 addrspace(1)*> addrspace(2490368)* %1, i32 160, i32 8, i1 false)
  call void @llvm.genx.GenISA.storerawvector.indexed.p2490368v4p1i32.v4p1i32(<4 x i32 addrspace(1)*> addrspace(2490368)* %1, i32 4, <4 x i32 addrspace(1)*> %2, i32 1, i1 false)
  ret void
}

declare <4 x i32 addrspace(1)*> @llvm.genx.GenISA.ldrawvector.indexed.v4p1i32.p2490368v4p1i32(<4 x i32 addrspace(1)*> addrspace(2490368)*, i32, i32, i1)
declare void @llvm.genx.GenISA.storerawvector.indexed.p2490368v4p1i32.v4p1i32(<4 x i32 addrspace(1)*> addrspace(2490368)*, i32, <4 x i32 addrspace(1)*>, i32, i1)
