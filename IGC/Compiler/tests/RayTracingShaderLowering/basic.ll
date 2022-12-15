;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -enable-debugify --igc-raytracing-shader-lowering -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; RayTracingShaderLowering
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

; Test checks:
; 1) Pass adds fences for calls
; 2) Replaces AsyncStackPtr
; 3) Reduces redundant converts

define void @rtlow(i32 addrspace(1)* %src1, i64 %src2) {
; CHECK-LABEL: @rtlow(
; CHECK:    call void @llvm.genx.GenISA.LSCFence(i32 0, i32 1, i32 0)
; CHECK:    call void @llvm.genx.GenISA.TraceRayAsync.p1i32(i32 addrspace(1)* [[SRC1:%.*]], i32 2)
; CHECK:    call void @llvm.genx.GenISA.LSCFence(i32 0, i32 1, i32 0)
; CHECK:    call void @llvm.genx.GenISA.BindlessThreadDispatch.p1i32(i32 addrspace(1)* [[SRC1]], i16 3, i64 [[SRC2:%.*]])
; CHECK:    [[TMP1:%.*]] = inttoptr i64 [[SRC2]] to i32 addrspace(1)*
; CHECK:    [[TMP2:%.*]] = ptrtoint i32 addrspace(1)* [[TMP1]] to i64
; CHECK:    [[TMP3:%.*]] = inttoptr i64 [[SRC2]] to i32 addrspace(1)*
; CHECK:    store i32 14, i32 addrspace(1)* [[TMP3]]
; CHECK:    [[TMP4:%.*]] = bitcast i32 addrspace(1)* [[TMP1]] to float addrspace(1)*
; CHECK:    [[TMP5:%.*]] = inttoptr i64 [[SRC2]] to float addrspace(1)*
; CHECK:    [[TMP6:%.*]] = ptrtoint float addrspace(1)* [[TMP5]] to i32
; CHECK:    [[TMP7:%.*]] = bitcast i32 [[TMP6]] to float
; CHECK:    store float [[TMP7]], float addrspace(1)* [[TMP5]]
; CHECK:    ret void
;
  call void @llvm.genx.GenISA.TraceRayAsync.p1i32(i32 addrspace(1)* %src1, i32 2)
  call void @llvm.genx.GenISA.BindlessThreadDispatch.p1i32(i32 addrspace(1)* %src1, i16 3, i64 %src2)
  %1 = call i32 addrspace(1)* @llvm.genx.GenISA.AsyncStackPtr.p1i32.i64(i64 %src2)
  %2 = ptrtoint i32 addrspace(1)* %1 to i64
  %3 = inttoptr i64 %2 to i32 addrspace(1)*
  store i32 14, i32 addrspace(1)* %3
  %4 = bitcast i32 addrspace(1)* %1 to float addrspace(1)*
  %5 = ptrtoint float addrspace(1)* %4 to i32
  %6 = bitcast i32 %5 to float
  store float %6, float addrspace(1)* %4
  ret void
}

declare i32 addrspace(1)* @llvm.genx.GenISA.AsyncStackPtr.p1i32.i64(i64)
declare void @llvm.genx.GenISA.TraceRayAsync.p1i32(i32 addrspace(1)*, i32)
declare void @llvm.genx.GenISA.BindlessThreadDispatch.p1i32(i32 addrspace(1)*, i16, i64)
