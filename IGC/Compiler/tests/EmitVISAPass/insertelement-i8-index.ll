;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; RUN: igc_opt --opaque-pointers -platformdg2 -simd-mode 8 -igc-emit-visa %s -regkey DumpVISAASMToConsole | FileCheck %s

; Test that insertelement with an i8 index does not produce a zero-element
; uw alias variable. The i8 index should be zero-extended to uw before the
; address computation.

; CHECK: .kernel "foo"
; Verify the i8 index is zero-extended to uw and used in the mul for address computation.
; The mov zero-extends the i8 index into a uw variable, immediately followed by the mul.
; CHECK:      mul (M1_NM, 1) [[OFF:[A-Za-z0-9_]*]](0,0)<1> [[IDX_UW:[A-Za-z0-9_]*]](0,0)<0;1,0> 0x2:uw
; CHECK-NEXT: addr_add (M1_NM, 1) {{.*}} [[OFF]](0,0)<0;1,0>

define spir_kernel void @foo(ptr addrspace(1) %p) {
entry:
  %L2 = load i8, ptr addrspace(1) %p, align 4
  %I3 = insertelement <4 x i16> zeroinitializer, i16 0, i8 %L2
  %bc = bitcast <4 x i16> %I3 to i64
  store i64 %bc, ptr addrspace(1) %p, align 4
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
