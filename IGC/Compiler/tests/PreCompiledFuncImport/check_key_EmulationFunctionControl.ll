;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
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
; REQUIRES: llvm-14-plus, regkeys
;
; 1. Test EmulationFunctionControl=1, forcing inline
; RUN: igc_opt --opaque-pointers %s -regkey TestIGCPreCompiledFunctions=1,EmulationFunctionControl=1 \
; RUN:            --platformdg2 --igc-precompiled-import --print-codegencontext -S 2>&1 \
; RUN:   | FileCheck %s --check-prefix=FC1
;
; FC1: m_enableSubroutine: 1
; FC1-LABEL: define spir_kernel void @test_emudp
; FC1: {{; Function Attrs:}}
; FC1: {{; Function Attrs: alwaysinline}}
; FC1-NEXT: define internal double @__igcbuiltin_dp_sqrt


;
; 2. Test EmulationFunctionControl=2, forcing subroutine
; RUN: igc_opt --opaque-pointers %s -regkey TestIGCPreCompiledFunctions=1,EmulationFunctionControl=2 \
; RUN:            --platformdg2 --igc-precompiled-import --print-codegencontext -S 2>&1 \
; RUN:   | FileCheck %s --check-prefix=FC2
;
; FC2: m_enableSubroutine: 1
; FC2-LABEL: define spir_kernel void @test_emudp
; FC2: {{; Function Attrs:}}
; FC2: {{; Function Attrs: noinline}}
; FC2-NEXT: define internal double @__igcbuiltin_dp_sqrt
; FC2-NOT:"visaStackCall"


;
; 3. Test EmulationFunctionControl=3, forcing subroutine
;
; RUN: igc_opt --opaque-pointers %s -regkey TestIGCPreCompiledFunctions=1,EmulationFunctionControl=3 \
; RUN:            --platformdg2 --igc-precompiled-import --print-codegencontext -S 2>&1 \
; RUN:   | FileCheck %s --check-prefix=FC3
;
; FC3: m_enableSubroutine: 1
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

!0 = !{!"ModuleMD", !71}
!71 = !{!"FuncMD", !72, !73}
!72 = !{!"FuncMDMap[0]", void (double addrspace(1)*, double addrspace(1)*, <8 x i32>, <8 x i32>, i8*)* @test_emudp}
!73 = !{!"FuncMDValue[0]", !107, !152}
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
!152 = !{!"m_OpenCLArgTypeQualifiers", !153, !154}
!153 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!154 = !{!"m_OpenCLArgTypeQualifiersVec[1]", !""}
!158 = !{!"m_OpenCLArgScalarAsPointers"}

!325 = !{void (double addrspace(1)*, double addrspace(1)*, <8 x i32>, <8 x i32>, i8*)* @test_emudp, !326}
!326 = !{!327, !328}
!327 = !{!"function_type", i32 0}
!328 = !{!"implicit_arg_desc", !329, !330, !331}
!329 = !{i32 0}
!330 = !{i32 1}
!331 = !{i32 13}
