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
; VectorProcess : store
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_vectorpro(<2 x i16>* %src1, <2 x i8>* %src2, <3 x i32> addrspace(2)* %src3, i32 %src4) {
; CHECK-LABEL: @test_vectorpro(
; CHECK:    [[TMP1:%.*]] = alloca <2 x i16>*, align 4
; CHECK:    [[VPTRCAST:%.*]] = bitcast <2 x i16>** [[TMP1]] to <2 x i32>*
; CHECK:    [[TMP2:%.*]] = ptrtoint <2 x i16>* [[SRC1:%.*]] to i64
; CHECK:    [[TMP3:%.*]] = bitcast i64 [[TMP2]] to <2 x i32>
; CHECK:    store <2 x i32> [[TMP3]], <2 x i32>* [[VPTRCAST]], align 2
; CHECK:    [[TMP4:%.*]] = alloca <2 x i16*>, align 4
; CHECK:    [[TMP5:%.*]] = inttoptr i32 [[SRC4:%.*]] to <2 x i16*> addrspace(1)*
; CHECK:    [[INTTOPTR2:%.*]] = inttoptr i32 [[SRC4]] to <4 x i32> addrspace(1)*
; CHECK:    [[VCASTLOAD:%.*]] = load <4 x i32>, <4 x i32> addrspace(1)* [[INTTOPTR2]], align 1
; CHECK:    [[TMP6:%.*]] = bitcast <4 x i32> [[VCASTLOAD]] to <2 x i64>
; CHECK:    [[TMP7:%.*]] = extractelement <2 x i64> [[TMP6]], i64 0
; CHECK:    [[TMP8:%.*]] = inttoptr i64 [[TMP7]] to i16*
; CHECK:    [[TMP9:%.*]] = insertelement <2 x i16*> undef, i16* [[TMP8]], i64 0
; CHECK:    [[TMP10:%.*]] = extractelement <2 x i64> [[TMP6]], i64 1
; CHECK:    [[TMP11:%.*]] = inttoptr i64 [[TMP10]] to i16*
; CHECK:    [[TMP12:%.*]] = insertelement <2 x i16*> [[TMP9]], i16* [[TMP11]], i64 1
; CHECK:    [[VPTRCAST1:%.*]] = bitcast <2 x i16*>* [[TMP4]] to <4 x i32>*
; CHECK:    [[TMP13:%.*]] = extractelement <2 x i16*> [[TMP12]], i64 0
; CHECK:    [[TMP14:%.*]] = ptrtoint i16* [[TMP13]] to i64
; CHECK:    [[TMP15:%.*]] = insertelement <2 x i64> undef, i64 [[TMP14]], i64 0
; CHECK:    [[TMP16:%.*]] = extractelement <2 x i16*> [[TMP12]], i64 1
; CHECK:    [[TMP17:%.*]] = ptrtoint i16* [[TMP16]] to i64
; CHECK:    [[TMP18:%.*]] = insertelement <2 x i64> [[TMP15]], i64 [[TMP17]], i64 1
; CHECK:    [[TMP19:%.*]] = bitcast <2 x i64> [[TMP18]] to <4 x i32>
; CHECK:    store <4 x i32> [[TMP19]], <4 x i32>* [[VPTRCAST1]], align 1
; CHECK:    [[TMP20:%.*]] = alloca i16 addrspace(1)*, align 4
; CHECK:    [[TMP21:%.*]] = inttoptr i64 15 to i16 addrspace(1)*
; CHECK:    [[VPTRCAST2:%.*]] = bitcast i16 addrspace(1)** [[TMP20]] to <2 x i32>*
; CHECK:    store <2 x i32> bitcast (<1 x i64> <i64 15> to <2 x i32>), <2 x i32>* [[VPTRCAST2]], align 4
; CHECK:    [[VPTRCAST3:%.*]] = bitcast <2 x i8>* [[SRC2:%.*]] to i16*
; CHECK:    store i16 bitcast (<2 x i8> <i8 1, i8 8> to i16), i16* [[VPTRCAST3]], align 1
; CHECK:    ret void
;
  %1 = alloca <2 x i16>*, align 4
  store <2 x i16>* %src1, <2 x i16>** %1, align 2
  %2 = alloca <2 x i16*>, align 4
  %3 = inttoptr i32 %src4 to <2 x i16*> addrspace(1)*
  %4 = load <2 x i16*>, <2 x i16*> addrspace(1)* %3, align 1
  store <2 x i16*> %4, <2 x i16*>* %2, align 1
  %5 = alloca i16 addrspace(1)*, align 4
  %6 = inttoptr i64 15 to i16 addrspace(1)*
  store i16 addrspace(1)* %6, i16 addrspace(1)** %5, align 4
  store <2 x i8> <i8 1, i8 8>, <2 x i8>* %src2, align 1
  ret void
}
