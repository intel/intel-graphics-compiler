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

; Test checks which integer mul + add pairs are matched to mad, given what gen
; mad accepts in src0 (src2 in gen) before XE3P: a 16-bit immediate, a scalar
; word or the lower word of a dword, but not a packed word vector.
; - %x1: both factors are word vectors (zext i16). One of them must become a
;   dword, which moves the other into src0, so mul + add is kept.
; - %x3: lower word * upper word of dwords: the lower word goes to src0, the
;   upper word to src1, which takes any stride and offset.
; - %x4: lower word * word vector: the lower word goes to src0, the packed word
;   vector to src1 as is.

; CHECK-NOT: mad (M1, 16) x1
; CHECK:     mul (M1, 16) [[M1:[A-z0-9_]*]](0,0)<1>
; CHECK:     add (M1, 16) x1(0,0)<1> [[M1]](0,0)<1;1,0>
; CHECK:     mad (M1, 16) x3(0,0)<1> {{[A-z0-9_]*}}(0,0)<16;8,2> {{[A-z0-9_]*}}(0,1)<16;8,2>
; CHECK:     mad (M1, 16) x4(0,0)<1> {{[A-z0-9_]*}}(0,0)<16;8,2> {{[A-z0-9_]*}}(0,0)<1;1,0>

define spir_kernel void @compute16(ptr addrspace(1) %ptr, i32 %_A, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane5 = add i16 %lane, 5
  %za = zext i16 %lane to i32
  %zb = zext i16 %lane5 to i32
  %m1 = mul i32 %za, %zb
  %x1 = add i32 %m1, %_A
  %d3a = add i32 %za, %_A
  %d3b = xor i32 %zb, %_A
  %lo3 = and i32 %d3a, 65535
  %hi3 = lshr i32 %d3b, 16
  %m3 = mul i32 %lo3, %hi3
  %x3 = add i32 %m3, %_A
  %d4 = or i32 %za, %_A
  %lo4 = and i32 %d4, 65535
  %m4 = mul i32 %lo4, %zb
  %x4 = add i32 %m4, %_A
  %p1 = getelementptr i32, ptr addrspace(1) %ptr, i32 %za
  store i32 %x1, ptr addrspace(1) %p1, align 4
  %p3 = getelementptr i32, ptr addrspace(1) %ptr, i32 3
  store i32 %x3, ptr addrspace(1) %p3, align 4
  %p4 = getelementptr i32, ptr addrspace(1) %ptr, i32 4
  store i32 %x4, ptr addrspace(1) %p4, align 4
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
