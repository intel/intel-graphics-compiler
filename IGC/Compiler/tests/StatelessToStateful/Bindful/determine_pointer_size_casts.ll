;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys, llvm-16-plus
; RUN: igc_opt --typed-pointers --regkey EnableOptionalBufferOffset=1 --regkey EnableSupportBufferOffset=1  -igc-stateless-to-stateful-resolution -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,%LLVM_DEPENDENT_CHECK_PREFIX%
; RUN: igc_opt --opaque-pointers --regkey EnableOptionalBufferOffset=1 --regkey EnableSupportBufferOffset=1  -igc-stateless-to-stateful-resolution -S < %s 2>&1 | FileCheck %s --check-prefixes=CHECK,%LLVM_DEPENDENT_CHECK_PREFIX%
; ------------------------------------------------
; StatelessToStateful - check if determinePointerAlignment handles bitcasts correctly:
; the found alignment is biggest alignment of load / stores, so 4.
; ------------------------------------------------


; CHECK-LLVM-16: [[add1:%.*]] = add i32 %bufferOffset1, 1
; CHECK-LLVM-16: [[inttoptr1:%.*]] = inttoptr i32 [[add1]] to {{ptr|i8}} addrspace({{[0-9]+}}){{.*}}

; CHECK-LLVM-14: [[inttoptr1:%.*]] = inttoptr i32 1 to i8
; CHECK-LLVM-15: [[inttoptr1:%.*]] = inttoptr i32 1 to i8

; CHECK: [[load1:%.*]] = load i8, {{ptr|i8}} addrspace({{[0-9]+}}){{.*}} [[inttoptr1]], align 1

; CHECK-LLVM-16: [[add2:%.*]] = add i32 %bufferOffset, 1
; CHECK-LLVM-16: [[inttoptr2:%.*]] = inttoptr i32 [[add2]] to {{ptr|i8}} addrspace({{[0-9]+}}){{.*}}

; CHECK-LLVM-14: [[inttoptr2:%.*]] = inttoptr i32 1 to i8
; CHECK-LLVM-15: [[inttoptr2:%.*]] = inttoptr i32 1 to i8

; CHECK: store i8 [[load1]], {{ptr|i8}} addrspace({{[0-9]+}}){{.*}} [[inttoptr2]], align 1

; Function Attrs: convergent nounwind
define spir_kernel void @_ZTS16KernelTestMemcpy(i8 addrspace(1)* align 1 %0, i8 addrspace(1)* readonly align 1 %1, <8 x i32> %r0, <3 x i32> %globalOffset, i32 %bufferOffset, i32 %bufferOffset1) #0 {
  %3 = addrspacecast i8 addrspace(1)* %0 to i8 addrspace(4)*
  %4 = addrspacecast i8 addrspace(1)* %1 to i8 addrspace(4)*

  %5 = getelementptr inbounds i8, i8 addrspace(1)* %1, i64 1
  %6 = load i8, i8 addrspace(1)* %5, align 1

  %7 = getelementptr inbounds i8, i8 addrspace(1)* %0, i64 1
  store i8 %6, i8 addrspace(1)* %7, align 1

  %8 = bitcast i8 addrspace(1)* %1 to i32 addrspace(1)*
  %9 = bitcast i8 addrspace(1)* %0 to i32 addrspace(1)*

  %10 = load i32, i32 addrspace(1)* %8, align 4
  store i32 %10, i32 addrspace(1)* %9, align 4

  ret void
}

attributes #0 = { convergent nounwind "less-precise-fpmad"="true" }

!igc.functions = !{!3}
!IGCMetadata = !{!0}

!3 = !{void (i8 addrspace(1)*, i8 addrspace(1)*, <8 x i32>, <3 x i32>, i32, i32)* @_ZTS16KernelTestMemcpy, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8, !9, !11}
!7 = !{i32 0}
!8 = !{i32 2}
!9 = !{i32 15, !10}
!10 = !{!"explicit_arg_num", i32 0}
!11 = !{i32 15, !12}
!12 = !{!"explicit_arg_num", i32 1}

!0 = !{!"ModuleMD", !160, !100, !101}
!100 = !{!"isPrecise", i1 false}

!101 = !{!"compOpt"}

!160 = !{!"FuncMD", !161, !162}
!161 = !{!"FuncMDMap[0]", void (i8 addrspace(1)*, i8 addrspace(1)*, <8 x i32>, <3 x i32>, i32, i32)* @_ZTS16KernelTestMemcpy}
!162 = !{!"FuncMDValue[0]", !163, !164, !168, !169, !170, !191, !206, !207, !208, !209, !210, !211, !212, !213, !214, !215, !216, !217, !218, !219, !220, !221, !222, !223, !224, !227, !230, !233, !236, !239, !242, !243}

!163 = !{!"localOffsets"}
!164 = !{!"workGroupWalkOrder", !165, !166, !167}
!165 = !{!"dim0", i32 0}
!166 = !{!"dim1", i32 1}
!167 = !{!"dim2", i32 2}
!168 = !{!"funcArgs"}
!169 = !{!"functionType", !"KernelFunction"}
!170 = !{!"rtInfo", !171, !172, !173, !174, !175, !176, !177, !178, !179, !180, !181, !182, !183, !184, !185, !186, !188, !189, !190}
!171 = !{!"callableShaderType", !"NumberOfCallableShaderTypes"}
!172 = !{!"isContinuation", i1 false}
!173 = !{!"hasTraceRayPayload", i1 false}
!174 = !{!"hasHitAttributes", i1 false}
!175 = !{!"hasCallableData", i1 false}
!176 = !{!"ShaderStackSize", i32 0}
!177 = !{!"ShaderHash", i64 0}
!178 = !{!"ShaderName", !""}
!179 = !{!"ParentName", !""}
!180 = !{!"SlotNum", i1* null}
!181 = !{!"NOSSize", i32 0}
!182 = !{!"globalRootSignatureSize", i32 0}
!183 = !{!"Entries"}
!184 = !{!"SpillUnions"}
!185 = !{!"CustomHitAttrSizeInBytes", i32 0}
!186 = !{!"Types", !187}
!187 = !{!"FullFrameTys"}
!188 = !{!"Aliases"}
!189 = !{!"numSyncRTStacks", i32 0}
!190 = !{!"NumCoherenceHintBits", i32 0}
!191 = !{!"resAllocMD", !192, !193, !194, !195, !205}
!192 = !{!"uavsNumType", i32 0}
!193 = !{!"srvsNumType", i32 0}
!194 = !{!"samplersNumType", i32 0}
!195 = !{!"argAllocMDList", !196, !200, !201, !202, !203, !204}
!196 = !{!"argAllocMDListVec[0]", !197, !198, !199}
!197 = !{!"type", i32 0}
!198 = !{!"extensionType", i32 -1}
!199 = !{!"indexType", i32 -1}
!200 = !{!"argAllocMDListVec[1]", !197, !198, !199}
!201 = !{!"argAllocMDListVec[2]", !197, !198, !199}
!202 = !{!"argAllocMDListVec[3]", !197, !198, !199}
!203 = !{!"argAllocMDListVec[4]", !197, !198, !199}
!204 = !{!"argAllocMDListVec[5]", !197, !198, !199}
!205 = !{!"inlineSamplersMD"}
!206 = !{!"maxByteOffsets"}
!207 = !{!"IsInitializer", i1 false}
!208 = !{!"IsFinalizer", i1 false}
!209 = !{!"CompiledSubGroupsNumber", i32 0}
!210 = !{!"hasInlineVmeSamplers", i1 false}
!211 = !{!"localSize", i32 0}
!212 = !{!"localIDPresent", i1 false}
!213 = !{!"groupIDPresent", i1 false}
!214 = !{!"privateMemoryPerWI", i32 0}
!215 = !{!"prevFPOffset", i32 0}
!216 = !{!"globalIDPresent", i1 false}
!217 = !{!"hasSyncRTCalls", i1 false}
!218 = !{!"hasPrintfCalls", i1 false}
!219 = !{!"hasIndirectCalls", i1 false}
!220 = !{!"hasNonKernelArgLoad", i1 false}
!221 = !{!"hasNonKernelArgStore", i1 false}
!222 = !{!"hasNonKernelArgAtomic", i1 false}
!223 = !{!"UserAnnotations"}
!224 = !{!"m_OpenCLArgAddressSpaces", !225, !226}
!225 = !{!"m_OpenCLArgAddressSpacesVec[0]", i32 1}
!226 = !{!"m_OpenCLArgAddressSpacesVec[1]", i32 1}
!227 = !{!"m_OpenCLArgAccessQualifiers", !228, !229}
!228 = !{!"m_OpenCLArgAccessQualifiersVec[0]", !"none"}
!229 = !{!"m_OpenCLArgAccessQualifiersVec[1]", !"none"}
!230 = !{!"m_OpenCLArgTypes", !231, !232}
!231 = !{!"m_OpenCLArgTypesVec[0]", !"char*"}
!232 = !{!"m_OpenCLArgTypesVec[1]", !"char*"}
!233 = !{!"m_OpenCLArgBaseTypes", !234, !235}
!234 = !{!"m_OpenCLArgBaseTypesVec[0]", !"char*"}
!235 = !{!"m_OpenCLArgBaseTypesVec[1]", !"char*"}
!236 = !{!"m_OpenCLArgTypeQualifiers", !237, !238}
!237 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!238 = !{!"m_OpenCLArgTypeQualifiersVec[1]", !""}
!239 = !{!"m_OpenCLArgNames", !240, !241}
!240 = !{!"m_OpenCLArgNamesVec[0]", !""}
!241 = !{!"m_OpenCLArgNamesVec[1]", !""}
!242 = !{!"m_OpenCLArgScalarAsPointers"}
!243 = !{!"m_OptsToDisablePerFunc", !244, !245, !246}
!244 = !{!"m_OptsToDisablePerFuncSet[0]", !"IGC-AddressArithmeticSinking"}
!245 = !{!"m_OptsToDisablePerFuncSet[1]", !"IGC-AllowSimd32Slicing"}
!246 = !{!"m_OptsToDisablePerFuncSet[2]", !"IGC-SinkLoadOpt"}
