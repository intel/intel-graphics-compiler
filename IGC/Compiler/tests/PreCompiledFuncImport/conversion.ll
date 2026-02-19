;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; RUN: igc_opt --typed-pointers -regkey TestIGCPreCompiledFunctions=1 --platformdg2 --igc-precompiled-import -S < %s | FileCheck %s
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

define spir_kernel void @fptoui_kernel(double addrspace(1)* %inA, i32 addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1) #0 {
; CHECK-LABEL: @fptoui_kernel(
; CHECK: entry:
; CHECK:  [[DPEmuFlag:%.*]] = alloca i32
; CHECK:  [[TMP0:%.*]] = extractelement <8 x i32> %payloadHeader, i64 0
; CHECK:  [[TMP1:%.*]] = extractelement <3 x i32> %enqueuedLocalSize, i64 0
; CHECK:  [[TMP2:%.*]] = extractelement <8 x i32> %r0, i64 1
; CHECK:  [[MUL:%.*]] = mul i32 [[TMP1]], [[TMP2]]
; CHECK:  [[LOCAL_ID_X:%.*]] = zext i16 %localIdX to i32
; CHECK:  [[ADD0:%.*]] = add i32 [[MUL]], [[LOCAL_ID_X]]
; CHECK:  [[ADD1:%.*]] = add i32 [[ADD0]], [[TMP0]]
; CHECK:  [[CONV0:%.*]] = zext i32 [[ADD1]] to i64
; CHECK:  [[ARRAY_IDX0:%.*]] = getelementptr inbounds double, double addrspace(1)* %inA, i64 [[CONV0]]
; CHECK:  [[TMP3:%.*]] = load double, double addrspace(1)* [[ARRAY_IDX0]], align 8
; CHECK:  [[CALL_TMP:%.*]] = call i32 @__igcbuiltin_dp_to_uint32(double [[TMP3]], i32 3, i32 0, i32* [[DPEmuFlag]])
; CHECK:  [[ARRAY_IDX1:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 [[CONV0]]
; CHECK:  store i32 [[CALL_TMP]], i32 addrspace(1)* [[ARRAY_IDX1]], align 4
; CHECK:  ret void
;
entry:
  %payloadHeader.scalar = extractelement <8 x i32> %payloadHeader, i64 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i64 0
  %r0.scalar18 = extractelement <8 x i32> %r0, i64 1
  %mul.i.i.i = mul i32 %enqueuedLocalSize.scalar, %r0.scalar18
  %localIdX3 = zext i16 %localIdX to i32
  %add.i.i.i = add i32 %mul.i.i.i, %localIdX3
  %add4.i.i.i = add i32 %add.i.i.i, %payloadHeader.scalar
  %conv.i.i.i = zext i32 %add4.i.i.i to i64
  %arrayidx = getelementptr inbounds double, double addrspace(1)* %inA, i64 %conv.i.i.i
  %0 = load double, double addrspace(1)* %arrayidx, align 8
  %call1 = fptoui double %0 to i32
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %conv.i.i.i
  store i32 %call1, i32 addrspace(1)* %arrayidx2, align 4
  ret void
}

define spir_kernel void @fptosi_kernel(double addrspace(1)* %inA, i32 addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1) #0 {
; CHECK-LABEL: @fptosi_kernel(
; CHECK: entry:
; CHECK:  [[DPEmuFlag:%.*]] = alloca i32
; CHECK:  [[TMP0:%.*]] = extractelement <8 x i32> %payloadHeader, i64 0
; CHECK:  [[TMP1:%.*]] = extractelement <3 x i32> %enqueuedLocalSize, i64 0
; CHECK:  [[TMP2:%.*]] = extractelement <8 x i32> %r0, i64 1
; CHECK:  [[MUL:%.*]] = mul i32 [[TMP1]], [[TMP2]]
; CHECK:  [[LOCAL_ID_X:%.*]] = zext i16 %localIdX to i32
; CHECK:  [[ADD0:%.*]] = add i32 [[MUL]], [[LOCAL_ID_X]]
; CHECK:  [[ADD1:%.*]] = add i32 [[ADD0]], [[TMP0]]
; CHECK:  [[CONV0:%.*]] = zext i32 [[ADD1]] to i64
; CHECK:  [[ARRAY_IDX0:%.*]] = getelementptr inbounds double, double addrspace(1)* %inA, i64 [[CONV0]]
; CHECK:  [[TMP3:%.*]] = load double, double addrspace(1)* [[ARRAY_IDX0]], align 8
; CHECK:  [[CALL_TMP:%.*]] = call i32 @__igcbuiltin_dp_to_int32(double [[TMP3]], i32 3, i32 0, i32* [[DPEmuFlag]])
; CHECK:  [[ARRAY_IDX1:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 [[CONV0]]
; CHECK:  store i32 [[CALL_TMP]], i32 addrspace(1)* [[ARRAY_IDX1]], align 4
; CHECK:  ret void
;
entry:
  %payloadHeader.scalar = extractelement <8 x i32> %payloadHeader, i64 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i64 0
  %r0.scalar18 = extractelement <8 x i32> %r0, i64 1
  %mul.i.i.i = mul i32 %enqueuedLocalSize.scalar, %r0.scalar18
  %localIdX3 = zext i16 %localIdX to i32
  %add.i.i.i = add i32 %mul.i.i.i, %localIdX3
  %add4.i.i.i = add i32 %add.i.i.i, %payloadHeader.scalar
  %conv.i.i.i = zext i32 %add4.i.i.i to i64
  %arrayidx = getelementptr inbounds double, double addrspace(1)* %inA, i64 %conv.i.i.i
  %0 = load double, double addrspace(1)* %arrayidx, align 8
  %call1 = fptosi double %0 to i32
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %out, i64 %conv.i.i.i
  store i32 %call1, i32 addrspace(1)* %arrayidx2, align 4
  ret void
}

define spir_kernel void @fptosi64_kernel(double addrspace(1)* %inA, i64 addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1, i32 %bindlessOffset, i32 %bindlessOffset2) #0 {
; CHECK-LABEL: @fptosi64_kernel(
; CHECK: entry:
; CHECK:  [[DPEmuFlag:%.*]] = alloca i32
; CHECK:  [[TMP0:%.*]] = extractelement <8 x i32> %payloadHeader, i64 0
; CHECK:  [[TMP1:%.*]] = extractelement <3 x i32> %enqueuedLocalSize, i64 0
; CHECK:  [[TMP2:%.*]] = extractelement <8 x i32> %r0, i64 1
; CHECK:  [[MUL:%.*]] = mul i32 [[TMP1]], [[TMP2]]
; CHECK:  [[LOCAL_ID_X:%.*]] = zext i16 %localIdX to i32
; CHECK:  [[ADD0:%.*]] = add i32 [[MUL]], [[LOCAL_ID_X]]
; CHECK:  [[ADD1:%.*]] = add i32 [[ADD0]], [[TMP0]]
; CHECK:  [[CONV0:%.*]] = zext i32 [[ADD1]] to i64
; CHECK:  [[ARRAY_IDX0:%.*]] = getelementptr inbounds double, double addrspace(1)* %inA, i64 [[CONV0]]
; CHECK:  [[TMP3:%.*]] = load double, double addrspace(1)* [[ARRAY_IDX0]], align 8
; CHECK:  [[CALL_TMP:%.*]] = call i64 @__igcbuiltin_dp_to_int64(double [[TMP3]], i32 3, i32 0, i32* [[DPEmuFlag]])
; CHECK:  [[ARRAY_IDX1:%.*]] = getelementptr inbounds i64, i64 addrspace(1)* %out, i64 [[CONV0]]
; CHECK:  store i64 [[CALL_TMP]], i64 addrspace(1)* [[ARRAY_IDX1]], align 8
; CHECK:  ret void
entry:
  %payloadHeader.scalar = extractelement <8 x i32> %payloadHeader, i64 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i64 0
  %r0.scalar19 = extractelement <8 x i32> %r0, i64 1
  %0 = mul i32 %enqueuedLocalSize.scalar, %r0.scalar19
  %localIdX4 = zext i16 %localIdX to i32
  %1 = add i32 %0, %localIdX4
  %2 = add i32 %1, %payloadHeader.scalar
  %3 = zext i32 %2 to i64
  %arrayidx = getelementptr inbounds double, double addrspace(1)* %inA, i64 %3
  %4 = load double, double addrspace(1)* %arrayidx, align 8
  %call1 = fptosi double %4 to i64
  %arrayidx2 = getelementptr inbounds i64, i64 addrspace(1)* %out, i64 %3
  store i64 %call1, i64 addrspace(1)* %arrayidx2, align 8
  ret void
}

define spir_kernel void @fptoui64_kernel(double addrspace(1)* %inA, i64 addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1, i32 %bindlessOffset, i32 %bindlessOffset2) #0 {
; CHECK-LABEL: @fptoui64_kernel(
; CHECK: entry:
; CHECK:  [[DPEmuFlag:%.*]] = alloca i32
; CHECK:  [[TMP0:%.*]] = extractelement <8 x i32> %payloadHeader, i64 0
; CHECK:  [[TMP1:%.*]] = extractelement <3 x i32> %enqueuedLocalSize, i64 0
; CHECK:  [[TMP2:%.*]] = extractelement <8 x i32> %r0, i64 1
; CHECK:  [[MUL:%.*]] = mul i32 [[TMP1]], [[TMP2]]
; CHECK:  [[LOCAL_ID_X:%.*]] = zext i16 %localIdX to i32
; CHECK:  [[ADD0:%.*]] = add i32 [[MUL]], [[LOCAL_ID_X]]
; CHECK:  [[ADD1:%.*]] = add i32 [[ADD0]], [[TMP0]]
; CHECK:  [[CONV0:%.*]] = zext i32 [[ADD1]] to i64
; CHECK:  [[ARRAY_IDX0:%.*]] = getelementptr inbounds double, double addrspace(1)* %inA, i64 [[CONV0]]
; CHECK:  [[TMP3:%.*]] = load double, double addrspace(1)* [[ARRAY_IDX0]], align 8
; CHECK:  [[CALL_TMP:%.*]] = call i64 @__igcbuiltin_dp_to_uint64(double [[TMP3]], i32 3, i32 0, i32* [[DPEmuFlag]])
; CHECK:  [[ARRAY_IDX1:%.*]] = getelementptr inbounds i64, i64 addrspace(1)* %out, i64 [[CONV0]]
; CHECK:  store i64 [[CALL_TMP]], i64 addrspace(1)* [[ARRAY_IDX1]], align 8
; CHECK:  ret void
entry:
  %payloadHeader.scalar = extractelement <8 x i32> %payloadHeader, i64 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i64 0
  %r0.scalar19 = extractelement <8 x i32> %r0, i64 1
  %0 = mul i32 %enqueuedLocalSize.scalar, %r0.scalar19
  %localIdX4 = zext i16 %localIdX to i32
  %1 = add i32 %0, %localIdX4
  %2 = add i32 %1, %payloadHeader.scalar
  %3 = zext i32 %2 to i64
  %arrayidx = getelementptr inbounds double, double addrspace(1)* %inA, i64 %3
  %4 = load double, double addrspace(1)* %arrayidx, align 8
  %call1 = fptoui double %4 to i64
  %arrayidx2 = getelementptr inbounds i64, i64 addrspace(1)* %out, i64 %3
  store i64 %call1, i64 addrspace(1)* %arrayidx2, align 8
  ret void
}

define spir_kernel void @ui64tofp_kernel(i64 addrspace(1)* %inA, double addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1, i32 %bindlessOffset, i32 %bindlessOffset2) #0 {
; CHECK-LABEL: @ui64tofp_kernel(
; CHECK: entry:
; CHECK:  [[DPEmuFlag:%.*]] = alloca i32
; CHECK:  [[TMP0:%.*]] = extractelement <8 x i32> %payloadHeader, i64 0
; CHECK:  [[TMP1:%.*]] = extractelement <3 x i32> %enqueuedLocalSize, i64 0
; CHECK:  [[TMP2:%.*]] = extractelement <8 x i32> %r0, i64 1
; CHECK:  [[MUL:%.*]] = mul i32 [[TMP1]], [[TMP2]]
; CHECK:  [[LOCAL_ID_X:%.*]] = zext i16 %localIdX to i32
; CHECK:  [[ADD0:%.*]] = add i32 [[MUL]], [[LOCAL_ID_X]]
; CHECK:  [[ADD1:%.*]] = add i32 [[ADD0]], [[TMP0]]
; CHECK:  [[CONV0:%.*]] = zext i32 [[ADD1]] to i64
; CHECK:  [[ARRAY_IDX0:%.*]] = getelementptr inbounds i64, i64 addrspace(1)* %inA, i64 [[CONV0]]
; CHECK:  [[TMP3:%.*]] = load i64, i64 addrspace(1)* [[ARRAY_IDX0]], align 8
; CHECK:  [[CALL_TMP:%.*]] = call double @__igcbuiltin_uint64_to_dp(i64 [[TMP3]], i32 0, i32* [[DPEmuFlag]])
; CHECK:  [[ARRAY_IDX1:%.*]] = getelementptr inbounds double, double addrspace(1)* %out, i64 [[CONV0]]
; CHECK:  store double [[CALL_TMP]], double addrspace(1)* [[ARRAY_IDX1]], align 8
; CHECK:  ret void
entry:
  %payloadHeader.scalar = extractelement <8 x i32> %payloadHeader, i64 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i64 0
  %r0.scalar19 = extractelement <8 x i32> %r0, i64 1
  %0 = mul i32 %enqueuedLocalSize.scalar, %r0.scalar19
  %localIdX4 = zext i16 %localIdX to i32
  %1 = add i32 %0, %localIdX4
  %2 = add i32 %1, %payloadHeader.scalar
  %3 = zext i32 %2 to i64
  %arrayidx = getelementptr inbounds i64, i64 addrspace(1)* %inA, i64 %3
  %4 = load i64, i64 addrspace(1)* %arrayidx, align 8
  %call1 = uitofp i64 %4 to double
  %arrayidx2 = getelementptr inbounds double, double addrspace(1)* %out, i64 %3
  store double %call1, double addrspace(1)* %arrayidx2, align 8
  ret void
}

define spir_kernel void @si64tofp_kernel(i64 addrspace(1)* %inA, double addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1, i32 %bindlessOffset, i32 %bindlessOffset2) #0 {
; CHECK-LABEL: @si64tofp_kernel(
; CHECK: entry:
; CHECK:  [[DPEmuFlag:%.*]] = alloca i32
; CHECK:  [[TMP0:%.*]] = extractelement <8 x i32> %payloadHeader, i64 0
; CHECK:  [[TMP1:%.*]] = extractelement <3 x i32> %enqueuedLocalSize, i64 0
; CHECK:  [[TMP2:%.*]] = extractelement <8 x i32> %r0, i64 1
; CHECK:  [[MUL:%.*]] = mul i32 [[TMP1]], [[TMP2]]
; CHECK:  [[LOCAL_ID_X:%.*]] = zext i16 %localIdX to i32
; CHECK:  [[ADD0:%.*]] = add i32 [[MUL]], [[LOCAL_ID_X]]
; CHECK:  [[ADD1:%.*]] = add i32 [[ADD0]], [[TMP0]]
; CHECK:  [[CONV0:%.*]] = zext i32 [[ADD1]] to i64
; CHECK:  [[ARRAY_IDX0:%.*]] = getelementptr inbounds i64, i64 addrspace(1)* %inA, i64 [[CONV0]]
; CHECK:  [[TMP3:%.*]] = load i64, i64 addrspace(1)* [[ARRAY_IDX0]], align 8
; CHECK:  [[CALL_TMP:%.*]] = call double @__igcbuiltin_int64_to_dp(i64 [[TMP3]], i32 0, i32* [[DPEmuFlag]])
; CHECK:  [[ARRAY_IDX1:%.*]] = getelementptr inbounds double, double addrspace(1)* %out, i64 [[CONV0]]
; CHECK:  store double [[CALL_TMP]], double addrspace(1)* [[ARRAY_IDX1]], align 8
; CHECK:  ret void
entry:
  %payloadHeader.scalar = extractelement <8 x i32> %payloadHeader, i64 0
  %enqueuedLocalSize.scalar = extractelement <3 x i32> %enqueuedLocalSize, i64 0
  %r0.scalar19 = extractelement <8 x i32> %r0, i64 1
  %0 = mul i32 %enqueuedLocalSize.scalar, %r0.scalar19
  %localIdX4 = zext i16 %localIdX to i32
  %1 = add i32 %0, %localIdX4
  %2 = add i32 %1, %payloadHeader.scalar
  %3 = zext i32 %2 to i64
  %arrayidx = getelementptr inbounds i64, i64 addrspace(1)* %inA, i64 %3
  %4 = load i64, i64 addrspace(1)* %arrayidx, align 8
  %call1 = sitofp i64 %4 to double
  %arrayidx2 = getelementptr inbounds double, double addrspace(1)* %out, i64 %3
  store double %call1, double addrspace(1)* %arrayidx2, align 8
  ret void
}
