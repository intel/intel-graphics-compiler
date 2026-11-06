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

; %w is %v cast to <4 x i32> without data movement, so DeSSA puts both in one
; variable. Two extracts read the same 8 bytes of it, with different element
; sizes:
;   %a = %v[1] (i64, elt_ix 1): it is in the DeSSA group {%a, %x, %r} of the
;        join phi %r.
;   %e = %w[2] (i32, elt_ix 2): it is in the registers of %v[1]. It is
;        single-valued, and it is used after %x is defined.
; With VectorAlias=9 both may be aliased to %v. Then %x, which shares a
; variable with %a, would also live in %v[1] and overwrite %e before
; "%e1 = add %e, 1". VRA must see that %a and %e overlap (by bytes, not by
; element index), so only one of {%a, %x, %r} and %e is aliased to %v.
; With VectorAlias=1 nothing is aliased: %a and %e are both movs.

; @test and @test_rev use uniform values, and %w is a bitcast (DeSSA aliases
; a vector bitcast with a larger source element only if it is uniform).

; In @test, %a comes first, so {%a, %x, %r} is aliased to %v[1] first.
; Then %e interferes with %x and gets its own variable (mov before %x is
; written).
; CHECK-LABEL: .kernel "test"
; CHECK:       lsc_load.ugm (M1_NM, 1) [[V:[A-Za-z0-9_]+]]:d64x2t
; VA1-NEXT:    mov (M1_NM, 1) [[A:[A-Za-z0-9_]+]](0,0)<1> [[V]](0,1)<0;1,0>
; CHECK:       (P{{[0-9]+}}) goto (M1, 1) [[ELSE:[A-Za-z0-9_]+]]
; CHECK:       {{^}}[[ELSE]]:
; CHECK-NEXT:  mov (M1_NM, 1) [[E:[A-Za-z0-9_]+]](0,0)<1> [[W:[A-Za-z0-9_]+]](0,2)<0;1,0>
; VA1-NEXT:    add (M1_NM, 1) [[A]](0,0)<1> k(0,0)<0;1,0> 0x5:q
; VA9-NEXT:    add (M1_NM, 1) [[V]](0,1)<1> k(0,0)<0;1,0> 0x5:q
; CHECK:       add (M1_NM, 1) [[E1:[A-Za-z0-9_]+]](0,0)<1> [[E]](0,0)<0;1,0> 0x1:w
; CHECK:       lsc_store.ugm (M1_NM, 1) {{.*}} [[E1]]:d32t
; CHECK:       {{^}}[[EXIT:[A-Za-z0-9_]+]]:
; VA1-NEXT:    add (M1_NM, 1) {{[A-Za-z0-9_]+}}(0,0)<1> [[A]](0,0)<0;1,0> 0x1:q
; VA9-NEXT:    add (M1_NM, 1) {{[A-Za-z0-9_]+}}(0,0)<1> [[V]](0,1)<0;1,0> 0x1:q
; CHECK-DAG:   // .decl [[V]] v_type=G type=q num_elts=2 align=wordx32{{$}}
; CHECK-DAG:   // .decl [[W]] v_type=G type=d num_elts=4 align=wordx32 alias=<[[V]], 0>{{$}}
; CHECK-DAG:   // .decl [[E]] v_type=G type=d num_elts=1 align=dword{{$}}

define spir_kernel void @test(ptr addrspace(1) %in, ptr addrspace(1) %out, ptr addrspace(1) %o2, ptr addrspace(1) %o3, i64 %k, i32 %c, <8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
  %v = load <2 x i64>, ptr addrspace(1) %in, align 16
  %a = extractelement <2 x i64> %v, i32 1
  %w = bitcast <2 x i64> %v to <4 x i32>
  %cc = icmp eq i32 %c, 0
  br i1 %cc, label %else, label %then

then:
  store i64 7, ptr addrspace(1) %o2, align 8
  br label %exit

else:
  %e = extractelement <4 x i32> %w, i32 2
  %x = add i64 %k, 5
  %x3 = mul i64 %x, 3
  store i64 %x3, ptr addrspace(1) %o2, align 8
  %e1 = add i32 %e, 1
  store i32 %e1, ptr addrspace(1) %o3, align 4
  br label %exit

exit:
  %r = phi i64 [ %a, %then ], [ %x, %else ]
  %r1 = add i64 %r, 1
  store i64 %r1, ptr addrspace(1) %out, align 8
  ret void
}

; In @test_rev, %else comes first, so %e is aliased to %v first (no mov).
; Then {%a, %x, %r} interferes with %e and gets its own variable.
; CHECK-LABEL: .kernel "test_rev"
; CHECK:       lsc_load.ugm (M1_NM, 1) [[V:[A-Za-z0-9_]+]]:d64x2t
; CHECK:       (!P{{[0-9]+}}) goto (M1, 1) [[THEN:[A-Za-z0-9_]+]]
; VA1:         mov (M1_NM, 1) [[E:[A-Za-z0-9_]+]](0,0)<1> [[W:[A-Za-z0-9_]+]](0,2)<0;1,0>
; CHECK:       add (M1_NM, 1) [[X:[A-Za-z0-9_]+]](0,0)<1> k(0,0)<0;1,0> 0x5:q
; VA1:         add (M1_NM, 1) [[E1:[A-Za-z0-9_]+]](0,0)<1> [[E]](0,0)<0;1,0> 0x1:w
; VA9:         add (M1_NM, 1) [[E1:[A-Za-z0-9_]+]](0,0)<1> [[W:[A-Za-z0-9_]+]](0,2)<0;1,0> 0x1:w
; CHECK:       lsc_store.ugm (M1_NM, 1) {{.*}} [[E1]]:d32t
; CHECK:       {{^}}[[THEN]]:
; CHECK-NEXT:  mov (M1_NM, 1) [[X]](0,0)<1> [[V]](0,1)<0;1,0>
; CHECK:       add (M1_NM, 1) {{[A-Za-z0-9_]+}}(0,0)<1> [[X]](0,0)<0;1,0> 0x1:q
; CHECK-DAG:   // .decl [[V]] v_type=G type=q num_elts=2 align=wordx32{{$}}
; CHECK-DAG:   // .decl [[W]] v_type=G type=d num_elts=4 align=wordx32 alias=<[[V]], 0>{{$}}
; CHECK-DAG:   // .decl [[X]] v_type=G type=q num_elts=1 align={{.*}}

define spir_kernel void @test_rev(ptr addrspace(1) %in, ptr addrspace(1) %out, ptr addrspace(1) %o2, ptr addrspace(1) %o3, i64 %k, i32 %c, <8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
  %v = load <2 x i64>, ptr addrspace(1) %in, align 16
  %w = bitcast <2 x i64> %v to <4 x i32>
  %cc = icmp eq i32 %c, 0
  br i1 %cc, label %else, label %then

else:
  %e = extractelement <4 x i32> %w, i32 2
  %x = add i64 %k, 5
  %x3 = mul i64 %x, 3
  store i64 %x3, ptr addrspace(1) %o2, align 8
  %e1 = add i32 %e, 1
  store i32 %e1, ptr addrspace(1) %o3, align 4
  br label %exit

then:
  %a = extractelement <2 x i64> %v, i32 1
  store i64 7, ptr addrspace(1) %o2, align 8
  br label %exit

exit:
  %r = phi i64 [ %a, %then ], [ %x, %else ]
  %r1 = add i64 %r, 1
  store i64 %r1, ptr addrspace(1) %out, align 8
  ret void
}

; @test_shuffle is @test with non-uniform values (SIMD32), so %w is a
; SubgroupBitcastShuffle, which DeSSA aliases to %v also if it is not
; uniform. A non-uniform alias at byte offset B of %v starts at register
; byte B * 32 (one value per lane), so %a and %e still overlap: both start
; at v(4,0).
; CHECK-LABEL: .kernel "test_shuffle"
; CHECK:       lsc_load.ugm (M1, 32) [[V:[A-Za-z0-9_]+]]:d64x2{{ }}
; VA1-NEXT:    mov (M1, 32) [[A:[A-Za-z0-9_]+]](0,0)<1> [[V]](4,0)<1;1,0>
; CHECK:       (P{{[0-9]+}}) goto (M1, 1) [[ELSE:[A-Za-z0-9_]+]]
; CHECK:       {{^}}[[ELSE]]:
; CHECK-NEXT:  mov (M1, 32) [[E:[A-Za-z0-9_]+]](0,0)<1> [[W:[A-Za-z0-9_]+]](4,0)<1;1,0>
; VA1-NEXT:    add (M1, 32) [[A]](0,0)<1> kk(0,0)<1;1,0> 0x5:q
; VA9-NEXT:    add (M1, 32) [[V]](4,0)<1> kk(0,0)<1;1,0> 0x5:q
; CHECK:       add (M1, 32) [[E1:[A-Za-z0-9_]+]](0,0)<1> [[E]](0,0)<1;1,0> 0x1:w
; CHECK:       lsc_store.ugm (M1, 32) {{.*}} [[E1]]:d32{{$| }}
; CHECK:       {{^}}[[EXIT:[A-Za-z0-9_]+]]:
; VA1-NEXT:    add (M1, 32) {{[A-Za-z0-9_]+}}(0,0)<1> [[A]](0,0)<1;1,0> 0x1:q
; VA9-NEXT:    add (M1, 32) {{[A-Za-z0-9_]+}}(0,0)<1> [[V]](4,0)<1;1,0> 0x1:q
; CHECK-DAG:   // .decl [[V]] v_type=G type=q num_elts=64 align=wordx32{{$}}
; CHECK-DAG:   // .decl [[W]] v_type=G type=d num_elts=128 align=wordx32 alias=<[[V]], 0>{{$}}
; CHECK-DAG:   // .decl [[E]] v_type=G type=d num_elts=32 align=wordx32{{$}}
define spir_kernel void @test_shuffle(ptr addrspace(1) %in, ptr addrspace(1) %out, ptr addrspace(1) %o2, ptr addrspace(1) %o3, i64 %k, i32 %c, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %id = zext i16 %localIdX to i64
  %in.addr = getelementptr <2 x i64>, ptr addrspace(1) %in, i64 %id
  %out.addr = getelementptr i64, ptr addrspace(1) %out, i64 %id
  %o2.addr = getelementptr i64, ptr addrspace(1) %o2, i64 %id
  %o3.addr = getelementptr i32, ptr addrspace(1) %o3, i64 %id
  %kk = add i64 %k, %id
  %v = load <2 x i64>, ptr addrspace(1) %in.addr, align 16
  %a = extractelement <2 x i64> %v, i32 1
  %w = call <4 x i32> @llvm.genx.GenISA.SubgroupBitcastShuffle.v4i32.v2i64(<2 x i64> %v)
  %cc = icmp eq i32 %c, 0
  br i1 %cc, label %else, label %then

then:
  store i64 7, ptr addrspace(1) %o2.addr, align 8
  br label %exit

else:
  %e = extractelement <4 x i32> %w, i32 2
  %x = add i64 %kk, 5
  %x3 = mul i64 %x, 3
  store i64 %x3, ptr addrspace(1) %o2.addr, align 8
  %e1 = add i32 %e, 1
  store i32 %e1, ptr addrspace(1) %o3.addr, align 4
  br label %exit

exit:
  %r = phi i64 [ %a, %then ], [ %x, %else ]
  %r1 = add i64 %r, 1
  store i64 %r1, ptr addrspace(1) %out.addr, align 8
  ret void
}

declare <4 x i32> @llvm.genx.GenISA.SubgroupBitcastShuffle.v4i32.v2i64(<2 x i64>)

!igc.functions = !{!0, !16, !19}
!0 = !{ptr @test, !1}
!16 = !{ptr @test_rev, !1}
!19 = !{ptr @test_shuffle, !1}
!1 = !{!{!"function_type", i32 0}}

!IGCMetadata = !{!2}
!2 = !{!"ModuleMD", !3}
!3 = !{!"FuncMD", !4, !5, !17, !18, !20, !21}
!4 = !{!"FuncMDMap[0]", ptr @test}
!5 = !{!"FuncMDValue[0]", !6, !11, !15}
!6 = !{!"resAllocMD", !7}
!7 = !{!"argAllocMDList", !8, !8, !8, !8, !8, !8, !8, !8}
!8 = !{!"argAllocMDListVec[0]", !9, !10, !10}
!9 = !{!"type", i32 0}
!10 = !{!"extensionType", i32 -1}
!11 = !{!"implicitArgInfoList", !12, !13}
!12 = !{!"implicitArgInfoListVec[0]", !{!"argId", i32 0}}
!13 = !{!"implicitArgInfoListVec[1]", !{!"argId", i32 1}}
!15 = !{!"requiredSubGroupSize", i32 32}
!17 = !{!"FuncMDMap[1]", ptr @test_rev}
!18 = !{!"FuncMDValue[1]", !6, !11, !15}
!20 = !{!"FuncMDMap[2]", ptr @test_shuffle}
!21 = !{!"FuncMDValue[2]", !22, !24, !15}
!22 = !{!"resAllocMD", !23}
!23 = !{!"argAllocMDList", !8, !8, !8, !8, !8, !8, !8, !8, !8}
!24 = !{!"implicitArgInfoList", !12, !13, !25}
!25 = !{!"implicitArgInfoListVec[2]", !{!"argId", i32 8}}
