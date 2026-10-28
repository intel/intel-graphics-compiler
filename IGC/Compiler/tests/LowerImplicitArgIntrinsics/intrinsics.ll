;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: %if llvm-16-plus %{ igc_opt --opaque-pointers -enable-debugify -igc-lower-implicit-arg-intrinsic -S < %s 2>&1 | FileCheck --check-prefixes=CHECK,CHECK-OPAQUE %s %}
; RUN: %if !llvm-17-plus %{ igc_opt --typed-pointers -enable-debugify -igc-lower-implicit-arg-intrinsic -S < %s 2>&1 | FileCheck --check-prefixes=CHECK,CHECK-TYPED %s %}
; ------------------------------------------------
; LowerImplicitArgIntrinsics
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define spir_func void @test(<3 x i32> addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset) #0 {
; CHECK-LABEL: @test(
; CHECK:  entry:
; CHECK-TYPED:    [[TMP0:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetImplicitBufferPtr.p1i32()
; CHECK-OPAQUE:    [[TMP0:%.*]] = call ptr addrspace(1) @llvm.genx.GenISA.GetImplicitBufferPtr.p1()
; CHECK-TYPED:    [[TMP1:%.*]] = ptrtoint i32 addrspace(1)* [[TMP0]] to i64
; CHECK-OPAQUE:    [[TMP1:%.*]] = ptrtoint ptr addrspace(1) [[TMP0]] to i64
; CHECK:    [[TMP2:%.*]] = add i64 [[TMP1]], 4
; CHECK-TYPED:    [[TMP3:%.*]] = inttoptr i64 [[TMP2]] to i32 addrspace(1)*
; CHECK-OPAQUE:    [[TMP3:%.*]] = inttoptr i64 [[TMP2]] to ptr addrspace(1)
; CHECK-TYPED:    [[TMP4:%.*]] = bitcast i32 addrspace(1)* [[TMP3]] to <3 x i32> addrspace(1)*
; CHECK-TYPED:    [[TMP5:%.*]] = load <3 x i32>, <3 x i32> addrspace(1)* [[TMP4]], align 4
; CHECK-OPAQUE:    [[TMP5:%.*]] = load <3 x i32>, ptr addrspace(1) [[TMP3]], align 4
; CHECK-TYPED:    store <3 x i32> [[TMP5]], <3 x i32> addrspace(1)* [[DST:%.*]], align 4
; CHECK-OPAQUE:    store <3 x i32> [[TMP5]], ptr addrspace(1) [[DST:%.*]], align 4

; CHECK-TYPED:    [[TMP6:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetImplicitBufferPtr.p1i32()
; CHECK-OPAQUE:    [[TMP6:%.*]] = call ptr addrspace(1) @llvm.genx.GenISA.GetImplicitBufferPtr.p1()
; CHECK-TYPED:    [[TMP7:%.*]] = ptrtoint i32 addrspace(1)* [[TMP6]] to i64
; CHECK-OPAQUE:    [[TMP7:%.*]] = ptrtoint ptr addrspace(1) [[TMP6]] to i64
; CHECK:    [[TMP8:%.*]] = add i64 [[TMP7]], 48
; CHECK-TYPED:    [[TMP9:%.*]] = inttoptr i64 [[TMP8]] to i32 addrspace(1)*
; CHECK-OPAQUE:    [[TMP9:%.*]] = inttoptr i64 [[TMP8]] to ptr addrspace(1)
; CHECK-TYPED:    [[TMP10:%.*]] = bitcast i32 addrspace(1)* [[TMP9]] to <3 x i64> addrspace(1)*
; CHECK-TYPED:    [[TMP11:%.*]] = load <3 x i64>, <3 x i64> addrspace(1)* [[TMP10]], align 8
; CHECK-OPAQUE:    [[TMP11:%.*]] = load <3 x i64>, ptr addrspace(1) [[TMP9]], align 8
; CHECK:    [[TMP12:%.*]] = extractelement <3 x i64> [[TMP11]], i64 0
; CHECK:    [[TMP13:%.*]] = trunc i64 [[TMP12]] to i32
; CHECK:    [[TMP14:%.*]] = insertelement <3 x i32> undef, i32 [[TMP13]], i64 0
; CHECK:    [[TMP15:%.*]] = extractelement <3 x i64> [[TMP11]], i64 1
; CHECK:    [[TMP16:%.*]] = trunc i64 [[TMP15]] to i32
; CHECK:    [[TMP17:%.*]] = insertelement <3 x i32> [[TMP14]], i32 [[TMP16]], i64 1
; CHECK:    [[TMP18:%.*]] = extractelement <3 x i64> [[TMP11]], i64 2
; CHECK:    [[TMP19:%.*]] = trunc i64 [[TMP18]] to i32
; CHECK:    [[TMP20:%.*]] = insertelement <3 x i32> [[TMP17]], i32 [[TMP19]], i64 2
; CHECK-TYPED:    store <3 x i32> [[TMP20]], <3 x i32> addrspace(1)* [[DST]], align 4
; CHECK-OPAQUE:    store <3 x i32> [[TMP20]], ptr addrspace(1) [[DST]], align 4

; CHECK-TYPED:    [[TMP21:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetImplicitBufferPtr.p1i32()
; CHECK-OPAQUE:    [[TMP21:%.*]] = call ptr addrspace(1) @llvm.genx.GenISA.GetImplicitBufferPtr.p1()
; CHECK-TYPED:    [[TMP22:%.*]] = ptrtoint i32 addrspace(1)* [[TMP21]] to i64
; CHECK-OPAQUE:    [[TMP22:%.*]] = ptrtoint ptr addrspace(1) [[TMP21]] to i64
; CHECK:    [[TMP23:%.*]] = add i64 [[TMP22]], 16
; CHECK-TYPED:    [[TMP24:%.*]] = inttoptr i64 [[TMP23]] to i32 addrspace(1)*
; CHECK-OPAQUE:    [[TMP24:%.*]] = inttoptr i64 [[TMP23]] to ptr addrspace(1)
; CHECK-TYPED:    [[TMP25:%.*]] = bitcast i32 addrspace(1)* [[TMP24]] to <3 x i64> addrspace(1)*
; CHECK-TYPED:    [[TMP26:%.*]] = load <3 x i64>, <3 x i64> addrspace(1)* [[TMP25]], align 8
; CHECK-OPAQUE:    [[TMP26:%.*]] = load <3 x i64>, ptr addrspace(1) [[TMP24]], align 8
; CHECK:    [[TMP27:%.*]] = extractelement <3 x i64> [[TMP26]], i64 0
; CHECK:    [[TMP28:%.*]] = trunc i64 [[TMP27]] to i32
; CHECK:    [[TMP29:%.*]] = insertelement <3 x i32> undef, i32 [[TMP28]], i64 0
; CHECK:    [[TMP30:%.*]] = extractelement <3 x i64> [[TMP26]], i64 1
; CHECK:    [[TMP31:%.*]] = trunc i64 [[TMP30]] to i32
; CHECK:    [[TMP32:%.*]] = insertelement <3 x i32> [[TMP29]], i32 [[TMP31]], i64 1
; CHECK:    [[TMP33:%.*]] = extractelement <3 x i64> [[TMP26]], i64 2
; CHECK:    [[TMP34:%.*]] = trunc i64 [[TMP33]] to i32
; CHECK:    [[TMP35:%.*]] = insertelement <3 x i32> [[TMP32]], i32 [[TMP34]], i64 2
; CHECK-TYPED:    store <3 x i32> [[TMP35]], <3 x i32> addrspace(1)* [[DST]], align 4
; CHECK-OPAQUE:    store <3 x i32> [[TMP35]], ptr addrspace(1) [[DST]], align 4

; CHECK-TYPED:    [[TMP36:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetImplicitBufferPtr.p1i32()
; CHECK-OPAQUE:    [[TMP36:%.*]] = call ptr addrspace(1) @llvm.genx.GenISA.GetImplicitBufferPtr.p1()
; CHECK-TYPED:    [[TMP37:%.*]] = ptrtoint i32 addrspace(1)* [[TMP36]] to i64
; CHECK-OPAQUE:    [[TMP37:%.*]] = ptrtoint ptr addrspace(1) [[TMP36]] to i64
; CHECK:    [[TMP38:%.*]] = add i64 [[TMP37]], 80
; CHECK-TYPED:    [[TMP39:%.*]] = inttoptr i64 [[TMP38]] to i32 addrspace(1)*
; CHECK-OPAQUE:    [[TMP39:%.*]] = inttoptr i64 [[TMP38]] to ptr addrspace(1)
; CHECK-TYPED:    [[TMP40:%.*]] = bitcast i32 addrspace(1)* [[TMP39]] to <3 x i32> addrspace(1)*
; CHECK-TYPED:    [[TMP41:%.*]] = load <3 x i32>, <3 x i32> addrspace(1)* [[TMP40]], align 4
; CHECK-OPAQUE:    [[TMP41:%.*]] = load <3 x i32>, ptr addrspace(1) [[TMP39]], align 4
; CHECK-TYPED:    store <3 x i32> [[TMP41]], <3 x i32> addrspace(1)* [[DST]], align 4
; CHECK-OPAQUE:    store <3 x i32> [[TMP41]], ptr addrspace(1) [[DST]], align 4

; CHECK-TYPED:    [[TMP42:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetImplicitBufferPtr.p1i32()
; CHECK-OPAQUE:    [[TMP42:%.*]] = call ptr addrspace(1) @llvm.genx.GenISA.GetImplicitBufferPtr.p1()
; CHECK-TYPED:    [[TMP43:%.*]] = ptrtoint i32 addrspace(1)* [[TMP42]] to i64
; CHECK-OPAQUE:    [[TMP43:%.*]] = ptrtoint ptr addrspace(1) [[TMP42]] to i64
; CHECK:    [[TMP44:%.*]] = add i64 [[TMP43]], 40
; CHECK-TYPED:    [[TMP45:%.*]] = inttoptr i64 [[TMP44]] to i32 addrspace(1)*
; CHECK-OPAQUE:    [[TMP45:%.*]] = inttoptr i64 [[TMP44]] to ptr addrspace(1)
; CHECK-TYPED:    [[TMP46:%.*]] = bitcast i32 addrspace(1)* [[TMP45]] to <1 x i64> addrspace(1)*
; CHECK-TYPED:    [[TMP47:%.*]] = load <1 x i64>, <1 x i64> addrspace(1)* [[TMP46]], align 8
; CHECK-OPAQUE:    [[TMP47:%.*]] = load <1 x i64>, ptr addrspace(1) [[TMP45]], align 8
; CHECK:    [[TMP48:%.*]] = bitcast <1 x i64> [[TMP47]] to i64
; CHECK-TYPED:    [[TMP49:%.*]] = inttoptr i64 [[TMP48]] to i32 addrspace(1)*
; CHECK-OPAQUE:    [[TMP49:%.*]] = inttoptr i64 [[TMP48]] to ptr addrspace(1)
; CHECK-TYPED:    [[TMP50:%.*]] = call i32 addrspace(1)* @llvm.genx.GenISA.GetImplicitBufferPtr.p1i32()
; CHECK-OPAQUE:    [[TMP50:%.*]] = call ptr addrspace(1) @llvm.genx.GenISA.GetImplicitBufferPtr.p1()
; CHECK-TYPED:    [[TMP51:%.*]] = ptrtoint i32 addrspace(1)* [[TMP50]] to i64
; CHECK-OPAQUE:    [[TMP51:%.*]] = ptrtoint ptr addrspace(1) [[TMP50]] to i64
; CHECK:    [[TMP52:%.*]] = add i64 [[TMP51]], 0
; CHECK-TYPED:    [[TMP53:%.*]] = inttoptr i64 [[TMP52]] to i32 addrspace(1)*
; CHECK-OPAQUE:    [[TMP53:%.*]] = inttoptr i64 [[TMP52]] to ptr addrspace(1)
; CHECK-TYPED:    [[TMP54:%.*]] = bitcast i32 addrspace(1)* [[TMP53]] to <1 x i32> addrspace(1)*
; CHECK-TYPED:    [[TMP55:%.*]] = load <1 x i32>, <1 x i32> addrspace(1)* [[TMP54]], align 4
; CHECK-OPAQUE:    [[TMP55:%.*]] = load <1 x i32>, ptr addrspace(1) [[TMP53]], align 4
; CHECK:    [[TMP56:%.*]] = bitcast <1 x i32> [[TMP55]] to i32
; CHECK:    [[TMP57:%.*]] = lshr i32 [[TMP56]], 16
; CHECK:    [[TMP58:%.*]] = and i32 [[TMP57]], 255
; CHECK-TYPED:    store i32 [[TMP58]], i32 addrspace(1)* [[TMP49]], align 4
; CHECK-OPAQUE:    store i32 [[TMP58]], ptr addrspace(1) [[TMP49]], align 4

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
!31 = !{!"FuncMDMap[0]", void (<3 x i32> addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i8*, i32)* @test}
!32 = !{!"FuncMDValue[0]", !30}
!33 = !{!"FuncMD", !31, !32}
!34 = !{!"ModuleMD", !33}
!IGCMetadata = !{!34}
