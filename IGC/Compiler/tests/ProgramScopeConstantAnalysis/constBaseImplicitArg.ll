;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; This is the "Unify_after_DeadCodeElimination.ll" of below ocl case:
; __constant float colorPalette[7 * 4] =
; {
;     0.0f, 0.0f, 0.0f, 1.0f,     // black
;     1.0f, 0.0f, 0.0f, 1.0f,     // red
;     1.0f, 1.0f, 0.0f, 1.0f,     // yellow
;     1.0f, 1.0f, 1.0f, 1.0f,     // white
;     1.0f, 1.0f, 0.0f, 1.0f,     // yellow
;     1.0f, 0.0f, 0.0f, 1.0f,     // red
;     0.0f, 0.0f, 0.0f, 1.0f      // black
; };
;
; __kernel void kernel1(__global int* a)
; { a[get_global_id(0)] = colorPalette[get_global_id(0)]; }

; The purpose of this test is to check that implicit kernel argument
; "ConstBase" is removed from PVC+ platforms.

; RUN: igc_opt --platformpvc --serialize-igc-metadata -igc-programscope-constant-analysis \
; RUN:    -igc-add-implicit-args -S < %s | FileCheck %s --check-prefix=PVC
; PVC-NOT: %constBase

; RUN: igc_opt --platformdg2 --serialize-igc-metadata -igc-programscope-constant-analysis \
; RUN:    -igc-add-implicit-args -S < %s | FileCheck %s --check-prefix=DG2
; DG2: %constBase

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

@colorPalette = addrspace(2) constant [28 x float] [float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 0.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 0.000000e+00, float 1.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 1.000000e+00], align 4

; Function Attrs: convergent nounwind
define spir_kernel void @kernel1(i32 addrspace(1)* %a) #0 {
entry:
  %call.i.i.i = call spir_func i32 @__builtin_IB_get_group_id(i32 0) #3
  %call1.i.i.i = call spir_func i32 @__builtin_IB_get_enqueued_local_size(i32 0) #3
  %mul.i.i.i = mul i32 %call1.i.i.i, %call.i.i.i
  %call.i.i.i.i = call spir_func i32 @__builtin_IB_get_local_id_x() #3
  %cmp11.i.i.i.i = icmp ult i32 %call.i.i.i.i, 65536
  call void @llvm.assume(i1 %cmp11.i.i.i.i) #2
  %add.i.i.i = add i32 %call.i.i.i.i, %mul.i.i.i
  %call3.i.i.i = call spir_func i32 @__builtin_IB_get_global_offset(i32 0) #3
  %add4.i.i.i = add i32 %add.i.i.i, %call3.i.i.i
  %conv.i.i.i = zext i32 %add4.i.i.i to i64
  %vecinit.i.i = insertelement <3 x i64> undef, i64 %conv.i.i.i, i32 0
  %call.i5.i.i = call spir_func i32 @__builtin_IB_get_group_id(i32 1) #3
  %call1.i6.i.i = call spir_func i32 @__builtin_IB_get_enqueued_local_size(i32 1) #3
  %mul.i7.i.i = mul i32 %call1.i6.i.i, %call.i5.i.i
  %call3.i.i.i.i = call spir_func i32 @__builtin_IB_get_local_id_y() #3
  %cmp11.i.i8.i.i = icmp ult i32 %call3.i.i.i.i, 65536
  call void @llvm.assume(i1 %cmp11.i.i8.i.i) #2
  %add.i9.i.i = add i32 %call3.i.i.i.i, %mul.i7.i.i
  %call3.i10.i.i = call spir_func i32 @__builtin_IB_get_global_offset(i32 1) #3
  %add4.i11.i.i = add i32 %add.i9.i.i, %call3.i10.i.i
  %conv.i12.i.i = zext i32 %add4.i11.i.i to i64
  %vecinit2.i.i = insertelement <3 x i64> %vecinit.i.i, i64 %conv.i12.i.i, i32 1
  %call.i13.i.i = call spir_func i32 @__builtin_IB_get_group_id(i32 2) #3
  %call1.i14.i.i = call spir_func i32 @__builtin_IB_get_enqueued_local_size(i32 2) #3
  %mul.i15.i.i = mul i32 %call1.i14.i.i, %call.i13.i.i
  %call7.i.i.i.i = call spir_func i32 @__builtin_IB_get_local_id_z() #3
  %cmp11.i.i16.i.i = icmp ult i32 %call7.i.i.i.i, 65536
  call void @llvm.assume(i1 %cmp11.i.i16.i.i) #2
  %add.i17.i.i = add i32 %call7.i.i.i.i, %mul.i15.i.i
  %call3.i18.i.i = call spir_func i32 @__builtin_IB_get_global_offset(i32 2) #3
  %add4.i19.i.i = add i32 %add.i17.i.i, %call3.i18.i.i
  %conv.i20.i.i = zext i32 %add4.i19.i.i to i64
  %vecinit4.i.i = insertelement <3 x i64> %vecinit2.i.i, i64 %conv.i20.i.i, i32 2
  %call = extractelement <3 x i64> %vecinit4.i.i, i32 0
  %arrayidx = getelementptr inbounds [28 x float], [28 x float] addrspace(2)* @colorPalette, i64 0, i64 %call
  %0 = load float, float addrspace(2)* %arrayidx, align 4
  %conv = fptosi float %0 to i32
  %call.i.i.i1 = call spir_func i32 @__builtin_IB_get_group_id(i32 0) #3
  %call1.i.i.i2 = call spir_func i32 @__builtin_IB_get_enqueued_local_size(i32 0) #3
  %mul.i.i.i3 = mul i32 %call1.i.i.i2, %call.i.i.i1
  %call.i.i.i.i4 = call spir_func i32 @__builtin_IB_get_local_id_x() #3
  %cmp11.i.i.i.i5 = icmp ult i32 %call.i.i.i.i4, 65536
  call void @llvm.assume(i1 %cmp11.i.i.i.i5) #2
  %add.i.i.i6 = add i32 %call.i.i.i.i4, %mul.i.i.i3
  %call3.i.i.i7 = call spir_func i32 @__builtin_IB_get_global_offset(i32 0) #3
  %add4.i.i.i8 = add i32 %add.i.i.i6, %call3.i.i.i7
  %conv.i.i.i9 = zext i32 %add4.i.i.i8 to i64
  %vecinit.i.i10 = insertelement <3 x i64> undef, i64 %conv.i.i.i9, i32 0
  %call.i5.i.i11 = call spir_func i32 @__builtin_IB_get_group_id(i32 1) #3
  %call1.i6.i.i12 = call spir_func i32 @__builtin_IB_get_enqueued_local_size(i32 1) #3
  %mul.i7.i.i13 = mul i32 %call1.i6.i.i12, %call.i5.i.i11
  %call3.i.i.i.i14 = call spir_func i32 @__builtin_IB_get_local_id_y() #3
  %cmp11.i.i8.i.i15 = icmp ult i32 %call3.i.i.i.i14, 65536
  call void @llvm.assume(i1 %cmp11.i.i8.i.i15) #2
  %add.i9.i.i16 = add i32 %call3.i.i.i.i14, %mul.i7.i.i13
  %call3.i10.i.i17 = call spir_func i32 @__builtin_IB_get_global_offset(i32 1) #3
  %add4.i11.i.i18 = add i32 %add.i9.i.i16, %call3.i10.i.i17
  %conv.i12.i.i19 = zext i32 %add4.i11.i.i18 to i64
  %vecinit2.i.i20 = insertelement <3 x i64> %vecinit.i.i10, i64 %conv.i12.i.i19, i32 1
  %call.i13.i.i21 = call spir_func i32 @__builtin_IB_get_group_id(i32 2) #3
  %call1.i14.i.i22 = call spir_func i32 @__builtin_IB_get_enqueued_local_size(i32 2) #3
  %mul.i15.i.i23 = mul i32 %call1.i14.i.i22, %call.i13.i.i21
  %call7.i.i.i.i24 = call spir_func i32 @__builtin_IB_get_local_id_z() #3
  %cmp11.i.i16.i.i25 = icmp ult i32 %call7.i.i.i.i24, 65536
  call void @llvm.assume(i1 %cmp11.i.i16.i.i25) #2
  %add.i17.i.i26 = add i32 %call7.i.i.i.i24, %mul.i15.i.i23
  %call3.i18.i.i27 = call spir_func i32 @__builtin_IB_get_global_offset(i32 2) #3
  %add4.i19.i.i28 = add i32 %add.i17.i.i26, %call3.i18.i.i27
  %conv.i20.i.i29 = zext i32 %add4.i19.i.i28 to i64
  %vecinit4.i.i30 = insertelement <3 x i64> %vecinit2.i.i20, i64 %conv.i20.i.i29, i32 2
  %call1 = extractelement <3 x i64> %vecinit4.i.i30, i32 0
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %a, i64 %call1
  store i32 %conv, i32 addrspace(1)* %arrayidx2, align 4
  ret void
}

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_group_id(i32) local_unnamed_addr #1

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_enqueued_local_size(i32) local_unnamed_addr #1

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_local_id_x() local_unnamed_addr #1

; Function Attrs: nounwind
declare void @llvm.assume(i1) #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_global_offset(i32) local_unnamed_addr #1

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_local_id_y() local_unnamed_addr #1

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_local_id_z() local_unnamed_addr #1

attributes #0 = { convergent nounwind "less-precise-fpmad"="true" }
attributes #1 = { convergent nounwind readnone "correctly-rounded-divide-sqrt-fp-math"="false" "denorms-are-zero"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind }
attributes #3 = { convergent nounwind readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!291}
!opencl.ocl.version = !{!301, !301, !301, !301, !301}
!opencl.spir.version = !{!301, !301, !301, !301, !301}
!llvm.module.flags = !{!303}

!0 = !{!"ModuleMD", !1, !2, !69, !133, !163, !164, !168, !169, !170, !199, !215, !227, !228, !229, !242, !243, !244, !245, !246, !247, !248, !249, !250, !251, !255, !256, !257, !258, !259, !260, !261, !262, !263, !264, !265, !266, !267, !268, !270, !274, !275, !276, !277, !278, !279, !280, !281, !118, !282, !283, !284, !286, !289, !290}
!1 = !{!"isPrecise", i1 false}
!2 = !{!"compOpt", !3, !4, !5, !6, !7, !8, !9, !10, !11, !12, !13, !14, !15, !16, !17, !18, !19, !20, !21, !22, !23, !24, !25, !26, !27, !28, !29, !30, !31, !32, !33, !34, !35, !36, !37, !38, !39, !40, !41, !42, !43, !44, !45, !46, !47, !48, !49, !50, !51, !52, !53, !54, !55, !56, !57, !58, !59, !60, !61, !62, !63, !64, !65, !66, !67, !68}
!3 = !{!"DenormsAreZero", i1 false}
!4 = !{!"BFTFDenormsAreZero", i1 false}
!5 = !{!"CorrectlyRoundedDivSqrt", i1 false}
!6 = !{!"OptDisable", i1 false}
!7 = !{!"MadEnable", i1 false}
!8 = !{!"NoSignedZeros", i1 false}
!9 = !{!"NoNaNs", i1 false}
!10 = !{!"FloatRoundingMode", i32 0}
!11 = !{!"FloatCvtIntRoundingMode", i32 3}
!12 = !{!"VISAPreSchedRPThreshold", i32 0}
!13 = !{!"SetLoopUnrollThreshold", i32 0}
!14 = !{!"UnsafeMathOptimizations", i1 false}
!15 = !{!"FiniteMathOnly", i1 false}
!16 = !{!"FastRelaxedMath", i1 false}
!17 = !{!"DashGSpecified", i1 false}
!18 = !{!"FastCompilation", i1 false}
!19 = !{!"UseScratchSpacePrivateMemory", i1 true}
!20 = !{!"RelaxedBuiltins", i1 false}
!21 = !{!"SubgroupIndependentForwardProgressRequired", i1 true}
!22 = !{!"GreaterThan2GBBufferRequired", i1 true}
!23 = !{!"GreaterThan4GBBufferRequired", i1 true}
!24 = !{!"DisableA64WA", i1 false}
!25 = !{!"ForceEnableA64WA", i1 false}
!26 = !{!"PushConstantsEnable", i1 true}
!27 = !{!"HasPositivePointerOffset", i1 false}
!28 = !{!"HasBufferOffsetArg", i1 false}
!29 = !{!"BufferOffsetArgOptional", i1 true}
!30 = !{!"HasSubDWAlignedPtrArg", i1 false}
!31 = !{!"replaceGlobalOffsetsByZero", i1 false}
!32 = !{!"forcePixelShaderSIMDMode", i32 0}
!33 = !{!"pixelShaderDoNotAbortOnSpill", i1 false}
!34 = !{!"UniformWGS", i1 false}
!35 = !{!"disableVertexComponentPacking", i1 false}
!36 = !{!"disablePartialVertexComponentPacking", i1 false}
!37 = !{!"PreferBindlessImages", i1 false}
!38 = !{!"UseBindlessMode", i1 false}
!39 = !{!"UseLegacyBindlessMode", i1 true}
!40 = !{!"disableMathRefactoring", i1 false}
!41 = !{!"atomicBranch", i1 false}
!42 = !{!"ForceInt32DivRemEmu", i1 false}
!43 = !{!"ForceInt32DivRemEmuSP", i1 false}
!44 = !{!"WaveIntrinsicUsed", i1 false}
!45 = !{!"DisableMultiPolyPS", i1 false}
!46 = !{!"NeedTexture3DLODWA", i1 false}
!47 = !{!"DisableFastestSingleCSSIMD", i1 false}
!48 = !{!"DisableFastestLinearScan", i1 false}
!49 = !{!"UseStatelessforPrivateMemory", i1 false}
!50 = !{!"EnableTakeGlobalAddress", i1 false}
!51 = !{!"IsLibraryCompilation", i1 false}
!52 = !{!"FastVISACompile", i1 false}
!53 = !{!"MatchSinCosPi", i1 false}
!54 = !{!"ExcludeIRFromZEBinary", i1 false}
!55 = !{!"EmitZeBinVISASections", i1 false}
!56 = !{!"FP64GenEmulationEnabled", i1 false}
!57 = !{!"allowDisableRematforCS", i1 false}
!58 = !{!"DisableIncSpillCostAllAddrTaken", i1 false}
!59 = !{!"DisableCPSOmaskWA", i1 false}
!60 = !{!"DisableFastestGopt", i1 false}
!61 = !{!"WaForceHalfPromotion", i1 false}
!62 = !{!"DisableConstantCoalescing", i1 false}
!63 = !{!"EnableUndefAlphaOutputAsRed", i1 true}
!64 = !{!"WaEnableALTModeVisaWA", i1 false}
!65 = !{!"WaEnableAtomicWaveFusion", i1 false}
!66 = !{!"NewSpillCostFunction", i1 false}
!67 = !{!"EnableVRT", i1 false}
!68 = !{!"DisableEUFusion", i1 false}
!69 = !{!"FuncMD", !70, !71}
!70 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*)* @kernel1}
!71 = !{!"FuncMDValue[0]", !72, !73, !77, !78, !79, !80, !81, !82, !83, !104, !110, !111, !112, !113, !114, !115, !116, !117, !118, !119, !120, !121, !122, !123, !124, !125, !126, !127, !128, !129, !130, !131, !132}
!72 = !{!"localOffsets"}
!73 = !{!"workGroupWalkOrder", !74, !75, !76}
!74 = !{!"dim0", i32 0}
!75 = !{!"dim1", i32 0}
!76 = !{!"dim2", i32 0}
!77 = !{!"funcArgs"}
!78 = !{!"functionType", !"KernelFunction"}
!79 = !{!"inlineDynConstants"}
!80 = !{!"inlineDynRootConstant"}
!81 = !{!"inlineDynConstantDescTable"}
!82 = !{!"m_pInterestingConstants"}
!83 = !{!"rtInfo", !84, !85, !86, !87, !88, !89, !90, !91, !92, !93, !94, !95, !96, !97, !98, !99, !103}
!84 = !{!"callableShaderType", !"NumberOfCallableShaderTypes"}
!85 = !{!"isContinuation", i1 false}
!86 = !{!"hasTraceRayPayload", i1 false}
!87 = !{!"hasHitAttributes", i1 false}
!88 = !{!"hasCallableData", i1 false}
!89 = !{!"ShaderStackSize", i32 0}
!90 = !{!"ShaderHash", i64 0}
!91 = !{!"ShaderName", !""}
!92 = !{!"ParentName", !""}
!93 = !{!"SlotNum", i1* null}
!94 = !{!"NOSSize", i32 0}
!95 = !{!"globalRootSignatureSize", i32 0}
!96 = !{!"Entries"}
!97 = !{!"SpillUnions"}
!98 = !{!"CustomHitAttrSizeInBytes", i32 0}
!99 = !{!"Types", !100, !101, !102}
!100 = !{!"FrameStartTys"}
!101 = !{!"ArgumentTys"}
!102 = !{!"FullFrameTys"}
!103 = !{!"Aliases"}
!104 = !{!"resAllocMD", !105, !106, !107, !108, !109}
!105 = !{!"uavsNumType", i32 0}
!106 = !{!"srvsNumType", i32 0}
!107 = !{!"samplersNumType", i32 0}
!108 = !{!"argAllocMDList"}
!109 = !{!"inlineSamplersMD"}
!110 = !{!"maxByteOffsets"}
!111 = !{!"IsInitializer", i1 false}
!112 = !{!"IsFinalizer", i1 false}
!113 = !{!"CompiledSubGroupsNumber", i32 0}
!114 = !{!"hasInlineVmeSamplers", i1 false}
!115 = !{!"localSize", i32 0}
!116 = !{!"localIDPresent", i1 false}
!117 = !{!"groupIDPresent", i1 false}
!118 = !{!"privateMemoryPerWI", i32 0}
!119 = !{!"globalIDPresent", i1 false}
!120 = !{!"hasSyncRTCalls", i1 false}
!121 = !{!"hasNonKernelArgLoad", i1 false}
!122 = !{!"hasNonKernelArgStore", i1 false}
!123 = !{!"hasNonKernelArgAtomic", i1 false}
!124 = !{!"UserAnnotations"}
!125 = !{!"m_OpenCLArgAddressSpaces"}
!126 = !{!"m_OpenCLArgAccessQualifiers"}
!127 = !{!"m_OpenCLArgTypes"}
!128 = !{!"m_OpenCLArgBaseTypes"}
!129 = !{!"m_OpenCLArgTypeQualifiers"}
!130 = !{!"m_OpenCLArgNames"}
!131 = !{!"m_OpenCLArgScalarAsPointers"}
!132 = !{!"m_OptsToDisablePerFunc"}
!133 = !{!"pushInfo", !134, !135, !136, !139, !140, !141, !142, !143, !144, !145, !146, !159, !160, !161, !162}
!134 = !{!"pushableAddresses"}
!135 = !{!"bindlessPushInfo"}
!136 = !{!"dynamicBufferInfo", !137, !138}
!137 = !{!"firstIndex", i32 0}
!138 = !{!"numOffsets", i32 0}
!139 = !{!"MaxNumberOfPushedBuffers", i32 0}
!140 = !{!"inlineConstantBufferSlot", i32 -1}
!141 = !{!"inlineConstantBufferOffset", i32 -1}
!142 = !{!"inlineConstantBufferGRFOffset", i32 -1}
!143 = !{!"constants"}
!144 = !{!"inputs"}
!145 = !{!"constantReg"}
!146 = !{!"simplePushInfoArr", !147, !156, !157, !158}
!147 = !{!"simplePushInfoArrVec[0]", !148, !149, !150, !151, !152, !153, !154, !155}
!148 = !{!"cbIdx", i32 0}
!149 = !{!"pushableAddressGrfOffset", i32 -1}
!150 = !{!"pushableOffsetGrfOffset", i32 -1}
!151 = !{!"offset", i32 0}
!152 = !{!"size", i32 0}
!153 = !{!"isStateless", i1 false}
!154 = !{!"isBindless", i1 false}
!155 = !{!"simplePushLoads"}
!156 = !{!"simplePushInfoArrVec[1]", !148, !149, !150, !151, !152, !153, !154, !155}
!157 = !{!"simplePushInfoArrVec[2]", !148, !149, !150, !151, !152, !153, !154, !155}
!158 = !{!"simplePushInfoArrVec[3]", !148, !149, !150, !151, !152, !153, !154, !155}
!159 = !{!"simplePushBufferUsed", i32 0}
!160 = !{!"pushAnalysisWIInfos"}
!161 = !{!"inlineRTGlobalPtrOffset", i32 0}
!162 = !{!"rtSyncSurfPtrOffset", i32 0}
!163 = !{!"WaEnableICBPromotion", i1 false}
!164 = !{!"vsInfo", !165, !166, !167}
!165 = !{!"DrawIndirectBufferIndex", i32 -1}
!166 = !{!"vertexReordering", i32 -1}
!167 = !{!"MaxNumOfOutputs", i32 0}
!168 = !{!"dsInfo", !167}
!169 = !{!"gsInfo", !167}
!170 = !{!"psInfo", !171, !172, !173, !174, !175, !176, !177, !178, !179, !180, !181, !182, !183, !184, !185, !186, !187, !188, !189, !190, !191, !192, !193, !194, !195, !196, !197, !198}
!171 = !{!"BlendStateDisabledMask", i8 0}
!172 = !{!"SkipSrc0Alpha", i1 false}
!173 = !{!"DualSourceBlendingDisabled", i1 false}
!174 = !{!"ForceEnableSimd32", i1 false}
!175 = !{!"outputDepth", i1 false}
!176 = !{!"outputStencil", i1 false}
!177 = !{!"outputMask", i1 false}
!178 = !{!"blendToFillEnabled", i1 false}
!179 = !{!"forceEarlyZ", i1 false}
!180 = !{!"hasVersionedLoop", i1 false}
!181 = !{!"forceSingleSourceRTWAfterDualSourceRTW", i1 false}
!182 = !{!"requestCPSizeRelevant", i1 false}
!183 = !{!"requestCPSize", i1 false}
!184 = !{!"texelMaskFastClearMode", !"Disabled"}
!185 = !{!"NumSamples", i8 0}
!186 = !{!"blendOptimizationMode"}
!187 = !{!"colorOutputMask"}
!188 = !{!"ProvokingVertexModeLast", i1 false}
!189 = !{!"VertexAttributesBypass", i1 false}
!190 = !{!"LegacyBaryAssignmentDisableLinear", i1 false}
!191 = !{!"LegacyBaryAssignmentDisableLinearNoPerspective", i1 false}
!192 = !{!"LegacyBaryAssignmentDisableLinearCentroid", i1 false}
!193 = !{!"LegacyBaryAssignmentDisableLinearNoPerspectiveCentroid", i1 false}
!194 = !{!"LegacyBaryAssignmentDisableLinearSample", i1 false}
!195 = !{!"LegacyBaryAssignmentDisableLinearNoPerspectiveSample", i1 false}
!196 = !{!"MeshShaderWAPerPrimitiveUserDataEnable", i1 false}
!197 = !{!"generatePatchesForRTWriteSends", i1 false}
!198 = !{!"WaDisableVRS", i1 false}
!199 = !{!"csInfo", !200, !201, !202, !203, !204, !12, !13, !205, !206, !207, !208, !209, !210, !211, !212, !41, !213, !214}
!200 = !{!"maxWorkGroupSize", i32 0}
!201 = !{!"waveSize", i32 0}
!202 = !{!"ComputeShaderSecondCompile"}
!203 = !{!"forcedSIMDSize", i8 0}
!204 = !{!"forceTotalGRFNum", i32 0}
!205 = !{!"allowLowerSimd", i1 false}
!206 = !{!"disableSimd32Slicing", i1 false}
!207 = !{!"disableSplitOnSpill", i1 false}
!208 = !{!"forcedVISAPreRAScheduler", i1 false}
!209 = !{!"disableLocalIdOrderOptimizations", i1 false}
!210 = !{!"disableDispatchAlongY", i1 false}
!211 = !{!"neededThreadIdLayout", i1* null}
!212 = !{!"forceTileYWalk", i1 false}
!213 = !{!"ResForHfPacking"}
!214 = !{!"hasWaveMatrix", i1 false}
!215 = !{!"msInfo", !216, !217, !218, !219, !220, !221, !222, !223, !224, !225, !196, !226}
!216 = !{!"PrimitiveTopology", i32 3}
!217 = !{!"MaxNumOfPrimitives", i32 0}
!218 = !{!"MaxNumOfVertices", i32 0}
!219 = !{!"MaxNumOfPerPrimitiveOutputs", i32 0}
!220 = !{!"MaxNumOfPerVertexOutputs", i32 0}
!221 = !{!"WorkGroupSize", i32 0}
!222 = !{!"WorkGroupMemorySizeInBytes", i32 0}
!223 = !{!"IndexFormat", i32 6}
!224 = !{!"SubgroupSize", i32 0}
!225 = !{!"VPandRTAIndexAutostripEnable", i1 false}
!226 = !{!"numPrimitiveAttributesPatchBaseName", !""}
!227 = !{!"taskInfo", !167, !221, !222, !224}
!228 = !{!"NBarrierCnt", i32 0}
!229 = !{!"rtInfo", !230, !231, !232, !233, !234, !235, !236, !237, !238, !239, !240, !241}
!230 = !{!"RayQueryAllocSizeInBytes", i32 0}
!231 = !{!"NumContinuations", i32 0}
!232 = !{!"RTAsyncStackAddrspace", i32 -1}
!233 = !{!"RTAsyncStackSurfaceStateOffset", i1* null}
!234 = !{!"SWHotZoneAddrspace", i32 -1}
!235 = !{!"SWHotZoneSurfaceStateOffset", i1* null}
!236 = !{!"SWStackAddrspace", i32 -1}
!237 = !{!"SWStackSurfaceStateOffset", i1* null}
!238 = !{!"RTSyncStackAddrspace", i32 -1}
!239 = !{!"RTSyncStackSurfaceStateOffset", i1* null}
!240 = !{!"doSyncDispatchRays", i1 false}
!241 = !{!"MemStyle", !"Xe"}
!242 = !{!"EnableTextureIndirection", i1 false}
!243 = !{!"EnableSamplerIndirection", i1 false}
!244 = !{!"samplerStateStride", i32 0}
!245 = !{!"samplerStateOffset", i32 0}
!246 = !{!"textureStateStride", i32 0}
!247 = !{!"textureStateOffset", i32 0}
!248 = !{!"CurUniqueIndirectIdx", i32 0}
!249 = !{!"inlineDynTextures"}
!250 = !{!"inlineResInfoData"}
!251 = !{!"immConstant", !252, !253, !254}
!252 = !{!"data"}
!253 = !{!"sizes"}
!254 = !{!"zeroIdxs"}
!255 = !{!"stringConstants"}
!256 = !{!"inlineConstantBuffers"}
!257 = !{!"inlineGlobalBuffers"}
!258 = !{!"GlobalPointerProgramBinaryInfos"}
!259 = !{!"ConstantPointerProgramBinaryInfos"}
!260 = !{!"GlobalBufferAddressRelocInfo"}
!261 = !{!"ConstantBufferAddressRelocInfo"}
!262 = !{!"forceLscCacheList"}
!263 = !{!"SrvMap"}
!264 = !{!"RootConstantBufferOffsetInBytes"}
!265 = !{!"RasterizerOrderedByteAddressBuffer"}
!266 = !{!"MinNOSPushConstantSize", i32 0}
!267 = !{!"inlineProgramScopeOffsets"}
!268 = !{!"shaderData", !269}
!269 = !{!"numReplicas", i32 0}
!270 = !{!"URBInfo", !271, !272, !273}
!271 = !{!"has64BVertexHeaderInput", i1 false}
!272 = !{!"has64BVertexHeaderOutput", i1 false}
!273 = !{!"hasVertexHeader", i1 true}
!274 = !{!"UseBindlessImage", i1 false}
!275 = !{!"enableRangeReduce", i1 false}
!276 = !{!"disableNewTrigFuncRangeReduction", i1 false}
!277 = !{!"allowMatchMadOptimizationforVS", i1 false}
!278 = !{!"disableMemOptforNegativeOffsetLoads", i1 false}
!279 = !{!"enableThreeWayLoadSpiltOpt", i1 false}
!280 = !{!"statefulResourcesNotAliased", i1 false}
!281 = !{!"disableMixMode", i1 false}
!282 = !{!"PrivateMemoryPerFG"}
!283 = !{!"m_OptsToDisable"}
!284 = !{!"capabilities", !285}
!285 = !{!"globalVariableDecorationsINTEL", i1 false}
!286 = !{!"m_ShaderResourceViewMcsMask", !287, !288}
!287 = !{!"m_ShaderResourceViewMcsMaskVec[0]", i64 0}
!288 = !{!"m_ShaderResourceViewMcsMaskVec[1]", i64 0}
!289 = !{!"computedDepthMode", i32 0}
!290 = !{!"isHDCFastClearShader", i1 false}
!291 = !{void (i32 addrspace(1)*)* @kernel1, !292}
!292 = !{!293, !294}
!293 = !{!"function_type", i32 0}
!294 = !{!"implicit_arg_desc", !295, !296, !297, !298, !299, !300}
!295 = !{i32 0}
!296 = !{i32 1}
!297 = !{i32 7}
!298 = !{i32 8}
!299 = !{i32 9}
!300 = !{i32 6}
!301 = !{i32 2, i32 0}
!303 = !{i32 1, !"wchar_size", i32 4}
