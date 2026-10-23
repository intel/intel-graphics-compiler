;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt %s --opaque-pointers -platformpvc -igc-emit-visa -simd-mode 16 \
; RUN:         -regkey DumpVISAASMToConsole,AddVISADumpDeclarationsToEnd=1,VectorAlias=1 \
; RUN: | FileCheck %s
; RUN: igc_opt %s --opaque-pointers -platformpvc -igc-emit-visa -simd-mode 16 \
; RUN:         -regkey DumpVISAASMToConsole,AddVISADumpDeclarationsToEnd=1,VectorAlias=9 \
; RUN: | FileCheck %s

; A dpas result %b is written at the bottom of the loop. One extract %b2
; feeds the loop-carried phi %h, which is read at the top of the loop. The
; phi's value on loop entry is a constant (no extract).
;
; If %b2 were aliased to row 2 of %b (VectorAlias=9), %h would live in row 2
; of %b, so the whole 4-row %b would be live around the loop to keep one row.
; VRA skips this alias: %b2's DeSSA group has a loop-header phi (%h). So
; VectorAlias=9 gives the same code as VectorAlias=1: %h has its own 1-GRF
; variable, and %b is live only from the dpas to the extract mov.

; CHECK-LABEL: .kernel "test"
; CHECK:       mov (M1, 16) [[H:[A-Za-z0-9_]+]](0,0)<1> 0x0:f
; CHECK:       {{^}}[[LOOP:[A-Za-z0-9_]+]]:
; CHECK:       mul (M1, 16) {{.*}} [[H]](0,0)<1;1,0> [[H]](0,0)<1;1,0>
; CHECK:       dpas.{{.*}} (M1, 16) [[B:[A-Za-z0-9_]+]].0
; CHECK:       mov (M1, 16) [[H]](0,0)<1> [[B]](2,0)<1;1,0>
; CHECK:       (P{{[0-9]+}}) goto (M1, 1) [[LOOP]]
; CHECK-DAG:   .decl [[H]] v_type=G type=f num_elts=16 align=wordx32{{$}}
; CHECK-DAG:   .decl [[B]] v_type=G type=f num_elts=64 align=wordx32{{$}}

define spir_kernel void @test(i64 %base, i32 %x, i32 %n, ptr addrspace(1) %out, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %id = zext i16 %localIdX to i64
  %out.addr = getelementptr float, ptr addrspace(1) %out, i64 %id
  %A = call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %base, i32 63, i32 0, i32 63, i32 %x, i32 0, i32 16, i32 16, i32 4, i32 1, i1 false, i1 false, i32 0)
  %B = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 0, i32 16, i32 16, i32 16, i32 1, i1 false, i1 true, i32 0)
  %C = call <4 x float> @llvm.genx.GenISA.LSC2DBlockRead.v4f32(i64 %base, i32 63, i32 1, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 4, i32 1, i1 false, i1 false, i32 0)
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %h = phi float [ 0.0, %entry ], [ %b2, %loop ]
  %s = fmul float %h, %h
  store float %s, ptr addrspace(1) %out.addr, align 4
  %b = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v8i32(<4 x float> %C, <4 x i16> %A, <8 x i32> %B, i32 12, i32 12, i32 8, i32 4, i1 false)
  %b2 = extractelement <4 x float> %b, i32 2
  %i.next = add i32 %i, 1
  %c = icmp slt i32 %i.next, %n
  br i1 %c, label %loop, label %exit

exit:
  ret void
}

declare <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <4 x float> @llvm.genx.GenISA.LSC2DBlockRead.v4f32(i64, i32, i32, i32, i32, i32, i32, i32, i32, i32, i1, i1, i32)
declare <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v8i32(<4 x float>, <4 x i16>, <8 x i32>, i32, i32, i32, i32, i1)

!igc.functions = !{!0}
!0 = !{ptr @test, !1}
!1 = !{!{!"function_type", i32 0}}

!IGCMetadata = !{!2}
!2 = !{!"ModuleMD", !3}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", ptr @test}
!5 = !{!"FuncMDValue[0]", !6, !11, !15}
!6 = !{!"resAllocMD", !7}
!7 = !{!"argAllocMDList", !8, !8, !8, !8, !8, !8, !8}
!8 = !{!"argAllocMDListVec[0]", !9, !10, !10}
!9 = !{!"type", i32 0}
!10 = !{!"extensionType", i32 -1}
!11 = !{!"implicitArgInfoList", !12, !13, !14}
!12 = !{!"implicitArgInfoListVec[0]", !{!"argId", i32 0}}
!13 = !{!"implicitArgInfoListVec[1]", !{!"argId", i32 1}}
!14 = !{!"implicitArgInfoListVec[2]", !{!"argId", i32 8}}
!15 = !{!"requiredSubGroupSize", i32 16}
