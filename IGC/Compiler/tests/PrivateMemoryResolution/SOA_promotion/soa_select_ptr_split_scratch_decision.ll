;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; UNSUPPORTED: llvm-17-plus
;
; SplitSelectsOfAllocaPointers runs before PrivateMemoryResolution, so it must
; not read ModuleMetaData::compOpt.UseScratchSpacePrivateMemory: that field is
; only written by PrivateMemoryResolution itself and still holds its default
; (true) at this point. The scratch-space decision has to come from
; ModuleAllocaAnalysis::safeToUseScratchSpace instead.
;
; Here the metadata claims UseScratchSpacePrivateMemory = true, but OptDisable
; makes safeToUseScratchSpace return false, so the duplicated loads must still be
; predicated.
;
; RUN: igc_opt --typed-pointers -regkey EnableSelectOfAllocaPtrSplit=1,EnablePrivMemNewSOAForScalarArrays=1 --ocl --platformPtl \
; RUN:   --igc-split-selects-of-alloca-pointers --igc-private-mem-resolution -S %s | FileCheck %s
;
; CHECK-LABEL: @test_load_select_stale_scratch_flag(
; CHECK:       [[NC:%[A-Za-z0-9_.]+]] = xor i1 %c, true
; CHECK:       call float @llvm.genx.GenISA.PredicatedLoad.f32.p4f32.f32(float addrspace(4)* {{.*}}, i64 4, i1 %c, float poison)
; CHECK:       call float @llvm.genx.GenISA.PredicatedLoad.f32.p0f32.f32(float* {{.*}}, i64 4, i1 [[NC]], float poison)
; CHECK:       select i1 %c, float
; CHECK-NOT:   select i1 %c, float addrspace(4)*

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

define spir_kernel void @test_load_select_stale_scratch_flag(float addrspace(1)* nocapture writeonly %d, i8 addrspace(4)* %ext, i1 %c, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase) #0 {
entry:
  %pb = alloca [33 x float], align 4
  %idx = zext i32 %ix to i64
  %g = getelementptr inbounds [33 x float], [33 x float]* %pb, i64 0, i64 %idx
  %g_i8 = bitcast float* %g to i8*
  %g_as4 = addrspacecast i8* %g_i8 to i8 addrspace(4)*
  %sel = select i1 %c, i8 addrspace(4)* %ext, i8 addrspace(4)* %g_as4
  %p_f = bitcast i8 addrspace(4)* %sel to float addrspace(4)*
  %v = load float, float addrspace(4)* %p_f, align 4
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %d, i64 %idx
  store float %v, float addrspace(1)* %arrayidx, align 4
  ret void
}

attributes #0 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" }

!IGCMetadata = !{!3}
!igc.functions = !{!17}

!3 = !{!"ModuleMD", !4, !7, !13, !10}
!4 = !{!"compOpt", !5, !6}
!5 = !{!"OptDisable", i1 true}
!6 = !{!"UseScratchSpacePrivateMemory", i1 true}
!7 = !{!"FuncMD", !11, !12}
!10 = !{!"privateMemoryPerWI", i32 0}
!11 = !{!"FuncMDMap[0]", void (float addrspace(1)*, i8 addrspace(4)*, i1, i32, <8 x i32>, <8 x i32>, i8*)* @test_load_select_stale_scratch_flag}
!12 = !{!"FuncMDValue[0]", !10, !40}
!13 = !{!"MinNOSPushConstantSize", i32 0}
!17 = !{void (float addrspace(1)*, i8 addrspace(4)*, i1, i32, <8 x i32>, <8 x i32>, i8*)* @test_load_select_stale_scratch_flag, !18}
!18 = !{!19}
!19 = !{!"function_type", i32 0}
!28 = !{!"argId", i32 0}
!29 = !{!"implicitArgInfoListVec[0]", !28}
!30 = !{!"argId", i32 1}
!31 = !{!"implicitArgInfoListVec[1]", !30}
!32 = !{!"argId", i32 13}
!33 = !{!"implicitArgInfoListVec[2]", !32}
!40 = !{!"implicitArgInfoList", !29, !31, !33}
