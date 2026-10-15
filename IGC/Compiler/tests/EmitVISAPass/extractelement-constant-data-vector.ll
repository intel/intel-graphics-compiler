;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; RUN: igc_opt --opaque-pointers -platformpvc -igc-emit-visa %s -regkey DumpVISAASMToConsole | FileCheck %s

; CHECK: .kernel "foo"
; CHECK:      mov (M1_NM, 1) [[VEC:[A-Za-z0-9_]+]](0,0)<1> 0x1:d
; CHECK:      mov (M1_NM, 1) [[VEC]](0,1)<1> 0x2:d
; CHECK:      mov (M1_NM, 1) [[E0:[A-Za-z0-9_]+]](0,0)<1> [[VEC]](0,0)<0;1,0>
; CHECK:      mov (M1_NM, 1) [[E1:[A-Za-z0-9_]+]](0,0)<1> [[VEC]](0,1)<0;1,0>
; CHECK:      or (M1_NM, 1) {{[A-Za-z0-9_]+}}(0,0)<1> [[E0]](0,0)<0;1,0>
; CHECK:      or (M1_NM, 1) {{[A-Za-z0-9_]+}}(0,0)<1> [[E1]](0,0)<0;1,0>

define spir_kernel void @foo(ptr addrspace(1) %p) {
entry:
  %p1 = getelementptr inbounds i32, ptr addrspace(1) %p, i64 1
  %a = load i32, ptr addrspace(1) %p, align 4
  %b = load i32, ptr addrspace(1) %p1, align 4
  %e0 = extractelement <2 x i32> <i32 1, i32 2>, i32 0
  %e1 = extractelement <2 x i32> <i32 1, i32 2>, i32 1
  %o0 = or i32 %e0, %a
  %o1 = or i32 %e1, %b
  store i32 %o0, ptr addrspace(1) %p, align 4
  store i32 %o1, ptr addrspace(1) %p1, align 4
  ret void
}

!igc.functions = !{!1}

!1 = !{ptr @foo, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}

!IGCMetadata = !{!20}
!20 = !{!"ModuleMD", !21}
!21 = !{!"FuncMD", !22, !23}
!22 = !{!"FuncMDMap[0]", ptr @foo}
!23 = !{!"FuncMDValue[0]", !24, !25}
!24 = !{!"functionType", !"KernelFunction"}
!25 = !{!"resAllocMD", !26, !27, !28, !29, !31}
!26 = !{!"uavsNumType", i32 1}
!27 = !{!"srvsNumType", i32 0}
!28 = !{!"samplersNumType", i32 0}
!29 = !{!"argAllocMDList", !30}
!30 = !{!"argAllocMDListVec[0]", !32, !33, !34}
!31 = !{!"inlineSamplersMD"}
!32 = !{!"type", i32 1}
!33 = !{!"extensionType", i32 -1}
!34 = !{!"indexType", i32 0}
