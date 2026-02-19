;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify -igc-lower-implicit-arg-intrinsic -S 2>&1 < %s | FileCheck %s
; ------------------------------------------------
; LowerImplicitArgIntrinsics
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_func void @test(i32 addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset) #0 {
; CHECK:  entry:
; CHECK:    [[TMP0:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId.i16()
; CHECK:    [[DST_ADDR:%.*]] = alloca i32 addrspace(1)*
; CHECK:    [[X:%.*]] = alloca i32
; CHECK:    [[Y:%.*]] = alloca i32
; CHECK:    [[Z:%.*]] = alloca i32
; CHECK:    store i32 addrspace(1)* [[DST:%.*]], i32 addrspace(1)** [[DST_ADDR]]
; CHECK:    [[TMP1:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetLocalIdBufferPtr.p1i32()
; CHECK:    [[TMP2:%.*]] = ptrtoint i32 addrspace(1)* [[TMP1]] to i64
; CHECK:    [[TMP3:%.*]] = mul i16 [[TMP0]], 6
; CHECK:    [[TMP4:%.*]] = zext i16 [[TMP3]] to i64
; CHECK:    [[TMP5:%.*]] = add i64 [[TMP4]], [[TMP2]]
; CHECK:    [[TMP6:%.*]] = inttoptr i64 [[TMP5]] to i16 addrspace(1)*
; CHECK:    [[TMP7:%.*]] = load i16, i16 addrspace(1)* [[TMP6]]
; CHECK:    [[EXT1:%.*]] = zext i16 [[TMP7]] to i32
; CHECK:    store i32 [[EXT1]], i32* [[X]]
; CHECK:    [[TMP8:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetLocalIdBufferPtr.p1i32()
; CHECK:    [[TMP9:%.*]] = ptrtoint i32 addrspace(1)* [[TMP8]] to i64
; CHECK:    [[TMP10:%.*]] = mul i16 [[TMP0]], 6
; CHECK:    [[TMP11:%.*]] = add i16 [[TMP10]], 2
; CHECK:    [[TMP12:%.*]] = zext i16 [[TMP11]] to i64
; CHECK:    [[TMP13:%.*]] = add i64 [[TMP12]], [[TMP9]]
; CHECK:    [[TMP14:%.*]] = inttoptr i64 [[TMP13]] to i16 addrspace(1)*
; CHECK:    [[TMP15:%.*]] = load i16, i16 addrspace(1)* [[TMP14]]
; CHECK:    [[EXT2:%.*]] = zext i16 [[TMP15]] to i32
; CHECK:    store i32 [[EXT2]], i32* [[Y]]
; CHECK:    [[TMP16:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetLocalIdBufferPtr.p1i32()
; CHECK:    [[TMP17:%.*]] = ptrtoint i32 addrspace(1)* [[TMP16]] to i64
; CHECK:    [[TMP18:%.*]] = mul i16 [[TMP0]], 6
; CHECK:    [[TMP19:%.*]] = add i16 [[TMP18]], 4
; CHECK:    [[TMP20:%.*]] = zext i16 [[TMP19]] to i64
; CHECK:    [[TMP21:%.*]] = add i64 [[TMP20]], [[TMP17]]
; CHECK:    [[TMP22:%.*]] = inttoptr i64 [[TMP21]] to i16 addrspace(1)*
; CHECK:    [[TMP23:%.*]] = load i16, i16 addrspace(1)* [[TMP22]]
; CHECK:    [[EXT3:%.*]] = zext i16 [[TMP23]] to i32
; CHECK:    store i32 [[EXT3]], i32* [[Z]]
; CHECK:    [[TMP24:%.*]] = load i32, i32* [[X]]
; CHECK:    [[TMP25:%.*]] = load i32, i32* [[Y]]
; CHECK:    [[ADD:%.*]] = add nsw i32 [[TMP24]], [[TMP25]]
; CHECK:    [[TMP26:%.*]] = load i32, i32* [[Z]]
; CHECK:    [[ADD5:%.*]] = add nsw i32 [[ADD]], [[TMP26]]
; CHECK:    [[TMP27:%.*]] = load i32 addrspace(1)*, i32 addrspace(1)** [[DST_ADDR]]
; CHECK:    [[ARRAYIDX:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* [[TMP27]], i64 0
; CHECK:    store i32 [[ADD5]], i32 addrspace(1)* [[ARRAYIDX]]
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
!9 = !{i32 8}
!10 = !{i32 9}
!11 = !{i32 10}
!12 = !{i32 13}
!13 = !{i32 15, !14}
!14 = !{!"explicit_arg_num", i32 0}
