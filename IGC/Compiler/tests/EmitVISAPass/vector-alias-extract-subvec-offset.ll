;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt %s --opaque-pointers -platformCri -igc-emit-visa -simd-mode 32 \
; RUN:         -regkey DumpVISAASMToConsole,AddVISADumpDeclarationsToEnd=1,VectorAlias=1 \
; RUN: | FileCheck %s --check-prefixes=CHECK,VA1
; RUN: igc_opt %s --opaque-pointers -platformCri -igc-emit-visa -simd-mode 32 \
; RUN:         -regkey DumpVISAASMToConsole,AddVISADumpDeclarationsToEnd=1,VectorAlias=9 \
; RUN: | FileCheck %s --check-prefixes=CHECK,VA9

; An extract from a sub-vector that is itself aliased into a bigger vector,
; at a non-zero offset:
;   %s = {%w[2], %w[3]}       ; %s is aliased to %w at offset 2
;   %e = extractelement %s, 0 ; %e is %s[0], which is %w[2]
;
; Another extract of the same element is in the DeSSA congruent class (DCC)
; of a join phi:
;   %a = extractelement %w, 2 in %then: it is in the DCC {%a, %x, %r}.
;   %e in %else: it is single-valued, and it is used after %x is defined.
;
; %then comes first, so with VectorAlias=9 {%a, %x, %r} is aliased to %w[2]
; first. When %e is visited, VRA looks for other values in the registers %e
; would use. %e's index 0 is relative to %s, so it maps to %w[2]. At %w[2]
; it finds {%a, %x, %r}: %x would overwrite %e before "store %e". So %e
; must get its own variable (a mov), and "store %e" must store %e, not %x.
;
; With VectorAlias=1 extracts are not aliased (only %s is): %a and %e are both
; movs.

; CHECK-LABEL: .kernel "test_offset"
; CHECK:       lsc_load.ugm (M1, 32) [[W:[A-Za-z0-9_]+]]:d32x4
; CHECK:       (P{{[0-9]+}}) goto (M1, 1) [[ELSE:[A-Za-z0-9_]+]]
; CHECK:       {{^}}[[THEN:[A-Za-z0-9_]+]]:
; VA1-NEXT:    mov (M1, 32) [[A:[A-Za-z0-9_]+]](0,0)<1> [[W]](4,0)<1;1,0>
; CHECK-NEXT:  lsc_store.ugm (M1, 32) {{.*}} [[W]]:d32x4
; CHECK-NEXT:  goto (M1, 1) [[EXIT:[A-Za-z0-9_]+]]
; CHECK:       {{^}}[[ELSE]]:
; CHECK-NEXT:  mov (M1, 32) [[E:[A-Za-z0-9_]+]](0,0)<1> [[W]](4,0)<1;1,0>
; VA1-NEXT:    mul (M1, 32) [[A]](0,0)<1> [[E]](0,0)<1;1,0> 0x3:w
; VA9-NEXT:    mul (M1, 32) [[W]](4,0)<1> [[E]](0,0)<1;1,0> 0x3:w
; CHECK-NEXT:  lsc_store.ugm (M1, 32) {{.*}} [[E]]:d32{{ }}
; CHECK:       {{^}}[[EXIT]]:
; VA1-NEXT:    lsc_store.ugm (M1, 32) {{.*}} [[A]]:d32{{ }}
; VA9-NEXT:    lsc_store.ugm (M1, 32) {{.*}} [[W]].256:d32{{ }}
; CHECK-DAG:   // .decl [[W]] v_type=G type=d num_elts=128 align=wordx32{{$}}
; CHECK-DAG:   // .decl [[E]] v_type=G type=d num_elts=32 align=wordx32{{$}}
; VA1-DAG:     // .decl [[A]] v_type=G type=d num_elts=32 align=wordx32{{$}}

define spir_kernel void @test_offset(ptr addrspace(1) %in, ptr addrspace(1) %out, ptr addrspace(1) %o2, ptr addrspace(1) %o3, i32 %c, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %id = zext i16 %localIdX to i64
  %in.addr = getelementptr <4 x i32>, ptr addrspace(1) %in, i64 %id
  %out.addr = getelementptr i32, ptr addrspace(1) %out, i64 %id
  %o2.addr = getelementptr <4 x i32>, ptr addrspace(1) %o2, i64 %id
  %o3.addr = getelementptr i32, ptr addrspace(1) %o3, i64 %id
  %w = load <4 x i32>, ptr addrspace(1) %in.addr, align 16
  %w2 = extractelement <4 x i32> %w, i32 2
  %w3 = extractelement <4 x i32> %w, i32 3
  %s0 = insertelement <2 x i32> undef, i32 %w2, i32 0
  %s = insertelement <2 x i32> %s0, i32 %w3, i32 1
  %cc = icmp eq i32 %c, 0
  br i1 %cc, label %else, label %then

then:
  %a = extractelement <4 x i32> %w, i32 2
  store <4 x i32> %w, ptr addrspace(1) %o2.addr, align 4
  br label %exit

else:
  %e = extractelement <2 x i32> %s, i32 0
  %x = mul i32 %e, 3
  store i32 %e, ptr addrspace(1) %o3.addr, align 4
  br label %exit

exit:
  %r = phi i32 [ %a, %then ], [ %x, %else ]
  store i32 %r, ptr addrspace(1) %out.addr, align 4
  ret void
}

!igc.functions = !{!0}
!0 = !{ptr @test_offset, !1}
!1 = !{!{!"function_type", i32 0}}

!IGCMetadata = !{!2}
!2 = !{!"ModuleMD", !3}
!3 = !{!"FuncMD", !4, !5}
!4 = !{!"FuncMDMap[0]", ptr @test_offset}
!5 = !{!"FuncMDValue[0]", !6, !11, !15}
!6 = !{!"resAllocMD", !7}
!7 = !{!"argAllocMDList", !8, !8, !8, !8, !8, !8, !8, !8}
!8 = !{!"argAllocMDListVec[0]", !9, !10, !10}
!9 = !{!"type", i32 0}
!10 = !{!"extensionType", i32 -1}
!11 = !{!"implicitArgInfoList", !12, !13, !14}
!12 = !{!"implicitArgInfoListVec[0]", !{!"argId", i32 0}}
!13 = !{!"implicitArgInfoListVec[1]", !{!"argId", i32 1}}
!14 = !{!"implicitArgInfoListVec[2]", !{!"argId", i32 8}}
!15 = !{!"requiredSubGroupSize", i32 32}
