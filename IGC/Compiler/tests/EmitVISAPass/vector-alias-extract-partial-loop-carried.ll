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
; RUN: | FileCheck %s --check-prefix=CHECK
; RUN: igc_opt %s --opaque-pointers -platformpvc -igc-emit-visa -simd-mode 16 \
; RUN:         -regkey DumpVISAASMToConsole,AddVISADumpDeclarationsToEnd=1,VectorAlias=9 \
; RUN: | FileCheck %s --check-prefix=CHECK

; Rows 0, 1, 3 of dpas result %a (in "then") and all rows of dpas result %b
; (in the loop) feed the loop-carried phis %h0..%h3. Row 2 is a constant on
; both paths into the loop, so it has no extract from %a.
;
; VectorAlias=1: no extract aliasing. Each phi gets its own variable and the
; extracts are movs.
;
; VectorAlias=9 gives the same code. Every extract (%a0, %a1, %a3, %b0..%b3)
; has a loop-header phi in its DeSSA group, so VRA does not alias it. If rows
; 0, 1, 3 of %h were aliased to %a, the whole 4-row %a would be live around the
; loop to keep 3 rows. If row 2 were aliased to %b, the whole %b would be live
; around the loop to keep one row.

; FIXME: Improve VRA: aliasing all rows of %h to %b (no movs, nothing wasted)
; would be better. It needs VRA to decide for all extracts of %b together, or
; to use Register Pressure Analysis.

; CHECK-LABEL: .kernel "test"
; CHECK:       mov (M1, 16) [[M0:[A-Za-z0-9_]+]](0,0)<1> 0x0:f
; CHECK:       mov (M1, 16) [[M1:[A-Za-z0-9_]+]](0,0)<1> 0x0:f
; CHECK:       mov (M1_NM, 1) [[M2:[A-Za-z0-9_]+]](0,0)<1> 0x0:f
; CHECK:       mov (M1, 16) [[M3:[A-Za-z0-9_]+]](0,0)<1> 0x0:f
; CHECK:       (!P{{[0-9]+}}) goto (M1, 1) [[MERGE:[A-Za-z0-9_]+]]
; CHECK:       {{^[A-Za-z0-9_]+}}:
; CHECK:       dpas.{{.*}} (M1, 16) [[A:[A-Za-z0-9_]+]].0
; CHECK:       mov (M1, 16) [[M0]](0,0)<1> [[A]](0,0)<1;1,0>
; CHECK:       mov (M1, 16) [[M1]](0,0)<1> [[A]](1,0)<1;1,0>
; CHECK:       mov (M1, 16) [[M3]](0,0)<1> [[A]](3,0)<1;1,0>
; CHECK:       mov (M1_NM, 1) [[M2]](0,0)<1> 0x7fc00000:f
; CHECK:       {{^}}[[MERGE]]:
; CHECK:       mov (M1, 16) [[H2:[A-Za-z0-9_]+]](0,0)<1> [[M2]](0,0)<0;1,0>
; CHECK:       {{^}}[[LOOP:[A-Za-z0-9_]+]]:
; CHECK:       mul (M1, 16) {{.*}} [[M0]](0,0)<1;1,0> [[M1]](0,0)<1;1,0>
; CHECK:       mul (M1, 16) {{.*}} [[H2]](0,0)<1;1,0> [[M3]](0,0)<1;1,0>
; CHECK:       dpas.{{.*}} (M1, 16) [[B:[A-Za-z0-9_]+]].0
; CHECK:       mov (M1, 16) [[M0]](0,0)<1> [[B]](0,0)<1;1,0>
; CHECK:       mov (M1, 16) [[M1]](0,0)<1> [[B]](1,0)<1;1,0>
; CHECK:       mov (M1, 16) [[H2]](0,0)<1> [[B]](2,0)<1;1,0>
; CHECK:       mov (M1, 16) [[M3]](0,0)<1> [[B]](3,0)<1;1,0>
; CHECK:       (P{{[0-9]+}}) goto (M1, 1) [[LOOP]]
; CHECK:       {{^[A-Za-z0-9_]+}}:
; CHECK-DAG:   .decl [[A]] v_type=G type=f num_elts=64 align=wordx32{{$}}
; CHECK-DAG:   .decl [[B]] v_type=G type=f num_elts=64 align=wordx32{{$}}
; CHECK-DAG:   .decl [[M0]] v_type=G type=f num_elts=16 align=wordx32{{$}}
; CHECK-DAG:   .decl [[H2]] v_type=G type=f num_elts=16 align=wordx32{{$}}

define spir_kernel void @test(i64 %base, i32 %x, i32 %n, ptr addrspace(1) %out, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %id = zext i16 %localIdX to i64
  %out.addr = getelementptr float, ptr addrspace(1) %out, i64 %id
  %A = call <4 x i16> @llvm.genx.GenISA.LSC2DBlockRead.v4i16(i64 %base, i32 63, i32 0, i32 63, i32 %x, i32 0, i32 16, i32 16, i32 4, i32 1, i1 false, i1 false, i32 0)
  %B = call <8 x i32> @llvm.genx.GenISA.LSC2DBlockRead.v8i32(i64 %base, i32 63, i32 15, i32 63, i32 %x, i32 0, i32 16, i32 16, i32 16, i32 1, i1 false, i1 true, i32 0)
  %C = call <4 x float> @llvm.genx.GenISA.LSC2DBlockRead.v4f32(i64 %base, i32 63, i32 1, i32 63, i32 %x, i32 0, i32 32, i32 16, i32 4, i32 1, i1 false, i1 false, i32 0)
  %cmp = icmp sgt i32 %n, 0
  br i1 %cmp, label %then, label %merge

then:
  %a = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v8i32(<4 x float> %C, <4 x i16> %A, <8 x i32> %B, i32 12, i32 12, i32 8, i32 4, i1 false)
  %a0 = extractelement <4 x float> %a, i32 0
  %a1 = extractelement <4 x float> %a, i32 1
  %a3 = extractelement <4 x float> %a, i32 3
  br label %merge

merge:
  %m0 = phi float [ 0.0, %entry ], [ %a0, %then ]
  %m1 = phi float [ 0.0, %entry ], [ %a1, %then ]
  %m2 = phi float [ 0.0, %entry ], [ 0x7FF8000000000000, %then ]
  %m3 = phi float [ 0.0, %entry ], [ %a3, %then ]
  br label %loop

loop:
  %i = phi i32 [ 0, %merge ], [ %i.next, %loop ]
  %h0 = phi float [ %m0, %merge ], [ %b0, %loop ]
  %h1 = phi float [ %m1, %merge ], [ %b1, %loop ]
  %h2 = phi float [ %m2, %merge ], [ %b2, %loop ]
  %h3 = phi float [ %m3, %merge ], [ %b3, %loop ]
  %s01 = fmul float %h0, %h1
  %s23 = fmul float %h2, %h3
  %s = fmul float %s01, %s23
  store float %s, ptr addrspace(1) %out.addr, align 4
; Vector %b is created inside the loop.
  %b = call <4 x float> @llvm.genx.GenISA.sub.group.dpas.v4f32.v4f32.v4i16.v8i32(<4 x float> %C, <4 x i16> %A, <8 x i32> %B, i32 12, i32 12, i32 8, i32 4, i1 false)
; No extract is aliased. %a0, %a1, %a3 and %b0..%b3 are all rejected by the
; loop-header check: the DCC of each one contains a loop-header phi, e.g.
; DCC { %a0, %m0, %h0, %b0 } and DCC { %h2, %b2 }.
  %b0 = extractelement <4 x float> %b, i32 0
  %b1 = extractelement <4 x float> %b, i32 1
  %b2 = extractelement <4 x float> %b, i32 2
  %b3 = extractelement <4 x float> %b, i32 3
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
