;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; Ensure image reads from images with read_write access qualifiers are not routed
; through sampling engine.

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%spirv.Image._void_0_0_1_0_0_0_2 = type opaque
%spirv.Image._void_0_0_1_0_0_0_0 = type opaque
%spirv.Image._void_0_0_1_0_0_0_1 = type opaque

; RUN: igc_opt --opaque-pointers %s -S -o - -igc-conv-ocl-to-common -platformmtl | FileCheck %s

; Function Attrs: convergent nounwind
define spir_kernel void @testKernel(%spirv.Image._void_0_0_1_0_0_0_2 addrspace(1)* %img, %spirv.Image._void_0_0_1_0_0_0_0 addrspace(1)* %newImg, %spirv.Image._void_0_0_1_0_0_0_1 addrspace(1)* %storeResultsImg, <8 x i32> %r0, <3 x i32> %globalOffset, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8 addrspace(1)* %indirectDataPointer, i8 addrspace(1)* %scratchPointer) #0 {
entry:
; CHECK-NOT: ldptr
; CHECK:     typedread
; CHECK:     bitcast <4 x float> %{{.*}} to <4 x i32>
  %vecinit = insertelement <2 x i32> undef, i32 1, i32 0
  %vecinit3 = insertelement <2 x i32> %vecinit, i32 2, i32 1
  %a0 = ptrtoint %spirv.Image._void_0_0_1_0_0_0_2 addrspace(1)* %img to i64
  %a1 = call spir_func <4 x float> @__builtin_IB_OCL_1darr_ld_rw(i64 noundef %a0, <2 x i32> noundef %vecinit3, i32 noundef 0) #4
  %b0 = ptrtoint %spirv.Image._void_0_0_1_0_0_0_2 addrspace(1)* %img to i64
  %b1 = call spir_func <4 x i32> @__builtin_IB_OCL_1darr_ldui_rw(i64 noundef %b0, <2 x i32> noundef %vecinit3, i32 noundef 0) #4
  ret void
}

; Function Attrs: convergent
declare spir_func <4 x float> @__builtin_IB_OCL_1darr_ld_rw(i64 noundef, <2 x i32> noundef, i32 noundef) local_unnamed_addr #1

; Function Attrs: convergent
declare spir_func <4 x i32> @__builtin_IB_OCL_1darr_ldui_rw(i64 noundef, <2 x i32> noundef, i32 noundef) local_unnamed_addr #1

attributes #0 = { convergent nounwind "less-precise-fpmad"="true" }
attributes #1 = { convergent "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { convergent mustprogress nofree nounwind readnone willreturn "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #3 = { inaccessiblememonly nocallback nofree nosync nounwind willreturn }
attributes #4 = { convergent nounwind }

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!spirv.Generator = !{!2}
!igc.functions = !{!3}
!IGCMetadata = !{!15}
!opencl.ocl.version = !{!474, !474, !474, !474, !474, !474, !474, !474, !474, !474, !474, !474, !474, !474, !474, !474}
!opencl.spir.version = !{!474, !474, !474, !474, !474, !474, !474, !474, !474, !474, !474, !474, !474, !474, !474, !474}
!llvm.ident = !{!475, !475, !475, !475, !475, !475, !475, !475, !475, !475, !475, !475, !475, !475, !475, !475}
!llvm.module.flags = !{!476}

!0 = !{i32 2, i32 2}
!1 = !{i32 3, i32 200000}
!2 = !{i16 6, i16 14}
!3 = !{void (%spirv.Image._void_0_0_1_0_0_0_2 addrspace(1)*, %spirv.Image._void_0_0_1_0_0_0_0 addrspace(1)*, %spirv.Image._void_0_0_1_0_0_0_1 addrspace(1)*, <8 x i32>, <3 x i32>, <3 x i32>, i16, i16, i16, i8 addrspace(1)*, i8 addrspace(1)*)* @testKernel, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8, !9, !10, !11, !12, !13, !14}
!7 = !{i32 0}
!8 = !{i32 2}
!9 = !{i32 7}
!10 = !{i32 8}
!11 = !{i32 9}
!12 = !{i32 10}
!13 = !{i32 56}
!14 = !{i32 57}
!15 = !{!"ModuleMD", !135, !431}
!135 = !{!"FuncMD", !136, !137}
!136 = !{!"FuncMDMap[0]", void (%spirv.Image._void_0_0_1_0_0_0_2 addrspace(1)*, %spirv.Image._void_0_0_1_0_0_0_0 addrspace(1)*, %spirv.Image._void_0_0_1_0_0_0_1 addrspace(1)*, <8 x i32>, <3 x i32>, <3 x i32>, i16, i16, i16, i8 addrspace(1)*, i8 addrspace(1)*)* @testKernel}
!137 = !{!"FuncMDValue[0]", !170, !211, !215, !219, !223, !227, !231, !235, !236}
!170 = !{!"resAllocMD", !171, !172, !173, !174, !194}
!171 = !{!"uavsNumType", i32 3}
!172 = !{!"srvsNumType", i32 0}
!173 = !{!"samplersNumType", i32 0}
!174 = !{!"argAllocMDList", !175, !179, !181, !183, !187, !188, !189, !190, !191, !192, !193}
!175 = !{!"argAllocMDListVec[0]", !176, !177, !178}
!176 = !{!"type", i32 4}
!177 = !{!"extensionType", i32 0}
!178 = !{!"indexType", i32 0}
!179 = !{!"argAllocMDListVec[1]", !176, !177, !180}
!180 = !{!"indexType", i32 1}
!181 = !{!"argAllocMDListVec[2]", !176, !177, !182}
!182 = !{!"indexType", i32 2}
!183 = !{!"argAllocMDListVec[3]", !184, !185, !186}
!184 = !{!"type", i32 0}
!185 = !{!"extensionType", i32 -1}
!186 = !{!"indexType", i32 -1}
!187 = !{!"argAllocMDListVec[4]", !184, !185, !186}
!188 = !{!"argAllocMDListVec[5]", !184, !185, !186}
!189 = !{!"argAllocMDListVec[6]", !184, !185, !186}
!190 = !{!"argAllocMDListVec[7]", !184, !185, !186}
!191 = !{!"argAllocMDListVec[8]", !184, !185, !186}
!192 = !{!"argAllocMDListVec[9]", !184, !185, !186}
!193 = !{!"argAllocMDListVec[10]", !184, !185, !186}
!194 = !{!"inlineSamplersMD"}
!211 = !{!"m_OpenCLArgAddressSpaces", !212, !213, !214}
!212 = !{!"m_OpenCLArgAddressSpacesVec[0]", i32 1}
!213 = !{!"m_OpenCLArgAddressSpacesVec[1]", i32 1}
!214 = !{!"m_OpenCLArgAddressSpacesVec[2]", i32 1}
!215 = !{!"m_OpenCLArgAccessQualifiers", !216, !217, !218}
!216 = !{!"m_OpenCLArgAccessQualifiersVec[0]", !"read_write"}
!217 = !{!"m_OpenCLArgAccessQualifiersVec[1]", !"read_only"}
!218 = !{!"m_OpenCLArgAccessQualifiersVec[2]", !"write_only"}
!219 = !{!"m_OpenCLArgTypes", !220, !221, !222}
!220 = !{!"m_OpenCLArgTypesVec[0]", !"image1d_array_t"}
!221 = !{!"m_OpenCLArgTypesVec[1]", !"image1d_array_t"}
!222 = !{!"m_OpenCLArgTypesVec[2]", !"image1d_array_t"}
!223 = !{!"m_OpenCLArgBaseTypes", !224, !225, !226}
!224 = !{!"m_OpenCLArgBaseTypesVec[0]", !"image1d_array_t"}
!225 = !{!"m_OpenCLArgBaseTypesVec[1]", !"image1d_array_t"}
!226 = !{!"m_OpenCLArgBaseTypesVec[2]", !"image1d_array_t"}
!227 = !{!"m_OpenCLArgTypeQualifiers", !228, !229, !230}
!228 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!229 = !{!"m_OpenCLArgTypeQualifiersVec[1]", !""}
!230 = !{!"m_OpenCLArgTypeQualifiersVec[2]", !""}
!231 = !{!"m_OpenCLArgNames", !232, !233, !234}
!232 = !{!"m_OpenCLArgNamesVec[0]", !"img"}
!233 = !{!"m_OpenCLArgNamesVec[1]", !"newImg"}
!234 = !{!"m_OpenCLArgNamesVec[2]", !"storeResultsImg"}
!235 = !{!"m_OpenCLArgScalarAsPointers"}
!236 = !{!"m_OptsToDisablePerFunc"}
!431 = !{!"UseBindlessImage", i1 true}
!474 = !{i32 2, i32 0}
!475 = !{!"clang version 15.0.0"}
!476 = !{i32 1, !"wchar_size", i32 4}
