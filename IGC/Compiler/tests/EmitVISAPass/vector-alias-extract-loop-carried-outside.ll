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
; RUN: | FileCheck %s --check-prefixes=CHECK,VA1
; RUN: igc_opt %s --opaque-pointers -platformpvc -igc-emit-visa -simd-mode 16 \
; RUN:         -regkey DumpVISAASMToConsole,AddVISADumpDeclarationsToEnd=1,VectorAlias=9 \
; RUN: | FileCheck %s --check-prefixes=CHECK,VA9

; A <2 x float> load %v is done before the loop. Its extract %v1 is the
; loop-entry value of the loop-carried phi %h. %v is not used in the loop.
;
; If %v1 were aliased to element 1 of %v (VectorAlias=9), %h would live in %v,
; so the whole %v would be live around the loop to keep one element. VRA skips
; this alias: %v1's DeSSA group has a loop-header phi (%h); %h has its own
; variable, set by a mov before the loop.
;
; In @test_if, %v1 feeds a phi that is not in a loop header (an if/else join),
; so VectorAlias=9 still puts %h in element 1 of %v (no mov).

; CHECK-LABEL: .kernel "test"
; CHECK:       svm_gather.4.2 (M1, 16) {{.*}} [[V:[A-Za-z0-9_]+]].0
; CHECK:       mov (M1, 16) [[H:[A-Za-z0-9_]+]](0,0)<1> [[V]](1,0)<1;1,0>
; CHECK:       {{^}}[[LOOP:[A-Za-z0-9_]+]]:
; CHECK:       mul (M1, 16) [[H]](0,0)<1> [[H]](0,0)<1;1,0> [[H]](0,0)<1;1,0>
; CHECK:       (P{{[0-9]+}}) goto (M1, 1) [[LOOP]]
; CHECK-DAG:   .decl [[H]] v_type=G type=f num_elts=16 align=wordx32{{$}}
; CHECK-DAG:   .decl [[V]] v_type=G type=f num_elts=32 align=wordx32{{$}}

; CHECK-LABEL: .kernel "test_if"
; CHECK:       svm_gather.4.2 (M1, 16) {{.*}} [[V:[A-Za-z0-9_]+]].0
; VA1:         mov (M1, 16) [[H:[A-Za-z0-9_]+]](0,0)<1> [[V]](1,0)<1;1,0>
; VA1:         mul (M1, 16) [[H]](0,0)<1> [[H]](0,0)<1;1,0> [[H]](0,0)<1;1,0>
; VA9-NOT:     mov (M1, 16) {{.*}} [[V]](1,0)
; VA9:         mul (M1, 16) [[V]](1,0)<1> [[V]](1,0)<1;1,0> [[V]](1,0)<1;1,0>

define spir_kernel void @test(ptr addrspace(1) %in, ptr addrspace(1) %out, i32 %n, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %id = zext i16 %localIdX to i64
  %in.addr = getelementptr <2 x float>, ptr addrspace(1) %in, i64 %id
  %out.addr = getelementptr float, ptr addrspace(1) %out, i64 %id
  %v = load <2 x float>, ptr addrspace(1) %in.addr, align 8
  %v0 = extractelement <2 x float> %v, i32 0
  %v1 = extractelement <2 x float> %v, i32 1
  store float %v0, ptr addrspace(1) %out.addr, align 4
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %h = phi float [ %v1, %entry ], [ %h.next, %loop ]
  %h.next = fmul float %h, %h
  %i.next = add i32 %i, 1
  %c = icmp slt i32 %i.next, %n
  br i1 %c, label %loop, label %exit

exit:
  store float %h.next, ptr addrspace(1) %out.addr, align 4
  ret void
}

define spir_kernel void @test_if(ptr addrspace(1) %in, ptr addrspace(1) %out, i32 %n, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %id = zext i16 %localIdX to i64
  %in.addr = getelementptr <2 x float>, ptr addrspace(1) %in, i64 %id
  %out.addr = getelementptr float, ptr addrspace(1) %out, i64 %id
  %v = load <2 x float>, ptr addrspace(1) %in.addr, align 8
  %v1 = extractelement <2 x float> %v, i32 1
  %c = icmp sgt i32 %n, 0
  br i1 %c, label %then, label %exit

then:
  %sq = fmul float %v1, %v1
  br label %exit

exit:
  %h = phi float [ %v1, %entry ], [ %sq, %then ]
  store float %h, ptr addrspace(1) %out.addr, align 4
  ret void
}

!igc.functions = !{!0, !16}
!0 = !{ptr @test, !1}
!1 = !{!{!"function_type", i32 0}}
!16 = !{ptr @test_if, !1}

!IGCMetadata = !{!2}
!2 = !{!"ModuleMD", !3}
!3 = !{!"FuncMD", !4, !5, !17, !18}
!4 = !{!"FuncMDMap[0]", ptr @test}
!5 = !{!"FuncMDValue[0]", !6, !11, !15}
!6 = !{!"resAllocMD", !7}
!7 = !{!"argAllocMDList", !8, !8, !8, !8, !8, !8}
!8 = !{!"argAllocMDListVec[0]", !9, !10, !10}
!9 = !{!"type", i32 0}
!10 = !{!"extensionType", i32 -1}
!11 = !{!"implicitArgInfoList", !12, !13, !14}
!12 = !{!"implicitArgInfoListVec[0]", !{!"argId", i32 0}}
!13 = !{!"implicitArgInfoListVec[1]", !{!"argId", i32 1}}
!14 = !{!"implicitArgInfoListVec[2]", !{!"argId", i32 8}}
!15 = !{!"requiredSubGroupSize", i32 16}
!17 = !{!"FuncMDMap[1]", ptr @test_if}
!18 = !{!"FuncMDValue[1]", !6, !11, !15}
