;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -ocl -simd-mode 16 -platformPtl -igc-emit-visa -regkey DumpVISAASMToConsole < %s | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Test checks that the add3 skip for a multi-use folded add (Add3SkipMask) does not apply when the
; folded add's own match absorbs nothing: %x1 = %a1 + %m1 has two add users, but %m1 (dword * dword) cannot become a
; mad on PTL, so both users fold %x1 into add3 and %x1 is never emitted.

; CHECK: mul (M1, 16) [[M:[A-z0-9]*]](0,0)<1>
; CHECK-NOT: add (M1, 16)
; CHECK: add3 (M1, 16) {{.*}}[[M]](0,0)<1;1,0> _A(0,0)<0;1,0>
; CHECK-NOT: add (M1, 16)
; CHECK: add3 (M1, 16) {{.*}}[[M]](0,0)<1;1,0> 0xf:d

define spir_kernel void @compute16(ptr addrspace(1) %ptr, i32 %_A, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane32 = zext i16 %lane to i32
  %a1 = xor i32 %lane32, 1001
  %b1 = xor i32 %lane32, 1002
  %c1 = xor i32 %lane32, 1003
  %m1 = mul i32 %b1, %c1
  %x1 = add i32 %a1, %m1
  %y1 = add i32 %x1, %_A
  %z1 = add i32 %x1, 15
  %q0 = getelementptr i32, ptr addrspace(1) %ptr, i32 0
  store i32 %y1, ptr addrspace(1) %q0, align 4
  %q1 = getelementptr i32, ptr addrspace(1) %ptr, i32 1
  store i32 %z1, ptr addrspace(1) %q1, align 4
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()

!IGCMetadata = !{!0}
!igc.functions = !{!311}

!0 = !{!"ModuleMD", !67}
!67 = !{!"FuncMD", !68, !69}
!68 = !{!"FuncMDMap[0]", ptr @compute16}
!69 = !{!"FuncMDValue[0]", !101, !149, !326}
!101 = !{!"resAllocMD", !105}
!105 = !{!"argAllocMDList", !106, !110, !111, !114, !115, !116}
!106 = !{!"argAllocMDListVec[0]", !107, !108, !109}
!107 = !{!"type", i32 0}
!108 = !{!"extensionType", i32 -1}
!109 = !{!"indexType", i32 -1}
!110 = !{!"argAllocMDListVec[1]", !107, !108, !109}
!111 = !{!"argAllocMDListVec[2]", !112, !108, !113}
!112 = !{!"type", i32 1}
!113 = !{!"indexType", i32 0}
!114 = !{!"argAllocMDListVec[3]", !107, !108, !109}
!115 = !{!"argAllocMDListVec[4]", !107, !108, !109}
!116 = !{!"argAllocMDListVec[5]", !107, !108, !109}
!149 = !{!"m_OpenCLArgTypeQualifiers", !150, !151, !152}
!150 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!151 = !{!"m_OpenCLArgTypeQualifiersVec[1]", !""}
!152 = !{!"m_OpenCLArgTypeQualifiersVec[2]", !""}
!311 = !{ptr @compute16, !312}
!312 = !{!313}
!313 = !{!"function_type", i32 0}
!319 = !{!"argId", i32 0}
!320 = !{!"implicitArgInfoListVec[0]", !319}
!321 = !{!"argId", i32 1}
!322 = !{!"implicitArgInfoListVec[1]", !321}
!323 = !{!"argId", i32 15}
!324 = !{!"explicitArgNum", i32 2}
!325 = !{!"implicitArgInfoListVec[2]", !323, !324}
!326 = !{!"implicitArgInfoList", !320, !322, !325}
