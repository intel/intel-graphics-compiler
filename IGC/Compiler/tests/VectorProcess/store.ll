;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -enable-debugify --igc-vectorprocess -S < %s 2>&1 | FileCheck --check-prefixes=CHECK,CHECK-OPAQUE %s %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -enable-debugify --igc-vectorprocess -S < %s 2>&1 | FileCheck --check-prefixes=CHECK,CHECK-TYPED %s %}
; ------------------------------------------------
; VectorProcess : store
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_vectorpro(<2 x i16>* %src1, <2 x i8>* %src2, <3 x i32> addrspace(2)* %src3, i32 %src4) {
; CHECK-LABEL: @test_vectorpro(
; CHECK-TYPED:    [[TMP1:%.*]] = alloca <2 x i16>*, align 4
; CHECK-OPAQUE:    [[TMP1:%.*]] = alloca ptr, align 4
; CHECK-TYPED:    [[VPTRCAST:%.*]] = bitcast <2 x i16>** [[TMP1]] to <2 x i32>*
; CHECK-TYPED:    [[TMP2:%.*]] = ptrtoint <2 x i16>* [[SRC1:%.*]] to i64
; CHECK-OPAQUE:    [[TMP2:%.*]] = ptrtoint ptr [[SRC1:%.*]] to i64
; CHECK:    [[TMP3:%.*]] = bitcast i64 [[TMP2]] to <2 x i32>
; CHECK-TYPED:    store <2 x i32> [[TMP3]], <2 x i32>* [[VPTRCAST]], align 2
; CHECK-OPAQUE:    store <2 x i32> [[TMP3]], ptr [[TMP1]], align 2
; CHECK-TYPED:    [[TMP4:%.*]] = alloca <2 x i16*>, align 4
; CHECK-OPAQUE:    [[TMP4:%.*]] = alloca <2 x ptr>, align 4
; CHECK-TYPED:    [[TMP5:%.*]] = inttoptr i32 [[SRC4:%.*]] to <2 x i16*> addrspace(1)*
; CHECK-OPAQUE:    [[TMP5:%.*]] = inttoptr i32 [[SRC4:%.*]] to ptr addrspace(1)
; CHECK-TYPED:    [[INTTOPTR2:%.*]] = inttoptr i32 [[SRC4]] to <4 x i32> addrspace(1)*
; CHECK-OPAQUE:    [[INTTOPTR2:%.*]] = inttoptr i32 [[SRC4]] to ptr addrspace(1)
; CHECK-TYPED:    [[VCASTLOAD:%.*]] = load <4 x i32>, <4 x i32> addrspace(1)* [[INTTOPTR2]], align 1
; CHECK-OPAQUE:    [[VCASTLOAD:%.*]] = load <4 x i32>, ptr addrspace(1) [[INTTOPTR2]], align 1
; CHECK:    [[TMP6:%.*]] = bitcast <4 x i32> [[VCASTLOAD]] to <2 x i64>
; CHECK:    [[TMP7:%.*]] = extractelement <2 x i64> [[TMP6]], i64 0
; CHECK-TYPED:    [[TMP8:%.*]] = inttoptr i64 [[TMP7]] to i16*
; CHECK-OPAQUE:    [[TMP8:%.*]] = inttoptr i64 [[TMP7]] to ptr,
; CHECK-TYPED:    [[TMP9:%.*]] = insertelement <2 x i16*> undef, i16* [[TMP8]], i64 0
; CHECK-OPAQUE:    [[TMP9:%.*]] = insertelement <2 x ptr> undef, ptr [[TMP8]], i64 0
; CHECK:    [[TMP10:%.*]] = extractelement <2 x i64> [[TMP6]], i64 1
; CHECK-TYPED:    [[TMP11:%.*]] = inttoptr i64 [[TMP10]] to i16*
; CHECK-OPAQUE:    [[TMP11:%.*]] = inttoptr i64 [[TMP10]] to ptr,
; CHECK-TYPED:    [[TMP12:%.*]] = insertelement <2 x i16*> [[TMP9]], i16* [[TMP11]], i64 1
; CHECK-OPAQUE:    [[TMP12:%.*]] = insertelement <2 x ptr> [[TMP9]], ptr [[TMP11]], i64 1
; CHECK-TYPED:    [[VPTRCAST1:%.*]] = bitcast <2 x i16*>* [[TMP4]] to <4 x i32>*
; CHECK-TYPED:    [[TMP13:%.*]] = extractelement <2 x i16*> [[TMP12]], i64 0
; CHECK-OPAQUE:    [[TMP13:%.*]] = extractelement <2 x ptr> [[TMP12]], i64 0
; CHECK-TYPED:    [[TMP14:%.*]] = ptrtoint i16* [[TMP13]] to i64
; CHECK-OPAQUE:    [[TMP14:%.*]] = ptrtoint ptr [[TMP13]] to i64
; CHECK:    [[TMP15:%.*]] = insertelement <2 x i64> undef, i64 [[TMP14]], i64 0
; CHECK-TYPED:    [[TMP16:%.*]] = extractelement <2 x i16*> [[TMP12]], i64 1
; CHECK-OPAQUE:    [[TMP16:%.*]] = extractelement <2 x ptr> [[TMP12]], i64 1
; CHECK-TYPED:    [[TMP17:%.*]] = ptrtoint i16* [[TMP16]] to i64
; CHECK-OPAQUE:    [[TMP17:%.*]] = ptrtoint ptr [[TMP16]] to i64
; CHECK:    [[TMP18:%.*]] = insertelement <2 x i64> [[TMP15]], i64 [[TMP17]], i64 1
; CHECK:    [[TMP19:%.*]] = bitcast <2 x i64> [[TMP18]] to <4 x i32>
; CHECK-TYPED:    store <4 x i32> [[TMP19]], <4 x i32>* [[VPTRCAST1]], align 1
; CHECK-OPAQUE:    store <4 x i32> [[TMP19]], ptr [[TMP4]], align 1
; CHECK-TYPED:    [[TMP20:%.*]] = alloca i16 addrspace(1)*, align 4
; CHECK-OPAQUE:    [[TMP20:%.*]] = alloca ptr addrspace(1), align 4
; CHECK-TYPED:    [[TMP21:%.*]] = inttoptr i64 15 to i16 addrspace(1)*
; CHECK-OPAQUE:    [[TMP21:%.*]] = inttoptr i64 15 to ptr addrspace(1)
; CHECK-TYPED:    [[VPTRCAST2:%.*]] = bitcast i16 addrspace(1)** [[TMP20]] to <2 x i32>*
; CHECK-TYPED:    store <2 x i32> bitcast (<1 x i64> <i64 15> to <2 x i32>), <2 x i32>* [[VPTRCAST2]], align 4
; CHECK-OPAQUE:    store <2 x i32> bitcast (<1 x i64> {{<i64 15>|splat \(i64 15\)}} to <2 x i32>), ptr [[TMP20]], align 4
; CHECK-TYPED:    [[VPTRCAST3:%.*]] = bitcast <2 x i8>* [[SRC2:%.*]] to i16*
; CHECK-TYPED:    store i16 bitcast (<2 x i8> <i8 1, i8 8> to i16), i16* [[VPTRCAST3]], align 1
; CHECK-OPAQUE:    store i16 bitcast (<2 x i8> <i8 1, i8 8> to i16), ptr [[SRC2:%.*]], align 1
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
