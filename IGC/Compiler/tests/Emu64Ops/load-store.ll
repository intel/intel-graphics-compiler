;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --typed-pointers --platformdg2 --igc-emu64ops -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; Emu64Ops
; ------------------------------------------------

; load/store/inttoptr/ptrtoint emulation


target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"


define void @test_ptrs(i16 %src1) {
; CHECK-LABEL: @test_ptrs(
; CHECK-NEXT:    [[TMP1:%.*]] = zext i16 [[SRC1:%.*]] to i32
; CHECK-NEXT:    [[TMP2:%.*]] = call i64 addrspace(1)* @llvm.genx.GenISA.pair.to.ptr.p1i64(i32 [[TMP1]], i32 0)
; CHECK-NEXT:    [[TMP3:%.*]] = bitcast i64 addrspace(1)* [[TMP2]] to <2 x i32> addrspace(1)*
; CHECK-NEXT:    [[TMP4:%.*]] = load <2 x i32>, <2 x i32> addrspace(1)* [[TMP3]]
; CHECK-NEXT:    [[TMP5:%.*]] = extractelement <2 x i32> [[TMP4]], i32 0
; CHECK-NEXT:    [[TMP6:%.*]] = extractelement <2 x i32> [[TMP4]], i32 1
; CHECK-NEXT:    [[TMP7:%.*]] = insertelement <2 x i32> undef, i32 [[TMP5]], i32 0
; CHECK-NEXT:    [[TMP8:%.*]] = insertelement <2 x i32> [[TMP7]], i32 [[TMP6]], i32 1
; CHECK-NEXT:    [[TMP9:%.*]] = bitcast <2 x i32> [[TMP8]] to i64
; CHECK-NEXT:    call void @use.i64(i64 [[TMP9]])
; CHECK-NEXT:    [[TMP10:%.*]] = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p1i64(i64 addrspace(1)* [[TMP2]])
; CHECK-NEXT:    [[TMP11:%.*]] = extractvalue { i32, i32 } [[TMP10]], 0
; CHECK-NEXT:    [[TMP12:%.*]] = extractvalue { i32, i32 } [[TMP10]], 1
; CHECK-NEXT:    [[TMP13:%.*]] = insertelement <2 x i32> undef, i32 [[TMP11]], i32 0
; CHECK-NEXT:    [[TMP14:%.*]] = insertelement <2 x i32> [[TMP13]], i32 [[TMP12]], i32 1
; CHECK-NEXT:    [[TMP15:%.*]] = bitcast i64 addrspace(1)* [[TMP2]] to <2 x i32> addrspace(1)*
; CHECK-NEXT:    store <2 x i32> [[TMP14]], <2 x i32> addrspace(1)* [[TMP15]]
; CHECK-NEXT:    ret void
;
  %1 = inttoptr i16 %src1 to i64 addrspace(1)*
  %2 = load i64, i64 addrspace(1)* %1, align 4
  call void @use.i64(i64 %2)
  %3 = ptrtoint i64 addrspace(1)* %1 to i64
  store i64 %3, i64 addrspace(1)* %1, align 4
  ret void
}

declare void @use.i64(i64)

!igc.functions = !{!0}

!0 = !{void (i16)* @test_ptrs, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
