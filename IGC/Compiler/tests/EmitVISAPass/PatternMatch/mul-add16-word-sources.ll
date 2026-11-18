;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -ocl -simd-mode 16 -platformPtl -igc-emit-visa -regkey DumpVISAASMToConsole < %s | FileCheck %s --check-prefix=PTL
; RUN: igc_opt --opaque-pointers -ocl -simd-mode 16 -platformNvl -igc-emit-visa -regkey DumpVISAASMToConsole < %s | FileCheck %s --check-prefix=NVL
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Test checks the 16-bit sources of a 2-source add/sub/mul (MatchMulAdd16): which halves of a dword and zext/sext of an
; i16 value are read as W/UW sources, and where, so that the finalizer adds no mov. Each block is one case.

define spir_kernel void @compute16(ptr addrspace(1) %ptr, i32 %_A, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
entry:
  %lane = call i16 @llvm.genx.GenISA.simdLaneId()
  %lane32 = zext i16 %lane to i32
  %d = xor i32 %lane32, 1001
  %a = xor i32 %lane32, 1002
  br label %case_zext_add

; A single-use zext of a non-uniform i16 is read as a packed UW vector. XE3P: not in src1 of an add, so it is moved to
; src0.
; PTL-LABEL: _case_zext_add:
; PTL: add (M1, 16) r1(0,0)<1> d(0,0)<1;1,0> h1{{[A-z0-9_]*}}(0,0)<1;1,0>
; NVL-LABEL: _case_zext_add:
; NVL: add (M1, 16) r1(0,0)<1> h1{{[A-z0-9_]*}}(0,0)<1;1,0> d(0,0)<1;1,0>
case_zext_add:
  %h1 = xor i16 %lane, 11
  %z1 = zext i16 %h1 to i32
  %r1 = add i32 %d, %z1
  %p1 = getelementptr i32, ptr addrspace(1) %ptr, i32 1
  store i32 %r1, ptr addrspace(1) %p1, align 4
  br label %case_sext_mul

; A sext in a mul is read as a packed W vector. XE3P needs flat mul sources and adds a mov, which costs no more than
; emitting the sext.
; PTL-LABEL: _case_sext_mul:
; PTL-NOT: mov (M1, 16) s2
; PTL: mul (M1, 16) r2(0,0)<1> h2(0,0)<1;1,0> d(0,0)<1;1,0>
; NVL-LABEL: _case_sext_mul:
; NVL-NOT: mov (M1, 16) s2
; NVL: mul (M1, 16) r2(0,0)<1> h2(0,0)<1;1,0> d(0,0)<1;1,0>
case_sext_mul:
  %h2 = xor i16 %lane, 12
  %s2 = sext i16 %h2 to i32
  %r2 = mul i32 %s2, %d
  %p2 = getelementptr i32, ptr addrspace(1) %ptr, i32 2
  store i32 %r2, ptr addrspace(1) %p2, align 4
  br label %case_zext_sub

; A zext subtracted: the neg applies to the widened word, so it is read as -h3:uw.
; PTL-LABEL: _case_zext_sub:
; PTL-NOT: mov (M1, 16) z3
; PTL: add (M1, 16) r3(0,0)<1> d(0,0)<1;1,0> (-)h3{{[A-z0-9_]*}}(0,0)<1;1,0>
; NVL-LABEL: _case_zext_sub:
; NVL-NOT: mov (M1, 16) z3
; NVL: add (M1, 16) r3(0,0)<1> d(0,0)<1;1,0> (-)h3{{[A-z0-9_]*}}(0,0)<1;1,0>
case_zext_sub:
  %h3 = xor i16 %lane, 13
  %z3 = zext i16 %h3 to i32
  %r3 = sub i32 %d, %z3
  %p3 = getelementptr i32, ptr addrspace(1) %ptr, i32 3
  store i32 %r3, ptr addrspace(1) %p3, align 4
  br label %case_zext_multiuse

; A zext with another use is emitted anyway, so the add reads the dword.
; PTL-LABEL: _case_zext_multiuse:
; PTL: mov (M1, 16) z4(0,0)<1> h4{{[A-z0-9_]*}}(0,0)<1;1,0>
; PTL: add (M1, 16) r4(0,0)<1> d(0,0)<1;1,0> z4(0,0)<1;1,0>
case_zext_multiuse:
  %h4 = xor i16 %lane, 14
  %z4 = zext i16 %h4 to i32
  %r4 = add i32 %d, %z4
  %p4 = getelementptr i32, ptr addrspace(1) %ptr, i32 4
  store i32 %r4, ptr addrspace(1) %p4, align 4
  %p4b = getelementptr i32, ptr addrspace(1) %ptr, i32 5
  store i32 %z4, ptr addrspace(1) %p4b, align 4
  br label %case_hi_add

; The upper half of a dword in src1 of an add: PTL reads it there; XE3P moves it to src0.
; PTL-LABEL: _case_hi_add:
; PTL: add (M1, 16) r5(0,0)<1> d(0,0)<1;1,0> a{{[A-z0-9_]*}}(0,1)<16;8,2>
; NVL-LABEL: _case_hi_add:
; NVL: add (M1, 16) r5(0,0)<1> a{{[A-z0-9_]*}}(0,1)<16;8,2> d(0,0)<1;1,0>
case_hi_add:
  %hi5 = lshr i32 %a, 16
  %r5 = add i32 %d, %hi5
  %p5 = getelementptr i32, ptr addrspace(1) %ptr, i32 6
  store i32 %r5, ptr addrspace(1) %p5, align 4
  br label %case_hi_mul

; The upper half of a dword in a mul is read as a word on both (XE3P adds a mov, instead of the shr).
; PTL-LABEL: _case_hi_mul:
; PTL: mul (M1, 16) r6(0,0)<1> a{{[A-z0-9_]*}}(0,1)<16;8,2> d(0,0)<1;1,0>
; NVL-LABEL: _case_hi_mul:
; NVL-NOT: shr (M1, 16) hi6
; NVL: mul (M1, 16) r6(0,0)<1> a{{[A-z0-9_]*}}(0,1)<16;8,2> d(0,0)<1;1,0>
case_hi_mul:
  %hi6 = lshr i32 %a, 16
  %r6 = mul i32 %hi6, %d
  %p6 = getelementptr i32, ptr addrspace(1) %ptr, i32 7
  store i32 %r6, ptr addrspace(1) %p6, align 4
  br label %case_lo_mul_wide

; The lower half of a dword times a 32-bit immediate stays a word: the finalizer adds a mov, but reading the dword
; instead would make it a 32x32 multiply (mul + macl before XE3P).
; PTL-LABEL: _case_lo_mul_wide:
; PTL: mul (M1, 16) r7(0,0)<1> a{{[A-z0-9_]*}}(0,0)<16;8,2> 0x12345678:d
; NVL-LABEL: _case_lo_mul_wide:
; NVL: mul (M1, 16) r7(0,0)<1> a{{[A-z0-9_]*}}(0,0)<16;8,2> 0x12345678:d
case_lo_mul_wide:
  %lo7 = and i32 %a, 65535
  %r7 = mul i32 %lo7, 305419896
  %p7 = getelementptr i32, ptr addrspace(1) %ptr, i32 8
  store i32 %r7, ptr addrspace(1) %p7, align 4
  br label %case_neg_mul

; A negated 16-bit form in a mul next to another word: the word with a neg (before XE3P only accepted next to a word).
; PTL-LABEL: _case_neg_mul:
; PTL: mul (M1, 16) r8(0,0)<1> {{[A-z0-9_]*}}(0,0)<16;8,2> (-){{[A-z0-9_]*}}(0,0)<16;8,2>
; NVL-LABEL: _case_neg_mul:
; NVL: mul (M1, 16) r8(0,0)<1> {{[A-z0-9_]*}}(0,0)<16;8,2> (-){{[A-z0-9_]*}}(0,0)<16;8,2>
case_neg_mul:
  %lo8 = and i32 %a, 65535
  %ld8 = and i32 %d, 65535
  %n8 = sub i32 0, %ld8
  %r8 = mul i32 %lo8, %n8
  %p8 = getelementptr i32, ptr addrspace(1) %ptr, i32 9
  store i32 %r8, ptr addrspace(1) %p8, align 4
  br label %case_kept_extend

; A zext of an i16 with several users is kept as its dword, and not read as its lower word either: the finalizer
; propagates the i16 source of the zext into the muls, which a <16;8,2> region of the dword would prevent.
; PTL-LABEL: _case_kept_extend:
; PTL: mul (M1, 16) r9(0,0)<1> _A(0,0)<0;1,0> z9(0,0)<1;1,0>
; NVL-LABEL: _case_kept_extend:
; NVL: mul (M1, 16) r9(0,0)<1> _A(0,0)<0;1,0> z9(0,0)<1;1,0>
case_kept_extend:
  %h9 = xor i16 %lane, 9
  %z9 = zext i16 %h9 to i32
  %r9 = mul i32 %_A, %z9
  %s9 = mul i32 %z9, %bufferOffset
  %p9 = getelementptr i32, ptr addrspace(1) %ptr, i32 10
  store i32 %r9, ptr addrspace(1) %p9, align 4
  %q9 = getelementptr i32, ptr addrspace(1) %ptr, i32 11
  store i32 %s9, ptr addrspace(1) %q9, align 4
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
