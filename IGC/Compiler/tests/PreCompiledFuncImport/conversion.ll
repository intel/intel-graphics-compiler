;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -regkey TestIGCPreCompiledFunctions=1 --platformdg2 --igc-precompiled-import -S < %s | FileCheck %s
; ------------------------------------------------
; PreCompiledFuncImport
; ------------------------------------------------

; Check that appropriate call instruction to emulated code are inserted
; when we use PreCompiledFuncImport pass.

define void @fptrunc_kernel(double addrspace(1)* %inA, float addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1) #0 {
; CHECK-LABEL: @fptrunc_kernel(
; CHECK: entry:
; CHECK:  [[DPEmuFlag:%.*]] = alloca i32
; CHECK:  [[TMP0:%.*]] = extractelement <8 x i32> %payloadHeader, i32 0
; CHECK:  [[TMP1:%.*]] = extractelement <3 x i32> %enqueuedLocalSize, i32 0
; CHECK:  [[TMP2:%.*]] = extractelement <8 x i32> %r0, i32 1
; CHECK:  [[MUL:%.*]] = mul i32 [[TMP1]], [[TMP2]]
; CHECK:  [[LOCAL_ID_X:%.*]] = zext i16 %localIdX to i32
; CHECK:  [[ADD0:%.*]] = add i32 [[MUL]], [[LOCAL_ID_X]]
; CHECK:  [[ADD1:%.*]] = add i32 [[ADD0]], [[TMP0]]
; CHECK:  [[CONV0:%.*]] = zext i32 [[ADD1]] to i64
; CHECK:  [[ARRAY_IDX0:%.*]] = getelementptr inbounds double, double addrspace(1)* %inA, i64 [[CONV0]]
; CHECK:  [[TMP3:%.*]] = load double, double addrspace(1)* [[ARRAY_IDX0]], align 8
; CHECK:  [[CALL_FTMP:%.*]] = call float @__igcbuiltin_dp_to_sp(double [[TMP3]], i32 0, i32 0, i32 0, i32* [[DPEmuFlag]])
; CHECK:  [[ARRAY_IDX2:%.*]] = getelementptr inbounds float, float addrspace(1)* %out, i64 [[CONV0]]
; CHECK:  store float [[CALL_FTMP]], float addrspace(1)* [[ARRAY_IDX2]], align 4
; CHECK:  ret void
;
entry:
  %payloadHeader.scalar = extractelement <8 x i32> %payloadHeader, i32 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i32 0
  %r0.scalar18 = extractelement <8 x i32> %r0, i32 1
  %mul.i.i.i = mul i32 %enqueuedLocalSize.scalar, %r0.scalar18
  %localIdX3 = zext i16 %localIdX to i32
  %add.i.i.i = add i32 %mul.i.i.i, %localIdX3
  %add4.i.i.i = add i32 %add.i.i.i, %payloadHeader.scalar
  %conv.i.i.i = zext i32 %add4.i.i.i to i64
  %arrayidx = getelementptr inbounds double, double addrspace(1)* %inA, i64 %conv.i.i.i
  %0 = load double, double addrspace(1)* %arrayidx, align 8
  %call1 = fptrunc double %0 to float
  %arrayidx2 = getelementptr inbounds float, float addrspace(1)* %out, i64 %conv.i.i.i
  store float %call1, float addrspace(1)* %arrayidx2, align 4
  ret void
}

define void @fpext_kernel(float addrspace(1)* %inA, double addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1) #0 {
; CHECK-LABEL: @fpext_kernel(
; CHECK: entry:
; CHECK:  [[DPEmuFlag:%.*]] = alloca i32
; CHECK:  [[TMP0:%.*]] = extractelement <8 x i32> %payloadHeader, i32 0
; CHECK:  [[TMP1:%.*]] = extractelement <3 x i32> %enqueuedLocalSize, i32 0
; CHECK:  [[TMP2:%.*]] = extractelement <8 x i32> %r0, i32 1
; CHECK:  [[MUL:%.*]] = mul i32 [[TMP1]], [[TMP2]]
; CHECK:  [[LOCAL_ID_X:%.*]] = zext i16 %localIdX to i32
; CHECK:  [[ADD0:%.*]] = add i32 [[MUL]], [[LOCAL_ID_X]]
; CHECK:  [[ADD1:%.*]] = add i32 [[ADD0]], [[TMP0]]
; CHECK:  [[CONV0:%.*]] = zext i32 [[ADD1]] to i64
; CHECK:  [[ARRAY_IDX0:%.*]] = getelementptr inbounds float, float addrspace(1)* %inA, i64 [[CONV0]]
; CHECK:  [[TMP3:%.*]] = load float, float addrspace(1)* [[ARRAY_IDX0]], align 4
; CHECK:  [[CALL_FTMP:%.*]] = call double @__igcbuiltin_sp_to_dp(float [[TMP3]], i32 0, i32* [[DPEmuFlag]])
; CHECK:  [[ARRAY_IDX2:%.*]] = getelementptr inbounds double, double addrspace(1)* %out, i64 [[CONV0]]
; CHECK:  store double [[CALL_FTMP]], double addrspace(1)* [[ARRAY_IDX2]], align 8
; CHECK:  ret void
;
entry:
  %payloadHeader.scalar = extractelement <8 x i32> %payloadHeader, i32 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i32 0
  %r0.scalar18 = extractelement <8 x i32> %r0, i32 1
  %mul.i.i.i = mul i32 %enqueuedLocalSize.scalar, %r0.scalar18
  %localIdX3 = zext i16 %localIdX to i32
  %add.i.i.i = add i32 %mul.i.i.i, %localIdX3
  %add4.i.i.i = add i32 %add.i.i.i, %payloadHeader.scalar
  %conv.i.i.i = zext i32 %add4.i.i.i to i64
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %inA, i64 %conv.i.i.i
  %0 = load float, float addrspace(1)* %arrayidx, align 4
  %call1 = fpext float %0 to double
  %arrayidx2 = getelementptr inbounds double, double addrspace(1)* %out, i64 %conv.i.i.i
  store double %call1, double addrspace(1)* %arrayidx2, align 8
  ret void
}

define spir_kernel void @sitofp_kernel(i32 addrspace(1)* %inA, double addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1) #0 {
; CHECK-LABEL: @sitofp_kernel(
; CHECK: entry:
; CHECK:  [[TMP0:%.*]] = extractelement <8 x i32> %payloadHeader, i32 0
; CHECK:  [[TMP1:%.*]] = extractelement <3 x i32> %enqueuedLocalSize, i32 0
; CHECK:  [[TMP2:%.*]] = extractelement <8 x i32> %r0, i32 1
; CHECK:  [[MUL:%.*]] = mul i32 [[TMP1]], [[TMP2]]
; CHECK:  [[LOCAL_ID_X:%.*]] = zext i16 %localIdX to i32
; CHECK:  [[ADD0:%.*]] = add i32 [[MUL]], [[LOCAL_ID_X]]
; CHECK:  [[ADD1:%.*]] = add i32 [[ADD0]], [[TMP0]]
; CHECK:  [[CONV0:%.*]] = zext i32 [[ADD1]] to i64
; CHECK:  [[ARRAY_IDX0:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* %inA, i64 [[CONV0]]
; CHECK:  [[TMP3:%.*]] = load i32, i32 addrspace(1)* [[ARRAY_IDX0]], align 4
; CHECK:  [[CALL_TMP:%.*]] = call double @__igcbuiltin_int32_to_dp(i32 [[TMP3]])
; CHECK:  [[ARRAY_IDX1:%.*]] = getelementptr inbounds double, double addrspace(1)* %out, i64 [[CONV0]]
; CHECK:  store double [[CALL_TMP]], double addrspace(1)* [[ARRAY_IDX1]], align 8
; CHECK:  ret void
;
entry:
  %payloadHeader.scalar = extractelement <8 x i32> %payloadHeader, i32 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i32 0
  %r0.scalar18 = extractelement <8 x i32> %r0, i32 1
  %mul.i.i.i = mul i32 %enqueuedLocalSize.scalar, %r0.scalar18
  %localIdX3 = zext i16 %localIdX to i32
  %add.i.i.i = add i32 %mul.i.i.i, %localIdX3
  %add4.i.i.i = add i32 %add.i.i.i, %payloadHeader.scalar
  %conv.i.i.i = zext i32 %add4.i.i.i to i64
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %inA, i64 %conv.i.i.i
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %call1 = sitofp i32 %0 to double
  %arrayidx2 = getelementptr inbounds double, double addrspace(1)* %out, i64 %conv.i.i.i
  store double %call1, double addrspace(1)* %arrayidx2, align 8
  ret void
}

define spir_kernel void @uitofp_kernel(i32 addrspace(1)* %inA, double addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1) #0 {
; CHECK-LABEL: @uitofp_kernel(
; CHECK: entry:
; CHECK:  [[TMP0:%.*]] = extractelement <8 x i32> %payloadHeader, i32 0
; CHECK:  [[TMP1:%.*]] = extractelement <3 x i32> %enqueuedLocalSize, i32 0
; CHECK:  [[TMP2:%.*]] = extractelement <8 x i32> %r0, i32 1
; CHECK:  [[MUL:%.*]] = mul i32 [[TMP1]], [[TMP2]]
; CHECK:  [[LOCAL_ID_X:%.*]] = zext i16 %localIdX to i32
; CHECK:  [[ADD0:%.*]] = add i32 [[MUL]], [[LOCAL_ID_X]]
; CHECK:  [[ADD1:%.*]] = add i32 [[ADD0]], [[TMP0]]
; CHECK:  [[CONV0:%.*]] = zext i32 [[ADD1]] to i64
; CHECK:  [[ARRAY_IDX0:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* %inA, i64 [[CONV0]]
; CHECK:  [[TMP3:%.*]] = load i32, i32 addrspace(1)* [[ARRAY_IDX0]], align 4
; CHECK:  [[CALL_TMP:%.*]] = call double @__igcbuiltin_uint32_to_dp(i32 [[TMP3]])
; CHECK:  [[ARRAY_IDX1:%.*]] = getelementptr inbounds double, double addrspace(1)* %out, i64 [[CONV0]]
; CHECK:  store double [[CALL_TMP]], double addrspace(1)* [[ARRAY_IDX1]], align 8
; CHECK:  ret void
;
entry:
  %payloadHeader.scalar = extractelement <8 x i32> %payloadHeader, i32 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i32 0
  %r0.scalar18 = extractelement <8 x i32> %r0, i32 1
  %mul.i.i.i = mul i32 %enqueuedLocalSize.scalar, %r0.scalar18
  %localIdX3 = zext i16 %localIdX to i32
  %add.i.i.i = add i32 %mul.i.i.i, %localIdX3
  %add4.i.i.i = add i32 %add.i.i.i, %payloadHeader.scalar
  %conv.i.i.i = zext i32 %add4.i.i.i to i64
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %inA, i64 %conv.i.i.i
  %0 = load i32, i32 addrspace(1)* %arrayidx, align 4
  %call1 = uitofp i32 %0 to double
  %arrayidx2 = getelementptr inbounds double, double addrspace(1)* %out, i64 %conv.i.i.i
  store double %call1, double addrspace(1)* %arrayidx2, align 8
  ret void
}
