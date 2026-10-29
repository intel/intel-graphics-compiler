;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt %s --opaque-pointers -platformpvc -igc-emit-visa -simd-mode 16 -regkey DumpVISAASMToConsole,AddVISADumpDeclarationsToEnd=1,VectorAlias=8 | FileCheck %s

; Each extractelement is the dpas accumulator input, so VRA coalesces it with
; the dpas result (dpas dst). A dpas dst must be GRF-aligned. With 16 lanes of
; i16, element i starts at byte i * 32 of the vector. So only elements 0 and 2
; can be aliased to the vector; elements 1 and 3 must not be.

; CHECK-LABEL: .kernel "test"
; CHECK:       lsc_load_block2d.ugm (M1, 1)  [[ACC:[A-Za-z0-9_]+]]:d16.32x2nn
; CHECK:       mov (M1, 16) [[E1:[A-Za-z0-9_]+]](0,0)<1> [[ACC]](0,16)<1;1,0>
; CHECK:       mov (M1, 16) [[E3:[A-Za-z0-9_]+]](0,0)<1> [[ACC]](1,16)<1;1,0>
; CHECK:       dpas.bf.bf.8.1 (M1, 16) [[ACCA:[A-Za-z0-9_]+]].0
; CHECK-NOT:   dpas{{.*}} (M1, 16) [[ACCA]].{{32|96}}
; CHECK:       dpas.bf.bf.8.1 (M1, 16) [[E1A:[A-Za-z0-9_]+]].0
; CHECK-NOT:   dpas{{.*}} (M1, 16) [[ACCA]].{{32|96}}
; CHECK:       dpas.bf.bf.8.1 (M1, 16) [[ACCA]].64
; CHECK-NOT:   dpas{{.*}} (M1, 16) [[ACCA]].{{32|96}}
; CHECK:       dpas.bf.bf.8.1 (M1, 16) [[E3A:[A-Za-z0-9_]+]].0
; CHECK-NOT:   dpas{{.*}} (M1, 16) [[ACCA]].{{32|96}}
; CHECK:       mov (M1, 16) [[OUT:[A-Za-z0-9_]+]](0,0)<1> [[ACC]](0,0)<1;1,0>
; CHECK:       mov (M1, 16) [[OUT]](0,16)<1> [[E1]](0,0)<1;1,0>
; CHECK:       mov (M1, 16) [[OUT]](1,0)<1>  [[ACC]](1,0)<1;1,0>
; CHECK:       mov (M1, 16) [[OUT]](1,16)<1> [[E3]](0,0)<1;1,0>
; CHECK:       lsc_store_block2d.ugm (M1, 1) {{.*}} [[OUT]]:d16.32x2nn
; CHECK:       .decl [[ACC]] v_type=G type=w num_elts=64
; CHECK:       .decl [[E1]] v_type=G type=w num_elts=16
; CHECK:       .decl [[E3]] v_type=G type=w num_elts=16
; CHECK:       .decl [[ACCA]] v_type=G type=bf num_elts=64 {{.*}} alias=<[[ACC]], 0>
; CHECK:       .decl [[E1A]] v_type=G type=bf num_elts=16 {{.*}} alias=<[[E1]], 0>
; CHECK:       .decl [[E3A]] v_type=G type=bf num_elts=16 {{.*}} alias=<[[E3]], 0>

define spir_kernel void @test(i64 %base, i32 %x, <8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
  %a = call i16 @llvm.genx.GenISA.LSC2DBlockRead.i16(i64 %base, i32 63, i32 0, i32 63, i32 %x, i32 0, i32 16, i32 16, i32 1, i32 1, i1 false, i1 false, i32 0)
  %b = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 0, i32 16, i32 16, i32 16, i32 1, i1 false, i1 true, i32 0)
  %acc = call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %base, i32 63, i32 1, i32 63, i32 %x, i32 0, i32 16, i32 32, i32 2, i32 1, i1 false, i1 false, i32 0)
  %e0 = extractelement <4 x i16> %acc, i32 0
  %e1 = extractelement <4 x i16> %acc, i32 1
  %e2 = extractelement <4 x i16> %acc, i32 2
  %e3 = extractelement <4 x i16> %acc, i32 3
  %d0 = call i16 @llvm.genx.GenISA.sub.group.dpas.i16.i16.i16.v8i32(i16 %e0, i16 %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 1, i1 false)
  %d1 = call i16 @llvm.genx.GenISA.sub.group.dpas.i16.i16.i16.v8i32(i16 %e1, i16 %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 1, i1 false)
  %d2 = call i16 @llvm.genx.GenISA.sub.group.dpas.i16.i16.i16.v8i32(i16 %e2, i16 %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 1, i1 false)
  %d3 = call i16 @llvm.genx.GenISA.sub.group.dpas.i16.i16.i16.v8i32(i16 %e3, i16 %a, <8 x i32> %b, i32 11, i32 11, i32 8, i32 1, i1 false)
  %v0 = insertelement <4 x i16> poison, i16 %d0, i64 0
  %v1 = insertelement <4 x i16> %v0, i16 %d1, i64 1
  %v2 = insertelement <4 x i16> %v1, i16 %d2, i64 2
  %v3 = insertelement <4 x i16> %v2, i16 %d3, i64 3
  call void @llvm.genx.GenISA.LSC2DBlockWrite.v4i16(i64 %base, i32 63, i32 1, i32 63, i32 %x, i32 0, i32 16, i32 32, i32 2, i32 1, i1 false, i1 false, i32 0, <4 x i16> %v3)
  ret void
}

declare i16 @llvm.genx.GenISA.LSC2DBlockRead.i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare void @llvm.genx.GenISA.LSC2DBlockWrite.v4i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32, <4 x i16>)
declare i16 @llvm.genx.GenISA.sub.group.dpas.i16.i16.i16.v8i32(i16, i16, <8 x i32>, i32, i32, i32, i32, i1)

!igc.functions = !{!0}
!0 = !{ptr @test, !1}
!1 = !{!{!"function_type", i32 0}}

!IGCMetadata = !{!2}
!2 = !{!"ModuleMD", !3}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", ptr @test}
!5 = !{!"FuncMDValue[0]", !6, !11, !14}
!6 = !{!"resAllocMD", !7}
!7 = !{!"argAllocMDList", !8, !8}
!8 = !{!"argAllocMDListVec[0]", !9, !10, !10}
!9 = !{!"type", i32 0}
!10 = !{!"extensionType", i32 -1}
!11 = !{!"implicitArgInfoList", !12, !13}
!12 = !{!"implicitArgInfoListVec[0]", !{!"argId", i32 0}}
!13 = !{!"implicitArgInfoListVec[1]", !{!"argId", i32 1}}
!14 = !{!"requiredSubGroupSize", i32 16}
