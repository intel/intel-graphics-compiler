;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -ocl -simd-mode 32 -platformpvc -igc-emit-visa -regkey DumpVISAASMToConsole < %s | FileCheck %s
; RUN: igc_opt --opaque-pointers -ocl -simd-mode 32 -platformpvc -igc-emit-visa -regkey DumpVISAASMToConsole -regkey Add3SkipMask=0 < %s | FileCheck %s --check-prefix=OFF
; RUN: igc_opt --opaque-pointers -ocl -simd-mode 32 -platformpvc -igc-emit-visa -regkey DumpVISAASMToConsole -regkey Add3SkipMask=5 < %s | FileCheck %s --check-prefix=NOCONST
; RUN: igc_opt --opaque-pointers -ocl -simd-mode 32 -platformpvc -igc-emit-visa -regkey DumpVISAASMToConsole -regkey Add3SkipMask=3 < %s | FileCheck %s --check-prefix=NOPEERS
; RUN: igc_opt --opaque-pointers -ocl -simd-mode 32 -platformpvc -igc-emit-visa -regkey DumpVISAASMToConsole -regkey Add3SkipMask=15 < %s | FileCheck %s --check-prefix=PREFER
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Test checks the add3 skip for a multi-use folded add (Add3SkipMask). Each block is one case. The prefixes other
; than CHECK (the default, 7) cover the cases that one bit of Add3SkipMask changes: OFF (0), NOCONST (5, 0x2 off),
; NOPEERS (3, 0x4 off), PREFER (15, 0x8 on).

define spir_kernel void @compute16(ptr addrspace(1) %ptr, i32 %_A, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i32 %bufferOffset) {
entry:
  br label %case_base

; %x has two uses, so it is emitted anyway. Folding it into %y as add3(%s2, %convz, 15) would force the single-use %s2
; to be materialized instead of being folded into %x's own add3. Expect
;   x = add3(s1, convy, convz); y = add(x, 15)
; Use shl to prevent MAD pattern match
; CHECK-LABEL: _case_base:
; CHECK: shl (M1_NM, 1) [[S1:[A-z0-9]*]](0,0)<1> {{.*}}0x5:d
; CHECK: add3 (M1_NM, 1) [[X:[A-z0-9]*]](0,0)<1> [[S1]](0,0)<0;1,0>
; CHECK: add (M1_NM, 1) {{.*}} [[X]](0,0)<0;1,0> 0xf:w
; CHECK-NOT: add3 (M1_NM, 1)
; OFF-LABEL: _case_base:
; OFF: shl (M1_NM, 1) [[S1:[A-z0-9]*]](0,0)<1> {{.*}}0x5:d
; OFF: add (M1_NM, 1) [[S2:[A-z0-9]*]](0,0)<1> [[S1]](0,0)<0;1,0>
; OFF: add3 (M1_NM, 1) {{.*}} [[S2]](0,0)<0;1,0> {{[A-z0-9_]*}}(0,0)<0;1,0> 0xf:d
case_base:
  %base_conv = zext i16 %localIdX to i32
  %base_convy = zext i16 %localIdY to i32
  %base_convz = zext i16 %localIdZ to i32
  %base_s1 = shl i32 %base_conv, 5
  %base_s2 = add i32 %base_s1, %base_convy
  %base_x = add i32 %base_s2, %base_convz
  %base_y = add i32 %base_x, 15
  %base_p0 = getelementptr i32, ptr addrspace(1) %ptr, i32 0
  store i32 %base_y, ptr addrspace(1) %base_p0, align 4
  %base_p1 = getelementptr i32, ptr addrspace(1) %ptr, i32 1
  store i32 %base_x, ptr addrspace(1) %base_p1, align 4
  br label %case_mad

; Same with a mad-foldable operand: folding %x into %y as add3(%m, %convy, 15) would force the single-use %m to be
; materialized instead of being folded into %x's own mad. Expect
;   x = mad(conv, 3, convy); y = add(x, 15)
; CHECK-LABEL: _case_mad:
; CHECK: mad (M1_NM, 1) [[X:[A-z0-9]*]](0,0)<1>
; CHECK-NOT: mul (M1_NM, 1)
; CHECK: add (M1_NM, 1) {{.*}} [[X]](0,0)<0;1,0> 0xf:w
; CHECK-NOT: add3 (M1_NM, 1)
; OFF-LABEL: _case_mad:
; OFF: mul (M1_NM, 1) [[M:[A-z0-9]*]](0,0)<1>
; OFF: add3 (M1_NM, 1) {{.*}} [[M]](0,0)<0;1,0>
case_mad:
  %mad_conv = zext i16 %localIdX to i32
  %mad_convy = zext i16 %localIdY to i32
  %mad_m = mul i32 %mad_conv, 3
  %mad_x = add i32 %mad_m, %mad_convy
  %mad_y = add i32 %mad_x, 15
  %mad_p0 = getelementptr i32, ptr addrspace(1) %ptr, i32 2
  store i32 %mad_y, ptr addrspace(1) %mad_p0, align 4
  %mad_p1 = getelementptr i32, ptr addrspace(1) %ptr, i32 3
  store i32 %mad_x, ptr addrspace(1) %mad_p1, align 4
  br label %case_simdconst

; 0x2: an operand of the folded add that is a constant expression on simdSize is not counted: %c = simdSize * 3504 + 16
; is emitted as an immediate (0x1b610 at SIMD32), so %x absorbing it saves nothing. %y keeps folding %x:
; add3(0x1b610, %s1, 64), and does not wait for %x.
; CHECK-LABEL: _case_simdconst:
; CHECK: shl (M1_NM, 1) [[S1:[A-z0-9]*]](0,0)<1>
; CHECK: add3 (M1_NM, 1) {{.*}} 0x1b610:d [[S1]](0,0)<0;1,0> 0x40:d
; CHECK-NOT: add (M1_NM, 1) {{.*}} 0x40:w
; NOCONST-LABEL: _case_simdconst:
; NOCONST: add (M1_NM, 1) {{.*}} sc_x(0,0)<0;1,0> 0x40:w
case_simdconst:
  %sc_conv = zext i16 %localIdX to i32
  %sc_ss = call i32 @llvm.genx.GenISA.simdSize()
  %sc_k = mul i32 %sc_ss, 3504
  %sc_c = add i32 %sc_k, 16
  %sc_s1 = shl i32 %sc_conv, 2
  %sc_x = add i32 %sc_c, %sc_s1
  %sc_y = add i32 %sc_x, 64
  %sc_p0 = getelementptr i32, ptr addrspace(1) %ptr, i32 4
  store i32 %sc_y, ptr addrspace(1) %sc_p0, align 4
  %sc_p1 = getelementptr i32, ptr addrspace(1) %ptr, i32 5
  store i32 %sc_x, ptr addrspace(1) %sc_p1, align 4
  br label %case_peers

; 0x4: the other users of the folded add that fold it too are folders, not readers: %x = %m + %convy is used only by
; the adds %y1 and %y2. Folding into both (mul + 2 x add3) costs the same as skipping (mad + 2 x add), so %x is not
; emitted and both stay add3.
; CHECK-LABEL: _case_peers:
; CHECK: mul (M1_NM, 1) [[M:[A-z0-9]*]](0,0)<1>
; CHECK-NOT: mad (M1_NM, 1)
; CHECK: add3 (M1_NM, 1) {{.*}} [[M]](0,0)<0;1,0> {{.*}} 0x1:d
; CHECK: add3 (M1_NM, 1) {{.*}} [[M]](0,0)<0;1,0> {{.*}} 0x2:d
; NOPEERS-LABEL: _case_peers:
; NOPEERS: mad (M1_NM, 1)
; NOPEERS-NOT: add3 (M1_NM, 1)
case_peers:
  %peers_conv = zext i16 %localIdX to i32
  %peers_convy = zext i16 %localIdY to i32
  %peers_m = mul i32 %peers_conv, 3
  %peers_x = add i32 %peers_m, %peers_convy
  %peers_y1 = add i32 %peers_x, 1
  %peers_y2 = add i32 %peers_x, 2
  %peers_p0 = getelementptr i32, ptr addrspace(1) %ptr, i32 6
  store i32 %peers_y1, ptr addrspace(1) %peers_p0, align 4
  %peers_p1 = getelementptr i32, ptr addrspace(1) %ptr, i32 7
  store i32 %peers_y2, ptr addrspace(1) %peers_p1, align 4
  br label %case_reader

; 0x4 with a reader: besides the adds that fold it, %x has a user that reads it (the compare). %x is emitted anyway, as
; the mad that absorbs %m, and both adds read it.
; CHECK-LABEL: _case_reader:
; CHECK: mad (M1_NM, 1) [[X:[A-z0-9]*]](0,0)<1>
; CHECK-NOT: mul (M1_NM, 1)
; CHECK-NOT: add3 (M1_NM, 1)
; CHECK-DAG: add (M1_NM, 1) {{.*}} [[X]](0,0)<0;1,0> 0x1:w
; CHECK-DAG: add (M1_NM, 1) {{.*}} [[X]](0,0)<0;1,0> 0x2:w
; NOPEERS-LABEL: _case_reader:
case_reader:
  %reader_conv = zext i16 %localIdX to i32
  %reader_convy = zext i16 %localIdY to i32
  %reader_m = mul i32 %reader_conv, 3
  %reader_x = add i32 %reader_m, %reader_convy
  %reader_y1 = add i32 %reader_x, 1
  %reader_y2 = add i32 %reader_x, 2
  %reader_neg = icmp slt i32 %reader_x, 0
  %reader_sel = select i1 %reader_neg, i32 %reader_y1, i32 %reader_y2
  %reader_p0 = getelementptr i32, ptr addrspace(1) %ptr, i32 8
  store i32 %reader_sel, ptr addrspace(1) %reader_p0, align 4
  br label %case_fallback

; When folding operand 0 doesn't pay off, operand 1 is folded instead: %x is read by the compare, so it is emitted
; anyway (as the mad that absorbs %m), and %y = %x + %w folds the single-use %w: add3(%x, %convz, 7). %z = %x + %v
; stays an add: %v has another use, so it is emitted anyway and folding it would save nothing.
; CHECK-LABEL: _case_fallback:
; CHECK: mad (M1_NM, 1) [[X:[A-z0-9]*]](0,0)<1>
; CHECK-NOT: add (M1_NM, 1) fb_w
; CHECK: add3 (M1_NM, 1) fb_y(0,0)<1> [[X]](0,0)<0;1,0> {{.*}} 0x7:d
; CHECK: add (M1_NM, 1) fb_v(0,0)<1>
; CHECK: add (M1_NM, 1) fb_z(0,0)<1> [[X]](0,0)<0;1,0> fb_v(0,0)<0;1,0>
; OFF-LABEL: _case_fallback:
; With Add3SkipMask=0, %x (3 uses) is rejected by the OpenCL threshold and operand 1 is not tried:
; OFF: add (M1_NM, 1) fb_w
; OFF: add (M1_NM, 1) fb_y(0,0)<1> fb_x(0,0)<0;1,0> fb_w(0,0)<0;1,0>
case_fallback:
  %fb_conv = zext i16 %localIdX to i32
  %fb_convy = zext i16 %localIdY to i32
  %fb_convz = zext i16 %localIdZ to i32
  %fb_m = mul i32 %fb_conv, 3
  %fb_x = add i32 %fb_m, %fb_convy
  %fb_w = add i32 %fb_convz, 7
  %fb_y = add i32 %fb_x, %fb_w
  %fb_v = add i32 %fb_convz, 9
  %fb_z = add i32 %fb_x, %fb_v
  %fb_neg = icmp slt i32 %fb_x, 0
  %fb_sel = select i1 %fb_neg, i32 %fb_y, i32 0
  %fb_p0 = getelementptr i32, ptr addrspace(1) %ptr, i32 11
  store i32 %fb_sel, ptr addrspace(1) %fb_p0, align 4
  %fb_p1 = getelementptr i32, ptr addrspace(1) %ptr, i32 12
  store i32 %fb_z, ptr addrspace(1) %fb_p1, align 4
  %fb_p2 = getelementptr i32, ptr addrspace(1) %ptr, i32 13
  store i32 %fb_v, ptr addrspace(1) %fb_p2, align 4
  br label %case_prefer

; 0x8: %x = %a + %b absorbs nothing, so folding it pays off, but it is read by the compare, so folding it removes no
; instruction; %w is single-use. With 0x8, %y = %x + %w folds %w instead: add3(%x, %c, 7), and %w is not emitted.
; Without it (the default), %y folds %x and %w stays an add.
; CHECK-LABEL: _case_prefer:
; CHECK: add (M1_NM, 1) pr_w
; CHECK: add3 (M1_NM, 1) pr_y(0,0)<1> {{.*}} pr_w(0,0)<0;1,0>
; PREFER-LABEL: _case_prefer:
; PREFER: add (M1_NM, 1) pr_x(0,0)<1>
; PREFER-NOT: add (M1_NM, 1) pr_w
; PREFER: add3 (M1_NM, 1) pr_y(0,0)<1> pr_x(0,0)<0;1,0> {{.*}} 0x7:d
case_prefer:
  %pr_a = zext i16 %localIdX to i32
  %pr_b = zext i16 %localIdY to i32
  %pr_c = zext i16 %localIdZ to i32
  %pr_x = add i32 %pr_a, %pr_b
  %pr_w = add i32 %pr_c, 7
  %pr_y = add i32 %pr_x, %pr_w
  %pr_neg = icmp slt i32 %pr_x, 0
  %pr_sel = select i1 %pr_neg, i32 %pr_y, i32 0
  %pr_p0 = getelementptr i32, ptr addrspace(1) %ptr, i32 14
  store i32 %pr_sel, ptr addrspace(1) %pr_p0, align 4
  br label %case_neg

; A negation (0 - x) is a free neg modifier, not an instruction, so it is not folded into an add3: %y = %x + (0 - %c)
; folds %x, add3(%a, %b, -%c), instead of making add3(%x, -%c, 0), and %z = %l + (0 - %c) stays add(%l, -%c) (with
; Add3SkipMask=0, the previous add3(%l, -%c, 0)). A negated 16-bit form is a negated word source: %v = %l + (0 -
; zext %localIdY) is add(%l, -localIdY) (MatchMulAdd16). A single-use negation of a single-use add is looked through:
; %t = (0 - (%a + %d)) + %l is add3(%l, -%a, -%d), and %u = (0 - (%b + %l)) - %c negates all three sources.
; CHECK-LABEL: _case_neg:
; CHECK: add3 (M1_NM, 1) ng_y(0,0)<1> {{.*}}(-)ng_c(0,0)<0;1,0>
; CHECK: add (M1_NM, 1) ng_z(0,0)<1> ng_l(0,0)<0;1,0> (-)ng_c(0,0)<0;1,0>
; CHECK: add (M1_NM, 1) ng_v(0,0)<1> ng_l(0,0)<0;1,0> (-){{.*}}
; CHECK: add3 (M1_NM, 1) ng_t(0,0)<1> ng_l(0,0)<0;1,0> (-){{.*}} (-){{.*}}
; CHECK: add3 (M1_NM, 1) ng_u(0,0)<1> (-){{.*}} (-){{.*}} (-)ng_c(0,0)<0;1,0>
; OFF-LABEL: _case_neg:
; OFF: add3 (M1_NM, 1) ng_z(0,0)<1> ng_l(0,0)<0;1,0> (-)ng_c(0,0)<0;1,0> 0x0:d
case_neg:
  %ng_a = zext i16 %localIdX to i32
  %ng_b = zext i16 %localIdY to i32
  %ng_cz = zext i16 %localIdZ to i32
  %ng_c = shl i32 %ng_cz, 2
  %ng_d = shl i32 %ng_b, 1
  %ng_x = add i32 %ng_a, %ng_b
  %ng_n = sub i32 0, %ng_c
  %ng_y = add i32 %ng_x, %ng_n
  %ng_l = shl i32 %ng_a, 3
  %ng_z = add i32 %ng_l, %ng_n
  %ng_wy = zext i16 %localIdY to i32
  %ng_w = sub i32 0, %ng_wy
  %ng_v = add i32 %ng_l, %ng_w
  %ng_ta = add i32 %ng_a, %ng_d
  %ng_tn = sub i32 0, %ng_ta
  %ng_t = add i32 %ng_tn, %ng_l
  %ng_ua = add i32 %ng_b, %ng_l
  %ng_un = sub i32 0, %ng_ua
  %ng_u = sub i32 %ng_un, %ng_c
  %ng_neg = icmp slt i32 %ng_x, 0
  %ng_sel = select i1 %ng_neg, i32 %ng_y, i32 %ng_z
  %ng_p0 = getelementptr i32, ptr addrspace(1) %ptr, i32 15
  store i32 %ng_sel, ptr addrspace(1) %ng_p0, align 4
  %ng_p1 = getelementptr i32, ptr addrspace(1) %ptr, i32 16
  store i32 %ng_v, ptr addrspace(1) %ng_p1, align 4
  %ng_p2 = getelementptr i32, ptr addrspace(1) %ptr, i32 17
  store i32 %ng_t, ptr addrspace(1) %ng_p2, align 4
  %ng_p3 = getelementptr i32, ptr addrspace(1) %ptr, i32 18
  store i32 %ng_u, ptr addrspace(1) %ng_p3, align 4
  br label %case_crossblock

; 0x4 across blocks: %y2 is in %case_crossblock_use, which is matched before %case_crossblock. %y1 is not matched yet
; then, but it is an add that folds %x too, so it is not assumed to read %x: both users fold %x (mul + 2 x add3) and
; %x is never emitted.
; CHECK-LABEL: _case_crossblock:
; CHECK: mul (M1_NM, 1) [[M:[A-z0-9]*]](0,0)<1>
; CHECK-NOT: mad (M1_NM, 1)
; CHECK: add3 (M1_NM, 1) {{.*}} [[M]](0,0)<0;1,0> {{.*}} 0x1:d
; CHECK: add3 (M1_NM, 1) {{.*}} [[M]](0,0)<0;1,0> {{.*}} 0x2:d
; NOPEERS-LABEL: _case_crossblock:
; NOPEERS: mad (M1_NM, 1)
; NOPEERS-NOT: add3 (M1_NM, 1)
case_crossblock:
  %cb_conv = zext i16 %localIdX to i32
  %cb_convy = zext i16 %localIdY to i32
  %cb_m = mul i32 %cb_conv, 3
  %cb_x = add i32 %cb_m, %cb_convy
  %cb_y1 = add i32 %cb_x, 1
  %cb_p0 = getelementptr i32, ptr addrspace(1) %ptr, i32 9
  store i32 %cb_y1, ptr addrspace(1) %cb_p0, align 4
  %cb_cmp = icmp eq i32 %_A, 0
  br i1 %cb_cmp, label %case_crossblock_use, label %exit

case_crossblock_use:
  %cb_y2 = add i32 %cb_x, 2
  %cb_p1 = getelementptr i32, ptr addrspace(1) %ptr, i32 10
  store i32 %cb_y2, ptr addrspace(1) %cb_p1, align 4
  br label %exit

exit:
  ret void
}

declare i32 @llvm.genx.GenISA.simdSize() nounwind readnone

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
