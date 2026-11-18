;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -ocl -simd-mode 16 -platformPtl -igc-emit-visa -regkey DumpVISAASMToConsole < %s | FileCheck %s
; RUN: igc_opt --opaque-pointers -ocl -simd-mode 16 -platformPtl -igc-emit-visa -regkey DumpVISAASMToConsole -regkey EnableMixIntKnownBits=0 < %s | FileCheck %s --check-prefix=CHECK-OFF
; RUN: igc_opt --opaque-pointers -ocl -simd-mode 16 -platformNvl -igc-emit-visa -regkey DumpVISAASMToConsole < %s | FileCheck %s --check-prefix=NVL
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Test checks that a dword multiplicand whose known bits fit in 16 bits is read as its lower word (EnableMixIntKnownBits)
; where there is no dword * dword mad (PTL): the word with the stride and offset of the dword is the value itself, W if
; it fits signed, else UW. A dword * dword mul + add becomes a mad, and a dword * dword mul a 16 x 32 mul.
; - %k1 (and 1023) and %k2 (ashr 20, can be negative) fit signed: W. %b is a lane id xor constant: UW.
; - %k3 (and 0xfffe) fits unsigned only: UW.
; - %k4: a mul alone.
; - %k5: a sub of the product: the neg goes on a word in src0.
; On XE3P (NVL) dword * dword mads are allowed: unchanged.

; CHECK-DAG: .decl k1a{{[A-z0-9_]*}} v_type=G type=w
; CHECK-DAG: .decl k2a{{[A-z0-9_]*}} v_type=G type=w
; CHECK-DAG: .decl k3a{{[A-z0-9_]*}} v_type=G type=uw
; CHECK: mad (M1, 16) k1(0,0)<1> b{{[A-z0-9_]*}}(0,0)<16;8,2> k1a{{[A-z0-9_]*}}(0,0)<16;8,2> c(0,0)<1;1,0>
; CHECK: mad (M1, 16) k2(0,0)<1> b{{[A-z0-9_]*}}(0,0)<16;8,2> k2a{{[A-z0-9_]*}}(0,0)<16;8,2> c(0,0)<1;1,0>
; CHECK: mad (M1, 16) k3(0,0)<1> b{{[A-z0-9_]*}}(0,0)<16;8,2> k3a{{[A-z0-9_]*}}(0,0)<16;8,2> c(0,0)<1;1,0>
; CHECK: mul (M1, 16) k4(0,0)<1> k4a{{[A-z0-9_]*}}(0,0)<16;8,2> b(0,0)<1;1,0>
; CHECK: mad (M1, 16) k5(0,0)<1> (-)d{{[A-z0-9_]*}}(0,0)<16;8,2> k5a{{[A-z0-9_]*}}(0,0)<16;8,2> c(0,0)<1;1,0>

; CHECK-OFF-NOT: mad (M1, 16)
; CHECK-OFF: mul (M1, 16) k1a(0,0)<1> k1a(0,0)<1;1,0> b(0,0)<1;1,0>
; CHECK-OFF: add (M1, 16) k1(0,0)<1> k1a(0,0)<1;1,0> c(0,0)<1;1,0>

; NVL: mad (M1, 16) k1(0,0)<1> b(0,0)<1;1,0> k1a(0,0)<1;1,0> c(0,0)<1;1,0>
; NVL: mul (M1, 16) k4(0,0)<1> k4a(0,0)<1;1,0> b(0,0)<1;1,0>

define spir_kernel void @compute16(ptr addrspace(1) %ptr, i32 %_A, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane32 = zext i16 %lane to i32
  %d = xor i32 %lane32, 1001
  %b = xor i32 %lane32, 1002
  %c = xor i32 %lane32, 1003
  %k1a = and i32 %d, 1023
  %k1m = mul i32 %k1a, %b
  %k1 = add i32 %k1m, %c
  %k2a = ashr i32 %d, 20
  %k2m = mul i32 %k2a, %b
  %k2 = add i32 %k2m, %c
  %k3a = and i32 %d, 65534
  %k3m = mul i32 %k3a, %b
  %k3 = add i32 %k3m, %c
  %k4a = and i32 %d, 4095
  %k4 = mul i32 %k4a, %b
  %k5a = and i32 %b, 1023
  %k5m = mul i32 %k5a, %d
  %k5 = sub i32 %c, %k5m
  %q1 = getelementptr i32, ptr addrspace(1) %ptr, i32 1
  store i32 %k1, ptr addrspace(1) %q1, align 4
  %q2 = getelementptr i32, ptr addrspace(1) %ptr, i32 2
  store i32 %k2, ptr addrspace(1) %q2, align 4
  %q3 = getelementptr i32, ptr addrspace(1) %ptr, i32 3
  store i32 %k3, ptr addrspace(1) %q3, align 4
  %q4 = getelementptr i32, ptr addrspace(1) %ptr, i32 4
  store i32 %k4, ptr addrspace(1) %q4, align 4
  %q5 = getelementptr i32, ptr addrspace(1) %ptr, i32 5
  store i32 %k5, ptr addrspace(1) %q5, align 4
  ret void
}
declare i16 @llvm.genx.GenISA.simdLaneId() nounwind readnone

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
