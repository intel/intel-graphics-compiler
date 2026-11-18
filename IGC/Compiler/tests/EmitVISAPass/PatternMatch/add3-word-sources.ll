;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -ocl -simd-mode 16 -platformPtl -igc-emit-visa -regkey DumpVISAASMToConsole < %s | FileCheck %s
; RUN: igc_opt --opaque-pointers -ocl -simd-mode 16 -platformPtl -igc-emit-visa -regkey DumpVISAASMToConsole -regkey EnableAdd3WordSources=0 < %s | FileCheck %s --check-prefix=CHECK-OFF
; RUN: igc_opt --opaque-pointers -ocl -simd-mode 16 -platformNvl -igc-emit-visa -regkey DumpVISAASMToConsole < %s | FileCheck %s --check-prefix=NVL
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Test checks that add3 reads a 16-bit half of a dword as a word source (EnableAdd3WordSources) instead of
; materializing the and/shift:
; - %r1: the lower half of %a (and 0xffff) is read as a:uw with stride 2, no and.
; - %r2: the upper half of %a (lshr 16) can't be src2 (not GRF aligned), so it is swapped into src1.
; - %r3: the lower half of %d is subtracted: the neg applies to the widened word, so it is read as -d:uw too.
; - %r4: the upper half of %c next to an immediate: fine on PTL (src0/src1 take any stride).
; On XE3P (NVL, HWConformityPro) src1 and src2 must be flat, and src0 may have a region only if src2 is a non-scalar
; register: %r2's upper half goes to src0, and %r4's is read as its dword (shr), since src2 is the immediate.

; CHECK-NOT: {{(and|bfn[.a-z0-9]*)}} (M1, 16) lowa
; CHECK: add3 (M1, 16) r1(0,0)<1> {{[A-z0-9_]*}}(0,0)<16;8,2> b(0,0)<1;1,0> c(0,0)<1;1,0>
; CHECK-NOT: shr (M1, 16) hia
; CHECK: add3 (M1, 16) r2(0,0)<1> b(0,0)<1;1,0> {{[A-z0-9_]*}}(0,1)<16;8,2> c(0,0)<1;1,0>
; CHECK-NOT: and (M1, 16) lowd
; CHECK: add3 (M1, 16) r3(0,0)<1> b(0,0)<1;1,0> c(0,0)<1;1,0> (-){{[A-z0-9_]*}}(0,0)<16;8,2>

; CHECK: add3 (M1, 16) r4(0,0)<1> {{[A-z0-9_]*}}(0,1)<16;8,2> b(0,0)<1;1,0> 0x7:d

; NVL: add3 (M1, 16) r1(0,0)<1> {{[A-z0-9_]*}}(0,0)<16;8,2> b(0,0)<1;1,0> c(0,0)<1;1,0>
; NVL: add3 (M1, 16) r2(0,0)<1> {{[A-z0-9_]*}}(0,1)<16;8,2> c(0,0)<1;1,0> b(0,0)<1;1,0>
; NVL: shr (M1, 16) hic{{[A-z0-9_]*}}(0,0)<1>
; NVL: add3 (M1, 16) r4(0,0)<1> hic(0,0)<1;1,0> b(0,0)<1;1,0> 0x7:d

; With EnableAdd3WordSources=0 the halves are materialized (the and is merged with the xor of %a into a bfn):
; CHECK-OFF: {{(and|bfn[.a-z0-9]*)}} (M1, 16) lowa(0,0)<1>
; CHECK-OFF: add3 (M1, 16) r1(0,0)<1> lowa(0,0)<1;1,0> b(0,0)<1;1,0> c(0,0)<1;1,0>
; CHECK-OFF: shr (M1, 16) hia{{[A-z0-9_]*}}(0,0)<1>
; CHECK-OFF: add3 (M1, 16) r2(0,0)<1> b(0,0)<1;1,0> c(0,0)<1;1,0> hia(0,0)<1;1,0>

define spir_kernel void @compute16(ptr addrspace(1) %ptr, i32 %_A, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane32 = zext i16 %lane to i32
  %a = xor i32 %lane32, 1001
  %b = xor i32 %lane32, 1002
  %c = xor i32 %lane32, 1003
  %d = add i32 %lane32, 1004
  %lowa = and i32 %a, 65535
  %t1 = add i32 %lowa, %b
  %r1 = add i32 %t1, %c
  %hia = lshr i32 %a, 16
  %t2 = add i32 %b, %c
  %r2 = add i32 %t2, %hia
  %lowd = and i32 %d, 65535
  %t3 = add i32 %b, %c
  %r3 = sub i32 %t3, %lowd
  %q0 = getelementptr i32, ptr addrspace(1) %ptr, i32 0
  store i32 %r1, ptr addrspace(1) %q0, align 4
  %q1 = getelementptr i32, ptr addrspace(1) %ptr, i32 1
  store i32 %r2, ptr addrspace(1) %q1, align 4
  %q2 = getelementptr i32, ptr addrspace(1) %ptr, i32 2
  store i32 %r3, ptr addrspace(1) %q2, align 4
  %hic = lshr i32 %c, 16
  %t4 = add i32 %hic, %b
  %r4 = add i32 %t4, 7
  %q3 = getelementptr i32, ptr addrspace(1) %ptr, i32 3
  store i32 %r4, ptr addrspace(1) %q3, align 4
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
