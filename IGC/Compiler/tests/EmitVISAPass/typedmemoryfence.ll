;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers -platformmtl -igc-emit-visa -simd-mode 32 %s -regkey DumpVISAASMToConsole | FileCheck %s

; This test verifies whether GenISA.typedmemoryfence intrinsic is properly translated and sampler flush is not emitted.

; The tested module has been reduced from the following OpenCL C code:
; kernel void test()
; {
;     atomic_work_item_fence(CLK_IMAGE_MEM_FENCE, memory_order_acq_rel, memory_scope_work_item);
; }

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test(<8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
; CHECK: lsc_fence.tgm.evict
; CHECK-NOT: sampler_cache_flush
  call void @llvm.genx.GenISA.typedmemoryfence(i1 true)
  ret void
}

; Function Attrs: convergent nounwind
declare void @llvm.genx.GenISA.typedmemoryfence(i1) #0

attributes #0 = { convergent nounwind }

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!spirv.Generator = !{!2}
!igc.functions = !{!3}
!IGCMetadata = !{!9}
!opencl.ocl.version = !{!310, !310, !310, !310, !310}
!opencl.spir.version = !{!310, !310, !310, !310, !310}
!llvm.ident = !{!311, !311, !311, !311, !311}
!llvm.module.flags = !{!312}
!printf.strings = !{}

!0 = !{i32 2, i32 2}
!1 = !{i32 3, i32 300000}
!2 = !{i16 6, i16 14}
!3 = !{void (<8 x i32>, <8 x i32>)* @test, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8}
!7 = !{i32 0}
!8 = !{i32 1}
!9 = !{!"ModuleMD", !10, !11, !90, !159, !190, !206, !228, !238, !240, !241, !255, !256, !257, !258, !262, !263, !270, !271, !272, !273, !274, !275, !276, !277, !278, !279, !280, !282, !286, !287, !288, !289, !290, !291, !292, !293, !294, !295, !296, !140, !297, !300, !301, !303, !306, !307, !308}
!10 = !{!"isPrecise", i1 false}
!11 = !{!"compOpt", !12, !13, !14, !15, !16, !17, !18, !19, !20, !21, !22, !23, !24, !25, !26, !27, !28, !29, !30, !31, !32, !33, !34, !35, !36, !37, !38, !39, !40, !41, !42, !43, !44, !45, !46, !47, !48, !49, !50, !51, !52, !53, !54, !55, !56, !57, !58, !59, !60, !61, !62, !63, !64, !65, !66, !67, !68, !69, !70, !71, !72, !73, !74, !75, !76, !77, !78, !79, !80, !81, !82, !83, !84, !85, !86, !87, !88, !89}
!12 = !{!"DenormsAreZero", i1 false}
!13 = !{!"BFTFDenormsAreZero", i1 false}
!14 = !{!"CorrectlyRoundedDivSqrt", i1 false}
!15 = !{!"OptDisable", i1 false}
!16 = !{!"MadEnable", i1 true}
!17 = !{!"NoSignedZeros", i1 false}
!18 = !{!"NoNaNs", i1 false}
!19 = !{!"FloatRoundingMode", i32 0}
!20 = !{!"FloatCvtIntRoundingMode", i32 3}
!21 = !{!"LoadCacheDefault", i32 4}
!22 = !{!"StoreCacheDefault", i32 2}
!23 = !{!"VISAPreSchedRPThreshold", i32 0}
!24 = !{!"SetLoopUnrollThreshold", i32 0}
!25 = !{!"UnsafeMathOptimizations", i1 false}
!26 = !{!"disableCustomUnsafeOpts", i1 false}
!27 = !{!"disableReducePow", i1 false}
!28 = !{!"disableSqrtOpt", i1 false}
!29 = !{!"FiniteMathOnly", i1 false}
!30 = !{!"FastRelaxedMath", i1 false}
!31 = !{!"DashGSpecified", i1 false}
!32 = !{!"FastCompilation", i1 false}
!33 = !{!"UseScratchSpacePrivateMemory", i1 true}
!34 = !{!"RelaxedBuiltins", i1 false}
!35 = !{!"SubgroupIndependentForwardProgressRequired", i1 true}
!36 = !{!"GreaterThan2GBBufferRequired", i1 true}
!37 = !{!"GreaterThan4GBBufferRequired", i1 true}
!38 = !{!"DisableA64WA", i1 false}
!39 = !{!"ForceEnableA64WA", i1 false}
!40 = !{!"PushConstantsEnable", i1 true}
!41 = !{!"HasPositivePointerOffset", i1 false}
!42 = !{!"HasBufferOffsetArg", i1 true}
!43 = !{!"BufferOffsetArgOptional", i1 true}
!44 = !{!"replaceGlobalOffsetsByZero", i1 false}
!45 = !{!"forcePixelShaderSIMDMode", i32 0}
!46 = !{!"pixelShaderDoNotAbortOnSpill", i1 false}
!47 = !{!"UniformWGS", i1 false}
!48 = !{!"disableVertexComponentPacking", i1 false}
!49 = !{!"disablePartialVertexComponentPacking", i1 false}
!50 = !{!"PreferBindlessImages", i1 false}
!51 = !{!"UseBindlessMode", i1 false}
!52 = !{!"UseLegacyBindlessMode", i1 true}
!53 = !{!"disableMathRefactoring", i1 false}
!54 = !{!"atomicBranch", i1 false}
!55 = !{!"spillCompression", i1 false}
!56 = !{!"DisableEarlyOut", i1 false}
!57 = !{!"ForceInt32DivRemEmu", i1 false}
!58 = !{!"ForceInt32DivRemEmuSP", i1 false}
!59 = !{!"DisableFastestSingleCSSIMD", i1 false}
!60 = !{!"DisableFastestLinearScan", i1 false}
!61 = !{!"UseStatelessforPrivateMemory", i1 false}
!62 = !{!"EnableTakeGlobalAddress", i1 false}
!63 = !{!"IsLibraryCompilation", i1 false}
!64 = !{!"LibraryCompileSIMDSize", i32 0}
!65 = !{!"FastVISACompile", i1 false}
!66 = !{!"MatchSinCosPi", i1 false}
!67 = !{!"ExcludeIRFromZEBinary", i1 false}
!68 = !{!"EmitZeBinVISASections", i1 false}
!69 = !{!"FP64GenEmulationEnabled", i1 false}
!70 = !{!"FP64GenConvEmulationEnabled", i1 false}
!71 = !{!"allowDisableRematforCS", i1 false}
!72 = !{!"DisableIncSpillCostAllAddrTaken", i1 false}
!73 = !{!"DisableCPSOmaskWA", i1 false}
!74 = !{!"DisableFastestGopt", i1 false}
!75 = !{!"WaForceHalfPromotionComputeShader", i1 false}
!76 = !{!"WaForceHalfPromotionPixelVertexShader", i1 false}
!77 = !{!"DisableConstantCoalescing", i1 false}
!78 = !{!"EnableUndefAlphaOutputAsRed", i1 true}
!79 = !{!"WaEnableALTModeVisaWA", i1 false}
!80 = !{!"NewSpillCostFunction", i1 false}
!81 = !{!"ForceLargeGRFNum4RQ", i1 false}
!82 = !{!"DisableEUFusion", i1 false}
!83 = !{!"DisableFDivToFMulInvOpt", i1 false}
!84 = !{!"initializePhiSampleSourceWA", i1 false}
!85 = !{!"WaDisableSubspanUseNoMaskForCB", i1 false}
!86 = !{!"DisableLoosenSimd32Occu", i1 false}
!87 = !{!"FastestS1Options", i32 0}
!88 = !{!"EnableFastestForWaveIntrinsicsCS", i1 false}
!89 = !{!"ForceLinearWalkOnLinearUAV", i1 false}
!90 = !{!"FuncMD", !91, !92}
!91 = !{!"FuncMDMap[0]", void (<8 x i32>, <8 x i32>)* @test}
!92 = !{!"FuncMDValue[0]", !93, !94, !98, !99, !100, !121, !132, !133, !134, !135, !136, !137, !138, !139, !140, !141, !142, !143, !144, !145, !146, !147, !148, !149, !150, !151, !152, !153, !154, !155}
!93 = !{!"localOffsets"}
!94 = !{!"workGroupWalkOrder", !95, !96, !97}
!95 = !{!"dim0", i32 0}
!96 = !{!"dim1", i32 1}
!97 = !{!"dim2", i32 2}
!98 = !{!"funcArgs"}
!99 = !{!"functionType", !"KernelFunction"}
!100 = !{!"rtInfo", !101, !102, !103, !104, !105, !106, !107, !108, !109, !110, !111, !112, !113, !114, !115, !116, !120}
!101 = !{!"callableShaderType", !"NumberOfCallableShaderTypes"}
!102 = !{!"isContinuation", i1 false}
!103 = !{!"hasTraceRayPayload", i1 false}
!104 = !{!"hasHitAttributes", i1 false}
!105 = !{!"hasCallableData", i1 false}
!106 = !{!"ShaderStackSize", i32 0}
!107 = !{!"ShaderHash", i64 0}
!108 = !{!"ShaderName", !""}
!109 = !{!"ParentName", !""}
!110 = !{!"SlotNum", i1* null}
!111 = !{!"NOSSize", i32 0}
!112 = !{!"globalRootSignatureSize", i32 0}
!113 = !{!"Entries"}
!114 = !{!"SpillUnions"}
!115 = !{!"CustomHitAttrSizeInBytes", i32 0}
!116 = !{!"Types", !117, !118, !119}
!117 = !{!"FrameStartTys"}
!118 = !{!"ArgumentTys"}
!119 = !{!"FullFrameTys"}
!120 = !{!"Aliases"}
!121 = !{!"resAllocMD", !122, !123, !124, !125, !131}
!122 = !{!"uavsNumType", i32 0}
!123 = !{!"srvsNumType", i32 0}
!124 = !{!"samplersNumType", i32 0}
!125 = !{!"argAllocMDList", !126, !130}
!126 = !{!"argAllocMDListVec[0]", !127, !128, !129}
!127 = !{!"type", i32 0}
!128 = !{!"extensionType", i32 -1}
!129 = !{!"indexType", i32 -1}
!130 = !{!"argAllocMDListVec[1]", !127, !128, !129}
!131 = !{!"inlineSamplersMD"}
!132 = !{!"maxByteOffsets"}
!133 = !{!"IsInitializer", i1 false}
!134 = !{!"IsFinalizer", i1 false}
!135 = !{!"CompiledSubGroupsNumber", i32 0}
!136 = !{!"hasInlineVmeSamplers", i1 false}
!137 = !{!"localSize", i32 0}
!138 = !{!"localIDPresent", i1 false}
!139 = !{!"groupIDPresent", i1 false}
!140 = !{!"privateMemoryPerWI", i32 0}
!141 = !{!"prevFPOffset", i32 0}
!142 = !{!"globalIDPresent", i1 false}
!143 = !{!"hasSyncRTCalls", i1 false}
!144 = !{!"hasNonKernelArgLoad", i1 false}
!145 = !{!"hasNonKernelArgStore", i1 false}
!146 = !{!"hasNonKernelArgAtomic", i1 false}
!147 = !{!"UserAnnotations"}
!148 = !{!"m_OpenCLArgAddressSpaces"}
!149 = !{!"m_OpenCLArgAccessQualifiers"}
!150 = !{!"m_OpenCLArgTypes"}
!151 = !{!"m_OpenCLArgBaseTypes"}
!152 = !{!"m_OpenCLArgTypeQualifiers"}
!153 = !{!"m_OpenCLArgNames"}
!154 = !{!"m_OpenCLArgScalarAsPointers"}
!155 = !{!"m_OptsToDisablePerFunc", !156, !157, !158}
!156 = !{!"m_OptsToDisablePerFuncSet[0]", !"IGC-AddressArithmeticSinking"}
!157 = !{!"m_OptsToDisablePerFuncSet[1]", !"IGC-AllowSimd32Slicing"}
!158 = !{!"m_OptsToDisablePerFuncSet[2]", !"IGC-SinkLoadOpt"}
!159 = !{!"pushInfo", !160, !161, !162, !166, !167, !168, !169, !170, !171, !172, !173, !186, !187, !188, !189}
!160 = !{!"pushableAddresses"}
!161 = !{!"bindlessPushInfo"}
!162 = !{!"dynamicBufferInfo", !163, !164, !165}
!163 = !{!"firstIndex", i32 0}
!164 = !{!"numOffsets", i32 0}
!165 = !{!"forceDisabled", i1 false}
!166 = !{!"MaxNumberOfPushedBuffers", i32 0}
!167 = !{!"inlineConstantBufferSlot", i32 -1}
!168 = !{!"inlineConstantBufferOffset", i32 -1}
!169 = !{!"inlineConstantBufferGRFOffset", i32 -1}
!170 = !{!"constants"}
!171 = !{!"inputs"}
!172 = !{!"constantReg"}
!173 = !{!"simplePushInfoArr", !174, !183, !184, !185}
!174 = !{!"simplePushInfoArrVec[0]", !175, !176, !177, !178, !179, !180, !181, !182}
!175 = !{!"cbIdx", i32 0}
!176 = !{!"pushableAddressGrfOffset", i32 -1}
!177 = !{!"pushableOffsetGrfOffset", i32 -1}
!178 = !{!"offset", i32 0}
!179 = !{!"size", i32 0}
!180 = !{!"isStateless", i1 false}
!181 = !{!"isBindless", i1 false}
!182 = !{!"simplePushLoads"}
!183 = !{!"simplePushInfoArrVec[1]", !175, !176, !177, !178, !179, !180, !181, !182}
!184 = !{!"simplePushInfoArrVec[2]", !175, !176, !177, !178, !179, !180, !181, !182}
!185 = !{!"simplePushInfoArrVec[3]", !175, !176, !177, !178, !179, !180, !181, !182}
!186 = !{!"simplePushBufferUsed", i32 0}
!187 = !{!"pushAnalysisWIInfos"}
!188 = !{!"inlineRTGlobalPtrOffset", i32 0}
!189 = !{!"rtSyncSurfPtrOffset", i32 0}
!190 = !{!"psInfo", !191, !192, !193, !194, !195, !196, !197, !198, !199, !200, !201, !202, !203, !204, !205}
!191 = !{!"BlendStateDisabledMask", i8 0}
!192 = !{!"SkipSrc0Alpha", i1 false}
!193 = !{!"DualSourceBlendingDisabled", i1 false}
!194 = !{!"ForceEnableSimd32", i1 false}
!195 = !{!"outputDepth", i1 false}
!196 = !{!"outputStencil", i1 false}
!197 = !{!"outputMask", i1 false}
!198 = !{!"blendToFillEnabled", i1 false}
!199 = !{!"forceEarlyZ", i1 false}
!200 = !{!"hasVersionedLoop", i1 false}
!201 = !{!"forceSingleSourceRTWAfterDualSourceRTW", i1 false}
!202 = !{!"NumSamples", i8 0}
!203 = !{!"blendOptimizationMode"}
!204 = !{!"colorOutputMask"}
!205 = !{!"WaDisableVRS", i1 false}
!206 = !{!"csInfo", !207, !208, !209, !210, !211, !23, !24, !212, !213, !214, !215, !216, !217, !218, !219, !220, !221, !222, !223, !54, !55, !224, !225, !226, !227}
!207 = !{!"maxWorkGroupSize", i32 0}
!208 = !{!"waveSize", i32 0}
!209 = !{!"ComputeShaderSecondCompile"}
!210 = !{!"forcedSIMDSize", i8 0}
!211 = !{!"forceTotalGRFNum", i32 0}
!212 = !{!"forceSpillCompression", i1 false}
!213 = !{!"allowLowerSimd", i1 false}
!214 = !{!"disableSimd32Slicing", i1 false}
!215 = !{!"disableSplitOnSpill", i1 false}
!216 = !{!"enableNewSpillCostFunction", i1 false}
!217 = !{!"forcedVISAPreRAScheduler", i1 false}
!218 = !{!"forceUniformBuffer", i1 false}
!219 = !{!"forceUniformSurfaceSampler", i1 false}
!220 = !{!"disableLocalIdOrderOptimizations", i1 false}
!221 = !{!"disableDispatchAlongY", i1 false}
!222 = !{!"neededThreadIdLayout", i1* null}
!223 = !{!"forceTileYWalk", i1 false}
!224 = !{!"disableEarlyOut", i1 false}
!225 = !{!"walkOrderEnabled", i1 false}
!226 = !{!"walkOrderOverride", i32 0}
!227 = !{!"ResForHfPacking"}
!228 = !{!"msInfo", !229, !230, !231, !232, !233, !234, !235, !236, !237}
!229 = !{!"PrimitiveTopology", i32 3}
!230 = !{!"MaxNumOfPrimitives", i32 0}
!231 = !{!"MaxNumOfVertices", i32 0}
!232 = !{!"MaxNumOfPerPrimitiveOutputs", i32 0}
!233 = !{!"MaxNumOfPerVertexOutputs", i32 0}
!234 = !{!"WorkGroupSize", i32 0}
!235 = !{!"WorkGroupMemorySizeInBytes", i32 0}
!236 = !{!"IndexFormat", i32 6}
!237 = !{!"SubgroupSize", i32 0}
!238 = !{!"taskInfo", !239, !234, !235, !237}
!239 = !{!"MaxNumOfOutputs", i32 0}
!240 = !{!"NBarrierCnt", i32 0}
!241 = !{!"rtInfo", !242, !243, !244, !245, !246, !247, !248, !249, !250, !251, !252, !253, !254}
!242 = !{!"RayQueryAllocSizeInBytes", i32 0}
!243 = !{!"NumContinuations", i32 0}
!244 = !{!"RTAsyncStackAddrspace", i32 -1}
!245 = !{!"RTAsyncStackSurfaceStateOffset", i1* null}
!246 = !{!"SWHotZoneAddrspace", i32 -1}
!247 = !{!"SWHotZoneSurfaceStateOffset", i1* null}
!248 = !{!"SWStackAddrspace", i32 -1}
!249 = !{!"SWStackSurfaceStateOffset", i1* null}
!250 = !{!"RTSyncStackAddrspace", i32 -1}
!251 = !{!"RTSyncStackSurfaceStateOffset", i1* null}
!252 = !{!"doSyncDispatchRays", i1 false}
!253 = !{!"MemStyle", !"Xe"}
!254 = !{!"GlobalDataStyle", !"Xe"}
!255 = !{!"CurUniqueIndirectIdx", i32 0}
!256 = !{!"inlineDynTextures"}
!257 = !{!"inlineResInfoData"}
!258 = !{!"immConstant", !259, !260, !261}
!259 = !{!"data"}
!260 = !{!"sizes"}
!261 = !{!"zeroIdxs"}
!262 = !{!"stringConstants"}
!263 = !{!"inlineBuffers", !264, !268, !269}
!264 = !{!"inlineBuffersVec[0]", !265, !266, !267}
!265 = !{!"alignment", i32 0}
!266 = !{!"allocSize", i64 0}
!267 = !{!"Buffer"}
!268 = !{!"inlineBuffersVec[1]", !265, !266, !267}
!269 = !{!"inlineBuffersVec[2]", !265, !266, !267}
!270 = !{!"GlobalPointerProgramBinaryInfos"}
!271 = !{!"ConstantPointerProgramBinaryInfos"}
!272 = !{!"GlobalBufferAddressRelocInfo"}
!273 = !{!"ConstantBufferAddressRelocInfo"}
!274 = !{!"forceLscCacheList"}
!275 = !{!"SrvMap"}
!276 = !{!"RasterizerOrderedByteAddressBuffer"}
!277 = !{!"RasterizerOrderedViews"}
!278 = !{!"MinNOSPushConstantSize", i32 0}
!279 = !{!"inlineProgramScopeOffsets"}
!280 = !{!"shaderData", !281}
!281 = !{!"numReplicas", i32 0}
!282 = !{!"URBInfo", !283, !284, !285}
!283 = !{!"has64BVertexHeaderInput", i1 false}
!284 = !{!"has64BVertexHeaderOutput", i1 false}
!285 = !{!"hasVertexHeader", i1 true}
!286 = !{!"UseBindlessImage", i1 false}
!287 = !{!"enableRangeReduce", i1 false}
!288 = !{!"allowMatchMadOptimizationforVS", i1 false}
!289 = !{!"disableMatchMadOptimizationForCS", i1 false}
!290 = !{!"disableMemOptforNegativeOffsetLoads", i1 false}
!291 = !{!"enableThreeWayLoadSpiltOpt", i1 false}
!292 = !{!"statefulResourcesNotAliased", i1 false}
!293 = !{!"disableMixMode", i1 false}
!294 = !{!"genericAccessesResolved", i1 false}
!295 = !{!"disableSeparateSpillPvtScratchSpace", i1 false}
!296 = !{!"disableSeparateScratchWA", i1 false}
!297 = !{!"PrivateMemoryPerFG", !298, !299}
!298 = !{!"PrivateMemoryPerFGMap[0]", void (<8 x i32>, <8 x i32>)* @test}
!299 = !{!"PrivateMemoryPerFGValue[0]", i32 0}
!300 = !{!"m_OptsToDisable"}
!301 = !{!"capabilities", !302}
!302 = !{!"globalVariableDecorationsINTEL", i1 false}
!303 = !{!"m_ShaderResourceViewMcsMask", !304, !305}
!304 = !{!"m_ShaderResourceViewMcsMaskVec[0]", i64 0}
!305 = !{!"m_ShaderResourceViewMcsMaskVec[1]", i64 0}
!306 = !{!"computedDepthMode", i32 0}
!307 = !{!"isHDCFastClearShader", i1 false}
!308 = !{!"argRegisterReservations", !309}
!309 = !{!"argRegisterReservationsVec[0]", i32 0}
!310 = !{i32 2, i32 0}
!311 = !{!"clang version 14.0.5"}
!312 = !{i32 1, !"wchar_size", i32 4}
!313 = !{!"80"}
!314 = !{!"-3"}
