;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-16-plus
;
; Verify that PHIs and SELECTs of alloca pointers are split correctly, and that split unblocks SoA promotion across function calls.
;
; RUN: igc_opt --opaque-pointers --ocl --platformpvc --igc-split-phis-of-alloca-pointers \
; RUN:   --igc-split-selects-of-alloca-pointers --igc-private-mem-resolution --regkey EnablePHIOfAllocaPtrSplit=1 \
; RUN:   --regkey EnableSelectOfAllocaPtrSplit=1 --regkey DisablePredicatedLoadForAllocaPtrSelectSplit=1 \
; RUN:   --regkey EnablePrivMemNewSOATranspose=2 --regkey EnableSOAAcrossCalls=1 -S %s | FileCheck %s

; CHECK-LABEL: define spir_kernel void @kernel(
; CHECK-NOT: = alloca
; CHECK: call void @phi_callee(ptr {{%.*}}, ptr addrspace(1) %d,
; CHECK: call void @select_callee(ptr {{%.*}}, ptr addrspace(1) %d,

; CHECK-LABEL: define internal void @phi_callee(
; CHECK: call i32 @llvm.genx.GenISA.simdSize()
; CHECK: [[BASE:%.*]] = ptrtoint ptr %stack to i64
; CHECK: then:
; CHECK: add {{.*}} i64 [[BASE]],
; CHECK: [[SOAV:%.*]] = load float, ptr
; CHECK: else:
; CHECK: [[GV:%.*]] = load float, ptr addrspace(1)
; CHECK: merge:
; CHECK: phi float [ [[SOAV]], %then ], [ [[GV]], %else ]
; CHECK-NOT: phi ptr

; CHECK-LABEL: define internal void @select_callee(
; CHECK: call i32 @llvm.genx.GenISA.simdSize()
; CHECK: [[SBASE:%.*]] = ptrtoint ptr %stack to i64
; CHECK: add {{.*}} i64 [[SBASE]],
; CHECK: [[SSV:%.*]] = load float, ptr
; CHECK: [[SGV:%.*]] = load float, ptr addrspace(1)
; CHECK: select i1 %c, float [[SSV]], float [[SGV]]
; CHECK-NOT: select i1 %c, ptr
define spir_kernel void @kernel(ptr addrspace(1) %d, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase) {
  %a = alloca [64 x float], align 4
  %i = extractelement <8 x i32> %payloadHeader, i64 0
  %c = icmp ne i32 %i, 0
  call void @phi_callee(ptr %a, ptr addrspace(1) %d, i32 %i, i1 %c)
  call void @select_callee(ptr %a, ptr addrspace(1) %d, i32 %i, i1 %c)
  ret void
}

define internal void @phi_callee(ptr %stack, ptr addrspace(1) %node, i32 %i, i1 %c) {
entry:
  br i1 %c, label %then, label %else
then:
  %e = getelementptr inbounds float, ptr %stack, i32 %i
  %ge = addrspacecast ptr %e to ptr addrspace(4)
  br label %merge
else:
  %n = getelementptr inbounds i8, ptr addrspace(1) %node, i64 8
  %gn = addrspacecast ptr addrspace(1) %n to ptr addrspace(4)
  br label %merge
merge:
  %p = phi ptr addrspace(4) [ %ge, %then ], [ %gn, %else ]
  %pg = addrspacecast ptr addrspace(4) %p to ptr addrspace(1)
  %v = load float, ptr addrspace(1) %pg, align 4
  store float %v, ptr addrspace(1) %node, align 4
  ret void
}

define internal void @select_callee(ptr %stack, ptr addrspace(1) %node, i32 %i, i1 %c) {
  %e = getelementptr inbounds float, ptr %stack, i32 %i
  %ge = addrspacecast ptr %e to ptr addrspace(4)
  %n = getelementptr inbounds i8, ptr addrspace(1) %node, i64 8
  %gn = addrspacecast ptr addrspace(1) %n to ptr addrspace(4)
  %p = select i1 %c, ptr addrspace(4) %ge, ptr addrspace(4) %gn
  %pg = addrspacecast ptr addrspace(4) %p to ptr addrspace(1)
  %v = load float, ptr addrspace(1) %pg, align 4
  store float %v, ptr addrspace(1) %node, align 4
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!500, !520, !521}
!0 = !{!"ModuleMD", !1, !3}
!1 = !{!"compOpt", !2}
!2 = !{!"UseScratchSpacePrivateMemory", i1 false}
!3 = !{!"FuncMD", !10, !11}
!10 = !{!"FuncMDMap[0]", ptr @kernel}
!11 = !{!"FuncMDValue[0]", !20}
!20 = !{!"implicitArgInfoList", !21, !22, !23, !24, !25, !26, !27}
!21 = !{!"implicitArgInfoListVec[0]", !30}
!22 = !{!"implicitArgInfoListVec[1]", !31}
!23 = !{!"implicitArgInfoListVec[2]", !32}
!24 = !{!"implicitArgInfoListVec[3]", !33}
!25 = !{!"implicitArgInfoListVec[4]", !34}
!26 = !{!"implicitArgInfoListVec[5]", !35}
!27 = !{!"implicitArgInfoListVec[6]", !36}
!30 = !{!"argId", i32 0}
!31 = !{!"argId", i32 1}
!32 = !{!"argId", i32 7}
!33 = !{!"argId", i32 8}
!34 = !{!"argId", i32 9}
!35 = !{!"argId", i32 10}
!36 = !{!"argId", i32 13}
!408 = !{!409, !410}
!409 = !{!"function_type", i32 0}
!410 = !{!"implicit_arg_desc", !411, !412, !413, !414, !415, !416, !417}
!411 = !{i32 0}
!412 = !{i32 1}
!413 = !{i32 7}
!414 = !{i32 8}
!415 = !{i32 9}
!416 = !{i32 10}
!417 = !{i32 13}
!450 = !{!451}
!451 = !{!"function_type", i32 1}
!500 = !{ptr @kernel, !408}
!520 = !{ptr @phi_callee, !450}
!521 = !{ptr @select_callee, !450}
