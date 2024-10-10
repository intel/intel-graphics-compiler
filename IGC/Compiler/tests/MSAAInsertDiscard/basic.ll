;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers -debugify -igc-MSAAInsertDiscard -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; MSAAInsertDiscard
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test(ptr addrspace(2) %src1, ptr %dst) {
; CHECK-LABEL: @test(
; CHECK:    [[TMP1:%.*]] = call <4 x i32> @llvm.genx.GenISA.ldmcsptr.v4i32.i32.p2(i32 1, i32 2, i32 3, i32 4, ptr addrspace(2) [[S1:%.*]], i32 0, i32 0, i32 0)
; CHECK:    [[TMP2:%.*]] = extractelement <4 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <4 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = icmp eq i32 [[TMP3]], 0
; CHECK:    call void @llvm.genx.GenISA.discard(i1 [[TMP4]])
; CHECK:    [[TMP5:%.*]] = extractelement <4 x i32> [[TMP1]], i32 2
; CHECK:    [[TMP6:%.*]] = extractelement <4 x i32> [[TMP1]], i32 3
; CHECK:    [[TMP7:%.*]] = call <4 x float> @llvm.genx.GenISA.ldmsptr.v4f32.p2(i32 [[TMP2]], i32 [[TMP3]], i32 [[TMP5]], i32 [[TMP6]], i32 1, i32 2, i32 3, ptr addrspace(2) [[S1]], i32 0, i32 0, i32 0)
; CHECK:    store <4 x float> [[TMP7]], ptr %dst
; CHECK:    ret void
;
  %1 = call <4 x i32> @llvm.genx.GenISA.ldmcsptr.v4i32.i32.p2(i32 1, i32 2, i32 3, i32 4, ptr addrspace(2) %src1, i32 0, i32 0, i32 0)
  %2 = extractelement <4 x i32> %1, i32 0
  %3 = extractelement <4 x i32> %1, i32 1
  %4 = extractelement <4 x i32> %1, i32 2
  %5 = extractelement <4 x i32> %1, i32 3
  %6 = call <4 x float> @llvm.genx.GenISA.ldmsptr.v4f32.p2(i32 %2, i32 %3, i32 %4, i32 %5, i32 1, i32 2, i32 3, ptr addrspace(2) %src1, i32 0, i32 0, i32 0)
  store <4 x float> %6, ptr %dst
  ret void
}

declare <4 x float> @llvm.genx.GenISA.ldmsptr.v4f32.p2(i32, i32, i32, i32, i32, i32, i32, ptr addrspace(2), i32, i32, i32)
declare <4 x i32> @llvm.genx.GenISA.ldmcsptr.v4i32.i32.p2(i32, i32, i32, i32, ptr addrspace(2), i32, i32, i32)
