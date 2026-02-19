;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers %s -S -o - --igc-private-mem-resolution --platformdg2 | FileCheck %s

; The test checks if all instructions are generated in the correct order.
; after callling getPrivateBase() and getR0() intrinsics.
; getImplicitArgValue() can move insertion point, the test verify it.

define void @test( i32 %src, i32 %bufferOffset, i32 %bufferOffset1) #0 {
; CHECK-LABEL: @test(
; CHECK: [[PRIV_BASE:%.*]] = call i8* @llvm.genx.GenISA.getPrivateBase.p0i8()
; CHECK: [[R0:%.*]] = call <8 x i32> @llvm.genx.GenISA.getR0.v8i32()
; CHECK: [[SIMD_LANEID:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId()
; CHECK: [[ZEXT:%.*]] = zext i16 [[SIMD_LANEID]] to i32
; CHECK: [[SIMD_SIZE:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; CHECK: [[THREAD_ID:%.*]] = call i32 @llvm.genx.GenISA.hw.thread.id.alloca.i32()
; CHECK: [[MUL:%.*]] = mul i32 [[SIMD_SIZE]], 4
; CHECK: [[MUL_1:%.*]] = mul i32 [[THREAD_ID]], [[MUL]]

  %bb = alloca i32, align 4
  %call = call <8 x i32> @llvm.genx.GenISA.getR0.v8i32()
  store i32 %src, i32* %bb, align 4
  ret void
}

declare <8 x i32> @llvm.genx.GenISA.getR0.v8i32() #0

attributes #0 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" }

!IGCMetadata = !{!3}
!igc.functions = !{!17}

!3 = !{!"ModuleMD", !4, !7, !13, !10}
!4 = !{!"compOpt", !5, !6}
!5 = !{!"OptDisable", i1 true}
!6 = !{!"UseScratchSpacePrivateMemory", i1 true}
!7 = !{!"FuncMD", !11, !12}
!10 = !{!"privateMemoryPerWI", i32 0}
!11 = !{!"FuncMDMap[0]", void (i32, i32, i32)* @test}
!12 = !{!"FuncMDValue[0]", !10}
!13 = !{!"MinNOSPushConstantSize", i32 0}
!17 = !{void (i32, i32, i32)* @test, !18}
!18 = !{!19, !20}
!19 = !{!"function_type", i32 1}
!20 = !{!"implicit_arg_desc", !24, !26}
!21 = !{i32 0}
!22 = !{i32 1}
!23 = !{i32 12}
!24 = !{i32 14, !25}
!25 = !{!"explicit_arg_num", i32 0}
!26 = !{i32 14, !27}
!27 = !{!"explicit_arg_num", i32 1}
