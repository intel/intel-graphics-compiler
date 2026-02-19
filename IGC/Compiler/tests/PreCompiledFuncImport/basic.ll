;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; RUN: igc_opt --typed-pointers -regkey TestIGCPreCompiledFunctions=1 --platformdg2 --igc-precompiled-import --print-codegencontext -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; PreCompiledFuncImport
; ------------------------------------------------

; Check that appropriate call instruction to emulated code are inserted
; when we use PreCompiledFuncImport pass.

; Check if PreCompiledFuncImport pass enables subroutines when importing dpemu:
; CHECK: m_enableSubroutine: 1

declare double @llvm.sqrt.f64(double) #3

define void @fadd_kernel(double addrspace(1)* %inA, double addrspace(1)* %inB, double addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1, i32 %bufferOffset2) #0 {
; CHECK-LABEL: @fadd_kernel(
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
; CHECK:  [[ARRAY_IDX1:%.*]] = getelementptr inbounds double, double addrspace(1)* %inB, i64 [[CONV0]]
; CHECK:  [[TMP4:%.*]] = load double, double addrspace(1)* [[ARRAY_IDX1]], align 8
; CHECK:  [[CALL_FADD:%.*]] = call double @__igcbuiltin_dp_add(double [[TMP3]], double [[TMP4]], i32 0, i32 0, i32 0, i32* [[DPEmuFlag]])
; CHECK:  [[ARRAY_IDX2:%.*]] = getelementptr inbounds double, double addrspace(1)* %out, i64 [[CONV0]]
; CHECK:  store double [[CALL_FADD]], double addrspace(1)* [[ARRAY_IDX2]], align 8
; CHECK:  ret void
;
entry:
  %scalar29 = extractelement <8 x i32> %payloadHeader, i32 0
  %scalar26 = extractelement <3 x i32> %enqueuedLocalSize, i32 0
  %scalar19 = extractelement <8 x i32> %r0, i32 1
  %mul.i.i.i = mul i32 %scalar26, %scalar19
  %localIdX4 = zext i16 %localIdX to i32
  %add.i.i.i = add i32 %mul.i.i.i, %localIdX4
  %add4.i.i.i = add i32 %add.i.i.i, %scalar29
  %conv.i.i.i = zext i32 %add4.i.i.i to i64
  %arrayidx = getelementptr inbounds double, double addrspace(1)* %inA, i64 %conv.i.i.i
  %0 = load double, double addrspace(1)* %arrayidx, align 8
  %arrayidx1 = getelementptr inbounds double, double addrspace(1)* %inB, i64 %conv.i.i.i
  %1 = load double, double addrspace(1)* %arrayidx1, align 8
  %add = fadd double %0, %1
  %arrayidx2 = getelementptr inbounds double, double addrspace(1)* %out, i64 %conv.i.i.i
  store double %add, double addrspace(1)* %arrayidx2, align 8
  ret void
}

define void @fsub_kernel(double addrspace(1)* %inA, double addrspace(1)* %inB, double addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1, i32 %bufferOffset2) #0 {
; CHECK-LABEL: @fsub_kernel(
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
; CHECK:  [[ARRAY_IDX1:%.*]] = getelementptr inbounds double, double addrspace(1)* %inB, i64 [[CONV0]]
; CHECK:  [[TMP4:%.*]] = load double, double addrspace(1)* [[ARRAY_IDX1]], align 8
; CHECK:  [[CALL_FSUB:%.*]] = call double @__igcbuiltin_dp_sub(double [[TMP3]], double [[TMP4]], i32 0, i32 0, i32 0, i32* [[DPEmuFlag]])
; CHECK:  [[ARRAY_IDX2:%.*]] = getelementptr inbounds double, double addrspace(1)* %out, i64 [[CONV0]]
; CHECK:  store double [[CALL_FSUB]], double addrspace(1)* [[ARRAY_IDX2]], align 8
; CHECK:  ret void
;
entry:
  %scalar29 = extractelement <8 x i32> %payloadHeader, i32 0
  %scalar26 = extractelement <3 x i32> %enqueuedLocalSize, i32 0
  %scalar19 = extractelement <8 x i32> %r0, i32 1
  %mul.i.i.i = mul i32 %scalar26, %scalar19
  %localIdX4 = zext i16 %localIdX to i32
  %add.i.i.i = add i32 %mul.i.i.i, %localIdX4
  %add4.i.i.i = add i32 %add.i.i.i, %scalar29
  %conv.i.i.i = zext i32 %add4.i.i.i to i64
  %arrayidx = getelementptr inbounds double, double addrspace(1)* %inA, i64 %conv.i.i.i
  %0 = load double, double addrspace(1)* %arrayidx, align 8
  %arrayidx1 = getelementptr inbounds double, double addrspace(1)* %inB, i64 %conv.i.i.i
  %1 = load double, double addrspace(1)* %arrayidx1, align 8
  %sub = fsub double %0, %1
  %arrayidx2 = getelementptr inbounds double, double addrspace(1)* %out, i64 %conv.i.i.i
  store double %sub, double addrspace(1)* %arrayidx2, align 8
  ret void
}

define void @fmul_kernel(double addrspace(1)* %inA, double addrspace(1)* %inB, double addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1, i32 %bufferOffset2) #0 {
; CHECK-LABEL: @fmul_kernel(
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
; CHECK:  [[ARRAY_IDX1:%.*]] = getelementptr inbounds double, double addrspace(1)* %inB, i64 [[CONV0]]
; CHECK:  [[TMP4:%.*]] = load double, double addrspace(1)* [[ARRAY_IDX1]], align 8
; CHECK:  [[CALL_FMUL:%.*]] = call double @__igcbuiltin_dp_mul(double [[TMP3]], double [[TMP4]], i32 0, i32 0, i32 0, i32* [[DPEmuFlag]])
; CHECK:  [[ARRAY_IDX2:%.*]] = getelementptr inbounds double, double addrspace(1)* %out, i64 [[CONV0]]
; CHECK:  store double [[CALL_FMUL]], double addrspace(1)* [[ARRAY_IDX2]], align 8
; CHECK:  ret void
;
entry:
  %scalar29 = extractelement <8 x i32> %payloadHeader, i32 0
  %scalar26 = extractelement <3 x i32> %enqueuedLocalSize, i32 0
  %scalar19 = extractelement <8 x i32> %r0, i32 1
  %mul.i.i.i = mul i32 %scalar26, %scalar19
  %localIdX4 = zext i16 %localIdX to i32
  %add.i.i.i = add i32 %mul.i.i.i, %localIdX4
  %add4.i.i.i = add i32 %add.i.i.i, %scalar29
  %conv.i.i.i = zext i32 %add4.i.i.i to i64
  %arrayidx = getelementptr inbounds double, double addrspace(1)* %inA, i64 %conv.i.i.i
  %0 = load double, double addrspace(1)* %arrayidx, align 8
  %arrayidx1 = getelementptr inbounds double, double addrspace(1)* %inB, i64 %conv.i.i.i
  %1 = load double, double addrspace(1)* %arrayidx1, align 8
  %mul = fmul double %0, %1
  %arrayidx2 = getelementptr inbounds double, double addrspace(1)* %out, i64 %conv.i.i.i
  store double %mul, double addrspace(1)* %arrayidx2, align 8
  ret void
}

define void @fdiv_kernel(double addrspace(1)* %inA, double addrspace(1)* %inB, double addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1, i32 %bufferOffset2) #0 {
; CHECK-LABEL: @fdiv_kernel(
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
; CHECK:  [[ARRAY_IDX1:%.*]] = getelementptr inbounds double, double addrspace(1)* %inB, i64 [[CONV0]]
; CHECK:  [[TMP4:%.*]] = load double, double addrspace(1)* [[ARRAY_IDX1]], align 8
; CHECK:  [[CALL_FDIV:%.*]] = call double @__igcbuiltin_dp_div(double [[TMP3]], double [[TMP4]], i32 0, i32 0, i32 0, i32* [[DPEmuFlag]])
; CHECK:  [[ARRAY_IDX2:%.*]] = getelementptr inbounds double, double addrspace(1)* %out, i64 [[CONV0]]
; CHECK:  store double [[CALL_FDIV]], double addrspace(1)* [[ARRAY_IDX2]], align 8
; CHECK:  ret void
;
entry:
  %scalar29 = extractelement <8 x i32> %payloadHeader, i32 0
  %scalar26 = extractelement <3 x i32> %enqueuedLocalSize, i32 0
  %scalar19 = extractelement <8 x i32> %r0, i32 1
  %mul.i.i.i = mul i32 %scalar26, %scalar19
  %localIdX4 = zext i16 %localIdX to i32
  %add.i.i.i = add i32 %mul.i.i.i, %localIdX4
  %add4.i.i.i = add i32 %add.i.i.i, %scalar29
  %conv.i.i.i = zext i32 %add4.i.i.i to i64
  %arrayidx = getelementptr inbounds double, double addrspace(1)* %inA, i64 %conv.i.i.i
  %0 = load double, double addrspace(1)* %arrayidx, align 8
  %arrayidx1 = getelementptr inbounds double, double addrspace(1)* %inB, i64 %conv.i.i.i
  %1 = load double, double addrspace(1)* %arrayidx1, align 8
  %div = fdiv double %0, %1
  %arrayidx2 = getelementptr inbounds double, double addrspace(1)* %out, i64 %conv.i.i.i
  store double %div, double addrspace(1)* %arrayidx2, align 8
  ret void
}

define void @sqrt_kernel(double addrspace(1)* %inA, double addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1, i32 %bufferOffset2) #0 {
; CHECK-LABEL: @sqrt_kernel(
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
; CHECK:  [[CALL_SQRT:%.*]] = call double @__igcbuiltin_dp_sqrt(double [[TMP3]], i32 0, i32 0, i32 0, i32* [[DPEmuFlag]])
; CHECK:  [[ARRAY_IDX2:%.*]] = getelementptr inbounds double, double addrspace(1)* %out, i64 [[CONV0]]
; CHECK:  store double [[CALL_SQRT]], double addrspace(1)* [[ARRAY_IDX2]], align 8
; CHECK:  ret void
;
entry:
  %scalar29 = extractelement <8 x i32> %payloadHeader, i32 0
  %scalar26 = extractelement <3 x i32> %enqueuedLocalSize, i32 0
  %scalar19 = extractelement <8 x i32> %r0, i32 1
  %mul.i.i.i = mul i32 %scalar26, %scalar19
  %localIdX4 = zext i16 %localIdX to i32
  %add.i.i.i = add i32 %mul.i.i.i, %localIdX4
  %add4.i.i.i = add i32 %add.i.i.i, %scalar29
  %conv.i.i.i = zext i32 %add4.i.i.i to i64
  %arrayidx = getelementptr inbounds double, double addrspace(1)* %inA, i64 %conv.i.i.i
  %0 = load double, double addrspace(1)* %arrayidx, align 8
  %sqrt = call double @llvm.sqrt.f64(double %0)
  %arrayidx2 = getelementptr inbounds double, double addrspace(1)* %out, i64 %conv.i.i.i
  store double %sqrt, double addrspace(1)* %arrayidx2, align 8
  ret void
}
