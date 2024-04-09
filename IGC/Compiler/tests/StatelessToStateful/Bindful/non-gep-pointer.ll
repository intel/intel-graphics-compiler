;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt %s -S -o - -igc-stateless-to-stateful-resolution | FileCheck %s

; This test was generated from the following OpenCL C kernel:
; kernel void test(global char* in, global char* out)
; {
;     out[0] = in[0];
; }

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @test(i8 addrspace(1)* %in, i8 addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, i32 %bufferOffset, i32 %bufferOffset1) #0 {
entry:
; CHECK: [[SRC:%.*]] = inttoptr i32 %bufferOffset to i8 addrspace(131072)*
; CHECK: [[VAL:%.*]] = load i8, i8 addrspace(131072)* [[SRC]], align 1
  %0 = load i8, i8 addrspace(1)* %in, align 1
; CHECK: [[DST:%.*]] = inttoptr i32 %bufferOffset1 to i8 addrspace(131073)*
; CHECK: store i8 [[VAL]], i8 addrspace(131073)* [[DST]], align 1
  store i8 %0, i8 addrspace(1)* %out, align 1
  ret void
}

attributes #0 = { convergent nounwind "less-precise-fpmad"="true" }

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!spirv.Generator = !{!2}
!igc.functions = !{!3}
!IGCMetadata = !{!13}
!opencl.ocl.version = !{!327, !327, !327, !327, !327}
!opencl.spir.version = !{!327, !327, !327, !327, !327}
!llvm.ident = !{!328, !328, !328, !328, !328}
!llvm.module.flags = !{!329}

!0 = !{i32 2, i32 2}
!1 = !{i32 3, i32 102000}
!2 = !{i16 6, i16 14}
!3 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, <8 x i32>, <8 x i32>, i32, i32)* @test, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8, !9, !11}
!7 = !{i32 0}
!8 = !{i32 1}
!9 = !{i32 14, !10}
!10 = !{!"explicit_arg_num", i32 0}
!11 = !{i32 14, !12}
!12 = !{!"explicit_arg_num", i32 1}
!13 = !{!"ModuleMD", !14, !15, !90, !178, !209, !225, !246, !256, !258, !259, !272, !273, !274, !275, !279, !280, !287, !288, !289, !290, !291, !292, !293, !294, !295, !296, !297, !299, !303, !304, !305, !306, !307, !308, !309, !310, !311, !312, !313, !147, !314, !317, !318, !320, !323, !324, !325}
!14 = !{!"isPrecise", i1 false}
!15 = !{!"compOpt", !16, !17, !18, !19, !20, !21, !22, !23, !24, !25, !26, !27, !28, !29, !30, !31, !32, !33, !34, !35, !36, !37, !38, !39, !40, !41, !42, !43, !44, !45, !46, !47, !48, !49, !50, !51, !52, !53, !54, !55, !56, !57, !58, !59, !60, !61, !62, !63, !64, !65, !66, !67, !68, !69, !70, !71, !72, !73, !74, !75, !76, !77, !78, !79, !80, !81, !82, !83, !84, !85, !86, !87, !88, !89}
!16 = !{!"DenormsAreZero", i1 false}
!17 = !{!"BFTFDenormsAreZero", i1 false}
!18 = !{!"CorrectlyRoundedDivSqrt", i1 false}
!19 = !{!"OptDisable", i1 false}
!20 = !{!"MadEnable", i1 true}
!21 = !{!"NoSignedZeros", i1 false}
!22 = !{!"NoNaNs", i1 false}
!23 = !{!"FloatRoundingMode", i32 0}
!24 = !{!"FloatCvtIntRoundingMode", i32 3}
!25 = !{!"LoadCacheDefault", i32 4}
!26 = !{!"StoreCacheDefault", i32 7}
!27 = !{!"VISAPreSchedRPThreshold", i32 0}
!28 = !{!"SetLoopUnrollThreshold", i32 0}
!29 = !{!"UnsafeMathOptimizations", i1 false}
!30 = !{!"disableCustomUnsafeOpts", i1 false}
!31 = !{!"disableReducePow", i1 false}
!32 = !{!"disableSqrtOpt", i1 false}
!33 = !{!"FiniteMathOnly", i1 false}
!34 = !{!"FastRelaxedMath", i1 false}
!35 = !{!"DashGSpecified", i1 false}
!36 = !{!"FastCompilation", i1 false}
!37 = !{!"UseScratchSpacePrivateMemory", i1 false}
!38 = !{!"RelaxedBuiltins", i1 false}
!39 = !{!"SubgroupIndependentForwardProgressRequired", i1 true}
!40 = !{!"GreaterThan2GBBufferRequired", i1 true}
!41 = !{!"GreaterThan4GBBufferRequired", i1 false}
!42 = !{!"DisableA64WA", i1 false}
!43 = !{!"ForceEnableA64WA", i1 false}
!44 = !{!"PushConstantsEnable", i1 true}
!45 = !{!"HasPositivePointerOffset", i1 false}
!46 = !{!"HasBufferOffsetArg", i1 true}
!47 = !{!"BufferOffsetArgOptional", i1 true}
!48 = !{!"replaceGlobalOffsetsByZero", i1 false}
!49 = !{!"forcePixelShaderSIMDMode", i32 0}
!50 = !{!"pixelShaderDoNotAbortOnSpill", i1 false}
!51 = !{!"UniformWGS", i1 false}
!52 = !{!"disableVertexComponentPacking", i1 false}
!53 = !{!"disablePartialVertexComponentPacking", i1 false}
!54 = !{!"PreferBindlessImages", i1 false}
!55 = !{!"UseBindlessMode", i1 false}
!56 = !{!"UseLegacyBindlessMode", i1 true}
!57 = !{!"disableMathRefactoring", i1 false}
!58 = !{!"atomicBranch", i1 false}
!59 = !{!"spillCompression", i1 false}
!60 = !{!"ForceInt32DivRemEmu", i1 false}
!61 = !{!"ForceInt32DivRemEmuSP", i1 false}
!62 = !{!"DisableFastestSingleCSSIMD", i1 false}
!63 = !{!"DisableFastestLinearScan", i1 false}
!64 = !{!"UseStatelessforPrivateMemory", i1 false}
!65 = !{!"EnableTakeGlobalAddress", i1 false}
!66 = !{!"IsLibraryCompilation", i1 false}
!67 = !{!"LibraryCompileSIMDSize", i32 0}
!68 = !{!"FastVISACompile", i1 false}
!69 = !{!"MatchSinCosPi", i1 false}
!70 = !{!"ExcludeIRFromZEBinary", i1 false}
!71 = !{!"EmitZeBinVISASections", i1 false}
!72 = !{!"FP64GenEmulationEnabled", i1 false}
!73 = !{!"allowDisableRematforCS", i1 false}
!74 = !{!"DisableIncSpillCostAllAddrTaken", i1 false}
!75 = !{!"DisableCPSOmaskWA", i1 false}
!76 = !{!"DisableFastestGopt", i1 false}
!77 = !{!"WaForceHalfPromotionComputeShader", i1 false}
!78 = !{!"WaForceHalfPromotionPixelVertexShader", i1 false}
!79 = !{!"DisableConstantCoalescing", i1 false}
!80 = !{!"EnableUndefAlphaOutputAsRed", i1 true}
!81 = !{!"WaEnableALTModeVisaWA", i1 false}
!82 = !{!"NewSpillCostFunction", i1 false}
!83 = !{!"ForceLargeGRFNum4RQ", i1 false}
!84 = !{!"DisableEUFusion", i1 false}
!85 = !{!"DisableFDivToFMulInvOpt", i1 false}
!86 = !{!"initializePhiSampleSourceWA", i1 false}
!87 = !{!"WaDisableSubspanUseNoMaskForCB", i1 false}
!88 = !{!"DisableLoosenSimd32Occu", i1 false}
!89 = !{!"FastestS1Options", i32 0}
!90 = !{!"FuncMD", !91, !92}
!91 = !{!"FuncMDMap[0]", void (i8 addrspace(1)*, i8 addrspace(1)*, <8 x i32>, <8 x i32>, i32, i32)* @test}
!92 = !{!"FuncMDValue[0]", !93, !94, !98, !99, !100, !121, !139, !140, !141, !142, !143, !144, !145, !146, !147, !148, !149, !150, !151, !152, !153, !154, !155, !158, !161, !164, !167, !170, !173, !174}
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
!121 = !{!"resAllocMD", !122, !123, !124, !125, !138}
!122 = !{!"uavsNumType", i32 0}
!123 = !{!"srvsNumType", i32 0}
!124 = !{!"samplersNumType", i32 0}
!125 = !{!"argAllocMDList", !126, !130, !132, !135, !136, !137}
!126 = !{!"argAllocMDListVec[0]", !127, !128, !129}
!127 = !{!"type", i32 1}
!128 = !{!"extensionType", i32 -1}
!129 = !{!"indexType", i32 0}
!130 = !{!"argAllocMDListVec[1]", !127, !128, !131}
!131 = !{!"indexType", i32 1}
!132 = !{!"argAllocMDListVec[2]", !133, !128, !134}
!133 = !{!"type", i32 0}
!134 = !{!"indexType", i32 -1}
!135 = !{!"argAllocMDListVec[3]", !133, !128, !134}
!136 = !{!"argAllocMDListVec[4]", !133, !128, !134}
!137 = !{!"argAllocMDListVec[5]", !133, !128, !134}
!138 = !{!"inlineSamplersMD"}
!139 = !{!"maxByteOffsets"}
!140 = !{!"IsInitializer", i1 false}
!141 = !{!"IsFinalizer", i1 false}
!142 = !{!"CompiledSubGroupsNumber", i32 0}
!143 = !{!"hasInlineVmeSamplers", i1 false}
!144 = !{!"localSize", i32 0}
!145 = !{!"localIDPresent", i1 false}
!146 = !{!"groupIDPresent", i1 false}
!147 = !{!"privateMemoryPerWI", i32 0}
!148 = !{!"prevFPOffset", i32 0}
!149 = !{!"globalIDPresent", i1 false}
!150 = !{!"hasSyncRTCalls", i1 false}
!151 = !{!"hasNonKernelArgLoad", i1 false}
!152 = !{!"hasNonKernelArgStore", i1 false}
!153 = !{!"hasNonKernelArgAtomic", i1 false}
!154 = !{!"UserAnnotations"}
!155 = !{!"m_OpenCLArgAddressSpaces", !156, !157}
!156 = !{!"m_OpenCLArgAddressSpacesVec[0]", i32 1}
!157 = !{!"m_OpenCLArgAddressSpacesVec[1]", i32 1}
!158 = !{!"m_OpenCLArgAccessQualifiers", !159, !160}
!159 = !{!"m_OpenCLArgAccessQualifiersVec[0]", !"none"}
!160 = !{!"m_OpenCLArgAccessQualifiersVec[1]", !"none"}
!161 = !{!"m_OpenCLArgTypes", !162, !163}
!162 = !{!"m_OpenCLArgTypesVec[0]", !"char*"}
!163 = !{!"m_OpenCLArgTypesVec[1]", !"char*"}
!164 = !{!"m_OpenCLArgBaseTypes", !165, !166}
!165 = !{!"m_OpenCLArgBaseTypesVec[0]", !"char*"}
!166 = !{!"m_OpenCLArgBaseTypesVec[1]", !"char*"}
!167 = !{!"m_OpenCLArgTypeQualifiers", !168, !169}
!168 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!169 = !{!"m_OpenCLArgTypeQualifiersVec[1]", !""}
!170 = !{!"m_OpenCLArgNames", !171, !172}
!171 = !{!"m_OpenCLArgNamesVec[0]", !"in"}
!172 = !{!"m_OpenCLArgNamesVec[1]", !"out"}
!173 = !{!"m_OpenCLArgScalarAsPointers"}
!174 = !{!"m_OptsToDisablePerFunc", !175, !176, !177}
!175 = !{!"m_OptsToDisablePerFuncSet[0]", !"IGC-AddressArithmeticSinking"}
!176 = !{!"m_OptsToDisablePerFuncSet[1]", !"IGC-AllowSimd32Slicing"}
!177 = !{!"m_OptsToDisablePerFuncSet[2]", !"IGC-SinkLoadOpt"}
!178 = !{!"pushInfo", !179, !180, !181, !185, !186, !187, !188, !189, !190, !191, !192, !205, !206, !207, !208}
!179 = !{!"pushableAddresses"}
!180 = !{!"bindlessPushInfo"}
!181 = !{!"dynamicBufferInfo", !182, !183, !184}
!182 = !{!"firstIndex", i32 0}
!183 = !{!"numOffsets", i32 0}
!184 = !{!"forceDisabled", i1 false}
!185 = !{!"MaxNumberOfPushedBuffers", i32 0}
!186 = !{!"inlineConstantBufferSlot", i32 -1}
!187 = !{!"inlineConstantBufferOffset", i32 -1}
!188 = !{!"inlineConstantBufferGRFOffset", i32 -1}
!189 = !{!"constants"}
!190 = !{!"inputs"}
!191 = !{!"constantReg"}
!192 = !{!"simplePushInfoArr", !193, !202, !203, !204}
!193 = !{!"simplePushInfoArrVec[0]", !194, !195, !196, !197, !198, !199, !200, !201}
!194 = !{!"cbIdx", i32 0}
!195 = !{!"pushableAddressGrfOffset", i32 -1}
!196 = !{!"pushableOffsetGrfOffset", i32 -1}
!197 = !{!"offset", i32 0}
!198 = !{!"size", i32 0}
!199 = !{!"isStateless", i1 false}
!200 = !{!"isBindless", i1 false}
!201 = !{!"simplePushLoads"}
!202 = !{!"simplePushInfoArrVec[1]", !194, !195, !196, !197, !198, !199, !200, !201}
!203 = !{!"simplePushInfoArrVec[2]", !194, !195, !196, !197, !198, !199, !200, !201}
!204 = !{!"simplePushInfoArrVec[3]", !194, !195, !196, !197, !198, !199, !200, !201}
!205 = !{!"simplePushBufferUsed", i32 0}
!206 = !{!"pushAnalysisWIInfos"}
!207 = !{!"inlineRTGlobalPtrOffset", i32 0}
!208 = !{!"rtSyncSurfPtrOffset", i32 0}
!209 = !{!"psInfo", !210, !211, !212, !213, !214, !215, !216, !217, !218, !219, !220, !221, !222, !223, !224}
!210 = !{!"BlendStateDisabledMask", i8 0}
!211 = !{!"SkipSrc0Alpha", i1 false}
!212 = !{!"DualSourceBlendingDisabled", i1 false}
!213 = !{!"ForceEnableSimd32", i1 false}
!214 = !{!"outputDepth", i1 false}
!215 = !{!"outputStencil", i1 false}
!216 = !{!"outputMask", i1 false}
!217 = !{!"blendToFillEnabled", i1 false}
!218 = !{!"forceEarlyZ", i1 false}
!219 = !{!"hasVersionedLoop", i1 false}
!220 = !{!"forceSingleSourceRTWAfterDualSourceRTW", i1 false}
!221 = !{!"NumSamples", i8 0}
!222 = !{!"blendOptimizationMode"}
!223 = !{!"colorOutputMask"}
!224 = !{!"WaDisableVRS", i1 false}
!225 = !{!"csInfo", !226, !227, !228, !229, !230, !27, !28, !231, !232, !233, !234, !235, !236, !237, !238, !239, !240, !241, !242, !58, !59, !243, !244, !245}
!226 = !{!"maxWorkGroupSize", i32 0}
!227 = !{!"waveSize", i32 0}
!228 = !{!"ComputeShaderSecondCompile"}
!229 = !{!"forcedSIMDSize", i8 0}
!230 = !{!"forceTotalGRFNum", i32 0}
!231 = !{!"forceSpillCompression", i1 false}
!232 = !{!"allowLowerSimd", i1 false}
!233 = !{!"disableSimd32Slicing", i1 false}
!234 = !{!"disableSplitOnSpill", i1 false}
!235 = !{!"enableNewSpillCostFunction", i1 false}
!236 = !{!"forcedVISAPreRAScheduler", i1 false}
!237 = !{!"forceUniformBuffer", i1 false}
!238 = !{!"forceUniformSurfaceSampler", i1 false}
!239 = !{!"disableLocalIdOrderOptimizations", i1 false}
!240 = !{!"disableDispatchAlongY", i1 false}
!241 = !{!"neededThreadIdLayout", i1* null}
!242 = !{!"forceTileYWalk", i1 false}
!243 = !{!"walkOrderEnabled", i1 false}
!244 = !{!"walkOrderOverride", i32 0}
!245 = !{!"ResForHfPacking"}
!246 = !{!"msInfo", !247, !248, !249, !250, !251, !252, !253, !254, !255}
!247 = !{!"PrimitiveTopology", i32 3}
!248 = !{!"MaxNumOfPrimitives", i32 0}
!249 = !{!"MaxNumOfVertices", i32 0}
!250 = !{!"MaxNumOfPerPrimitiveOutputs", i32 0}
!251 = !{!"MaxNumOfPerVertexOutputs", i32 0}
!252 = !{!"WorkGroupSize", i32 0}
!253 = !{!"WorkGroupMemorySizeInBytes", i32 0}
!254 = !{!"IndexFormat", i32 6}
!255 = !{!"SubgroupSize", i32 0}
!256 = !{!"taskInfo", !257, !252, !253, !255}
!257 = !{!"MaxNumOfOutputs", i32 0}
!258 = !{!"NBarrierCnt", i32 0}
!259 = !{!"rtInfo", !260, !261, !262, !263, !264, !265, !266, !267, !268, !269, !270, !271}
!260 = !{!"RayQueryAllocSizeInBytes", i32 0}
!261 = !{!"NumContinuations", i32 0}
!262 = !{!"RTAsyncStackAddrspace", i32 -1}
!263 = !{!"RTAsyncStackSurfaceStateOffset", i1* null}
!264 = !{!"SWHotZoneAddrspace", i32 -1}
!265 = !{!"SWHotZoneSurfaceStateOffset", i1* null}
!266 = !{!"SWStackAddrspace", i32 -1}
!267 = !{!"SWStackSurfaceStateOffset", i1* null}
!268 = !{!"RTSyncStackAddrspace", i32 -1}
!269 = !{!"RTSyncStackSurfaceStateOffset", i1* null}
!270 = !{!"doSyncDispatchRays", i1 false}
!271 = !{!"MemStyle", !"Xe"}
!272 = !{!"CurUniqueIndirectIdx", i32 0}
!273 = !{!"inlineDynTextures"}
!274 = !{!"inlineResInfoData"}
!275 = !{!"immConstant", !276, !277, !278}
!276 = !{!"data"}
!277 = !{!"sizes"}
!278 = !{!"zeroIdxs"}
!279 = !{!"stringConstants"}
!280 = !{!"inlineBuffers", !281, !285, !286}
!281 = !{!"inlineBuffersVec[0]", !282, !283, !284}
!282 = !{!"alignment", i32 0}
!283 = !{!"allocSize", i64 0}
!284 = !{!"Buffer"}
!285 = !{!"inlineBuffersVec[1]", !282, !283, !284}
!286 = !{!"inlineBuffersVec[2]", !282, !283, !284}
!287 = !{!"GlobalPointerProgramBinaryInfos"}
!288 = !{!"ConstantPointerProgramBinaryInfos"}
!289 = !{!"GlobalBufferAddressRelocInfo"}
!290 = !{!"ConstantBufferAddressRelocInfo"}
!291 = !{!"forceLscCacheList"}
!292 = !{!"SrvMap"}
!293 = !{!"RasterizerOrderedByteAddressBuffer"}
!294 = !{!"RasterizerOrderedViews"}
!295 = !{!"MinNOSPushConstantSize", i32 2}
!296 = !{!"inlineProgramScopeOffsets"}
!297 = !{!"shaderData", !298}
!298 = !{!"numReplicas", i32 0}
!299 = !{!"URBInfo", !300, !301, !302}
!300 = !{!"has64BVertexHeaderInput", i1 false}
!301 = !{!"has64BVertexHeaderOutput", i1 false}
!302 = !{!"hasVertexHeader", i1 true}
!303 = !{!"UseBindlessImage", i1 false}
!304 = !{!"enableRangeReduce", i1 false}
!305 = !{!"allowMatchMadOptimizationforVS", i1 false}
!306 = !{!"disableMatchMadOptimizationForCS", i1 false}
!307 = !{!"disableMemOptforNegativeOffsetLoads", i1 false}
!308 = !{!"enableThreeWayLoadSpiltOpt", i1 false}
!309 = !{!"statefulResourcesNotAliased", i1 false}
!310 = !{!"disableMixMode", i1 false}
!311 = !{!"genericAccessesResolved", i1 false}
!312 = !{!"disableSeparateSpillPvtScratchSpace", i1 false}
!313 = !{!"disableSeparateScratchWA", i1 false}
!314 = !{!"PrivateMemoryPerFG", !315, !316}
!315 = !{!"PrivateMemoryPerFGMap[0]", void (i8 addrspace(1)*, i8 addrspace(1)*, <8 x i32>, <8 x i32>, i32, i32)* @test}
!316 = !{!"PrivateMemoryPerFGValue[0]", i32 0}
!317 = !{!"m_OptsToDisable"}
!318 = !{!"capabilities", !319}
!319 = !{!"globalVariableDecorationsINTEL", i1 false}
!320 = !{!"m_ShaderResourceViewMcsMask", !321, !322}
!321 = !{!"m_ShaderResourceViewMcsMaskVec[0]", i64 0}
!322 = !{!"m_ShaderResourceViewMcsMaskVec[1]", i64 0}
!323 = !{!"computedDepthMode", i32 0}
!324 = !{!"isHDCFastClearShader", i1 false}
!325 = !{!"argRegisterReservations", !326}
!326 = !{!"argRegisterReservationsVec[0]", i32 0}
!327 = !{i32 2, i32 0}
!328 = !{!"clang version 14.0.5"}
!329 = !{i32 1, !"wchar_size", i32 4}