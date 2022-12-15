;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -enable-debugify -igc-lower-implicit-arg-intrinsic -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; LowerImplicitArgIntrinsics
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS
; XFAIL: *

define spir_func void @test(i32 addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset) #0 {
; CHECK-LABEL: @test(
; CHECK:  entry:
; CHECK:    [[DST_ADDR:%.*]] = alloca i32 addrspace(1)*, align 8
; CHECK:    [[X:%.*]] = alloca i32, align 4
; CHECK:    [[Y:%.*]] = alloca i32, align 4
; CHECK:    [[Z:%.*]] = alloca i32, align 4
; CHECK:    store i32 addrspace(1)* [[DST:%.*]], i32 addrspace(1)** [[DST_ADDR]], align 8
; CHECK:    [[TMP0:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId.i16()
; CHECK:    [[TMP1:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetLocalIdBufferPtr.p1i32()
; CHECK:    [[TMP2:%.*]] = ptrtoint i32 addrspace(1)* [[TMP1]] to i64
; CHECK:    [[TMP3:%.*]] = mul i16 [[TMP0]], 6
; CHECK:    [[TMP4:%.*]] = zext i16 [[TMP3]] to i64
; CHECK:    [[TMP5:%.*]] = add i64 [[TMP4]], [[TMP2]]
; CHECK:    [[TMP6:%.*]] = inttoptr i64 [[TMP5]] to i16 addrspace(1)*
; CHECK:    [[TMP7:%.*]] = load i16, i16 addrspace(1)* [[TMP6]]
; CHECK:    [[EXT1:%.*]] = zext i16 [[TMP7]] to i32
; CHECK:    store i32 [[EXT1]], i32* [[X]], align 4
; CHECK:    [[TMP8:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId.i16()
; CHECK:    [[TMP9:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetLocalIdBufferPtr.p1i32()
; CHECK:    [[TMP10:%.*]] = ptrtoint i32 addrspace(1)* [[TMP9]] to i64
; CHECK:    [[TMP11:%.*]] = mul i16 [[TMP8]], 6
; CHECK:    [[TMP12:%.*]] = add i16 [[TMP11]], 2
; CHECK:    [[TMP13:%.*]] = zext i16 [[TMP12]] to i64
; CHECK:    [[TMP14:%.*]] = add i64 [[TMP13]], [[TMP10]]
; CHECK:    [[TMP15:%.*]] = inttoptr i64 [[TMP14]] to i16 addrspace(1)*
; CHECK:    [[TMP16:%.*]] = load i16, i16 addrspace(1)* [[TMP15]]
; CHECK:    [[EXT2:%.*]] = zext i16 [[TMP16]] to i32
; CHECK:    store i32 [[EXT2]], i32* [[Y]], align 4
; CHECK:    [[TMP17:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId.i16()
; CHECK:    [[TMP18:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetLocalIdBufferPtr.p1i32()
; CHECK:    [[TMP19:%.*]] = ptrtoint i32 addrspace(1)* [[TMP18]] to i64
; CHECK:    [[TMP20:%.*]] = mul i16 [[TMP17]], 6
; CHECK:    [[TMP21:%.*]] = add i16 [[TMP20]], 4
; CHECK:    [[TMP22:%.*]] = zext i16 [[TMP21]] to i64
; CHECK:    [[TMP23:%.*]] = add i64 [[TMP22]], [[TMP19]]
; CHECK:    [[TMP24:%.*]] = inttoptr i64 [[TMP23]] to i16 addrspace(1)*
; CHECK:    [[TMP25:%.*]] = load i16, i16 addrspace(1)* [[TMP24]]
; CHECK:    [[EXT3:%.*]] = zext i16 [[TMP25]] to i32
; CHECK:    store i32 [[EXT3]], i32* [[Z]], align 4
; CHECK:    [[TMP26:%.*]] = load i32, i32* [[X]], align 4
; CHECK:    [[TMP27:%.*]] = load i32, i32* [[Y]], align 4
; CHECK:    [[ADD:%.*]] = add nsw i32 [[TMP26]], [[TMP27]]
; CHECK:    [[TMP28:%.*]] = load i32, i32* [[Z]], align 4
; CHECK:    [[ADD5:%.*]] = add nsw i32 [[ADD]], [[TMP28]]
; CHECK:    [[TMP29:%.*]] = load i32 addrspace(1)*, i32 addrspace(1)** [[DST_ADDR]], align 8
; CHECK:    [[ARRAYIDX:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* [[TMP29]], i64 0
; CHECK:    store i32 [[ADD5]], i32 addrspace(1)* [[ARRAYIDX]], align 4
; CHECK:    ret void
;
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %x = alloca i32, align 4
  %y = alloca i32, align 4
  %z = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  %call.i = call i16 @llvm.genx.GenISA.getLocalID.X()
  %ext1 = zext i16 %call.i to i32
  store i32 %ext1, i32* %x, align 4
  %call1.i = call i16 @llvm.genx.GenISA.getLocalID.Y()
  %ext2 = zext i16 %call1.i to i32
  store i32 %ext2, i32* %y, align 4
  %call2.i = call i16 @llvm.genx.GenISA.getLocalID.Z()
  %ext3 = zext i16 %call2.i to i32
  store i32 %ext3, i32* %z, align 4
  %0 = load i32, i32* %x, align 4
  %1 = load i32, i32* %y, align 4
  %add = add nsw i32 %0, %1
  %2 = load i32, i32* %z, align 4
  %add5 = add nsw i32 %add, %2
  %3 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %3, i64 0
  store i32 %add5, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

declare i16 @llvm.genx.GenISA.getLocalID.X()
declare i16 @llvm.genx.GenISA.getLocalID.Y()
declare i16 @llvm.genx.GenISA.getLocalID.Z()

attributes #0 = { convergent noinline nounwind optnone "visaStackCall" }

!igc.functions = !{!3}

!3 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i8*, i32)* @test, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 2}
!6 = !{!"implicit_arg_desc", !7, !8, !9, !10, !11, !12, !13}
!7 = !{i32 0}
!8 = !{i32 1}
!9 = !{i32 7}
!10 = !{i32 8}
!11 = !{i32 9}
!12 = !{i32 12}
!13 = !{i32 14, !14}
!14 = !{!"explicit_arg_num", i32 0}
