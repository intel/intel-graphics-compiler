;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -LSC-Cache-Optimization-pass -S < %s | FileCheck %s
; ------------------------------------------------
; LSCCacheOptimizationPass
; ------------------------------------------------

; Store region offset equal zero case

; CHECK: @test(
; CHECK:  entry:
; CHECK:    [[TMP0:%.*]] = call dereferenceable(4) float addrspace(23)* @llvm.genx.GenISA.SWHotZonePtr.p23f32.i32(i32 [[B:%.*]])
; CHECK:    [[TMP1:%.*]] = load float, float addrspace(23)* [[TMP0]]
; CHECK:    [[TMP2:%.*]] = bitcast float addrspace(23)* [[TMP0]] to i8 addrspace(23)*
; CHECK:    [[TMP3:%.*]] = getelementptr i8, i8 addrspace(23)* [[TMP2]], i64 4
; CHECK:    [[TMP4:%.*]] = bitcast i8 addrspace(23)* [[TMP3]] to <3 x float> addrspace(23)*
; CHECK:    [[TMP5:%.*]] = load <3 x float>, <3 x float> addrspace(23)* [[TMP4]]
; CHECK:    [[TMP6:%.*]] = extractelement <3 x float> [[TMP5]], i64 0
; CHECK:    [[TMP7:%.*]] = extractelement <3 x float> [[TMP5]], i64 1
; CHECK:    [[TMP8:%.*]] = extractelement <3 x float> [[TMP5]], i64 2
; CHECK:    [[TMP9:%.*]] = insertelement <4 x float> undef, float [[TMP1]], i64 0
; CHECK:    [[TMP10:%.*]] = insertelement <4 x float> [[TMP9]], float [[TMP6]], i64 1
; CHECK:    [[TMP11:%.*]] = insertelement <4 x float> [[TMP10]], float [[TMP7]], i64 2
; CHECK:    [[TMP12:%.*]] = insertelement <4 x float> [[TMP11]], float [[TMP8]], i64 3
; CHECK:    [[TMP13:%.*]] = bitcast float addrspace(23)* [[TMP0]] to <4 x float> addrspace(23)*
; CHECK:    store <4 x float> [[TMP12]], <4 x float> addrspace(23)* [[TMP13]]
; CHECK:    ret void
;

define spir_kernel void @test(i32 %b) {
entry:
  %i = call dereferenceable(4) float addrspace(23)* @llvm.genx.GenISA.SWHotZonePtr.p23f32.i32(i32 %b)
  %i1 = load float, float addrspace(23)* %i, align 4
  store float %i1, float addrspace(23)* %i, align 4
  ret void
}


declare dereferenceable(4) float addrspace(23)* @llvm.genx.GenISA.SWHotZonePtr.p23f32.i32(i32)

!IGCMetadata = !{!0}
!igc.functions = !{}

!0 = !{!"ModuleMD", !1, !4}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (i32)* @test}
!3 = !{!"FuncMDValue[0]", !4}
!4 = !{!"rtInfo", !5}
!5 = !{!"SWHotZoneAddrspace", i32 23}
