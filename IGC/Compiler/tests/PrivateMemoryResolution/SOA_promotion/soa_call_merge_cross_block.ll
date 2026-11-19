;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys, llvm-16-plus
;
; Check that PHI split is done correctly even if the load is not directly in the PHI block.
;
; RUN: igc_opt --opaque-pointers --ocl --platformdg2 --igc-split-phis-of-alloca-pointers --igc-private-mem-resolution \
; RUN:   --regkey EnablePHIOfAllocaPtrSplit=1 --regkey EnablePrivMemNewSOATranspose=2 --regkey EnableSOAAcrossCalls=1 \
; RUN:   -S %s | FileCheck %s
; RUN: igc_opt --opaque-pointers --ocl --platformPtl --igc-split-phis-of-alloca-pointers --igc-private-mem-resolution \
; RUN:   --regkey EnablePHIOfAllocaPtrSplit=1 --regkey EnablePrivMemNewSOATranspose=2 --regkey EnableSOAAcrossCalls=1 \
; RUN:   -S %s | FileCheck %s
; RUN: igc_opt --opaque-pointers --ocl --platformtgllp --igc-split-phis-of-alloca-pointers \
; RUN:   --regkey EnablePHIOfAllocaPtrSplit=1 --regkey EnableSOAAcrossCalls=1 -S %s | FileCheck %s --check-prefix=NOLSC

; CHECK-LABEL: define spir_kernel void @kernel(
; CHECK-NOT: = alloca
; CHECK: call void @cross_callee(ptr {{%.*}}, ptr addrspace(1) %d,

; CHECK-LABEL: define internal void @cross_callee(
; CHECK: ptrtoint ptr %stack to i64
; CHECK: merge:
; CHECK: [[ISSTACK:%.*]] = phi i1 [ true, %then ], [ false, %else ]
; CHECK: [[ISNODE:%.*]] = phi i1 [ false, %then ], [ true, %else ]
; CHECK: [[IDX:%.*]] = phi i64 [ %idx, %then ], [ 0, %else ]
; CHECK-NOT: phi ptr
; CHECK: use:
; CHECK: trunc i64 [[IDX]] to i32
; CHECK: [[SOAPTR:%.*]] = getelementptr float, ptr {{%.*}}, i32 0
; CHECK: [[A:%.*]] = call float @llvm.genx.GenISA.PredicatedLoad.f32.p0.f32(ptr [[SOAPTR]], i64 4, i1 [[ISSTACK]], float poison)
; CHECK: [[B:%.*]] = call float @llvm.genx.GenISA.PredicatedLoad.f32.p1.f32(ptr addrspace(1) {{%.*}}, i64 4, i1 [[ISNODE]], float poison)
; CHECK: select i1 [[ISSTACK]], float [[A]], float [[B]]

; Without LSC there are no predicated loads, so the merge is left as it is.
; NOLSC-LABEL: define internal void @cross_callee(
; NOLSC: merge:
; NOLSC: %p = phi ptr addrspace(4) [ %ge, %then ], [ %nodeg, %else ]
; NOLSC-NOT: PredicatedLoad
define spir_kernel void @kernel(ptr addrspace(1) %d, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, ptr %privateBase) {
  %a = alloca [64 x float], align 4
  %i = extractelement <8 x i32> %payloadHeader, i64 0
  %c = icmp ne i32 %i, 0
  call void @cross_callee(ptr %a, ptr addrspace(1) %d, i32 %i, i1 %c)
  ret void
}

define internal void @cross_callee(ptr %stack, ptr addrspace(1) %node, i32 %i, i1 %c) {
entry:
  %nodeg = addrspacecast ptr addrspace(1) %node to ptr addrspace(4)
  br i1 %c, label %then, label %else
then:
  %idx = zext i32 %i to i64
  %e = getelementptr inbounds float, ptr %stack, i64 %idx
  %ge = addrspacecast ptr %e to ptr addrspace(4)
  br label %merge
else:
  br label %merge
merge:
  %p = phi ptr addrspace(4) [ %ge, %then ], [ %nodeg, %else ]
  %k = load i8, ptr addrspace(1) %node, align 1
  %z = icmp eq i8 %k, -1
  br i1 %z, label %done, label %use
use:
  %q = getelementptr inbounds i8, ptr addrspace(4) %p, i64 4
  %qg = addrspacecast ptr addrspace(4) %q to ptr addrspace(1)
  %v = load float, ptr addrspace(1) %qg, align 4
  store float %v, ptr addrspace(1) %node, align 4
  br label %done
done:
  ret void
}

!IGCMetadata = !{!0}
!igc.functions = !{!500, !520}
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
!520 = !{ptr @cross_callee, !450}
