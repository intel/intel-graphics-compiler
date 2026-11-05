;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -platformbmg -igc-emit-visa %s -regkey DumpVISAASMToConsole | FileCheck %s

; A struct built by a phi (or taken from an argument or a call) has all its
; members defined. Struct select and insertvalue must copy every member from it.

; CHECK-LABEL: .kernel "struct_select_test"

; CHECK:      lifetime.start
; CHECK-NEXT: ([[F:P[0-9]+]]) mov (M1, {{[0-9]+}}) [[SEL:V[0-9]+]](0,0)<1> [[PHI:V[0-9]+]](0,0)<1;1,0>
; CHECK-NEXT: ([[F]]) mov (M1, {{[0-9]+}}) [[SEL]](2,0)<1> [[PHI]](2,0)<1;1,0>
; CHECK-NEXT: (![[F]]) mov (M1, {{[0-9]+}}) [[SEL]](0,0)<1> [[PAIR:V[0-9]+]](0,0)<1;1,0>
; CHECK-NEXT: (![[F]]) mov (M1, {{[0-9]+}}) [[SEL]](2,0)<1> [[PAIR]](2,0)<1;1,0>

; CHECK:      lifetime.start
; CHECK-NEXT: mov (M1, {{[0-9]+}}) [[UPD:V[0-9]+]](0,0)<1> [[PHI]](0,0)<1;1,0>
; CHECK-NEXT: mov (M1, {{[0-9]+}}) [[UPD]](2,0)<1> {{V[0-9]+}}(0,0)<1;1,0>

define spir_kernel void @struct_select_test(i32 %b, ptr %out_ptr, <8 x i32> %r0) {
entry:
  %lid16 = call i16 @llvm.genx.GenISA.simdLaneId()
  %lid = zext i16 %lid16 to i32
  %f = uitofp i32 %lid to float
  %flag = icmp ult i32 %lid, %b
  %c = icmp eq i32 %lid, 3
  %p0 = insertvalue { float, float } poison, float %f, 0
  %p1 = insertvalue { float, float } %p0, float 2.0, 1
  br i1 %c, label %then, label %join
then:
  %q0 = insertvalue { float, float } poison, float 5.0, 0
  %q1 = insertvalue { float, float } %q0, float %f, 1
  br label %join
join:
  %phi = phi { float, float } [ %p1, %entry ], [ %q1, %then ]
  %part = insertvalue { float, float } poison, float %f, 0
  %pair = insertvalue { float, float } %part, float 0.0, 1
  %sel = select i1 %flag, { float, float } %phi, { float, float } %pair
  %a = extractvalue { float, float } %sel, 0
  %d = extractvalue { float, float } %sel, 1
  %s = fadd float %a, %d
  %upd = insertvalue { float, float } %phi, float %s, 1
  %e = extractvalue { float, float } %upd, 0
  %g = extractvalue { float, float } %upd, 1
  %h = fmul float %e, %g
  %k = fadd float %h, %a
  store float %k, ptr %out_ptr, align 4
  ret void
}

declare i16 @llvm.genx.GenISA.simdLaneId()

!IGCMetadata = !{!0}
!igc.functions = !{!14}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", ptr @struct_select_test}
!3 = !{!"FuncMDValue[0]", !4, !27}
!4 = !{!"resAllocMD", !5}
!5 = distinct !{!"argAllocMDList", !6, !10, !11, !5, !6}
!6 = !{!"argAllocMDListVec[0]", !7, !8, !9}
!7 = !{!"type", i32 0}
!8 = !{!"extensionType", i32 -1}
!9 = !{!"indexType", i32 -1}
!10 = !{!"argAllocMDListVec[1]", !7, !8, !9}
!11 = !{!"argAllocMDListVec[2]", !12, !8, !13}
!12 = !{!"type", i32 1}
!13 = !{!"indexType", i32 0}
!14 = !{ptr @struct_select_test, !15}
!15 = !{!16}
!16 = !{!"function_type", i32 0}
!21 = !{!"argId", i32 0}
!22 = !{!"implicitArgInfoListVec[0]", !21}
!23 = !{!"argId", i32 2}
!24 = !{!"implicitArgInfoListVec[1]", !23}
!25 = !{!"argId", i32 13}
!26 = !{!"implicitArgInfoListVec[2]", !25}
!27 = !{!"implicitArgInfoList", !22, !24, !26}
