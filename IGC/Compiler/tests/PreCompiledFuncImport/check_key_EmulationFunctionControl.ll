;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; To test key EmulationFunctionControl for double emulation function
;
; The default of the key is for IGC to decide whether to inline or not, and it
; may change from time to time. To avoid wasting time to modifying this test,
; this test is only for non-default cases.
;
; REQUIRES: regkeys
;
; 1. Test EmulationFunctionControl=1, forcing inline
; RUN: igc_opt %s -regkey TestIGCPreCompiledFunctions=1,EmulationFunctionControl=1 \
; RUN:            --platformdg2 --igc-precompiled-import -S \
; RUN:   | FileCheck %s --check-prefix=FC1

;
; FC1-LABEL: define spir_kernel void @test_emudp
; FC1: {{; Function Attrs:}}
; FC1: {{; Function Attrs: alwaysinline}}
; FC1-NEXT: define internal double @__igcbuiltin_dp_sqrt


;
; 2. Test EmulationFunctionControl=2, forcing subroutine
; RUN: igc_opt %s -regkey TestIGCPreCompiledFunctions=1,EmulationFunctionControl=2 \
; RUN:            --platformdg2 --igc-precompiled-import -S \
; RUN:   | FileCheck %s --check-prefix=FC2
;
; FC2-LABEL: define spir_kernel void @test_emudp
; FC2: {{; Function Attrs:}}
; FC2: {{; Function Attrs: noinline}}
; FC2-NEXT: define internal double @__igcbuiltin_dp_sqrt
; FC2-NOT:"visaStackCall"


;
; 3. Test EmulationFunctionControl=3, forcing subroutine
;
; RUN: igc_opt %s -regkey TestIGCPreCompiledFunctions=1,EmulationFunctionControl=3 \
; RUN:            --platformdg2 --igc-precompiled-import -S \
; RUN:   | FileCheck %s --check-prefix=FC3
;
; FC3-LABEL: define spir_kernel void @test_emudp
; FC3: {{; Function Attrs:}}
; FC3: {{; Function Attrs: noinline }}
; FC3-NEXT: define internal double @__igcbuiltin_dp_sqrt
; FC3:"visaStackCall"
;



; Function Attrs: convergent nounwind
define spir_kernel void @test_emudp(double addrspace(1)* %Dst, double addrspace(1)* %Src, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase) #0 {
entry:
  %0 = load double, double addrspace(1)* %Src, align 8
  %call.i.i1 = call double @llvm.sqrt.f64(double %0)
  store double %call.i.i1, double addrspace(1)* %Dst, align 8
  ret void
}

; Function Attrs: nounwind readnone speculatable
declare double @llvm.sqrt.f64(double) #2

attributes #0 = { convergent nounwind "less-precise-fpmad"="true" }
attributes #2 = { nounwind readnone speculatable }

!IGCMetadata = !{!0}
!igc.functions = !{!325}

!0 = !{!"ModuleMD", !1, !71, !163, !193, !194, !198, !199, !200, !247, !259, !260, !274, !275, !276, !277, !278, !279, !280, !281, !282, !283, !287, !288, !289, !290, !291, !292, !293, !294, !295, !296, !297, !298, !299, !300, !302, !306, !307, !308, !309, !310, !311, !312, !313, !314, !315, !133, !316, !317, !318, !320, !323, !324}
!1 = !{!"isPrecise", i1 false}
!70 = !{!"EnableURBWritesMerging", i1 true}
!71 = !{!"FuncMD", !72, !73}
!72 = !{!"FuncMDMap[0]", void (double addrspace(1)*, double addrspace(1)*, <8 x i32>, <8 x i32>, i8*)* @test_emudp}
!73 = !{!"FuncMDValue[0]", !74, !75, !79, !80, !81, !82, !83, !84, !107, !125, !126, !127, !128, !129, !130, !131, !132, !133, !134, !135, !136, !137, !138, !139, !140, !143, !146, !149, !152, !155, !158, !159}
!74 = !{!"localOffsets"}
!75 = !{!"workGroupWalkOrder", !76, !77, !78}
!76 = !{!"dim0", i32 0}
!77 = !{!"dim1", i32 0}
!78 = !{!"dim2", i32 0}
!79 = !{!"funcArgs"}
!80 = !{!"functionType", !"KernelFunction"}
!81 = !{!"inlineDynConstants"}
!82 = !{!"inlineDynRootConstant"}
!83 = !{!"inlineDynConstantDescTable"}
!84 = !{!"m_pInterestingConstants"}
!86 = !{!"callableShaderType", !"NumberOfCallableShaderTypes"}
!87 = !{!"isContinuation", i1 false}
!88 = !{!"hasTraceRayPayload", i1 false}
!89 = !{!"hasHitAttributes", i1 false}
!90 = !{!"hasCallableData", i1 false}
!91 = !{!"ShaderStackSize", i32 0}
!92 = !{!"ShaderHash", i64 0}
!93 = !{!"ShaderName", !""}
!94 = !{!"ParentName", !""}
!95 = !{!"SlotNum", i1* null}
!96 = !{!"NOSSize", i32 0}
!97 = !{!"globalRootSignatureSize", i32 0}
!98 = !{!"Entries"}
!99 = !{!"SpillUnions"}
!100 = !{!"CustomHitAttrSizeInBytes", i32 0}
!101 = !{!"Types", !102, !103, !104}
!102 = !{!"FrameStartTys"}
!103 = !{!"ArgumentTys"}
!104 = !{!"FullFrameTys"}
!105 = !{!"Aliases"}
!106 = !{!"NumGRF", i32 0}
!107 = !{!"resAllocMD", !108, !109, !110, !111, !124}
!108 = !{!"uavsNumType", i32 3}
!109 = !{!"srvsNumType", i32 0}
!110 = !{!"samplersNumType", i32 0}
!111 = !{!"argAllocMDList", !112, !116, !118, !121, !122}
!112 = !{!"argAllocMDListVec[0]", !113, !114, !115}
!113 = !{!"type", i32 1}
!114 = !{!"extensionType", i32 -1}
!115 = !{!"indexType", i32 0}
!116 = !{!"argAllocMDListVec[1]", !113, !114, !117}
!117 = !{!"indexType", i32 1}
!118 = !{!"argAllocMDListVec[2]", !119, !114, !120}
!119 = !{!"type", i32 0}
!120 = !{!"indexType", i32 -1}
!121 = !{!"argAllocMDListVec[3]", !119, !114, !120}
!122 = !{!"argAllocMDListVec[4]", !113, !114, !123}
!123 = !{!"indexType", i32 2}
!124 = !{!"inlineSamplersMD"}
!125 = !{!"maxByteOffsets"}
!126 = !{!"IsInitializer", i1 false}
!127 = !{!"IsFinalizer", i1 false}
!128 = !{!"CompiledSubGroupsNumber", i32 0}
!129 = !{!"hasInlineVmeSamplers", i1 false}
!130 = !{!"localSize", i32 0}
!131 = !{!"localIDPresent", i1 false}
!132 = !{!"groupIDPresent", i1 false}
!133 = !{!"privateMemoryPerWI", i32 0}
!134 = !{!"globalIDPresent", i1 false}
!135 = !{!"hasSyncRTCalls", i1 false}
!136 = !{!"hasNonKernelArgLoad", i1 false}
!137 = !{!"hasNonKernelArgStore", i1 false}
!138 = !{!"hasNonKernelArgAtomic", i1 false}
!139 = !{!"UserAnnotations"}
!140 = !{!"m_OpenCLArgAddressSpaces", !141, !142}
!141 = !{!"m_OpenCLArgAddressSpacesVec[0]", i32 1}
!142 = !{!"m_OpenCLArgAddressSpacesVec[1]", i32 1}
!143 = !{!"m_OpenCLArgAccessQualifiers", !144, !145}
!144 = !{!"m_OpenCLArgAccessQualifiersVec[0]", !"none"}
!145 = !{!"m_OpenCLArgAccessQualifiersVec[1]", !"none"}
!146 = !{!"m_OpenCLArgTypes", !147, !148}
!147 = !{!"m_OpenCLArgTypesVec[0]", !"double*"}
!148 = !{!"m_OpenCLArgTypesVec[1]", !"double*"}
!149 = !{!"m_OpenCLArgBaseTypes", !150, !151}
!150 = !{!"m_OpenCLArgBaseTypesVec[0]", !"double*"}
!151 = !{!"m_OpenCLArgBaseTypesVec[1]", !"double*"}
!152 = !{!"m_OpenCLArgTypeQualifiers", !153, !154}
!153 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!154 = !{!"m_OpenCLArgTypeQualifiersVec[1]", !""}
!155 = !{!"m_OpenCLArgNames", !156, !157}
!156 = !{!"m_OpenCLArgNamesVec[0]", !"Dst"}
!157 = !{!"m_OpenCLArgNamesVec[1]", !"Src"}
!158 = !{!"m_OpenCLArgScalarAsPointers"}
!159 = !{!"m_OptsToDisablePerFunc", !160, !161, !162}
!160 = !{!"m_OptsToDisablePerFuncSet[0]", !"IGC-AddressArithmeticSinking"}
!161 = !{!"m_OptsToDisablePerFuncSet[1]", !"IGC-AllowSimd32Slicing"}
!162 = !{!"m_OptsToDisablePerFuncSet[2]", !"IGC-SinkLoadOpt"}
!163 = !{!"pushInfo", !164, !165, !166, !169, !170, !171, !172, !173, !174, !175, !176, !189, !190, !191, !192}
!164 = !{!"pushableAddresses"}
!165 = !{!"bindlessPushInfo"}
!166 = !{!"dynamicBufferInfo", !167, !168}
!167 = !{!"firstIndex", i32 0}
!168 = !{!"numOffsets", i32 0}
!169 = !{!"MaxNumberOfPushedBuffers", i32 0}
!170 = !{!"inlineConstantBufferSlot", i32 -1}
!171 = !{!"inlineConstantBufferOffset", i32 -1}
!172 = !{!"inlineConstantBufferGRFOffset", i32 -1}
!173 = !{!"constants"}
!174 = !{!"inputs"}
!175 = !{!"constantReg"}
!176 = !{!"simplePushInfoArr", !177, !186, !187, !188}
!177 = !{!"simplePushInfoArrVec[0]", !178, !179, !180, !181, !182, !183, !184, !185}
!178 = !{!"cbIdx", i32 0}
!179 = !{!"pushableAddressGrfOffset", i32 -1}
!180 = !{!"pushableOffsetGrfOffset", i32 -1}
!181 = !{!"offset", i32 0}
!182 = !{!"size", i32 0}
!183 = !{!"isStateless", i1 false}
!184 = !{!"isBindless", i1 false}
!185 = !{!"simplePushLoads"}
!186 = !{!"simplePushInfoArrVec[1]", !178, !179, !180, !181, !182, !183, !184, !185}
!187 = !{!"simplePushInfoArrVec[2]", !178, !179, !180, !181, !182, !183, !184, !185}
!188 = !{!"simplePushInfoArrVec[3]", !178, !179, !180, !181, !182, !183, !184, !185}
!189 = !{!"simplePushBufferUsed", i32 0}
!190 = !{!"pushAnalysisWIInfos"}
!191 = !{!"inlineRTGlobalPtrOffset", i32 0}
!192 = !{!"rtSyncSurfPtrOffset", i32 0}
!193 = !{!"WaEnableICBPromotion", i1 false}
!194 = !{!"vsInfo", !195, !196, !197}
!195 = !{!"DrawIndirectBufferIndex", i32 -1}
!196 = !{!"vertexReordering", i32 -1}
!197 = !{!"MaxNumOfOutputs", i32 0}
!198 = !{!"dsInfo", !197}
!199 = !{!"gsInfo", !197}
!200 = !{!"psInfo", !201, !202, !203, !204, !205, !206, !207, !208, !209, !210, !211, !212, !213, !214, !215, !216, !217, !218, !219, !220, !221, !222, !223, !224, !225, !226, !227, !228}
!201 = !{!"BlendStateDisabledMask", i8 0}
!202 = !{!"SkipSrc0Alpha", i1 false}
!203 = !{!"DualSourceBlendingDisabled", i1 false}
!204 = !{!"ForceEnableSimd32", i1 false}
!205 = !{!"outputDepth", i1 false}
!206 = !{!"outputStencil", i1 false}
!207 = !{!"outputMask", i1 false}
!208 = !{!"blendToFillEnabled", i1 false}
!209 = !{!"forceEarlyZ", i1 false}
!210 = !{!"hasVersionedLoop", i1 false}
!211 = !{!"forceSingleSourceRTWAfterDualSourceRTW", i1 false}
!212 = !{!"requestCPSizeRelevant", i1 false}
!213 = !{!"requestCPSize", i1 false}
!214 = !{!"texelMaskFastClearMode", !"Disabled"}
!215 = !{!"NumSamples", i8 0}
!216 = !{!"blendOptimizationMode"}
!217 = !{!"colorOutputMask"}
!218 = !{!"ProvokingVertexModeLast", i1 false}
!219 = !{!"VertexAttributesBypass", i1 false}
!220 = !{!"LegacyBaryAssignmentDisableLinear", i1 false}
!221 = !{!"LegacyBaryAssignmentDisableLinearNoPerspective", i1 false}
!222 = !{!"LegacyBaryAssignmentDisableLinearCentroid", i1 false}
!223 = !{!"LegacyBaryAssignmentDisableLinearNoPerspectiveCentroid", i1 false}
!224 = !{!"LegacyBaryAssignmentDisableLinearSample", i1 false}
!225 = !{!"LegacyBaryAssignmentDisableLinearNoPerspectiveSample", i1 false}
!226 = !{!"MeshShaderWAPerPrimitiveUserDataEnable", i1 false}
!227 = !{!"generatePatchesForRTWriteSends", i1 false}
!228 = !{!"WaDisableVRS", i1 false}
!230 = !{!"maxWorkGroupSize", i32 0}
!231 = !{!"waveSize", i32 0}
!232 = !{!"ComputeShaderSecondCompile"}
!233 = !{!"forcedSIMDSize", i8 0}
!234 = !{!"forceTotalGRFNum", i32 0}
!235 = !{!"allowLowerSimd", i1 false}
!236 = !{!"disableSimd32Slicing", i1 false}
!237 = !{!"disableSplitOnSpill", i1 false}
!238 = !{!"forcedVISAPreRAScheduler", i1 false}
!239 = !{!"disableLocalIdOrderOptimizations", i1 false}
!240 = !{!"disableDispatchAlongY", i1 false}
!241 = !{!"neededThreadIdLayout", i1* null}
!242 = !{!"forceTileYWalk", i1 false}
!243 = !{!"walkOrderEnabled", i1 false}
!244 = !{!"walkOrderOverride", i32 0}
!245 = !{!"ResForHfPacking"}
!246 = !{!"hasWaveMatrix", i1 false}
!247 = !{!"msInfo", !248, !249, !250, !251, !252, !253, !254, !255, !256, !257, !226, !218, !258}
!248 = !{!"PrimitiveTopology", i32 3}
!249 = !{!"MaxNumOfPrimitives", i32 0}
!250 = !{!"MaxNumOfVertices", i32 0}
!251 = !{!"MaxNumOfPerPrimitiveOutputs", i32 0}
!252 = !{!"MaxNumOfPerVertexOutputs", i32 0}
!253 = !{!"WorkGroupSize", i32 0}
!254 = !{!"WorkGroupMemorySizeInBytes", i32 0}
!255 = !{!"IndexFormat", i32 6}
!256 = !{!"SubgroupSize", i32 0}
!257 = !{!"VPandRTAIndexAutostripEnable", i1 false}
!258 = !{!"numPrimitiveAttributesPatchBaseName", !""}
!259 = !{!"taskInfo", !197, !253, !254, !256}
!260 = !{!"NBarrierCnt", i32 0}
!262 = !{!"RayQueryAllocSizeInBytes", i32 0}
!263 = !{!"NumContinuations", i32 0}
!264 = !{!"RTAsyncStackAddrspace", i32 -1}
!265 = !{!"RTAsyncStackSurfaceStateOffset", i1* null}
!266 = !{!"SWHotZoneAddrspace", i32 -1}
!267 = !{!"SWHotZoneSurfaceStateOffset", i1* null}
!268 = !{!"SWStackAddrspace", i32 -1}
!269 = !{!"SWStackSurfaceStateOffset", i1* null}
!270 = !{!"RTSyncStackAddrspace", i32 -1}
!271 = !{!"RTSyncStackSurfaceStateOffset", i1* null}
!272 = !{!"doSyncDispatchRays", i1 false}
!273 = !{!"MemStyle", !"Xe"}
!274 = !{!"EnableTextureIndirection", i1 false}
!275 = !{!"EnableSamplerIndirection", i1 false}
!276 = !{!"samplerStateStride", i32 0}
!277 = !{!"samplerStateOffset", i32 0}
!278 = !{!"textureStateStride", i32 0}
!279 = !{!"textureStateOffset", i32 0}
!280 = !{!"CurUniqueIndirectIdx", i32 0}
!281 = !{!"inlineDynTextures"}
!282 = !{!"inlineResInfoData"}
!283 = !{!"immConstant", !284, !285, !286}
!284 = !{!"data"}
!285 = !{!"sizes"}
!286 = !{!"zeroIdxs"}
!287 = !{!"stringConstants"}
!288 = !{!"inlineConstantBuffers"}
!289 = !{!"inlineGlobalBuffers"}
!290 = !{!"GlobalPointerProgramBinaryInfos"}
!291 = !{!"ConstantPointerProgramBinaryInfos"}
!292 = !{!"GlobalBufferAddressRelocInfo"}
!293 = !{!"ConstantBufferAddressRelocInfo"}
!294 = !{!"forceLscCacheList"}
!295 = !{!"SrvMap"}
!296 = !{!"RootConstantBufferOffsetInBytes"}
!297 = !{!"RasterizerOrderedByteAddressBuffer"}
!298 = !{!"MinNOSPushConstantSize", i32 0}
!299 = !{!"inlineProgramScopeOffsets"}
!300 = !{!"shaderData", !301}
!301 = !{!"numReplicas", i32 0}
!302 = !{!"URBInfo", !303, !304, !305}
!303 = !{!"has64BVertexHeaderInput", i1 false}
!304 = !{!"has64BVertexHeaderOutput", i1 false}
!305 = !{!"hasVertexHeader", i1 true}
!306 = !{!"UseBindlessImage", i1 false}
!307 = !{!"enableRangeReduce", i1 false}
!308 = !{!"disableNewTrigFuncRangeReduction", i1 false}
!309 = !{!"enableFRemToSRemOpt", i1 false}
!310 = !{!"allowMatchMadOptimizationforVS", i1 false}
!311 = !{!"disableMemOptforNegativeOffsetLoads", i1 false}
!312 = !{!"enableThreeWayLoadSpiltOpt", i1 false}
!313 = !{!"statefulResourcesNotAliased", i1 false}
!314 = !{!"disableMixMode", i1 false}
!315 = !{!"genericAccessesResolved", i1 false}
!316 = !{!"PrivateMemoryPerFG"}
!317 = !{!"m_OptsToDisable"}
!318 = !{!"capabilities", !319}
!319 = !{!"globalVariableDecorationsINTEL", i1 false}
!320 = !{!"m_ShaderResourceViewMcsMask", !321, !322}
!321 = !{!"m_ShaderResourceViewMcsMaskVec[0]", i64 0}
!322 = !{!"m_ShaderResourceViewMcsMaskVec[1]", i64 0}
!323 = !{!"computedDepthMode", i32 0}
!324 = !{!"isHDCFastClearShader", i1 false}
!325 = !{void (double addrspace(1)*, double addrspace(1)*, <8 x i32>, <8 x i32>, i8*)* @test_emudp, !326}
!326 = !{!327, !328}
!327 = !{!"function_type", i32 0}
!328 = !{!"implicit_arg_desc", !329, !330, !331}
!329 = !{i32 0}
!330 = !{i32 1}
!331 = !{i32 12}
