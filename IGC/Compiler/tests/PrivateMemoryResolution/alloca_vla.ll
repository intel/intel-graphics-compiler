;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
; RUN: igc_opt --typed-pointers --regkey EnablePrivMemNewSOATranspose=0 --igc-private-mem-resolution -S < %s 2>&1 | FileCheck %s

define spir_kernel void @test_alloca_vla(i64, i8* %privateBase) #0 {
; CHECK-LABEL: @test_alloca_vla
; CHECK:  [[simdLaneId16:%[A-z0-9]*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK:  [[simdLaneId:%[A-z0-9]*]] = zext i16 [[simdLaneId16]] to i32
; CHECK:  [[simdSize:%[A-z0-9]*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK:  [[TruncVLASize:%[A-z0-9]*]] = trunc i64 %0 to i32
; CHECK:  [[VLASizeWithType:%[A-z0-9]*]] = mul i32 [[TruncVLASize]], 8
; CHECK:  [[VLAPerLaneOffset:%[A-z0-9]*]] = mul i32 [[simdLaneId]], [[VLASizeWithType]]
; CHECK:  [[VLAStackAlloca:%[A-z0-9]*]] = call i8* @llvm.genx.GenISA.VLAStackAlloca(i32 [[VLAPerLaneOffset]], i32 [[VLASizeWithType]])
; CHECK:  [[privateBuffer:%[.A-z0-9]*]] = bitcast i8* [[VLAStackAlloca]] to i64*
; CHECK:  ret void
entry:
  %1 = alloca i64, i64 %0, align 8
  ret void
}

attributes #0 = { convergent noinline nounwind "hasVLA" }

!IGCMetadata = !{!0}
!igc.functions = !{!5}

!0 = !{!"ModuleMD", !1, !2}
!1 = !{!"FuncMD", !3, !4}
!2 = !{!"privateMemoryPerWI", i32 0}
!3 = !{!"FuncMDMap[0]", void (i64, i8*)* @test_alloca_vla}
!4 = !{!"FuncMDValue[0]", !2}
!5 = !{void (i64, i8*)* @test_alloca_vla, !6}
!6 = !{!7, !8}
!7 = !{!"function_type", i32 0}
!8 = !{!"implicit_arg_desc", !9}
!9 = !{i32 13}
