;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -platformbmg -igc-emit-visa -simd-mode 16 -inputocl -regkey DumpVISAASMToConsole,DisableUniformAnalysis -S %s | FileCheck %s --check-prefix=CHECK-16
; RUN: igc_opt -platformbmg -igc-emit-visa -simd-mode 32 -inputocl -regkey DumpVISAASMToConsole,DisableUniformAnalysis -S %s | FileCheck %s --check-prefix=CHECK-32
; ------------------------------------------------
; EmitVISAPass - PackPair: bitcast <2m x i32> (insertelement chain) to i64 / <m x i64>
; ------------------------------------------------
;
; Each half is written with one stride-2 mov, read in place: elements of a
; vector directly, halves of an i64 (bitcast i64 -> <2 x i32>) with a <2;1,0>
; region. The <2m x i32> is not built (e.g. a lowered shufflevector feeding a
; <m x i64> bitcast). SIMD32 splits each half into two SIMD16 movs.
;
; CHECK-16-LABEL: .kernel "test"
; CHECK-16:      lsc_load.ugm (M1, 16)  [[V:[a-zA-Z0-9_]+]]:d32x4
; Plain pack of two extracts of %v.
; CHECK-16:      mov (M1, 16) [[A:[a-zA-Z0-9_]+]]_0v(0,0)<2> [[V]]_0v(0,0)<1;1,0>
; CHECK-16-NEXT: mov (M1, 16) [[A]]_0v(0,1)<2> [[V]]_0v(2,0)<1;1,0>
; Live base: element 0 is read from %s (an alias of %a), element 1 from %v.
; CHECK-16:      mov (M1, 16) [[B:[a-zA-Z0-9_]+]]_0v(0,0)<2> [[A]]_0v(0,0)<1;1,0>
; CHECK-16-NEXT: mov (M1, 16) [[B]]_0v(0,1)<2> [[V]]_0v(3,0)<1;1,0>
; Halves of i64 values: hi of %x, lo of %b.
; CHECK-16:      lsc_load.ugm (M1, 16)  [[X:[a-zA-Z0-9_]+]]:d64
; CHECK-16:      mov (M1, 16) [[C:[a-zA-Z0-9_]+]]_0v(0,0)<2> [[X]]_0v(0,1)<2;1,0>
; CHECK-16-NEXT: mov (M1, 16) [[C]]_0v(0,1)<2> [[B]]_0v(0,0)<2;1,0>
; Vector form: shuffled <4 x i32> -> <2 x i64>; elements 0, 2 | 1, 3 of %v.
; CHECK-16:      mov (M1, 16) [[D:[a-zA-Z0-9_]+]]_0v(0,0)<2> [[V]]_0v(0,0)<1;1,0>
; CHECK-16-NEXT: mov (M1, 16) [[D]]_0v(0,1)<2> [[V]]_0v(2,0)<1;1,0>
; CHECK-16-NEXT: mov (M1, 16) [[D]]_0v(2,0)<2> [[V]]_0v(1,0)<1;1,0>
; CHECK-16-NEXT: mov (M1, 16) [[D]]_0v(2,1)<2> [[V]]_0v(3,0)<1;1,0>
; Vector form on a 64-bit split: lo of both, then hi of both.
; CHECK-16:      lsc_load.ugm (M1, 16)  [[W:[a-zA-Z0-9_]+]]:d64x2
; CHECK-16:      mov (M1, 16) [[E:[a-zA-Z0-9_]+]]_0v(0,0)<2> [[W]]_0v(0,0)<2;1,0>
; CHECK-16-NEXT: mov (M1, 16) [[E]]_0v(0,1)<2> [[W]]_0v(2,0)<2;1,0>
; CHECK-16-NEXT: mov (M1, 16) [[E]]_0v(2,0)<2> [[W]]_0v(0,1)<2;1,0>
; CHECK-16-NEXT: mov (M1, 16) [[E]]_0v(2,1)<2> [[W]]_0v(2,1)<2;1,0>
; CHECK-16:      lsc_store
;
; CHECK-32-LABEL: .kernel "test"
; CHECK-32:      lsc_load.ugm (M1, 32)  [[V:[a-zA-Z0-9_]+]]:d32x4
; CHECK-32:      mov (M1, 16) [[A:[a-zA-Z0-9_]+]]_0v(0,0)<2> [[V]]_0v(0,0)<1;1,0>
; CHECK-32-NEXT: mov (M5, 16) [[A]]_0v(2,0)<2> [[V]]_0v(1,0)<1;1,0>
; CHECK-32-NEXT: mov (M1, 16) [[A]]_0v(0,1)<2> [[V]]_0v(4,0)<1;1,0>
; CHECK-32-NEXT: mov (M5, 16) [[A]]_0v(2,1)<2> [[V]]_0v(5,0)<1;1,0>
; CHECK-32:      mov (M1, 16) [[B:[a-zA-Z0-9_]+]]_0v(0,0)<2> [[A]]_0v(0,0)<1;1,0>
; CHECK-32-NEXT: mov (M5, 16) [[B]]_0v(2,0)<2> [[A]]_0v(1,0)<1;1,0>
; CHECK-32-NEXT: mov (M1, 16) [[B]]_0v(0,1)<2> [[V]]_0v(6,0)<1;1,0>
; CHECK-32-NEXT: mov (M5, 16) [[B]]_0v(2,1)<2> [[V]]_0v(7,0)<1;1,0>
; CHECK-32:      lsc_load.ugm (M1, 32)  [[X:[a-zA-Z0-9_]+]]:d64
; CHECK-32:      mov (M1, 16) [[C:[a-zA-Z0-9_]+]]_0v(0,0)<2> [[X]]_0v(0,1)<2;1,0>
; CHECK-32-NEXT: mov (M5, 16) [[C]]_0v(2,0)<2> [[X]]_0v(2,1)<2;1,0>
; CHECK-32-NEXT: mov (M1, 16) [[C]]_0v(0,1)<2> [[B]]_0v(0,0)<2;1,0>
; CHECK-32-NEXT: mov (M5, 16) [[C]]_0v(2,1)<2> [[B]]_0v(2,0)<2;1,0>
; CHECK-32:      mov (M1, 16) [[D:[a-zA-Z0-9_]+]]_0v(0,0)<2> [[V]]_0v(0,0)<1;1,0>
; CHECK-32-NEXT: mov (M5, 16) [[D]]_0v(2,0)<2> [[V]]_0v(1,0)<1;1,0>
; CHECK-32-NEXT: mov (M1, 16) [[D]]_0v(0,1)<2> [[V]]_0v(4,0)<1;1,0>
; CHECK-32-NEXT: mov (M5, 16) [[D]]_0v(2,1)<2> [[V]]_0v(5,0)<1;1,0>
; CHECK-32-NEXT: mov (M1, 16) [[D]]_0v(4,0)<2> [[V]]_0v(2,0)<1;1,0>
; CHECK-32-NEXT: mov (M5, 16) [[D]]_0v(6,0)<2> [[V]]_0v(3,0)<1;1,0>
; CHECK-32-NEXT: mov (M1, 16) [[D]]_0v(4,1)<2> [[V]]_0v(6,0)<1;1,0>
; CHECK-32-NEXT: mov (M5, 16) [[D]]_0v(6,1)<2> [[V]]_0v(7,0)<1;1,0>
; CHECK-32:      lsc_load.ugm (M1, 32)  [[W:[a-zA-Z0-9_]+]]:d64x2
; CHECK-32:      mov (M1, 16) [[E:[a-zA-Z0-9_]+]]_0v(0,0)<2> [[W]]_0v(0,0)<2;1,0>
; CHECK-32-NEXT: mov (M5, 16) [[E]]_0v(2,0)<2> [[W]]_0v(2,0)<2;1,0>
; CHECK-32-NEXT: mov (M1, 16) [[E]]_0v(0,1)<2> [[W]]_0v(4,0)<2;1,0>
; CHECK-32-NEXT: mov (M5, 16) [[E]]_0v(2,1)<2> [[W]]_0v(6,0)<2;1,0>
; CHECK-32-NEXT: mov (M1, 16) [[E]]_0v(4,0)<2> [[W]]_0v(0,1)<2;1,0>
; CHECK-32-NEXT: mov (M5, 16) [[E]]_0v(6,0)<2> [[W]]_0v(2,1)<2;1,0>
; CHECK-32-NEXT: mov (M1, 16) [[E]]_0v(4,1)<2> [[W]]_0v(4,1)<2;1,0>
; CHECK-32-NEXT: mov (M5, 16) [[E]]_0v(6,1)<2> [[W]]_0v(6,1)<2;1,0>
; CHECK-32:      lsc_store

declare <2 x i32> @llvm.genx.GenISA.SubgroupBitcastShuffle.v2i32.i64(i64)

define spir_kernel void @test(i64 addrspace(1)* %src, i64 addrspace(1)* %dst, i32 %arg0, <3 x float> %val, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %vp = bitcast i64 addrspace(1)* %src to <4 x i32> addrspace(1)*
  %v = load <4 x i32>, <4 x i32> addrspace(1)* %vp
  %v0 = extractelement <4 x i32> %v, i32 0
  %v2 = extractelement <4 x i32> %v, i32 2
  %v3 = extractelement <4 x i32> %v, i32 3

  ; plain pack: both halves are extracts of %v
  %a0 = insertelement <2 x i32> undef, i32 %v0, i32 0
  %a1 = insertelement <2 x i32> %a0, i32 %v2, i32 1
  %a = bitcast <2 x i32> %a1 to i64
  %s = call <2 x i32> @llvm.genx.GenISA.SubgroupBitcastShuffle.v2i32.i64(i64 %a)

  ; live base: element 0 comes from %s, which stays live
  %s1 = extractelement <2 x i32> %s, i32 1
  %b1 = insertelement <2 x i32> %s, i32 %v3, i32 1
  %b = bitcast <2 x i32> %b1 to i64

  ; halves of i64 values split by bitcast: read with a stride-2 region
  %xp = getelementptr i64, i64 addrspace(1)* %src, i64 4
  %x = load i64, i64 addrspace(1)* %xp
  %hx = bitcast i64 %x to <2 x i32>
  %hb = bitcast i64 %b to <2 x i32>
  %x1 = extractelement <2 x i32> %hx, i32 1
  %b0 = extractelement <2 x i32> %hb, i32 0
  %c0 = insertelement <2 x i32> undef, i32 %x1, i32 0
  %c1 = insertelement <2 x i32> %c0, i32 %b0, i32 1
  %c = bitcast <2 x i32> %c1 to i64

  ; vector form (lowered shufflevector <0, 2, 1, 3> on a live vector) + <2 x i64> bitcast
  %u2 = extractelement <4 x i32> %v, i32 2
  %u1 = extractelement <4 x i32> %v, i32 1
  %d1 = insertelement <4 x i32> %v, i32 %u2, i32 1
  %d2 = insertelement <4 x i32> %d1, i32 %u1, i32 2
  %d = bitcast <4 x i32> %d2 to <2 x i64>

  ; vector form on a 64-bit split: bitcast <2 x i64> -> <4 x i32>, swap the middle dwords
  %wp = bitcast i64 addrspace(1)* %src to <2 x i64> addrspace(1)*
  %w = load <2 x i64>, <2 x i64> addrspace(1)* %wp
  %hw = bitcast <2 x i64> %w to <4 x i32>
  %w2 = extractelement <4 x i32> %hw, i32 2
  %w1 = extractelement <4 x i32> %hw, i32 1
  %e1 = insertelement <4 x i32> %hw, i32 %w2, i32 1
  %e2 = insertelement <4 x i32> %e1, i32 %w1, i32 2
  %e = bitcast <4 x i32> %e2 to <2 x i64>

  %de = add <2 x i64> %d, %e
  %de0 = extractelement <2 x i64> %de, i32 0
  %de1 = extractelement <2 x i64> %de, i32 1
  %s1z = zext i32 %s1 to i64
  %sum0 = add i64 %c, %s1z
  %sum1 = add i64 %sum0, %de0
  %r = add i64 %sum1, %de1
  store i64 %r, i64 addrspace(1)* %dst
  ret void
}

!IGCMetadata = !{!100}
!igc.functions = !{!0}

!0 = !{void (i64 addrspace(1)*, i64 addrspace(1)*, i32, <3 x float>, <8 x i32>, <8 x i32>, i16)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!100 = !{!"ModuleMD", !101}
!101 = !{!"FuncMD", !102, !103}
!102 = !{!"FuncMDMap[0]", void (i64 addrspace(1)*, i64 addrspace(1)*, i32, <3 x float>, <8 x i32>, <8 x i32>, i16)* @test}
!103 = !{!"FuncMDValue[0]", !104, !200, !311}
!104 = !{!"functionType", !"KernelFunction"}
!200 = !{!"resAllocMD", !201}
!201 = !{!"argAllocMDList", !202, !203, !204, !205, !206, !207, !208}
!202 = !{!"argAllocMDListVec[0]", !300, !301, !302}
!203 = !{!"argAllocMDListVec[1]", !300, !301, !302}
!204 = !{!"argAllocMDListVec[2]", !300, !301, !302}
!205 = !{!"argAllocMDListVec[3]", !300, !301, !302}
!206 = !{!"argAllocMDListVec[4]", !300, !301, !302}
!207 = !{!"argAllocMDListVec[5]", !300, !301, !302}
!208 = !{!"argAllocMDListVec[6]", !300, !301, !302}
!300 = !{!"type", i32 0}
!301 = !{!"extensionType", i32 -1}
!302 = !{!"indexType", i32 -1}
!303 = !{!"argId", i32 0}
!304 = !{!"implicitArgInfoListVec[0]", !303}
!305 = !{!"argId", i32 1}
!306 = !{!"implicitArgInfoListVec[1]", !305}
!307 = !{!"argId", i32 7}
!308 = !{!"implicitArgInfoListVec[2]", !307}
!309 = !{!"argId", i32 8}
!310 = !{!"implicitArgInfoListVec[3]", !309}
!311 = !{!"implicitArgInfoList", !304, !306, !308, !310}

