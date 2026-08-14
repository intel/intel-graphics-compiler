;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; With private memory in stateless global (UseScratchSpacePrivateMemory = false)
; the duplicated loads must be predicated, because the not-taken operand may be
; out of bounds and nothing masks the read. On a platform without LSC there is no
; predicated load, so SplitSelectsOfAllocaPointers normally declines to split --
; unless DisablePredicatedLoadForAllocaPtrSelectSplit forces plain loads.
;
; RUN: igc_opt --opaque-pointers --ocl --platformtgllp \
; RUN:   --igc-split-selects-of-alloca-pointers --igc-private-mem-resolution \
; RUN:   --regkey EnableSelectOfAllocaPtrSplit=1,DisablePredicatedLoadForAllocaPtrSelectSplit=1,EnablePrivMemNewSOAForScalarArrays=1 -S %s \
; RUN:   | FileCheck %s --check-prefix=FORCED
;
; RUN: igc_opt --opaque-pointers --ocl --platformtgllp \
; RUN:   --igc-split-selects-of-alloca-pointers --igc-private-mem-resolution \
; RUN:   --regkey EnableSelectOfAllocaPtrSplit=1,EnablePrivMemNewSOAForScalarArrays=1 -S %s \
; RUN:   | FileCheck %s --check-prefix=NOSPLIT

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Forced plain loads: one unpredicated load per operand and a value select. The
; stack operand is then SoA-promoted (per-lane stride 4 instead of the 132-byte
; array).
;
; FORCED-LABEL: @test_load_select_no_lsc(
; FORCED:       [[SIMDSIZE:%.*]] = call i32 @llvm.genx.GenISA.simdSize()
; FORCED:       mul i32 %{{.*}}, 4
; FORCED:       [[EXTV:%.*]] = load float, ptr addrspace(4) %ext, align 4
; FORCED:       lshr i32 %{{.*}}, 2
; FORCED:       [[STACKV:%.*]] = load float, ptr %{{.*}}, align 4
; FORCED:       select i1 %c, float [[EXTV]], float [[STACKV]]
; FORCED-NOT:   GenISA.PredicatedLoad
; FORCED-NOT:   select i1 %c, ptr
;
; Without the override the pointer select survives, so the alloca keeps its
; array-of-floats (132-byte per-lane) layout.
;
; NOSPLIT-LABEL: @test_load_select_no_lsc(
; NOSPLIT:      mul i32 %{{.*}}, 132
; NOSPLIT:      [[SEL:%.*]] = select i1 %c, ptr addrspace(4) %ext, ptr addrspace(4) %{{.*}}
; NOSPLIT:      load float, ptr addrspace(4) [[SEL]]
; NOSPLIT-NOT:  GenISA.PredicatedLoad

define spir_kernel void @test_load_select_no_lsc(ptr addrspace(1) nocapture writeonly %d, ptr addrspace(4) %ext, i1 %c, i32 %ix, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr %privateBase) #0 {
entry:
  %pb = alloca [33 x float], align 4
  %idx = zext i32 %ix to i64
  %g = getelementptr inbounds [33 x float], ptr %pb, i64 0, i64 %idx
  %g_as4 = addrspacecast ptr %g to ptr addrspace(4)
  %sel = select i1 %c, ptr addrspace(4) %ext, ptr addrspace(4) %g_as4
  %v = load float, ptr addrspace(4) %sel, align 4
  %arrayidx = getelementptr inbounds float, ptr addrspace(1) %d, i64 %idx
  store float %v, ptr addrspace(1) %arrayidx, align 4
  ret void
}

attributes #0 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" }

!IGCMetadata = !{!3}
!igc.functions = !{!17}

!3 = !{!"ModuleMD", !4, !7, !13, !10}
!4 = !{!"compOpt", !5, !6}
!5 = !{!"OptDisable", i1 true}
!6 = !{!"UseScratchSpacePrivateMemory", i1 false}
!7 = !{!"FuncMD", !11, !12}
!10 = !{!"privateMemoryPerWI", i32 0}
!11 = !{!"FuncMDMap[0]", ptr @test_load_select_no_lsc}
!12 = !{!"FuncMDValue[0]", !10, !40}
!13 = !{!"MinNOSPushConstantSize", i32 0}
!17 = !{ptr @test_load_select_no_lsc, !18}
!18 = !{!19}
!19 = !{!"function_type", i32 0}
!28 = !{!"argId", i32 0}
!29 = !{!"implicitArgInfoListVec[0]", !28}
!30 = !{!"argId", i32 1}
!31 = !{!"implicitArgInfoListVec[1]", !30}
!32 = !{!"argId", i32 13}
!33 = !{!"implicitArgInfoListVec[2]", !32}
!40 = !{!"implicitArgInfoList", !29, !31, !33}
