;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify -igc-lower-implicit-arg-intrinsic -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; LowerImplicitArgIntrinsics
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_func void @test(<3 x i32> addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset) #0 {
; CHECK-LABEL: @test(
; CHECK:  entry:
; CHECK:    [[TMP0:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetImplicitBufferPtr.p1i32()
; CHECK:    [[TMP1:%.*]] = ptrtoint i32 addrspace(1)* [[TMP0]] to i64
; CHECK:    [[TMP2:%.*]] = add i64 [[TMP1]], 4
; CHECK:    [[TMP3:%.*]] = inttoptr i64 [[TMP2]] to i32 addrspace(1)*
; CHECK:    [[TMP4:%.*]] = bitcast i32 addrspace(1)* [[TMP3]] to <3 x i32> addrspace(1)*
; CHECK:    [[TMP5:%.*]] = load <3 x i32>, <3 x i32> addrspace(1)* [[TMP4]], align 4
; CHECK:    store <3 x i32> [[TMP5]], <3 x i32> addrspace(1)* [[DST:%.*]], align 4

; CHECK:    [[TMP6:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetImplicitBufferPtr.p1i32()
; CHECK:    [[TMP7:%.*]] = ptrtoint i32 addrspace(1)* [[TMP6]] to i64
; CHECK:    [[TMP8:%.*]] = add i64 [[TMP7]], 48
; CHECK:    [[TMP9:%.*]] = inttoptr i64 [[TMP8]] to i32 addrspace(1)*
; CHECK:    [[TMP10:%.*]] = bitcast i32 addrspace(1)* [[TMP9]] to <3 x i64> addrspace(1)*
; CHECK:    [[TMP11:%.*]] = load <3 x i64>, <3 x i64> addrspace(1)* [[TMP10]], align 8
; CHECK:    [[TMP12:%.*]] = extractelement <3 x i64> [[TMP11]], i64 0
; CHECK:    [[TMP13:%.*]] = trunc i64 [[TMP12]] to i32
; CHECK:    [[TMP14:%.*]] = insertelement <3 x i32> undef, i32 [[TMP13]], i64 0
; CHECK:    [[TMP15:%.*]] = extractelement <3 x i64> [[TMP11]], i64 1
; CHECK:    [[TMP16:%.*]] = trunc i64 [[TMP15]] to i32
; CHECK:    [[TMP17:%.*]] = insertelement <3 x i32> [[TMP14]], i32 [[TMP16]], i64 1
; CHECK:    [[TMP18:%.*]] = extractelement <3 x i64> [[TMP11]], i64 2
; CHECK:    [[TMP19:%.*]] = trunc i64 [[TMP18]] to i32
; CHECK:    [[TMP20:%.*]] = insertelement <3 x i32> [[TMP17]], i32 [[TMP19]], i64 2
; CHECK:    store <3 x i32> [[TMP20]], <3 x i32> addrspace(1)* [[DST]], align 4

; CHECK:    [[TMP21:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetImplicitBufferPtr.p1i32()
; CHECK:    [[TMP22:%.*]] = ptrtoint i32 addrspace(1)* [[TMP21]] to i64
; CHECK:    [[TMP23:%.*]] = add i64 [[TMP22]], 16
; CHECK:    [[TMP24:%.*]] = inttoptr i64 [[TMP23]] to i32 addrspace(1)*
; CHECK:    [[TMP25:%.*]] = bitcast i32 addrspace(1)* [[TMP24]] to <3 x i64> addrspace(1)*
; CHECK:    [[TMP26:%.*]] = load <3 x i64>, <3 x i64> addrspace(1)* [[TMP25]], align 8
; CHECK:    [[TMP27:%.*]] = extractelement <3 x i64> [[TMP26]], i64 0
; CHECK:    [[TMP28:%.*]] = trunc i64 [[TMP27]] to i32
; CHECK:    [[TMP29:%.*]] = insertelement <3 x i32> undef, i32 [[TMP28]], i64 0
; CHECK:    [[TMP30:%.*]] = extractelement <3 x i64> [[TMP26]], i64 1
; CHECK:    [[TMP31:%.*]] = trunc i64 [[TMP30]] to i32
; CHECK:    [[TMP32:%.*]] = insertelement <3 x i32> [[TMP29]], i32 [[TMP31]], i64 1
; CHECK:    [[TMP33:%.*]] = extractelement <3 x i64> [[TMP26]], i64 2
; CHECK:    [[TMP34:%.*]] = trunc i64 [[TMP33]] to i32
; CHECK:    [[TMP35:%.*]] = insertelement <3 x i32> [[TMP32]], i32 [[TMP34]], i64 2
; CHECK:    store <3 x i32> [[TMP35]], <3 x i32> addrspace(1)* [[DST]], align 4

; CHECK:    [[TMP36:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetImplicitBufferPtr.p1i32()
; CHECK:    [[TMP37:%.*]] = ptrtoint i32 addrspace(1)* [[TMP36]] to i64
; CHECK:    [[TMP38:%.*]] = add i64 [[TMP37]], 80
; CHECK:    [[TMP39:%.*]] = inttoptr i64 [[TMP38]] to i32 addrspace(1)*
; CHECK:    [[TMP40:%.*]] = bitcast i32 addrspace(1)* [[TMP39]] to <3 x i32> addrspace(1)*
; CHECK:    [[TMP41:%.*]] = load <3 x i32>, <3 x i32> addrspace(1)* [[TMP40]], align 4
; CHECK:    store <3 x i32> [[TMP41]], <3 x i32> addrspace(1)* [[DST]], align 4

; CHECK:    [[TMP42:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetImplicitBufferPtr.p1i32()
; CHECK:    [[TMP43:%.*]] = ptrtoint i32 addrspace(1)* [[TMP42]] to i64
; CHECK:    [[TMP44:%.*]] = add i64 [[TMP43]], 40
; CHECK:    [[TMP45:%.*]] = inttoptr i64 [[TMP44]] to i32 addrspace(1)*
; CHECK:    [[TMP46:%.*]] = bitcast i32 addrspace(1)* [[TMP45]] to <1 x i64> addrspace(1)*
; CHECK:    [[TMP47:%.*]] = load <1 x i64>, <1 x i64> addrspace(1)* [[TMP46]], align 8
; CHECK:    [[TMP48:%.*]] = bitcast <1 x i64> [[TMP47]] to i64
; CHECK:    [[TMP49:%.*]] = inttoptr i64 [[TMP48]] to i32 addrspace(1)*
; CHECK:    [[TMP50:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetImplicitBufferPtr.p1i32()
; CHECK:    [[TMP51:%.*]] = ptrtoint i32 addrspace(1)* [[TMP50]] to i64
; CHECK:    [[TMP52:%.*]] = add i64 [[TMP51]], 0
; CHECK:    [[TMP53:%.*]] = inttoptr i64 [[TMP52]] to i32 addrspace(1)*
; CHECK:    [[TMP54:%.*]] = bitcast i32 addrspace(1)* [[TMP53]] to <1 x i32> addrspace(1)*
; CHECK:    [[TMP55:%.*]] = load <1 x i32>, <1 x i32> addrspace(1)* [[TMP54]], align 4
; CHECK:    [[TMP56:%.*]] = bitcast <1 x i32> [[TMP55]] to i32
; CHECK:    [[TMP57:%.*]] = lshr i32 [[TMP56]], 16
; CHECK:    [[TMP58:%.*]] = and i32 [[TMP57]], 255
; CHECK:    store i32 [[TMP58]], i32 addrspace(1)* [[TMP49]], align 4

entry:
  %0 = call <3 x i32> @llvm.genx.GenISA.getLocalSize.v4i32()
  store <3 x i32> %0, <3 x i32> addrspace(1)* %dst, align 4
  %1 = call <3 x i32> @llvm.genx.GenISA.getPayloadHeader.v4i32()
  store <3 x i32> %1, <3 x i32> addrspace(1)* %dst, align 4
  %2 = call <3 x i32> @llvm.genx.GenISA.getGlobalSize.v4i32()
  store <3 x i32> %2, <3 x i32> addrspace(1)* %dst, align 4
  %3 = call <3 x i32> @llvm.genx.GenISA.getNumWorkGroups.v4i32()
  store <3 x i32> %3, <3 x i32> addrspace(1)* %dst, align 4
  %4 = call i32 addrspace(1)* @llvm.genx.GenISA.getPrintfBuffer.p1i32()
  %5 = call i32 @llvm.genx.GenISA.getWorkDim.i32()
  store i32 %5, i32 addrspace(1)* %4, align 4
  ret void
}

declare <3 x i32> @llvm.genx.GenISA.getLocalSize.v4i32()

declare <3 x i32> @llvm.genx.GenISA.getPayloadHeader.v4i32()

declare <3 x i32> @llvm.genx.GenISA.getGlobalSize.v4i32()

declare <3 x i32> @llvm.genx.GenISA.getNumWorkGroups.v4i32()

declare i32 @llvm.genx.GenISA.getWorkDim.i32()

declare i32 addrspace(1)* @llvm.genx.GenISA.getPrintfBuffer.p1i32()

attributes #0 = { convergent noinline nounwind optnone "visaStackCall" }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }

!igc.functions = !{!3}

!3 = !{void (<3 x i32> addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i8*, i32)* @test, !4}
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
