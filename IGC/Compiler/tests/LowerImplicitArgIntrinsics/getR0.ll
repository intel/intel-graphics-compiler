;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt %s -S -o - --igc-lower-implicit-arg-intrinsic --platformdg2 | FileCheck %s

; Explanation:
; This tests checks if intrinsic calls are being lowered correctly.
; simdSize, simdLaneId and getR0 calls should be moved to the beggining and should not be repeated.
; To reproduce - compile kernel pattern with "-cl-opt-disable" option and flags:
; EnableGlobalStateBuffer=1
; ForceInlineStackCallWithImplArg=0

; Kernel pattern:
;
; size_t get_global()
; {
;   return get_global_id(0) + get_global_id(1) + get_global_id(2);
; }
;
; __kernel void test(__global int* out)
; {
;   out[0] = get_global();
; }


define spir_kernel void @test(i32 addrspace(1)* %out, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset) #0 {
entry:
  %out.addr = alloca i32 addrspace(1)*, align 8
  store i32 addrspace(1)* %out, i32 addrspace(1)** %out.addr, align 8
  %call = call spir_func i64 @get_global() #4
  %conv = trunc i64 %call to i32
  %0 = load i32 addrspace(1)*, i32 addrspace(1)** %out.addr, align 8
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 0
  store i32 %conv, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

; Function Attrs: convergent noinline nounwind optnone
define internal spir_func i64 @get_global() #1 {
entry:
; CHECK-DAG: [[R0:%.*]] = call <8 x i32> @llvm.genx.GenISA.getR0.v8i32()
; CHECK-DAG: [[SIMD_SIZE:%.*]] = call i32 @llvm.genx.GenISA.simdSize.i32()
; CHECK-DAG: [[SIMD_LANEID:%.*]] = call i16 @llvm.genx.GenISA.simdLaneId.i16()
; CHECK: extractelement <8 x i32> [[R0]], i32 2
; CHECK-NOT: call i16 @llvm.genx.GenISA.simdLaneId.i16()
; CHECK-NOT: call i32 @llvm.genx.GenISA.simdSize.i32()
; CHECK-NOT: call <8 x i32> @llvm.genx.GenISA.getR0.v8i32()

; CHECK: extractelement <8 x i32> [[R0]], i32 2
; CHECK-NOT: call i16 @llvm.genx.GenISA.simdLaneId.i16()
; CHECK-NOT: call i32 @llvm.genx.GenISA.simdSize.i32()
; CHECK-NOT: call <8 x i32> @llvm.genx.GenISA.getR0.v8i32()

; CHECK: extractelement <8 x i32> [[R0]], i32 2
; CHECK-NOT: call i16 @llvm.genx.GenISA.simdLaneId.i16()
; CHECK-NOT: call i32 @llvm.genx.GenISA.simdSize.i32()
; CHECK-NOT: call <8 x i32> @llvm.genx.GenISA.getR0.v8i32()

  %0 = call i16 @llvm.genx.GenISA.getLocalID.Z.i16()
  %1 = call i16 @llvm.genx.GenISA.getLocalID.Y.i16()
  %2 = call <8 x i32> @llvm.genx.GenISA.getPayloadHeader.v8i32()
  %scalar111 = extractelement <8 x i32> %2, i32 0
  %scalar112 = extractelement <8 x i32> %2, i32 1
  %scalar113 = extractelement <8 x i32> %2, i32 2
  %scalar114 = extractelement <8 x i32> %2, i32 3
  %scalar115 = extractelement <8 x i32> %2, i32 4
  %scalar116 = extractelement <8 x i32> %2, i32 5
  %scalar117 = extractelement <8 x i32> %2, i32 6
  %scalar118 = extractelement <8 x i32> %2, i32 7
  %3 = call i16 @llvm.genx.GenISA.getLocalID.X.i16()
  %4 = call <3 x i32> @llvm.genx.GenISA.getEnqueuedLocalSize.v3i32()
  %scalar = extractelement <3 x i32> %4, i32 0
  %scalar109 = extractelement <3 x i32> %4, i32 1
  %scalar110 = extractelement <3 x i32> %4, i32 2
  %5 = call <8 x i32> @llvm.genx.GenISA.getR0.v8i32()
  %cmpDim = icmp eq i32 0, 0
  %tmpOffsetR0 = select i1 %cmpDim, i32 1, i32 5
  %offsetR0 = add i32 0, %tmpOffsetR0
  %groupId = extractelement <8 x i32> %5, i32 %offsetR0
  %mul.i.i.i = mul i32 %scalar, %groupId
  %6 = zext i16 %3 to i32
  %cmp11.i.i.i.i = icmp ult i32 %6, 65536
  call void @llvm.assume(i1 %cmp11.i.i.i.i) #4
  %add.i.i.i = add i32 %6, %mul.i.i.i
  %add4.i.i.i = add i32 %add.i.i.i, %scalar111
  %conv.i.i.i = zext i32 %add4.i.i.i to i64
  %cmpDim61 = icmp eq i32 1, 0
  %tmpOffsetR062 = select i1 %cmpDim61, i32 1, i32 5
  %offsetR063 = add i32 1, %tmpOffsetR062
  %groupId64 = extractelement <8 x i32> %5, i32 %offsetR063
  %mul.i7.i.i = mul i32 %scalar109, %groupId64
  %7 = zext i16 %1 to i32
  %cmp11.i.i8.i.i = icmp ult i32 %7, 65536
  call void @llvm.assume(i1 %cmp11.i.i8.i.i) #4
  %add.i9.i.i = add i32 %7, %mul.i7.i.i
  %add4.i11.i.i = add i32 %add.i9.i.i, %scalar112
  %conv.i12.i.i = zext i32 %add4.i11.i.i to i64
  %cmpDim67 = icmp eq i32 2, 0
  %tmpOffsetR068 = select i1 %cmpDim67, i32 1, i32 5
  %offsetR069 = add i32 2, %tmpOffsetR068
  %groupId70 = extractelement <8 x i32> %5, i32 %offsetR069
  %mul.i15.i.i = mul i32 %scalar110, %groupId70
  %8 = zext i16 %0 to i32
  %cmp11.i.i16.i.i = icmp ult i32 %8, 65536
  call void @llvm.assume(i1 %cmp11.i.i16.i.i) #4
  %add.i17.i.i = add i32 %8, %mul.i15.i.i
  %add4.i19.i.i = add i32 %add.i17.i.i, %scalar113
  %conv.i20.i.i = zext i32 %add4.i19.i.i to i64
  %cmpDim73 = icmp eq i32 0, 0
  %tmpOffsetR074 = select i1 %cmpDim73, i32 1, i32 5
  %offsetR075 = add i32 0, %tmpOffsetR074
  %groupId76 = extractelement <8 x i32> %5, i32 %offsetR075
  %mul.i.i.i3 = mul i32 %scalar, %groupId76
  %9 = zext i16 %3 to i32
  %cmp11.i.i.i.i5 = icmp ult i32 %9, 65536
  call void @llvm.assume(i1 %cmp11.i.i.i.i5) #4
  %add.i.i.i6 = add i32 %9, %mul.i.i.i3
  %add4.i.i.i8 = add i32 %add.i.i.i6, %scalar111
  %conv.i.i.i9 = zext i32 %add4.i.i.i8 to i64
  %cmpDim79 = icmp eq i32 1, 0
  %tmpOffsetR080 = select i1 %cmpDim79, i32 1, i32 5
  %offsetR081 = add i32 1, %tmpOffsetR080
  %groupId82 = extractelement <8 x i32> %5, i32 %offsetR081
  %mul.i7.i.i13 = mul i32 %scalar109, %groupId82
  %10 = zext i16 %1 to i32
  %cmp11.i.i8.i.i15 = icmp ult i32 %10, 65536
  call void @llvm.assume(i1 %cmp11.i.i8.i.i15) #4
  %add.i9.i.i16 = add i32 %10, %mul.i7.i.i13
  %add4.i11.i.i18 = add i32 %add.i9.i.i16, %scalar112
  %conv.i12.i.i19 = zext i32 %add4.i11.i.i18 to i64
  %cmpDim85 = icmp eq i32 2, 0
  %tmpOffsetR086 = select i1 %cmpDim85, i32 1, i32 5
  %offsetR087 = add i32 2, %tmpOffsetR086
  %groupId88 = extractelement <8 x i32> %5, i32 %offsetR087
  %mul.i15.i.i23 = mul i32 %scalar110, %groupId88
  %11 = zext i16 %0 to i32
  %cmp11.i.i16.i.i25 = icmp ult i32 %11, 65536
  call void @llvm.assume(i1 %cmp11.i.i16.i.i25) #4
  %add.i17.i.i26 = add i32 %11, %mul.i15.i.i23
  %add4.i19.i.i28 = add i32 %add.i17.i.i26, %scalar113
  %conv.i20.i.i29 = zext i32 %add4.i19.i.i28 to i64
  %add = add i64 %conv.i.i.i, %conv.i12.i.i19
  %cmpDim91 = icmp eq i32 0, 0
  %tmpOffsetR092 = select i1 %cmpDim91, i32 1, i32 5
  %offsetR093 = add i32 0, %tmpOffsetR092
  %groupId94 = extractelement <8 x i32> %5, i32 %offsetR093
  %mul.i.i.i33 = mul i32 %scalar, %groupId94
  %12 = zext i16 %3 to i32
  %cmp11.i.i.i.i35 = icmp ult i32 %12, 65536
  call void @llvm.assume(i1 %cmp11.i.i.i.i35) #4
  %add.i.i.i36 = add i32 %12, %mul.i.i.i33
  %add4.i.i.i38 = add i32 %add.i.i.i36, %scalar111
  %conv.i.i.i39 = zext i32 %add4.i.i.i38 to i64
  %cmpDim97 = icmp eq i32 1, 0
  %tmpOffsetR098 = select i1 %cmpDim97, i32 1, i32 5
  %offsetR099 = add i32 1, %tmpOffsetR098
  %groupId100 = extractelement <8 x i32> %5, i32 %offsetR099
  %mul.i7.i.i43 = mul i32 %scalar109, %groupId100
  %13 = zext i16 %1 to i32
  %cmp11.i.i8.i.i45 = icmp ult i32 %13, 65536
  call void @llvm.assume(i1 %cmp11.i.i8.i.i45) #4
  %add.i9.i.i46 = add i32 %13, %mul.i7.i.i43
  %add4.i11.i.i48 = add i32 %add.i9.i.i46, %scalar112
  %conv.i12.i.i49 = zext i32 %add4.i11.i.i48 to i64
  %cmpDim103 = icmp eq i32 2, 0
  %tmpOffsetR0104 = select i1 %cmpDim103, i32 1, i32 5
  %offsetR0105 = add i32 2, %tmpOffsetR0104
  %groupId106 = extractelement <8 x i32> %5, i32 %offsetR0105
  %mul.i15.i.i53 = mul i32 %scalar110, %groupId106
  %14 = zext i16 %0 to i32
  %cmp11.i.i16.i.i55 = icmp ult i32 %14, 65536
  call void @llvm.assume(i1 %cmp11.i.i16.i.i55) #4
  %add.i17.i.i56 = add i32 %14, %mul.i15.i.i53
  %add4.i19.i.i58 = add i32 %add.i17.i.i56, %scalar113
  %conv.i20.i.i59 = zext i32 %add4.i19.i.i58 to i64
  %add3 = add i64 %add, %conv.i20.i.i59
  ret i64 %add3
}

; Function Attrs: nounwind willreturn
declare void @llvm.assume(i1) #2

; Function Attrs: nounwind readnone
declare <8 x i32> @llvm.genx.GenISA.getR0.v8i32() #3

; Function Attrs: nounwind
declare <3 x i32> @llvm.genx.GenISA.getEnqueuedLocalSize.v3i32() #4

; Function Attrs: nounwind
declare i16 @llvm.genx.GenISA.getLocalID.X.i16() #4

; Function Attrs: nounwind
declare <8 x i32> @llvm.genx.GenISA.getPayloadHeader.v8i32() #4

; Function Attrs: nounwind
declare i16 @llvm.genx.GenISA.getLocalID.Y.i16() #4

; Function Attrs: nounwind
declare i16 @llvm.genx.GenISA.getLocalID.Z.i16() #4

attributes #0 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" }
attributes #1 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" "visaStackCall" }
attributes #2 = { nounwind willreturn }
attributes #3 = { nounwind readnone }
attributes #4 = { nounwind }

!IGCMetadata = !{!0}
!igc.functions = !{!276, !280}
!opencl.ocl.version = !{!293, !293, !293, !293, !293}
!opencl.spir.version = !{!293, !293, !293, !293, !293}
!llvm.ident = !{!294, !294, !294, !294, !294}
!llvm.module.flags = !{!295}

!0 = !{!"ModuleMD", !1, !2, !61, !151, !181, !197, !214, !224, !226, !227, !238, !239, !240, !241, !245, !246, !247, !248, !249, !250, !251, !252, !253, !254, !255, !256, !258, !262, !263, !264, !265, !266, !267, !105, !268, !269, !271, !274, !275}
!1 = !{!"isPrecise", i1 false}
!2 = !{!"compOpt", !3, !4, !5, !6, !7, !8, !9, !10, !11, !12, !13, !14, !15, !16, !17, !18, !19, !20, !21, !22, !23, !24, !25, !26, !27, !28, !29, !30, !31, !32, !33, !34, !35, !36, !37, !38, !39, !40, !41, !42, !43, !44, !45, !46, !47, !48, !49, !50, !51, !52, !53, !54, !55, !56, !57, !58, !59, !60}
!3 = !{!"DenormsAreZero", i1 false}
!4 = !{!"CorrectlyRoundedDivSqrt", i1 false}
!5 = !{!"OptDisable", i1 true}
!6 = !{!"MadEnable", i1 true}
!7 = !{!"NoSignedZeros", i1 false}
!8 = !{!"NoNaNs", i1 false}
!9 = !{!"FloatRoundingMode", i32 0}
!10 = !{!"FloatCvtIntRoundingMode", i32 3}
!11 = !{!"VISAPreSchedRPThreshold", i32 0}
!12 = !{!"SetLoopUnrollThreshold", i32 0}
!13 = !{!"UnsafeMathOptimizations", i1 false}
!14 = !{!"FiniteMathOnly", i1 false}
!15 = !{!"FastRelaxedMath", i1 false}
!16 = !{!"DashGSpecified", i1 false}
!17 = !{!"FastCompilation", i1 false}
!18 = !{!"UseScratchSpacePrivateMemory", i1 true}
!19 = !{!"RelaxedBuiltins", i1 false}
!20 = !{!"SubgroupIndependentForwardProgressRequired", i1 true}
!21 = !{!"GreaterThan2GBBufferRequired", i1 true}
!22 = !{!"GreaterThan4GBBufferRequired", i1 false}
!23 = !{!"DisableA64WA", i1 false}
!24 = !{!"ForceEnableA64WA", i1 false}
!25 = !{!"PushConstantsEnable", i1 true}
!26 = !{!"HasPositivePointerOffset", i1 false}
!27 = !{!"HasBufferOffsetArg", i1 true}
!28 = !{!"BufferOffsetArgOptional", i1 true}
!29 = !{!"HasSubDWAlignedPtrArg", i1 false}
!30 = !{!"replaceGlobalOffsetsByZero", i1 false}
!31 = !{!"forcePixelShaderSIMDMode", i32 0}
!32 = !{!"pixelShaderDoNotAbortOnSpill", i1 false}
!33 = !{!"UniformWGS", i1 false}
!34 = !{!"disableVertexComponentPacking", i1 false}
!35 = !{!"disablePartialVertexComponentPacking", i1 false}
!36 = !{!"PreferBindlessImages", i1 false}
!37 = !{!"UseBindlessMode", i1 false}
!38 = !{!"UseLegacyBindlessMode", i1 true}
!39 = !{!"disableMathRefactoring", i1 false}
!40 = !{!"atomicBranch", i1 false}
!41 = !{!"ForceInt32DivRemEmu", i1 false}
!42 = !{!"ForceInt32DivRemEmuSP", i1 false}
!43 = !{!"ForceMinSimdSizeForFastestCS", i1 false}
!44 = !{!"EnableFastestLinearScan", i1 false}
!45 = !{!"UseStatelessforPrivateMemory", i1 false}
!46 = !{!"EnableTakeGlobalAddress", i1 false}
!47 = !{!"IsLibraryCompilation", i1 false}
!48 = !{!"FastVISACompile", i1 false}
!49 = !{!"MatchSinCosPi", i1 false}
!50 = !{!"CaptureCompilerStats", i1 false}
!51 = !{!"EnableZEBinary", i1 false}
!52 = !{!"ExcludeIRFromZEBinary", i1 false}
!53 = !{!"EmitZeBinVISASections", i1 false}
!54 = !{!"allowDisableRematforCS", i1 false}
!55 = !{!"DisableIncSpillCostAllAddrTaken", i1 false}
!56 = !{!"DisableCPSOmaskWA", i1 false}
!57 = !{!"DisableFastestGopt", i1 false}
!58 = !{!"WaForceHalfPromotion", i1 false}
!59 = !{!"EnableUndefAlphaOutputAsRed", i1 true}
!60 = !{!"WaEnableALTModeVisaWA", i1 false}
!61 = !{!"FuncMD", !62, !63, !118, !119}
!62 = !{!"FuncMDMap[0]", i64 ()* @get_global}
!63 = !{!"FuncMDValue[0]", !64, !65, !69, !70, !71, !91, !97, !98, !99, !100, !101, !102, !103, !104, !105, !106, !107, !108, !109, !110, !111, !112, !113, !114, !115, !116, !117}
!64 = !{!"localOffsets"}
!65 = !{!"workGroupWalkOrder", !66, !67, !68}
!66 = !{!"dim0", i32 0}
!67 = !{!"dim1", i32 0}
!68 = !{!"dim2", i32 0}
!69 = !{!"funcArgs"}
!70 = !{!"functionType", !"UserFunction"}
!71 = !{!"rtInfo", !72, !73, !74, !75, !76, !77, !78, !79, !80, !81, !82, !83, !84, !85, !86, !90}
!72 = !{!"callableShaderType", !"NumberOfCallableShaderTypes"}
!73 = !{!"isContinuation", i1 false}
!74 = !{!"hasTraceRayPayload", i1 false}
!75 = !{!"hasHitAttributes", i1 false}
!76 = !{!"hasCallableData", i1 false}
!77 = !{!"ShaderStackSize", i32 0}
!78 = !{!"ShaderHash", i64 0}
!79 = !{!"ShaderName", !""}
!80 = !{!"ParentName", !""}
!81 = !{!"SlotNum", i1* null}
!82 = !{!"NOSSize", i32 0}
!83 = !{!"Entries"}
!84 = !{!"SpillUnions"}
!85 = !{!"CustomHitAttrSizeInBytes", i32 0}
!86 = !{!"Types", !87, !88, !89}
!87 = !{!"FrameStartTys"}
!88 = !{!"ArgumentTys"}
!89 = !{!"FullFrameTys"}
!90 = !{!"Aliases"}
!91 = !{!"resAllocMD", !92, !93, !94, !95, !96}
!92 = !{!"uavsNumType", i32 0}
!93 = !{!"srvsNumType", i32 0}
!94 = !{!"samplersNumType", i32 0}
!95 = !{!"argAllocMDList"}
!96 = !{!"inlineSamplersMD"}
!97 = !{!"maxByteOffsets"}
!98 = !{!"IsInitializer", i1 false}
!99 = !{!"IsFinalizer", i1 false}
!100 = !{!"CompiledSubGroupsNumber", i32 0}
!101 = !{!"hasInlineVmeSamplers", i1 false}
!102 = !{!"localSize", i32 0}
!103 = !{!"localIDPresent", i1 false}
!104 = !{!"groupIDPresent", i1 false}
!105 = !{!"privateMemoryPerWI", i32 0}
!106 = !{!"globalIDPresent", i1 false}
!107 = !{!"hasSyncRTCalls", i1 false}
!108 = !{!"hasNonKernelArgLoad", i1 false}
!109 = !{!"hasNonKernelArgStore", i1 false}
!110 = !{!"hasNonKernelArgAtomic", i1 false}
!111 = !{!"UserAnnotations"}
!112 = !{!"m_OpenCLArgAddressSpaces"}
!113 = !{!"m_OpenCLArgAccessQualifiers"}
!114 = !{!"m_OpenCLArgTypes"}
!115 = !{!"m_OpenCLArgBaseTypes"}
!116 = !{!"m_OpenCLArgTypeQualifiers"}
!117 = !{!"m_OpenCLArgNames"}
!118 = !{!"FuncMDMap[1]", void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*, i32)* @test}
!119 = !{!"FuncMDValue[1]", !64, !65, !69, !120, !71, !121, !97, !98, !99, !100, !101, !102, !103, !104, !105, !106, !107, !108, !109, !110, !111, !139, !141, !143, !145, !147, !149}
!120 = !{!"functionType", !"KernelFunction"}
!121 = !{!"resAllocMD", !122, !93, !94, !123, !96}
!122 = !{!"uavsNumType", i32 2}
!123 = !{!"argAllocMDList", !124, !128, !131, !132, !133, !134, !135, !136, !138}
!124 = !{!"argAllocMDListVec[0]", !125, !126, !127}
!125 = !{!"type", i32 1}
!126 = !{!"extensionType", i32 -1}
!127 = !{!"indexType", i32 0}
!128 = !{!"argAllocMDListVec[1]", !129, !126, !130}
!129 = !{!"type", i32 0}
!130 = !{!"indexType", i32 -1}
!131 = !{!"argAllocMDListVec[2]", !129, !126, !130}
!132 = !{!"argAllocMDListVec[3]", !129, !126, !130}
!133 = !{!"argAllocMDListVec[4]", !129, !126, !130}
!134 = !{!"argAllocMDListVec[5]", !129, !126, !130}
!135 = !{!"argAllocMDListVec[6]", !129, !126, !130}
!136 = !{!"argAllocMDListVec[7]", !125, !126, !137}
!137 = !{!"indexType", i32 1}
!138 = !{!"argAllocMDListVec[8]", !129, !126, !130}
!139 = !{!"m_OpenCLArgAddressSpaces", !140}
!140 = !{!"m_OpenCLArgAddressSpacesVec[0]", i32 1}
!141 = !{!"m_OpenCLArgAccessQualifiers", !142}
!142 = !{!"m_OpenCLArgAccessQualifiersVec[0]", !"none"}
!143 = !{!"m_OpenCLArgTypes", !144}
!144 = !{!"m_OpenCLArgTypesVec[0]", !"int*"}
!145 = !{!"m_OpenCLArgBaseTypes", !146}
!146 = !{!"m_OpenCLArgBaseTypesVec[0]", !"int*"}
!147 = !{!"m_OpenCLArgTypeQualifiers", !148}
!148 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!149 = !{!"m_OpenCLArgNames", !150}
!150 = !{!"m_OpenCLArgNamesVec[0]", !"out"}
!151 = !{!"pushInfo", !152, !153, !154, !157, !158, !159, !160, !161, !162, !163, !164, !177, !178, !179, !180}
!152 = !{!"pushableAddresses"}
!153 = !{!"bindlessPushInfo"}
!154 = !{!"dynamicBufferInfo", !155, !156}
!155 = !{!"firstIndex", i32 0}
!156 = !{!"numOffsets", i32 0}
!157 = !{!"MaxNumberOfPushedBuffers", i32 0}
!158 = !{!"inlineConstantBufferSlot", i32 -1}
!159 = !{!"inlineConstantBufferOffset", i32 -1}
!160 = !{!"inlineConstantBufferGRFOffset", i32 -1}
!161 = !{!"constants"}
!162 = !{!"inputs"}
!163 = !{!"constantReg"}
!164 = !{!"simplePushInfoArr", !165, !174, !175, !176}
!165 = !{!"simplePushInfoArrVec[0]", !166, !167, !168, !169, !170, !171, !172, !173}
!166 = !{!"cbIdx", i32 0}
!167 = !{!"pushableAddressGrfOffset", i32 -1}
!168 = !{!"pushableOffsetGrfOffset", i32 -1}
!169 = !{!"offset", i32 0}
!170 = !{!"size", i32 0}
!171 = !{!"isStateless", i1 false}
!172 = !{!"isBindless", i1 false}
!173 = !{!"simplePushLoads"}
!174 = !{!"simplePushInfoArrVec[1]", !166, !167, !168, !169, !170, !171, !172, !173}
!175 = !{!"simplePushInfoArrVec[2]", !166, !167, !168, !169, !170, !171, !172, !173}
!176 = !{!"simplePushInfoArrVec[3]", !166, !167, !168, !169, !170, !171, !172, !173}
!177 = !{!"simplePushBufferUsed", i32 0}
!178 = !{!"pushAnalysisWIInfos"}
!179 = !{!"inlineRTGlobalPtrOffset", i32 0}
!180 = !{!"rtSyncSurfPtrOffset", i32 0}
!181 = !{!"psInfo", !182, !183, !184, !185, !186, !187, !188, !189, !190, !191, !192, !193, !194, !195, !196}
!182 = !{!"BlendStateDisabledMask", i8 0}
!183 = !{!"SkipSrc0Alpha", i1 false}
!184 = !{!"DualSourceBlendingDisabled", i1 false}
!185 = !{!"ForceEnableSimd32", i1 false}
!186 = !{!"outputDepth", i1 false}
!187 = !{!"outputStencil", i1 false}
!188 = !{!"outputMask", i1 false}
!189 = !{!"blendToFillEnabled", i1 false}
!190 = !{!"forceEarlyZ", i1 false}
!191 = !{!"hasVersionedLoop", i1 false}
!192 = !{!"forceSingleSourceRTWAfterDualSourceRTW", i1 false}
!193 = !{!"NumSamples", i8 0}
!194 = !{!"blendOptimizationMode"}
!195 = !{!"colorOutputMask"}
!196 = !{!"WaDisableVRS", i1 false}
!197 = !{!"csInfo", !198, !199, !200, !201, !202, !11, !12, !203, !204, !205, !206, !207, !208, !209, !210, !211, !212, !40, !213}
!198 = !{!"maxWorkGroupSize", i32 0}
!199 = !{!"waveSize", i32 0}
!200 = !{!"ComputeShaderSecondCompile"}
!201 = !{!"forcedSIMDSize", i8 0}
!202 = !{!"forceTotalGRFNum", i32 0}
!203 = !{!"allowLowerSimd", i1 false}
!204 = !{!"forcedVISAPreRAScheduler", i1 false}
!205 = !{!"disableLocalIdOrderOptimizations", i1 false}
!206 = !{!"disableDispatchAlongY", i1 false}
!207 = !{!"neededThreadIdLayout", i1* null}
!208 = !{!"forceTileYWalk", i1 false}
!209 = !{!"tileTGWidth_X", i32 0}
!210 = !{!"tileTGWidth_Y", i32 0}
!211 = !{!"maxTileTGDispatch_X", i32 0}
!212 = !{!"maxTileTGDispatch_Y", i32 0}
!213 = !{!"ResForHfPacking"}
!214 = !{!"msInfo", !215, !216, !217, !218, !219, !220, !221, !222, !223}
!215 = !{!"PrimitiveTopology", i32 3}
!216 = !{!"MaxNumOfPrimitives", i32 0}
!217 = !{!"MaxNumOfVertices", i32 0}
!218 = !{!"MaxNumOfPerPrimitiveOutputs", i32 0}
!219 = !{!"MaxNumOfPerVertexOutputs", i32 0}
!220 = !{!"WorkGroupSize", i32 0}
!221 = !{!"WorkGroupMemorySizeInBytes", i32 0}
!222 = !{!"IndexFormat", i32 6}
!223 = !{!"SubgroupSize", i32 0}
!224 = !{!"taskInfo", !225, !220, !221, !223}
!225 = !{!"MaxNumOfOutputs", i32 0}
!226 = !{!"NBarrierCnt", i32 0}
!227 = !{!"rtInfo", !228, !229, !230, !231, !232, !233, !234, !235, !236, !237}
!228 = !{!"RayQueryAllocSizeInBytes", i32 0}
!229 = !{!"NumContinuations", i32 -1}
!230 = !{!"RTAsyncStackAddrspace", i32 -1}
!231 = !{!"RTAsyncStackSurfaceStateOffset", i1* null}
!232 = !{!"SWHotZoneAddrspace", i32 -1}
!233 = !{!"SWHotZoneSurfaceStateOffset", i1* null}
!234 = !{!"SWStackAddrspace", i32 -1}
!235 = !{!"SWStackSurfaceStateOffset", i1* null}
!236 = !{!"RTSyncStackAddrspace", i32 -1}
!237 = !{!"RTSyncStackSurfaceStateOffset", i1* null}
!238 = !{!"CurUniqueIndirectIdx", i32 0}
!239 = !{!"inlineDynTextures"}
!240 = !{!"inlineResInfoData"}
!241 = !{!"immConstant", !242, !243, !244}
!242 = !{!"data"}
!243 = !{!"sizes"}
!244 = !{!"zeroIdxs"}
!245 = !{!"inlineConstantBuffers"}
!246 = !{!"inlineGlobalBuffers"}
!247 = !{!"GlobalPointerProgramBinaryInfos"}
!248 = !{!"ConstantPointerProgramBinaryInfos"}
!249 = !{!"GlobalBufferAddressRelocInfo"}
!250 = !{!"ConstantBufferAddressRelocInfo"}
!251 = !{!"forceLscCacheList"}
!252 = !{!"SrvMap"}
!253 = !{!"RasterizerOrderedByteAddressBuffer"}
!254 = !{!"MinNOSPushConstantSize", i32 0}
!255 = !{!"inlineProgramScopeOffsets"}
!256 = !{!"shaderData", !257}
!257 = !{!"numReplicas", i32 0}
!258 = !{!"URBInfo", !259, !260, !261}
!259 = !{!"has64BVertexHeaderInput", i1 false}
!260 = !{!"has64BVertexHeaderOutput", i1 false}
!261 = !{!"hasVertexHeader", i1 true}
!262 = !{!"UseBindlessImage", i1 false}
!263 = !{!"enableRangeReduce", i1 false}
!264 = !{!"allowMatchMadOptimizationforVS", i1 false}
!265 = !{!"disableMemOptforNegativeOffsetLoads", i1 false}
!266 = !{!"statefulResourcesNotAliased", i1 false}
!267 = !{!"disableMixMode", i1 false}
!268 = !{!"PrivateMemoryPerFG"}
!269 = !{!"capabilities", !270}
!270 = !{!"globalVariableDecorationsINTEL", i1 false}
!271 = !{!"m_ShaderResourceViewMcsMask", !272, !273}
!272 = !{!"m_ShaderResourceViewMcsMaskVec[0]", i64 0}
!273 = !{!"m_ShaderResourceViewMcsMaskVec[1]", i64 0}
!274 = !{!"computedDepthMode", i32 0}
!275 = !{!"isHDCFastClearShader", i1 false}
!276 = !{i64 ()* @get_global, !277}
!277 = !{!278, !279}
!278 = !{!"function_type", i32 2}
!279 = !{!"implicit_arg_desc"}
!280 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16, i8*, i32)* @test, !281}
!281 = !{!282, !283}
!282 = !{!"function_type", i32 0}
!283 = !{!"implicit_arg_desc", !284, !285, !286, !287, !288, !289, !290, !291}
!284 = !{i32 0}
!285 = !{i32 1}
!286 = !{i32 6}
!287 = !{i32 7}
!288 = !{i32 8}
!289 = !{i32 9}
!290 = !{i32 12}
!291 = !{i32 14, !292}
!292 = !{!"explicit_arg_num", i32 0}
!293 = !{i32 2, i32 0}
!294 = !{!"clang version 11.1.0"}
!295 = !{i32 1, !"wchar_size", i32 4}
