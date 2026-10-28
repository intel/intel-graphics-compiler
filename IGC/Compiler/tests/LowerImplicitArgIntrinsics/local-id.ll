;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -enable-debugify -igc-lower-implicit-arg-intrinsic -S 2>&1 < %s | FileCheck --check-prefixes=CHECK,CHECK-OPAQUE %s %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -enable-debugify -igc-lower-implicit-arg-intrinsic -S 2>&1 < %s | FileCheck --check-prefixes=CHECK,CHECK-TYPED %s %}
; ------------------------------------------------
; LowerImplicitArgIntrinsics
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_func void @test(i32 addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset) #0 {
; CHECK:  entry:
; CHECK:    [[TMP0:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId.i16()
; CHECK-TYPED:    [[DST_ADDR:%.*]] = alloca i32 addrspace(1)*
; CHECK-OPAQUE:    [[DST_ADDR:%.*]] = alloca ptr addrspace(1)
; CHECK:    [[X:%.*]] = alloca i32
; CHECK:    [[Y:%.*]] = alloca i32
; CHECK:    [[Z:%.*]] = alloca i32
; CHECK-TYPED:    store i32 addrspace(1)* [[DST:%.*]], i32 addrspace(1)** [[DST_ADDR]]
; CHECK-OPAQUE:    store ptr addrspace(1) [[DST:%.*]], ptr [[DST_ADDR]]
; CHECK-TYPED:    [[TMP1:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetLocalIdBufferPtr.p1i32()
; CHECK-OPAQUE:    [[TMP1:%.*]] = call ptr addrspace(1) @llvm.genx.GenISA.GetLocalIdBufferPtr.p1()
; CHECK-TYPED:    [[TMP2:%.*]] = ptrtoint i32 addrspace(1)* [[TMP1]] to i64
; CHECK-OPAQUE:    [[TMP2:%.*]] = ptrtoint ptr addrspace(1) [[TMP1]] to i64
; CHECK:    [[TMP3:%.*]] = mul i16 [[TMP0]], 6
; CHECK:    [[TMP4:%.*]] = zext i16 [[TMP3]] to i64
; CHECK:    [[TMP5:%.*]] = add i64 [[TMP4]], [[TMP2]]
; CHECK-TYPED:    [[TMP6:%.*]] = inttoptr i64 [[TMP5]] to i16 addrspace(1)*
; CHECK-OPAQUE:    [[TMP6:%.*]] = inttoptr i64 [[TMP5]] to ptr addrspace(1)
; CHECK-TYPED:    [[TMP7:%.*]] = load i16, i16 addrspace(1)* [[TMP6]]
; CHECK-OPAQUE:    [[TMP7:%.*]] = load i16, ptr addrspace(1) [[TMP6]]
; CHECK:    [[EXT1:%.*]] = zext i16 [[TMP7]] to i32
; CHECK-TYPED:    store i32 [[EXT1]], i32* [[X]]
; CHECK-OPAQUE:    store i32 [[EXT1]], ptr [[X]]
; CHECK-TYPED:    [[TMP8:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetLocalIdBufferPtr.p1i32()
; CHECK-OPAQUE:    [[TMP8:%.*]] = call ptr addrspace(1) @llvm.genx.GenISA.GetLocalIdBufferPtr.p1()
; CHECK-TYPED:    [[TMP9:%.*]] = ptrtoint i32 addrspace(1)* [[TMP8]] to i64
; CHECK-OPAQUE:    [[TMP9:%.*]] = ptrtoint ptr addrspace(1) [[TMP8]] to i64
; CHECK:    [[TMP10:%.*]] = mul i16 [[TMP0]], 6
; CHECK:    [[TMP11:%.*]] = add i16 [[TMP10]], 2
; CHECK:    [[TMP12:%.*]] = zext i16 [[TMP11]] to i64
; CHECK:    [[TMP13:%.*]] = add i64 [[TMP12]], [[TMP9]]
; CHECK-TYPED:    [[TMP14:%.*]] = inttoptr i64 [[TMP13]] to i16 addrspace(1)*
; CHECK-OPAQUE:    [[TMP14:%.*]] = inttoptr i64 [[TMP13]] to ptr addrspace(1)
; CHECK-TYPED:    [[TMP15:%.*]] = load i16, i16 addrspace(1)* [[TMP14]]
; CHECK-OPAQUE:    [[TMP15:%.*]] = load i16, ptr addrspace(1) [[TMP14]]
; CHECK:    [[EXT2:%.*]] = zext i16 [[TMP15]] to i32
; CHECK-TYPED:    store i32 [[EXT2]], i32* [[Y]]
; CHECK-OPAQUE:    store i32 [[EXT2]], ptr [[Y]]
; CHECK-TYPED:    [[TMP16:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetLocalIdBufferPtr.p1i32()
; CHECK-OPAQUE:    [[TMP16:%.*]] = call ptr addrspace(1) @llvm.genx.GenISA.GetLocalIdBufferPtr.p1()
; CHECK-TYPED:    [[TMP17:%.*]] = ptrtoint i32 addrspace(1)* [[TMP16]] to i64
; CHECK-OPAQUE:    [[TMP17:%.*]] = ptrtoint ptr addrspace(1) [[TMP16]] to i64
; CHECK:    [[TMP18:%.*]] = mul i16 [[TMP0]], 6
; CHECK:    [[TMP19:%.*]] = add i16 [[TMP18]], 4
; CHECK:    [[TMP20:%.*]] = zext i16 [[TMP19]] to i64
; CHECK:    [[TMP21:%.*]] = add i64 [[TMP20]], [[TMP17]]
; CHECK-TYPED:    [[TMP22:%.*]] = inttoptr i64 [[TMP21]] to i16 addrspace(1)*
; CHECK-OPAQUE:    [[TMP22:%.*]] = inttoptr i64 [[TMP21]] to ptr addrspace(1)
; CHECK-TYPED:    [[TMP23:%.*]] = load i16, i16 addrspace(1)* [[TMP22]]
; CHECK-OPAQUE:    [[TMP23:%.*]] = load i16, ptr addrspace(1) [[TMP22]]
; CHECK:    [[EXT3:%.*]] = zext i16 [[TMP23]] to i32
; CHECK-TYPED:    store i32 [[EXT3]], i32* [[Z]]
; CHECK-OPAQUE:    store i32 [[EXT3]], ptr [[Z]]
; CHECK-TYPED:    [[TMP24:%.*]] = load i32, i32* [[X]]
; CHECK-OPAQUE:    [[TMP24:%.*]] = load i32, ptr [[X]]
; CHECK-TYPED:    [[TMP25:%.*]] = load i32, i32* [[Y]]
; CHECK-OPAQUE:    [[TMP25:%.*]] = load i32, ptr [[Y]]
; CHECK:    [[ADD:%.*]] = add nsw i32 [[TMP24]], [[TMP25]]
; CHECK-TYPED:    [[TMP26:%.*]] = load i32, i32* [[Z]]
; CHECK-OPAQUE:    [[TMP26:%.*]] = load i32, ptr [[Z]]
; CHECK:    [[ADD5:%.*]] = add nsw i32 [[ADD]], [[TMP26]]
; CHECK-TYPED:    [[TMP27:%.*]] = load i32 addrspace(1)*, i32 addrspace(1)** [[DST_ADDR]]
; CHECK-OPAQUE:    [[TMP27:%.*]] = load ptr addrspace(1), ptr [[DST_ADDR]]
; CHECK-TYPED:    [[ARRAYIDX:%.*]] = getelementptr inbounds i32, i32 addrspace(1)* [[TMP27]], i64 0
; CHECK-OPAQUE:    [[ARRAYIDX:%.*]] = getelementptr inbounds i32, ptr addrspace(1) [[TMP27]], i64 0
; CHECK-TYPED:    store i32 [[ADD5]], i32 addrspace(1)* [[ARRAYIDX]]
; CHECK-OPAQUE:    store i32 [[ADD5]], ptr addrspace(1) [[ARRAYIDX]]
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
!4 = !{!5}
!5 = !{!"function_type", i32 2}
!15 = !{!"argId", i32 0}
!16 = !{!"implicitArgInfoListVec[0]", !15}
!17 = !{!"argId", i32 1}
!18 = !{!"implicitArgInfoListVec[1]", !17}
!19 = !{!"argId", i32 8}
!20 = !{!"implicitArgInfoListVec[2]", !19}
!21 = !{!"argId", i32 9}
!22 = !{!"implicitArgInfoListVec[3]", !21}
!23 = !{!"argId", i32 10}
!24 = !{!"implicitArgInfoListVec[4]", !23}
!25 = !{!"argId", i32 13}
!26 = !{!"implicitArgInfoListVec[5]", !25}
!27 = !{!"argId", i32 15}
!28 = !{!"explicitArgNum", i32 0}
!29 = !{!"implicitArgInfoListVec[6]", !27, !28}
!30 = !{!"implicitArgInfoList", !16, !18, !20, !22, !24, !26, !29}
!31 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i8*, i32)* @test}
!32 = !{!"FuncMDValue[0]", !30}
!33 = !{!"FuncMD", !31, !32}
!34 = !{!"ModuleMD", !33}
!IGCMetadata = !{!34}
