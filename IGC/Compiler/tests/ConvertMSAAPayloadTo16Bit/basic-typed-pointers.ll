;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify -igc-ConvertMSAAPayloadTo16Bit -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; ConvertMSAAPayloadTo16Bit
; ------------------------------------------------

; Test checks that intrisics are converted to i16 payload usage

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test_ldms_use(<4 x float> addrspace(2)* %src) {
; CHECK-LABEL: @test_ldms_use(
; CHECK:    [[TMP1:%.*]] = call <4 x i16> @llvm.genx.GenISA.ldmcsptr.v4i16.i32.p2v4f32(i32 1, i32 2, i32 3, i32 4, <4 x float> addrspace(2)* [[SRC:%.*]], i32 0, i32 0, i32 0)
; CHECK:    [[TMP2:%.*]] = extractelement <4 x i16> [[TMP1]], i32 0
; CHECK:    [[TMP3:%.*]] = extractelement <4 x i16> [[TMP1]], i32 1
; CHECK:    [[TMP4:%.*]] = extractelement <4 x i16> [[TMP1]], i32 2
; CHECK:    [[TMP5:%.*]] = extractelement <4 x i16> [[TMP1]], i32 3
; CHECK:    [[TMP6:%.*]] = bitcast <4 x i16> [[TMP1]] to <2 x i32>
; CHECK:    [[TMP9:%.*]] = extractelement <2 x i32> [[TMP6]], i32 1
; CHECK:    [[TMP10:%.*]] = call <4 x float> @llvm.genx.GenISA.ldmsptr16bit.v4f32.p2v4f32(i16 11, i16 [[TMP2]], i16 [[TMP3]], i16 [[TMP4]], i16 [[TMP5]], i16 44, i16 55, i16 66, i16 77, <4 x float> addrspace(2)* [[SRC]], i32 0, i32 0, i32 0)
; CHECK:    store <4 x float> [[TMP10]], <4 x float> addrspace(2)* [[SRC]], align 8
; CHECK:    ret void
;
  %1 = call <2 x i32> @llvm.genx.GenISA.ldmcsptr.v2i32.i32.p2v4f32(i32 1, i32 2, i32 3, i32 4, <4 x float> addrspace(2)* %src, i32 0, i32 0, i32 0)
  %2 = extractelement <2 x i32> %1, i32 1
  %3 = call <4 x float> @llvm.genx.GenISA.ldmsptr.v4f32.p2v4f32(i32 11, i32 %2, i32 33, i32 44, i32 55, i32 66, i32 77, <4 x float> addrspace(2)* %src, i32 0, i32 0, i32 0)
  store <4 x float> %3, <4 x float> addrspace(2)* %src, align 8
  ret void
}


define void @test_other_use(<4 x float> addrspace(2)* %src, <2 x i32>* %dst) {
; CHECK-LABEL: @test_other_use(
; CHECK:    [[TMP1:%.*]] = call <4 x i16> @llvm.genx.GenISA.ldmcsptr.v4i16.i32.p2v4f32(i32 1, i32 2, i32 3, i32 4, <4 x float> addrspace(2)* [[SRC:%.*]], i32 0, i32 0, i32 0)
; CHECK:    [[TMP2:%.*]] = bitcast <4 x i16> [[TMP1]] to <2 x i32>
; CHECK:    store <2 x i32> [[TMP2]], <2 x i32>* [[DST:%.*]], align 8
; CHECK:    ret void
;
  %1 = call <2 x i32> @llvm.genx.GenISA.ldmcsptr.v2i32.i32.p2v4f32(i32 1, i32 2, i32 3, i32 4, <4 x float> addrspace(2)* %src, i32 0, i32 0, i32 0)
  store <2 x i32> %1, <2 x i32>* %dst, align 8
  ret void
}

declare <2 x i32> @llvm.genx.GenISA.ldmcsptr.v2i32.i32.p2v4f32(i32, i32, i32, i32, <4 x float> addrspace(2)*, i32, i32, i32)
declare <4 x float> @llvm.genx.GenISA.ldmsptr.v4f32.p2v4f32(i32, i32, i32, i32, i32, i32, i32, <4 x float> addrspace(2)*, i32, i32, i32)
