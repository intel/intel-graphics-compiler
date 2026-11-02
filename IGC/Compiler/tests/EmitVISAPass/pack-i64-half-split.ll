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
; EmitVISAPass - i64 split with a half read in place
; ------------------------------------------------
;
; PackPair and PackI64 read a half of %x (bitcast i64 %x to <2 x i32>) straight
; from %x with a <2;1,0> region, so that half's extractelement is not emitted.
; The split must then move out only the half that is still extracted (lo);
; moving out hi too gives a mov whose result is never read.

; @pack_pair: {(lo & -64) | 4, hi}. lo goes through and/or (bfn), hi is packed
; as is by PackPair.
;
; CHECK-16-LABEL: .kernel "pack_pair"
; CHECK-16:      lsc_load.ugm (M1, 16)  {{[A-Za-z0-9_]+}}:d64
; CHECK-16-NEXT: mov (M1, 16) [[LO:[A-Za-z0-9_]+]](0,0)<1> {{[A-Za-z0-9_]+}}(0,0)<2;1,0>
; No hi half is moved out of %x before the pack.
; CHECK-16-NOT:  (0,1)<2;1,0>
; CHECK-16:      bfn.xf8 (M1, 16) {{[A-Za-z0-9_]+}}(0,0)<1> [[LO]](0,0)<1;1,0>
; CHECK-16-NEXT: mov (M1, 16) [[P:[A-Za-z0-9_]+]](0,0)<2> {{[A-Za-z0-9_]+}}(0,0)<1;1,0>
; CHECK-16-NEXT: mov (M1, 16) [[P]](0,1)<2> {{[A-Za-z0-9_]+}}(0,1)<2;1,0>
;
; CHECK-32-LABEL: .kernel "pack_pair"
; CHECK-32:      lsc_load.ugm (M1, 32)  {{[A-Za-z0-9_]+}}:d64
; CHECK-32-NEXT: mov (M1, 16) [[LO:[A-Za-z0-9_]+]](0,0)<1> [[X:[A-Za-z0-9_]+]](0,0)<2;1,0>
; CHECK-32-NEXT: mov (M5, 16) [[LO]](1,0)<1> [[X]](2,0)<2;1,0>
; CHECK-32-NOT:  ({{[0-9]+}},1)<2;1,0>
; CHECK-32:      bfn.xf8 (M1, 32) {{[A-Za-z0-9_]+}}(0,0)<1> [[LO]](0,0)<1;1,0>
; CHECK-32-NEXT: mov (M1, 16) [[P:[A-Za-z0-9_]+]](0,0)<2> {{[A-Za-z0-9_]+}}(0,0)<1;1,0>
; CHECK-32-NEXT: mov (M5, 16) [[P]](2,0)<2> {{[A-Za-z0-9_]+}}(1,0)<1;1,0>
; CHECK-32-NEXT: mov (M1, 16) [[P]](0,1)<2> [[XH:[A-Za-z0-9_]+]](0,1)<2;1,0>
; CHECK-32-NEXT: mov (M5, 16) [[P]](2,1)<2> [[XH]](2,1)<2;1,0>

define spir_kernel void @pack_pair(i64 addrspace(1)* %src, i64 addrspace(1)* %dst, i32 %arg0, <3 x float> %val, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %idx = zext i16 %localIdX to i64
  %xp = getelementptr i64, i64 addrspace(1)* %src, i64 %idx
  %x = load i64, i64 addrspace(1)* %xp
  %xv = bitcast i64 %x to <2 x i32>
  %lo = extractelement <2 x i32> %xv, i32 0
  %hi = extractelement <2 x i32> %xv, i32 1
  %lo.and = and i32 %lo, -64
  %lo.or = or i32 %lo.and, 4
  %p0 = insertelement <2 x i32> undef, i32 %lo.or, i32 0
  %p1 = insertelement <2 x i32> %p0, i32 %hi, i32 1
  %p = bitcast <2 x i32> %p1 to i64
  store i64 %p, i64 addrspace(1)* %dst
  ret void
}

; @pack_i64: {lo ^ %arg0, hi ^ %arg0}. PackI64 xors both halves in place; lo is
; also used by an add, so the split is still needed for lo.
;
; CHECK-16-LABEL: .kernel "pack_i64"
; CHECK-16:      lsc_load.ugm (M1, 16)  {{[A-Za-z0-9_]+}}:d64
; CHECK-16-NEXT: mov (M1, 16) [[LO:[A-Za-z0-9_]+]](0,0)<1> [[X:[A-Za-z0-9_]+]](0,0)<2;1,0>
; CHECK-16-NEXT: xor (M1, 16) [[P:[A-Za-z0-9_]+]](0,0)<2> [[X]](0,0)<2;1,0>
; CHECK-16-NEXT: xor (M1, 16) [[P]](0,1)<2> [[X]](0,1)<2;1,0>
; CHECK-16-NEXT: add (M1, 16) {{[A-Za-z0-9_]+}}(0,0)<1> [[LO]](0,0)<1;1,0> 0x1:w
;
; CHECK-32-LABEL: .kernel "pack_i64"
; CHECK-32:      lsc_load.ugm (M1, 32)  {{[A-Za-z0-9_]+}}:d64
; CHECK-32-NEXT: mov (M1, 16) [[LO:[A-Za-z0-9_]+]](0,0)<1> [[X:[A-Za-z0-9_]+]](0,0)<2;1,0>
; CHECK-32-NEXT: mov (M5, 16) [[LO]](1,0)<1> [[X]](2,0)<2;1,0>
; CHECK-32-NEXT: xor (M1, 16) [[P:[A-Za-z0-9_]+]](0,0)<2> [[X]](0,0)<2;1,0>
; CHECK-32-NEXT: xor (M5, 16) [[P]](2,0)<2> [[X]](2,0)<2;1,0>
; CHECK-32-NEXT: xor (M1, 16) [[P]](0,1)<2> [[X]](0,1)<2;1,0>
; CHECK-32-NEXT: xor (M5, 16) [[P]](2,1)<2> [[X]](2,1)<2;1,0>
; CHECK-32-NEXT: add (M1, 32) {{[A-Za-z0-9_]+}}(0,0)<1> [[LO]](0,0)<1;1,0> 0x1:w

define spir_kernel void @pack_i64(i64 addrspace(1)* %src, i64 addrspace(1)* %dst, i32 %arg0, <3 x float> %val, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %idx = zext i16 %localIdX to i64
  %xp = getelementptr i64, i64 addrspace(1)* %src, i64 %idx
  %x = load i64, i64 addrspace(1)* %xp
  %xv = bitcast i64 %x to <2 x i32>
  %lo = extractelement <2 x i32> %xv, i32 0
  %hi = extractelement <2 x i32> %xv, i32 1
  %lo.xor = xor i32 %lo, %arg0
  %hi.xor = xor i32 %hi, %arg0
  %p0 = insertelement <2 x i32> undef, i32 %lo.xor, i32 0
  %p1 = insertelement <2 x i32> %p0, i32 %hi.xor, i32 1
  %p = bitcast <2 x i32> %p1 to i64
  %lo.inc = add i32 %lo, 1
  %lo.zext = zext i32 %lo.inc to i64
  %r = add i64 %p, %lo.zext
  store i64 %r, i64 addrspace(1)* %dst
  ret void
}

!IGCMetadata = !{!100}
!igc.functions = !{!0, !1}

!0 = !{void (i64 addrspace(1)*, i64 addrspace(1)*, i32, <3 x float>, <8 x i32>, <8 x i32>, i16)* @pack_pair, !2}
!1 = !{void (i64 addrspace(1)*, i64 addrspace(1)*, i32, <3 x float>, <8 x i32>, <8 x i32>, i16)* @pack_i64, !2}
!2 = !{!3}
!3 = !{!"function_type", i32 0}
!100 = !{!"ModuleMD", !101}
!101 = !{!"FuncMD", !102, !103, !104, !105}
!102 = !{!"FuncMDMap[0]", void (i64 addrspace(1)*, i64 addrspace(1)*, i32, <3 x float>, <8 x i32>, <8 x i32>, i16)* @pack_pair}
!103 = !{!"FuncMDValue[0]", !110, !200, !311}
!104 = !{!"FuncMDMap[1]", void (i64 addrspace(1)*, i64 addrspace(1)*, i32, <3 x float>, <8 x i32>, <8 x i32>, i16)* @pack_i64}
!105 = !{!"FuncMDValue[1]", !110, !200, !311}
!110 = !{!"functionType", !"KernelFunction"}
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
