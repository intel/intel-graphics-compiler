;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers -platformbmg -igc-emit-visa %s -regkey DumpVISAASMToConsole | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Two adjacent predicated loads share one non-constant merge value, and the
; second load is its last use. Only the second load may take over the merge
; value's variable as its destination: if the first one did, the second load
; would overwrite its result before it is read.

; CHECK: mov (M1_NM, 1) [[MERGE:[^ (]+]](0,7)<1> defval(0,0)<0;1,0>
; CHECK: lsc_load.ugm
; CHECK-NOT: [[MERGE]](0,0)<1>
; CHECK: lsc_load.ugm
; CHECK-NEXT: (P{{[0-9]+}}.any) mov (M1_NM, 8) [[MERGE]](0,0)<1>

define spir_kernel void @test_kernel(ptr addrspace(1) %in, ptr addrspace(1) %out, i32 %defval, i32 %n) {
entry:
  %in.base = ptrtoint ptr addrspace(1) %in to i64
  %in.hi = add i64 %in.base, 32
  %in.hi.addr = inttoptr i64 %in.hi to ptr addrspace(1)
  %m0 = insertelement <8 x i32> poison, i32 %defval, i64 0
  %m1 = insertelement <8 x i32> %m0, i32 %defval, i32 1
  %m2 = insertelement <8 x i32> %m1, i32 %defval, i32 2
  %m3 = insertelement <8 x i32> %m2, i32 %defval, i32 3
  %m4 = insertelement <8 x i32> %m3, i32 %defval, i32 4
  %m5 = insertelement <8 x i32> %m4, i32 %defval, i32 5
  %m6 = insertelement <8 x i32> %m5, i32 %defval, i32 6
  %merge = insertelement <8 x i32> %m6, i32 %defval, i32 7
  %pred = icmp sgt i32 %n, 0
  %ld0 = call <8 x i32> @llvm.genx.GenISA.PredicatedLoad.v8i32.p1.v8i32(ptr addrspace(1) %in, i64 16, i1 %pred, <8 x i32> %merge)
  %ld1 = call <8 x i32> @llvm.genx.GenISA.PredicatedLoad.v8i32.p1.v8i32(ptr addrspace(1) %in.hi.addr, i64 16, i1 %pred, <8 x i32> %merge)
  %e0 = extractelement <8 x i32> %ld0, i32 0
  %e1 = extractelement <8 x i32> %ld0, i32 6
  %e2 = extractelement <8 x i32> %ld1, i32 1
  %e3 = extractelement <8 x i32> %ld1, i32 7
  %s0 = add i32 %e0, %e1
  %s1 = add i32 %e2, %e3
  %s = add i32 %s0, %s1
  store i32 %s, ptr addrspace(1) %out, align 4
  ret void
}

declare <8 x i32> @llvm.genx.GenISA.PredicatedLoad.v8i32.p1.v8i32(ptr addrspace(1), i64, i1, <8 x i32>)

!IGCMetadata = !{!0}
!igc.functions = !{!13}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", ptr @test_kernel}
!3 = !{!"FuncMDValue[0]", !4}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6, !10, !11, !12}
!6 = !{!"argAllocMDListVec[0]", !7, !8, !9}
!7 = !{!"type", i32 0}
!8 = !{!"extensionType", i32 -1}
!9 = !{!"indexType", i32 -1}
!10 = !{!"argAllocMDListVec[1]", !7, !8, !9}
!11 = !{!"argAllocMDListVec[2]", !7, !8, !9}
!12 = !{!"argAllocMDListVec[3]", !7, !8, !9}
!13 = !{ptr @test_kernel, !14}
!14 = !{!15}
!15 = !{!"function_type", i32 0}
