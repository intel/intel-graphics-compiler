;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-private-mem-resolution --platformpvc -S < %s 2>&1 | FileCheck %s

define spir_kernel void @testallocasmall(i8* %privateBase) {
entry:
  %0 = alloca [100 x float], align 4
  ret void
; CHECK-LABEL: entry:
; CHECK:  [[simdLaneId16:%[A-z0-9]*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK:  [[simdLaneId:%[A-z0-9]*]] = zext i16 [[simdLaneId16]] to i32
; CHECK:  [[simdSize:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK:  [[totalPrivateMemPerThread:%[.A-z0-9]*]] = mul i32 [[simdSize]], 400
; CHECK:  [[CALL:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.hw.thread.id.alloca.i32()
; CHECK:  [[perThreadOffset:%[A-z0-9]*]] = mul i32 [[CALL]], [[totalPrivateMemPerThread]]
; CHECK:  [[SIMDBufferOffset:%[.A-z0-9]*]] = mul i32 [[simdSize]], 0
; CHECK:  [[bufferOffsetForThread:%[.A-z0-9]*]] = add i32 [[perThreadOffset]], [[SIMDBufferOffset]]
; CHECK:  [[perLaneOffset:%[A-z0-9]*]] = mul i32 [[simdLaneId]], 400
; CHECK:  [[totalOffset:%[.A-z0-9]*]] = add i32 [[bufferOffsetForThread]], [[perLaneOffset]]
; CHECK:  [[totalOffsetExt:%[.A-z0-9]*]] = zext i32 [[totalOffset]] to i64
; CHECK:  [[privateBufferGEP:%[.A-z0-9]*]] = getelementptr i8, i8* %privateBase, i64 [[totalOffsetExt]]
; CHECK:  [[privateBuffer:%[.A-z0-9]*]] = bitcast i8* [[privateBufferGEP]] to [100 x float]*
; CHECK:  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!1}

!0 = !{!"ModuleMD"}
!1 = !{void (i8*)* @testallocasmall, !2}
!2 = !{!3, !4}
!3 = !{!"function_type", i32 0}
!4 = !{!"implicit_arg_desc", !5}
!5 = !{i32 12}
