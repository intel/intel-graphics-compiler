;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys

; Extract from a non-uniform vector at a non-zero index, used as a phi incoming
; value. The phi is the DeSSA root of the extract, so both get an alias to the
; vector. The alias offset must be the same for both.

; Aliasing of isolated values only
; RUN: igc_opt --opaque-pointers --CheckInstrTypes --igc-update-instrtypes-on-run --neo \
; RUN:         -platformbmg -igc-emit-visa -simd-mode 16 %s \
; RUN:         -regkey DumpVISAASMToConsole,AddVISADumpDeclarationsToEnd,VectorAlias=1 \
; RUN: | FileCheck %s --check-prefix=CHECK-VRA1

; Aliasing of isolated and non-isolated values
; RUN: igc_opt --opaque-pointers --CheckInstrTypes --igc-update-instrtypes-on-run --neo \
; RUN:         -platformbmg -igc-emit-visa -simd-mode 16 %s \
; RUN:         -regkey DumpVISAASMToConsole,AddVISADumpDeclarationsToEnd,VectorAlias=9 \
; RUN: | FileCheck %s --check-prefix=CHECK-VRA9

; CHECK-VRA1-LABEL: .kernel "test"
; CHECK-VRA1:         lsc_load.ugm (M1, 16)  [[V:[A-Za-z0-9_]+]]:d32x4  flat[{{.+}}]:a64
; CHECK-VRA1:         (!P{{[0-9]+}}) goto (M1, 1) [[ELSE:_[A-Za-z0-9_]+]]
; CHECK-VRA1:       {{_[A-Za-z0-9_]+}}:
; CHECK-VRA1-NEXT:    mov (M1, 16) [[P:[A-Za-z0-9_]+]](0,0)<1> [[V]](1,0)<1;1,0>
; CHECK-VRA1-NEXT:    goto (M1, 1) [[JOIN:_[A-Za-z0-9_]+]]
; CHECK-VRA1:       [[ELSE]]:
; CHECK-VRA1-NEXT:    add (M1_NM, 1) [[X:[A-Za-z0-9_]+]](0,0)<1> {{.+}}
; CHECK-VRA1-NEXT:    mov (M1, 16) [[P]](0,0)<1> [[X]](0,0)<0;1,0>
; CHECK-VRA1:       [[JOIN]]:
; CHECK-VRA1-NEXT:    mul (M1, 16) [[Q:[A-Za-z0-9_]+]](0,0)<1> [[P]](0,0)<1;1,0> [[P]](0,0)<1;1,0>
; CHECK-VRA1:         // .decl [[V]] v_type=G type=d num_elts=64 align=wordx32{{$}}
; CHECK-VRA1:         // .decl [[P]] v_type=G type=d num_elts=16 align=wordx32{{$}}
; CHECK-VRA1:         // .decl [[Q]] v_type=G type=d num_elts=16 align=wordx32{{$}}

; CHECK-VRA9-LABEL: .kernel "test"
; CHECK-VRA9:         lsc_load.ugm (M1, 16)  [[V:[A-Za-z0-9_]+]]:d32x4  flat[{{.+}}]:a64
; CHECK-VRA9:         (!P{{[0-9]+}}) goto (M1, 1) [[ELSE:_[A-Za-z0-9_]+]]
; CHECK-VRA9:       {{_[A-Za-z0-9_]+}}:
; CHECK-VRA9-NEXT:    goto (M1, 1) [[JOIN:_[A-Za-z0-9_]+]]
; CHECK-VRA9:       [[ELSE]]:
; CHECK-VRA9-NEXT:    add (M1_NM, 1) [[X:[A-Za-z0-9_]+]](0,0)<1> {{.+}}
; CHECK-VRA9-NEXT:    mov (M1, 16) [[V]](1,0)<1> [[X]](0,0)<0;1,0>
; CHECK-VRA9:       [[JOIN]]:
; CHECK-VRA9-NEXT:    mul (M1, 16) [[Q:[A-Za-z0-9_]+]](0,0)<1> [[V]](1,0)<1;1,0> [[V]](1,0)<1;1,0>
; CHECK-VRA9:         // .decl [[V]] v_type=G type=d num_elts=64 align=wordx32{{$}}
; CHECK-VRA9:         // .decl [[Q]] v_type=G type=d num_elts=16 align=wordx32{{$}}

define spir_kernel void @test(ptr addrspace(1) %in, ptr addrspace(1) %out, i32 %n, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %id = zext i16 %localIdX to i64
  %in.addr = getelementptr <4 x i32>, ptr addrspace(1) %in, i64 %id
  %v = load <4 x i32>, ptr addrspace(1) %in.addr, align 16
  %cmp = icmp sgt i32 %n, 0
  br i1 %cmp, label %then, label %else

then:
  %e1 = extractelement <4 x i32> %v, i32 1
  br label %join

else:
  %x = add i32 %n, 7
  br label %join

join:
  %p = phi i32 [ %e1, %then ], [ %x, %else ]
  %q = mul i32 %p, %p
  %out.addr = getelementptr i32, ptr addrspace(1) %out, i64 %id
  store i32 %q, ptr addrspace(1) %out.addr, align 4
  ret void
}

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", ptr @test}
!6 = !{!"FuncMDValue[0]", !7, !12, !16}
!7 = !{!"resAllocMD", !8}
!8 = !{!"argAllocMDList", !9, !9, !9, !9, !9, !9}
!9 = !{!"argAllocMDListVec[0]", !10, !11}
!10 = !{!"type", i32 0}
!11 = !{!"extensionType", i32 -1}
!12 = !{!"implicitArgInfoList", !13, !14, !15}
!13 = !{!"implicitArgInfoListVec[0]", !{!"argId", i32 0}}
!14 = !{!"implicitArgInfoListVec[1]", !{!"argId", i32 1}}
!15 = !{!"implicitArgInfoListVec[2]", !{!"argId", i32 8}}
!16 = !{!"requiredSubGroupSize", i32 16}
