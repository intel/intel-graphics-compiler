;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --typed-pointers -enable-debugify --igc-vectorprocess -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; VectorProcess : predicated load/store intrinsics
; ------------------------------------------------

; CHECK-COUNT-1: WARNING
; CHECK-SAME: Missing line 2
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_vectorpro_1(<2 x i16> addrspace(1)* %src, i1 %p) {
; CHECK-LABEL: @test_vectorpro_1(
; CHECK:    [[VPTRCAST:%.*]] = bitcast <2 x i16> addrspace(1)* %src to i32 addrspace(1)*
; CHECK:    [[TMP1:%.*]] = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)* [[VPTRCAST]], i64 2, i1 true, i32 bitcast (<2 x i16> <i16 1, i16 2> to i32))
; CHECK:    [[VPTRCAST1:%.*]] = bitcast <2 x i16> addrspace(1)* %src to i32 addrspace(1)*
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1i32.i32(i32 addrspace(1)* [[VPTRCAST1]], i32 [[TMP1]], i64 2, i1 true)
; CHECK:    [[TMP2:%.*]] = bitcast <2 x i16> addrspace(1)* %src to i32 addrspace(1)*
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1i32.i32(i32 addrspace(1)* [[TMP2]], i32 [[TMP1]], i64 4, i1 false)
; CHECK:    [[TMP3:%.*]] = bitcast i32 [[TMP1]] to float
; CHECK:    [[TMP4:%.*]] = bitcast <2 x i16> addrspace(1)* %src to float addrspace(1)*
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p1f32.f32(float addrspace(1)* [[TMP4]], float [[TMP3]], i64 4, i1 %p)
; CHECK:    ret void
;
  %1 = call <2 x i16> @llvm.genx.GenISA.PredicatedLoad.v2i16.p1v2i16.v2i16(<2 x i16> addrspace(1)* %src, i64 2, i1 true, <2 x i16> <i16 1, i16 2>)
  %2 = bitcast <2 x i16> %1 to i32
  call void @llvm.genx.GenISA.PredicatedStore.p1v2i16.v2i16(<2 x i16> addrspace(1)* %src, <2 x i16> %1, i64 2, i1 true)
  %3 = bitcast <2 x i16> addrspace(1)* %src to i32 addrspace(1)*
  call void @llvm.genx.GenISA.PredicatedStore.p1i32.i32(i32 addrspace(1)* %3, i32 %2, i64 4, i1 false)
  %4 = bitcast <2 x i16> %1 to float
  %5 = bitcast <2 x i16> addrspace(1)* %src to float addrspace(1)*
  call void @llvm.genx.GenISA.PredicatedStore.p1f32.f32(float addrspace(1)* %5, float %4, i64 4, i1 %p)
  ret void
}

declare <2 x i16> @llvm.genx.GenISA.PredicatedLoad.v2i16.p1v2i16.v2i16(<2 x i16> addrspace(1)*, i64, i1, <2 x i16>)
declare void @llvm.genx.GenISA.PredicatedStore.p1v2i16.v2i16(<2 x i16> addrspace(1)*, <2 x i16>, i64, i1)
declare void @llvm.genx.GenISA.PredicatedStore.p1i32.i32(i32 addrspace(1)*, i32, i64, i1)
declare void @llvm.genx.GenISA.PredicatedStore.p1f32.f32(float addrspace(1)*, float, i64, i1)

define void @test_vectorpro_2(<2 x i16>** %src1, <2 x i16>* %mrg1, <2 x i16*>* %src2, <2 x i16*> %mrg2, i1 %p) {
; CHECK-LABEL: @test_vectorpro_2(
; CHECK:    [[VPTRCAST:%.*]] = bitcast <2 x i16>** %src1 to <2 x i32>*
; CHECK:    [[TMP1:%.*]] = ptrtoint <2 x i16>* %mrg1 to i64
; CHECK:    [[TMP2:%.*]] = bitcast i64 [[TMP1]] to <2 x i32>
; CHECK:    [[TMP3:%.*]] = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p0v2i32.v2i32(<2 x i32>* [[VPTRCAST]], i64 4, i1 false, <2 x i32> [[TMP2]])
; CHECK:    [[TMP4:%.*]] = bitcast <2 x i32> [[TMP3]] to i64
; CHECK:    [[TMP5:%.*]] = inttoptr i64 [[TMP4]] to <2 x i16>*
; CHECK:    [[VPTRCAST1:%.*]] = bitcast <2 x i16*>* %src2 to <4 x i32>*
; CHECK:    [[TMP6:%.*]] = extractelement <2 x i16*> %mrg2, i64 0
; CHECK:    [[TMP7:%.*]] = ptrtoint i16* [[TMP6]] to i64
; CHECK:    [[TMP8:%.*]] = insertelement <2 x i64> undef, i64 [[TMP7]], i64 0
; CHECK:    [[TMP9:%.*]] = extractelement <2 x i16*> %mrg2, i64 1
; CHECK:    [[TMP10:%.*]] = ptrtoint i16* [[TMP9]] to i64
; CHECK:    [[TMP11:%.*]] = insertelement <2 x i64> [[TMP8]], i64 [[TMP10]], i64 1
; CHECK:    [[TMP12:%.*]] = bitcast <2 x i64> [[TMP11]] to <4 x i32>
; CHECK:    [[TMP13:%.*]] = call <4 x i32> @llvm.genx.GenISA.PredicatedLoad.v4i32.p0v4i32.v4i32(<4 x i32>* [[VPTRCAST1]], i64 1, i1 %p, <4 x i32> [[TMP12]])
; CHECK:    [[TMP14:%.*]] = bitcast <4 x i32> [[TMP13]] to <2 x i64>
; CHECK:    [[TMP15:%.*]] = extractelement <2 x i64> [[TMP14]], i64 0
; CHECK:    [[TMP16:%.*]] = inttoptr i64 [[TMP15]] to i16*
; CHECK:    [[TMP17:%.*]] = insertelement <2 x i16*> undef, i16* [[TMP16]], i64 0
; CHECK:    [[TMP18:%.*]] = extractelement <2 x i64> [[TMP14]], i64 1
; CHECK:    [[TMP19:%.*]] = inttoptr i64 [[TMP18]] to i16*
; CHECK:    [[TMP20:%.*]] = insertelement <2 x i16*> [[TMP17]], i16* [[TMP19]], i64 1
; CHECK:    [[INTTOPTR2:%.*]] = inttoptr i64 [[TMP4]] to i32*
; CHECK:    [[TMP21:%.*]] = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p0i32.i32(i32* [[INTTOPTR2]], i64 2, i1 true, i32 bitcast (<2 x i16> <i16 3, i16 4> to i32))
; CHECK:    [[TMP22:%.*]] = bitcast i32 [[TMP21]] to <2 x i16>
; CHECK:    [[TMP23:%.*]] = extractelement <2 x i16> [[TMP22]], i32 1
; CHECK:    [[TMP24:%.*]] = extractelement <2 x i16*> [[TMP20]], i32 0
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p0i16.i16(i16* [[TMP24]], i16 [[TMP23]], i64 2, i1 true)
; CHECK:    ret void
;
  %1 = call <2 x i16>* @llvm.genx.GenISA.PredicatedLoad.p0v2i16.p0p0v2i16.p0v2i16(<2 x i16>** %src1, i64 4, i1 false, <2 x i16>* %mrg1)
  %2 = call <2 x i16*> @llvm.genx.GenISA.PredicatedLoad.v2p0i16.p0v2p0i16.v2p0i16(<2 x i16*>* %src2, i64 1, i1 %p, <2 x i16*> %mrg2)
  %3 = call <2 x i16> @llvm.genx.GenISA.PredicatedLoad.v2i16.p0v2i16.v2i16(<2 x i16>* %1, i64 2, i1 true, <2 x i16> <i16 3, i16 4>)
  %4 = extractelement <2 x i16> %3, i32 1
  %5 = extractelement <2 x i16*> %2, i32 0
  call void @llvm.genx.GenISA.PredicatedStore.p0i16.i16(i16* %5, i16 %4, i64 2, i1 true)
  ret void
}

declare <2 x i16>* @llvm.genx.GenISA.PredicatedLoad.p0v2i16.p0p0v2i16.p0v2i16(<2 x i16>**, i64, i1, <2 x i16>*)
declare <2 x i16*> @llvm.genx.GenISA.PredicatedLoad.v2p0i16.p0v2p0i16.v2p0i16(<2 x i16*>*, i64, i1, <2 x i16*>)
declare <2 x i16> @llvm.genx.GenISA.PredicatedLoad.v2i16.p0v2i16.v2i16(<2 x i16>*, i64, i1, <2 x i16>)
declare void @llvm.genx.GenISA.PredicatedStore.p0i16.i16(i16*, i16, i64, i1)

define void @test_vectorpro_3(<2 x i16>* %src1, <2 x i8>* %src2, i32 %src4, <2 x i16*> %mrg1) {
; CHECK-LABEL: @test_vectorpro_3(
; CHECK:    [[TMP1:%.*]] = alloca <2 x i16>*, align 4
; CHECK:    [[VPTRCAST:%.*]] = bitcast <2 x i16>** [[TMP1]] to <2 x i32>*
; CHECK:    [[TMP2:%.*]] = ptrtoint <2 x i16>* %src1 to i64
; CHECK:    [[TMP3:%.*]] = bitcast i64 [[TMP2]] to <2 x i32>
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p0v2i32.v2i32(<2 x i32>* [[VPTRCAST]], <2 x i32> [[TMP3]], i64 2, i1 true)
; CHECK:    [[TMP4:%.*]] = alloca <2 x i16*>, align 4
; CHECK:    [[TMP5:%.*]] = inttoptr i32 %src4 to <2 x i16*> addrspace(1)*
; CHECK:    [[INTTOPTR2:%.*]] = inttoptr i32 %src4 to <4 x i32> addrspace(1)*
; CHECK:    [[TMP6:%.*]] = extractelement <2 x i16*> %mrg1, i64 0
; CHECK:    [[TMP7:%.*]] = ptrtoint i16* [[TMP6]] to i64
; CHECK:    [[TMP8:%.*]] = insertelement <2 x i64> undef, i64 [[TMP7]], i64 0
; CHECK:    [[TMP9:%.*]] = extractelement <2 x i16*> %mrg1, i64 1
; CHECK:    [[TMP10:%.*]] = ptrtoint i16* [[TMP9]] to i64
; CHECK:    [[TMP11:%.*]] = insertelement <2 x i64> [[TMP8]], i64 [[TMP10]], i64 1
; CHECK:    [[TMP12:%.*]] = bitcast <2 x i64> [[TMP11]] to <4 x i32>
; CHECK:    [[TMP13:%.*]] = call <4 x i32> @llvm.genx.GenISA.PredicatedLoad.v4i32.p1v4i32.v4i32(<4 x i32> addrspace(1)* [[INTTOPTR2]], i64 1, i1 true, <4 x i32> [[TMP12]])
; CHECK:    [[TMP14:%.*]] = bitcast <4 x i32> [[TMP13]] to <2 x i64>
; CHECK:    [[TMP15:%.*]] = extractelement <2 x i64> [[TMP14]], i64 0
; CHECK:    [[TMP16:%.*]] = inttoptr i64 [[TMP15]] to i16*
; CHECK:    [[TMP17:%.*]] = insertelement <2 x i16*> undef, i16* [[TMP16]], i64 0
; CHECK:    [[TMP18:%.*]] = extractelement <2 x i64> [[TMP14]], i64 1
; CHECK:    [[TMP19:%.*]] = inttoptr i64 [[TMP18]] to i16*
; CHECK:    [[TMP20:%.*]] = insertelement <2 x i16*> [[TMP17]], i16* [[TMP19]], i64 1
; CHECK:    [[VPTRCAST1:%.*]] = bitcast <2 x i16*>* [[TMP4]] to <4 x i32>*
; CHECK:    [[TMP21:%.*]] = extractelement <2 x i16*> [[TMP20]], i64 0
; CHECK:    [[TMP22:%.*]] = ptrtoint i16* [[TMP21]] to i64
; CHECK:    [[TMP23:%.*]] = insertelement <2 x i64> undef, i64 [[TMP22]], i64 0
; CHECK:    [[TMP24:%.*]] = extractelement <2 x i16*> [[TMP20]], i64 1
; CHECK:    [[TMP25:%.*]] = ptrtoint i16* [[TMP24]] to i64
; CHECK:    [[TMP26:%.*]] = insertelement <2 x i64> [[TMP23]], i64 [[TMP25]], i64 1
; CHECK:    [[TMP27:%.*]] = bitcast <2 x i64> [[TMP26]] to <4 x i32>
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p0v4i32.v4i32(<4 x i32>* [[VPTRCAST1]], <4 x i32> [[TMP27]], i64 1, i1 true)
; CHECK:    [[TMP28:%.*]] = alloca i16 addrspace(1)*, align 4
; CHECK:    [[TMP29:%.*]] = inttoptr i64 15 to i16 addrspace(1)*
; CHECK:    [[VPTRCAST2:%.*]] = bitcast i16 addrspace(1)** [[TMP28]] to <2 x i32>*
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p0v2i32.v2i32(<2 x i32>* [[VPTRCAST2]], <2 x i32> bitcast (<1 x i64> <i64 15> to <2 x i32>), i64 4, i1 true)
; CHECK:    [[VPTRCAST3:%.*]] = bitcast <2 x i8>* %src2 to i16*
; CHECK:    call void @llvm.genx.GenISA.PredicatedStore.p0i16.i16(i16* [[VPTRCAST3]], i16 bitcast (<2 x i8> <i8 1, i8 8> to i16), i64 1, i1 true)
; CHECK:    ret void
;
  %1 = alloca <2 x i16>*, align 4
  call void @llvm.genx.GenISA.PredicatedStore.p0p0v2i16.p0v2i16(<2 x i16>** %1, <2 x i16>* %src1, i64 2, i1 true)
  %2 = alloca <2 x i16*>, align 4
  %3 = inttoptr i32 %src4 to <2 x i16*> addrspace(1)*
  %4 = call <2 x i16*> @llvm.genx.GenISA.PredicatedLoad.v2p0i16.p1v2p0i16.v2p0i16(<2 x i16*> addrspace(1)* %3, i64 1, i1 true, <2 x i16*> %mrg1)
  call void @llvm.genx.GenISA.PredicatedStore.p0v2p0i16.v2p0i16(<2 x i16*>* %2, <2 x i16*> %4, i64 1, i1 true)
  %5 = alloca i16 addrspace(1)*, align 4
  %6 = inttoptr i64 15 to i16 addrspace(1)*
  call void @llvm.genx.GenISA.PredicatedStore.p0p1i16.p1i16(i16 addrspace(1)** %5, i16 addrspace(1)* %6, i64 4, i1 true)
  call void @llvm.genx.GenISA.PredicatedStore.p0v2i8.v2i8(<2 x i8>* %src2, <2 x i8> <i8 1, i8 8>, i64 1, i1 true)
  ret void
}

declare void @llvm.genx.GenISA.PredicatedStore.p0p0v2i16.p0v2i16(<2 x i16>**, <2 x i16>*, i64, i1)
declare <2 x i16*> @llvm.genx.GenISA.PredicatedLoad.v2p0i16.p1v2p0i16.v2p0i16(<2 x i16*> addrspace(1)*, i64, i1, <2 x i16*>)
declare void @llvm.genx.GenISA.PredicatedStore.p0v2p0i16.v2p0i16(<2 x i16*>*, <2 x i16*>, i64, i1)
declare void @llvm.genx.GenISA.PredicatedStore.p0p1i16.p1i16(i16 addrspace(1)**, i16 addrspace(1)*, i64, i1)
declare void @llvm.genx.GenISA.PredicatedStore.p0v2i8.v2i8(<2 x i8>*, <2 x i8>, i64, i1)

; checks different cases of merge values
define void @test_vectorpro_merge_values(<8 x i16> addrspace(1)* %src0, <2 x i16> addrspace(1)* %src1, <2 x i16>** %src2, <2 x i16*>* %src3) {
; CHECK-LABEL: @test_vectorpro_merge_values(

; CHECK:    [[VPTRCAST:%.*]] = bitcast <8 x i16> addrspace(1)* %src0 to <4 x i32> addrspace(1)*
; CHECK:    [[TMP1:%.*]] = call <4 x i32> @llvm.genx.GenISA.PredicatedLoad.v4i32.p1v4i32.v4i32(<4 x i32> addrspace(1)* [[VPTRCAST]], i64 16, i1 true, <4 x i32> zeroinitializer)
; CHECK:    [[TMP2:%.*]] = bitcast <4 x i32> [[TMP1]] to <8 x i16>
  %1 = call <8 x i16> @llvm.genx.GenISA.PredicatedLoad.v8i16.p1v8i16.v8i16(<8 x i16> addrspace(1)* %src0, i64 16, i1 true, <8 x i16> zeroinitializer)

; CHECK:    [[VPTRCAST1:%.*]] = bitcast <2 x i16> addrspace(1)* %src1 to i32 addrspace(1)*
; CHECK:    [[TMP3:%.*]] = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)* [[VPTRCAST1]], i64 4, i1 true, i32 0)
; CHECK:    [[TMP4:%.*]] = bitcast i32 [[TMP3]] to <2 x i16>
  %2 = call <2 x i16> @llvm.genx.GenISA.PredicatedLoad.v2i16.p1v2i16.v2i16(<2 x i16> addrspace(1)* %src1, i64 4, i1 true, <2 x i16> zeroinitializer)

; CHECK:    [[VPTRCAST2:%.*]] = bitcast <8 x i16> addrspace(1)* %src0 to <4 x i32> addrspace(1)*
; CHECK:    [[TMP5:%.*]] = call <4 x i32> @llvm.genx.GenISA.PredicatedLoad.v4i32.p1v4i32.v4i32(<4 x i32> addrspace(1)* [[VPTRCAST2]], i64 16, i1 true, <4 x i32> undef)
; CHECK:    [[TMP6:%.*]] = bitcast <4 x i32> [[TMP5]] to <8 x i16>
  %3 = call <8 x i16> @llvm.genx.GenISA.PredicatedLoad.v8i16.p1v8i16.v8i16(<8 x i16> addrspace(1)* %src0, i64 16, i1 true, <8 x i16> undef)

; CHECK:    [[VPTRCAST3:%.*]] = bitcast <8 x i16> addrspace(1)* %src0 to <4 x i32> addrspace(1)*
; CHECK:    [[TMP7:%.*]] = call <4 x i32> @llvm.genx.GenISA.PredicatedLoad.v4i32.p1v4i32.v4i32(<4 x i32> addrspace(1)* [[VPTRCAST3]], i64 16, i1 true, <4 x i32> poison)
; CHECK:    [[TMP8:%.*]] = bitcast <4 x i32> [[TMP7]] to <8 x i16>
  %4 = call <8 x i16> @llvm.genx.GenISA.PredicatedLoad.v8i16.p1v8i16.v8i16(<8 x i16> addrspace(1)* %src0, i64 16, i1 true, <8 x i16> poison)

; CHECK:    [[VPTRCAST4:%.*]] = bitcast <2 x i16>** %src2 to <2 x i32>*
; CHECK:    [[TMP9:%.*]] = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p0v2i32.v2i32(<2 x i32>* [[VPTRCAST4]], i64 4, i1 true, <2 x i32> zeroinitializer)
; CHECK:    [[TMP10:%.*]] = bitcast <2 x i32> [[TMP9]] to i64
; CHECK:    [[TMP11:%.*]] = inttoptr i64 [[TMP10]] to <2 x i16>*
  %5 = call <2 x i16>* @llvm.genx.GenISA.PredicatedLoad.p0v2i16.p0p0v2i16.p0v2i16(<2 x i16>** %src2, i64 4, i1 true, <2 x i16>* null)

; CHECK:    [[VPTRCAST5:%.*]] = bitcast <2 x i16*>* %src3 to <4 x i32>*
; CHECK:    [[TMP12:%.*]] = call <4 x i32> @llvm.genx.GenISA.PredicatedLoad.v4i32.p0v4i32.v4i32(<4 x i32>* [[VPTRCAST5]], i64 1, i1 true, <4 x i32> zeroinitializer)
; CHECK:    [[TMP13:%.*]] = bitcast <4 x i32> [[TMP12]] to <2 x i64>
; CHECK:    [[TMP14:%.*]] = extractelement <2 x i64> [[TMP13]], i64 0
; CHECK:    [[TMP15:%.*]] = inttoptr i64 [[TMP14]] to i16*
; CHECK:    [[TMP16:%.*]] = insertelement <2 x i16*> undef, i16* [[TMP15]], i64 0
; CHECK:    [[TMP17:%.*]] = extractelement <2 x i64> [[TMP13]], i64 1
; CHECK:    [[TMP18:%.*]] = inttoptr i64 [[TMP17]] to i16*
; CHECK:    [[TMP19:%.*]] = insertelement <2 x i16*> [[TMP16]], i16* [[TMP18]], i64 1
  %6 = call <2 x i16*> @llvm.genx.GenISA.PredicatedLoad.v2p0i16.p0v2p0i16.v2p0i16(<2 x i16*>* %src3, i64 1, i1 true, <2 x i16*> zeroinitializer)

  ret void
}

declare <8 x i16> @llvm.genx.GenISA.PredicatedLoad.v8i16.p1v8i16.v8i16(<8 x i16> addrspace(1)*, i64, i1, <8 x i16>)
