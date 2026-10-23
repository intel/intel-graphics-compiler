;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt %s --opaque-pointers -S -dce -platformbmg -igc-emit-visa -simd-mode 16 \
; RUN:         -regkey DumpVISAASMToConsole,,AddVISADumpDeclarationsToEnd=1,VectorAlias=9 \
; RUN: | FileCheck %s

; EmitVISA can make each extract an alias of the bitcast independently,
; including extractMask: only the used elements are copied, packed together.
; VRA would alias whole vector, making it less optimal. Test if VRA skips
; aliasing and leaves it to EmitVISA.

; 8 of 16 elements are used: %relayout has 8 rows (128 words), not 256, and
; the bitcast is 8 movs.
; CHECK-LABEL: .kernel "bitcast_relayout_strided"
; CHECK:         mov (M1, 16) [[RELAYOUT:[A-Za-z0-9_]+]](0,0)<1> [[DPAS:[A-Za-z0-9_]+]](0,0)<2;1,0>
; CHECK:         mov (M1, 16) [[RELAYOUT]](0,16)<1> [[DPAS]](1,0)<2;1,0>
; CHECK:         mov (M1, 16) [[RELAYOUT]](1,0)<1> [[DPAS]](2,0)<2;1,0>
; CHECK:         mov (M1, 16) [[RELAYOUT]](1,16)<1> [[DPAS]](3,0)<2;1,0>
; CHECK:         mov (M1, 16) [[RELAYOUT]](2,0)<1> [[DPAS]](4,0)<2;1,0>
; CHECK:         mov (M1, 16) [[RELAYOUT]](2,16)<1> [[DPAS]](5,0)<2;1,0>
; CHECK:         mov (M1, 16) [[RELAYOUT]](3,0)<1> [[DPAS]](6,0)<2;1,0>
; CHECK:         mov (M1, 16) [[RELAYOUT]](3,16)<1> [[DPAS]](7,0)<2;1,0>
; CHECK-NOT:     [[RELAYOUT]](4,
; CHECK:         goto
; CHECK-DAG:     .decl [[RELAYOUT]] v_type=G type=w num_elts=128 align=wordx32
; CHECK-DAG:     .decl [[DPAS]] v_type=G type=w num_elts=256 align=wordx32 alias=<{{.*}}, 0>

define spir_kernel void @bitcast_relayout_strided() {
entry:
  br label %loop

loop:
  %vectorized_phi = phi <8 x i32> [ zeroinitializer, %entry ], [ %backedge, %loop ]
  %phi_f = bitcast <8 x i32> %vectorized_phi to <8 x float>
  %dpas = call <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float> %phi_f, <8 x i16> zeroinitializer, <8 x i32> zeroinitializer, i32 0, i32 0, i32 0, i32 0, i1 false)
  %dpas_i = bitcast <8 x float> %dpas to <8 x i32>
  %relayout = bitcast <8 x i32> %dpas_i to <16 x i16>
  %e0 = extractelement <16 x i16> %relayout, i64 0
  %e1 = extractelement <16 x i16> %relayout, i64 2
  %e2 = extractelement <16 x i16> %relayout, i64 4
  %e3 = extractelement <16 x i16> %relayout, i64 6
  %e4 = extractelement <16 x i16> %relayout, i64 8
  %e5 = extractelement <16 x i16> %relayout, i64 10
  %e6 = extractelement <16 x i16> %relayout, i64 12
  %e7 = extractelement <16 x i16> %relayout, i64 14
  %i0 = insertelement <8 x i16> undef, i16 %e0, i64 0
  %i1 = insertelement <8 x i16> %i0, i16 %e1, i64 1
  %i2 = insertelement <8 x i16> %i1, i16 %e2, i64 2
  %i3 = insertelement <8 x i16> %i2, i16 %e3, i64 3
  %i4 = insertelement <8 x i16> %i3, i16 %e4, i64 4
  %i5 = insertelement <8 x i16> %i4, i16 %e5, i64 5
  %i6 = insertelement <8 x i16> %i5, i16 %e6, i64 6
  %lo = insertelement <8 x i16> %i6, i16 %e7, i64 7
  %widened = zext <8 x i16> %lo to <8 x i32>
  %backedge = add <8 x i32> %widened, %vectorized_phi
  br label %loop
}

declare <8 x float> @llvm.genx.GenISA.sub.group.dpas.v8f32.v8f32.v8i16.v8i32(<8 x float>, <8 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

!igc.functions = !{!0}
!0 = !{ptr @bitcast_relayout_strided, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

!IGCMetadata = !{!10}
!10 = !{!"ModuleMD", !11}
!11 = !{!"FuncMD", !12, !13}
!12 = !{!"FuncMDMap[0]", ptr @bitcast_relayout_strided}
!13 = !{!"FuncMDValue[0]", !20}
!20 = !{!"requiredSubGroupSize", i32 16}
