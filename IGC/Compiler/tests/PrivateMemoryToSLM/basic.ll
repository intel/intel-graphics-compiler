;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --typed-pointers --igc-move-private-memory-to-slm -dce -S < %s | FileCheck %s
; ------------------------------------------------
; PrivateMemoryToSLM
; ------------------------------------------------

@b = common addrspace(1) global i32 0, align 4

define spir_kernel void @test_slm(i32 addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset) #0 {
; CHECK-LABEL: @test_slm(
; CHECK:  entry:
; CHECK:    [[LOCALIDX1:%.*]] = zext i16 [[LOCALIDX:%.*]] to i32
; CHECK:    [[LOCALIDY2:%.*]] = zext i16 [[LOCALIDY:%.*]] to i32
; CHECK:    [[LOCALIDZ3:%.*]] = zext i16 [[LOCALIDZ:%.*]] to i32
; CHECK:    [[LOCALIDX4:%.*]] = zext i16 [[LOCALIDX]] to i32
; CHECK:    [[LOCALIDY5:%.*]] = zext i16 [[LOCALIDY]] to i32
; CHECK:    [[LOCALIDZ6:%.*]] = zext i16 [[LOCALIDZ]] to i32
; CHECK:    [[LOCALIDX7:%.*]] = zext i16 [[LOCALIDX]] to i32
; CHECK:    [[LOCALIDY8:%.*]] = zext i16 [[LOCALIDY]] to i32
; CHECK:    [[LOCALIDZ9:%.*]] = zext i16 [[LOCALIDZ]] to i32
; CHECK:    [[LOCALIDX10:%.*]] = zext i16 [[LOCALIDX]] to i32
; CHECK:    [[LOCALIDY11:%.*]] = zext i16 [[LOCALIDY]] to i32
; CHECK:    [[LOCALIDZ12:%.*]] = zext i16 [[LOCALIDZ]] to i32
; CHECK:    [[DST_ADDR_YOFFSET:%.*]] = mul i32 1, [[LOCALIDY11]]
; CHECK:    [[DST_ADDR_ZOFFSET:%.*]] = mul i32 2, [[LOCALIDZ12]]
; CHECK:    [[TMP0:%.*]] = add i32 [[DST_ADDR_YOFFSET]], [[DST_ADDR_ZOFFSET]]
; CHECK:    [[DST_ADDR_TOTALOFFSET:%.*]] = add i32 [[LOCALIDX10]], [[TMP0]]
; CHECK:    [[TMP1:%.*]] = getelementptr i32 addrspace(1)*, i32 addrspace(1)** addrspacecast (i32 addrspace(1)* addrspace(3)* getelementptr inbounds ([2 x i32 addrspace(1)*], [2 x i32 addrspace(1)*] addrspace(3)* @test_slm.dst.addr, i32 0, i32 0) to i32 addrspace(1)**), i32 [[DST_ADDR_TOTALOFFSET]]
; CHECK:    [[X_YOFFSET:%.*]] = mul i32 1, [[LOCALIDY2]]
; CHECK:    [[X_ZOFFSET:%.*]] = mul i32 2, [[LOCALIDZ3]]
; CHECK:    [[TMP2:%.*]] = add i32 [[X_YOFFSET]], [[X_ZOFFSET]]
; CHECK:    [[X_TOTALOFFSET:%.*]] = add i32 [[LOCALIDX1]], [[TMP2]]
; CHECK:    [[TMP3:%.*]] = getelementptr i32, i32* addrspacecast (i32 addrspace(3)* getelementptr inbounds ([2 x i32], [2 x i32] addrspace(3)* @test_slm.x, i32 0, i32 0) to i32*), i32 [[X_TOTALOFFSET]]
; CHECK:    [[Y_YOFFSET:%.*]] = mul i32 1, [[LOCALIDY5]]
; CHECK:    [[Y_ZOFFSET:%.*]] = mul i32 2, [[LOCALIDZ6]]
; CHECK:    [[TMP4:%.*]] = add i32 [[Y_YOFFSET]], [[Y_ZOFFSET]]
; CHECK:    [[Y_TOTALOFFSET:%.*]] = add i32 [[LOCALIDX4]], [[TMP4]]
; CHECK:    [[TMP5:%.*]] = getelementptr i32, i32* addrspacecast (i32 addrspace(3)* getelementptr inbounds ([2 x i32], [2 x i32] addrspace(3)* @test_slm.y, i32 0, i32 0) to i32*), i32 [[Y_TOTALOFFSET]]
; CHECK:    [[Z_YOFFSET:%.*]] = mul i32 1, [[LOCALIDY8]]
; CHECK:    [[Z_ZOFFSET:%.*]] = mul i32 2, [[LOCALIDZ9]]
; CHECK:    [[TMP6:%.*]] = add i32 [[Z_YOFFSET]], [[Z_ZOFFSET]]
; CHECK:    [[Z_TOTALOFFSET:%.*]] = add i32 [[LOCALIDX7]], [[TMP6]]
; CHECK:    [[TMP7:%.*]] = getelementptr i32, i32* addrspacecast (i32 addrspace(3)* getelementptr inbounds ([2 x i32], [2 x i32] addrspace(3)* @test_slm.z, i32 0, i32 0) to i32*), i32 [[Z_TOTALOFFSET]]
; CHECK:    store i32 addrspace(1)* [[DST:%.*]], i32 addrspace(1)** [[TMP1]], align 8
; CHECK:    store i32 1, i32* [[TMP3]], align 4
; CHECK:    store i32 2, i32* [[TMP5]], align 4
; CHECK:    store i32 3, i32* [[TMP7]], align 4
; CHECK:    ret void
;
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %x = alloca i32, align 4
  %y = alloca i32, align 4
  %z = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  store i32 1, i32* %x, align 4
  store i32 2, i32* %y, align 4
  store i32 3, i32* %z, align 4
  ret void
}


attributes #0 = { convergent noinline nounwind optnone }

!igc.functions = !{!3}

!3 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i8*, i32)* @test_slm, !4}
!4 = !{!5, !6, !15}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8, !9, !10, !11, !12, !13}
!7 = !{i32 0}
!8 = !{i32 1}
!9 = !{i32 8}
!10 = !{i32 9}
!11 = !{i32 10}
!12 = !{i32 13}
!13 = !{i32 15, !14}
!14 = !{!"explicit_arg_num", i32 0}
!15 = !{!"thread_group_size", i32 1, i32 2, i32 1}
