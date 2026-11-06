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

; Two extracts of the same element %v[0]:
;   %a in %then: it is in the DeSSA group {%a, %x, %r} of the join phi %r.
;   %e in %else: it is single-valued, and it is used after %x is defined.
; With VectorAlias=9 both may be aliased to %v[0]. Then %x, which shares a
; variable with %a, would also live in %v[0] and overwrite %e before
; "store %e". VRA must check for this, so only one of {%a, %x, %r} and %e
; is aliased to %v[0], and "store %e" stores %e, not %x.

; In @test, %then comes first, so {%a, %x, %r} is aliased to %v[0] first.
; Then %e interferes with %x and gets its own variable (mov).
; With VectorAlias=1 nothing is aliased: %a and %e are both movs.
; CHECK-LABEL: .kernel "test"
; CHECK:       lsc_load.ugm (M1, 32) [[V:[A-Za-z0-9_]+]]:d32x2
; CHECK:       (P{{[0-9]+}}) goto (M1, 1) [[ELSE:[A-Za-z0-9_]+]]
; CHECK:       {{^}}[[THEN:[A-Za-z0-9_]+]]:
; VA1-NEXT:    mov (M1, 32) [[A:[A-Za-z0-9_]+]](0,0)<1> [[V]](0,0)<1;1,0>
; CHECK-NEXT:  lsc_store.ugm (M1, 32) {{.*}} [[V]]:d32x2
; CHECK-NEXT:  goto (M1, 1) [[EXIT:[A-Za-z0-9_]+]]
; CHECK:       {{^}}[[ELSE]]:
; CHECK-NEXT:  mov (M1, 32) [[E:[A-Za-z0-9_]+]](0,0)<1> [[V]](0,0)<1;1,0>
; VA1-NEXT:    mul (M1, 32) [[A]](0,0)<1> [[E]](0,0)<1;1,0> 0x3:w
; VA9-NEXT:    mul (M1, 32) [[V]](0,0)<1> [[E]](0,0)<1;1,0> 0x3:w
; CHECK-NEXT:  lsc_store.ugm (M1, 32) {{.*}} [[E]]:d32{{ }}
; CHECK:       {{^}}[[EXIT]]:
; VA1-NEXT:    lsc_store.ugm (M1, 32) {{.*}} [[A]]:d32{{ }}
; VA9-NEXT:    lsc_store.ugm (M1, 32) {{.*}} [[V]]:d32{{ }}
; CHECK-DAG:   // .decl [[V]] v_type=G type=d num_elts=64 align=wordx32{{$}}
; CHECK-DAG:   // .decl [[E]] v_type=G type=d num_elts=32 align=wordx32{{$}}
; VA1-DAG:     // .decl [[A]] v_type=G type=d num_elts=32 align=wordx32{{$}}

define spir_kernel void @test(ptr addrspace(1) %in, ptr addrspace(1) %out, ptr addrspace(1) %o2, ptr addrspace(1) %o3, i32 %c, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %id = zext i16 %localIdX to i64
  %in.addr = getelementptr <2 x i32>, ptr addrspace(1) %in, i64 %id
  %out.addr = getelementptr i32, ptr addrspace(1) %out, i64 %id
  %o2.addr = getelementptr <2 x i32>, ptr addrspace(1) %o2, i64 %id
  %o3.addr = getelementptr i32, ptr addrspace(1) %o3, i64 %id
  %v = load <2 x i32>, ptr addrspace(1) %in.addr, align 8
  %cc = icmp eq i32 %c, 0
  br i1 %cc, label %else, label %then

then:
  %a = extractelement <2 x i32> %v, i32 0
  store <2 x i32> %v, ptr addrspace(1) %o2.addr, align 4
  br label %exit

else:
  %e = extractelement <2 x i32> %v, i32 0
  %x = mul i32 %e, 3
  store i32 %e, ptr addrspace(1) %o3.addr, align 4
  br label %exit

exit:
  %r = phi i32 [ %a, %then ], [ %x, %else ]
  store i32 %r, ptr addrspace(1) %out.addr, align 4
  ret void
}

; In @test_rev, %else comes first, so %e is aliased to %v[0] first (no mov).
; Then {%a, %x, %r} interferes with %e and gets its own variable.
; With VectorAlias=1 nothing is aliased: %e and %a are both movs.
; CHECK-LABEL: .kernel "test_rev"
; CHECK:       lsc_load.ugm (M1, 32) [[V:[A-Za-z0-9_]+]]:d32x2
; CHECK:       (!P{{[0-9]+}}) goto (M1, 1) [[THEN:[A-Za-z0-9_]+]]
; CHECK:       {{^}}[[ELSE:[A-Za-z0-9_]+]]:
; VA1-NEXT:    mov (M1, 32) [[E:[A-Za-z0-9_]+]](0,0)<1> [[V]](0,0)<1;1,0>
; VA1-NEXT:    mul (M1, 32) [[X:[A-Za-z0-9_]+]](0,0)<1> [[E]](0,0)<1;1,0> 0x3:w
; VA1-NEXT:    lsc_store.ugm (M1, 32) {{.*}} [[E]]:d32{{ }}
; VA9-NEXT:    mul (M1, 32) [[X:[A-Za-z0-9_]+]](0,0)<1> [[V]](0,0)<1;1,0> 0x3:w
; VA9-NEXT:    lsc_store.ugm (M1, 32) {{.*}} [[V]]:d32{{ }}
; CHECK-NEXT:  goto (M1, 1) [[EXIT:[A-Za-z0-9_]+]]
; CHECK:       {{^}}[[THEN]]:
; CHECK-NEXT:  mov (M1, 32) [[X]](0,0)<1> [[V]](0,0)<1;1,0>
; CHECK-NEXT:  lsc_store.ugm (M1, 32) {{.*}} [[V]]:d32x2
; CHECK:       {{^}}[[EXIT]]:
; CHECK-NEXT:  lsc_store.ugm (M1, 32) {{.*}} [[X]]:d32{{ }}
; CHECK-DAG:   // .decl [[V]] v_type=G type=d num_elts=64 align=wordx32{{$}}
; CHECK-DAG:   // .decl [[X]] v_type=G type=d num_elts=32 align=wordx32{{$}}
; VA1-DAG:     // .decl [[E]] v_type=G type=d num_elts=32 align=wordx32{{$}}

define spir_kernel void @test_rev(ptr addrspace(1) %in, ptr addrspace(1) %out, ptr addrspace(1) %o2, ptr addrspace(1) %o3, i32 %c, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %id = zext i16 %localIdX to i64
  %in.addr = getelementptr <2 x i32>, ptr addrspace(1) %in, i64 %id
  %out.addr = getelementptr i32, ptr addrspace(1) %out, i64 %id
  %o2.addr = getelementptr <2 x i32>, ptr addrspace(1) %o2, i64 %id
  %o3.addr = getelementptr i32, ptr addrspace(1) %o3, i64 %id
  %v = load <2 x i32>, ptr addrspace(1) %in.addr, align 8
  %cc = icmp eq i32 %c, 0
  br i1 %cc, label %else, label %then

else:
  %e = extractelement <2 x i32> %v, i32 0
  %x = mul i32 %e, 3
  store i32 %e, ptr addrspace(1) %o3.addr, align 4
  br label %exit

then:
  %a = extractelement <2 x i32> %v, i32 0
  store <2 x i32> %v, ptr addrspace(1) %o2.addr, align 4
  br label %exit

exit:
  %r = phi i32 [ %a, %then ], [ %x, %else ]
  store i32 %r, ptr addrspace(1) %out.addr, align 4
  ret void
}

!igc.functions = !{!0, !16}
!0 = !{ptr @test, !1}
!1 = !{!{!"function_type", i32 0}}
!16 = !{ptr @test_rev, !1}

!IGCMetadata = !{!2}
!2 = !{!"ModuleMD", !3}
!3 = !{!"FuncMD", !4, !5, !17, !18}
!4 = !{!"FuncMDMap[0]", ptr @test}
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
!17 = !{!"FuncMDMap[1]", ptr @test_rev}
!18 = !{!"FuncMDValue[1]", !6, !11, !15}
