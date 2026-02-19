;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -debugify -igc-mcs-optimization -check-debugify -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; MCSOptimization
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

define void @test(<4 x float> addrspace(2)* %s1, <4 x float>* %dst) {
; CHECK-LABEL: @test(
; CHECK:    [[TMP1:%[A-z0-9]*]] = call <4 x i32> @llvm.genx.GenISA.ldmcsptr.v4i32.i32.p2v4f32(i32 1, i32 2, i32 3, i32 4, <4 x float> addrspace(2)* [[S1:%[A-z0-9]*]], i32 0, i32 0, i32 0)
; CHECK:    [[TMP2:%[A-z0-9]*]] = extractelement <4 x i32> [[TMP1]], i32 0
; CHECK:    [[TMP3:%[A-z0-9]*]] = call <4 x float> @llvm.genx.GenISA.ldmsptr.v4f32.p2v4f32(i32 2, i32 [[TMP2]], i32 4, i32 5, i32 1, i32 2, i32 3, <4 x float> addrspace(2)* [[S1]], i32 0, i32 0, i32 0)
; CHECK:    [[TMP4:%[A-z0-9]*]] = icmp ne i32 [[TMP2]], 0
; CHECK:    [[TMP5:%[A-z0-9]*]] = or i1 [[TMP4]], true
; CHECK:    br i1 [[TMP5]], label [[TMP6:%[A-z0-9]*]], label [[TMP8:%[A-z0-9]*]]
; CHECK:  6:
; CHECK:    [[TMP7:%[A-z0-9]*]] = call <4 x float> @llvm.genx.GenISA.ldmsptr.v4f32.p2v4f32(i32 2, i32 [[TMP2]], i32 4, i32 5, i32 1, i32 2, i32 3, <4 x float> addrspace(2)* [[S1]], i32 0, i32 0, i32 0)
; CHECK:    br label [[TMP8]]
; CHECK:  8:
; CHECK:    [[TMP9:%[A-z0-9]*]] = phi <4 x float> [ [[TMP7]], [[TMP6]] ], [ [[TMP3]], [[TMP0:%[A-z0-9]*]] ]
; CHECK:    [[TMP10:%[A-z0-9]*]] = fadd <4 x float> [[TMP3]], [[TMP9]]
; CHECK:    store <4 x float> [[TMP10]], <4 x float>* [[DST:%[A-z0-9]*]]
; CHECK:    ret void
;
  %1 = call <4 x i32> @llvm.genx.GenISA.ldmcsptr.v4i32.i32.p2v4f32(i32 1, i32 2, i32 3, i32 4, <4 x float> addrspace(2)* %s1, i32 0, i32 0, i32 0)
  %2 = extractelement <4 x i32> %1, i32 0
  %3 = call <4 x float> @llvm.genx.GenISA.ldmsptr.v4f32.p2v4f32(i32 2, i32 %2, i32 4, i32 5, i32 1, i32 2, i32 3, <4 x float> addrspace(2)* %s1, i32 0, i32 0, i32 0)
  %4 = call <4 x float> @llvm.genx.GenISA.ldmsptr.v4f32.p2v4f32(i32 2, i32 %2, i32 4, i32 5, i32 1, i32 2, i32 3, <4 x float> addrspace(2)* %s1, i32 0, i32 0, i32 0)
  %5 = fadd <4 x float> %3, %4
  store <4 x float> %5, <4 x float>* %dst
  ret void
}

declare <4 x float> @llvm.genx.GenISA.ldmsptr.v4f32.p2v4f32(i32, i32, i32, i32, i32, i32, i32, <4 x float> addrspace(2)*, i32, i32, i32)
declare <4 x i32> @llvm.genx.GenISA.ldmcsptr.v4i32.i32.p2v4f32(i32, i32, i32, i32, <4 x float> addrspace(2)*, i32, i32, i32)

!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"m_ShaderResourceViewMcsMask", !2, !3}
!2 = !{!"m_ShaderResourceViewMcsMaskVec[0]", i64 4}
!3 = !{!"m_ShaderResourceViewMcsMaskVec[1]", i64 4}
