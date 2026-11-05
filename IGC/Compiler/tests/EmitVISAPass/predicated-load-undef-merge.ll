;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers -platformbmg -igc-emit-visa %s -regkey DumpVISAASMToConsole | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; A predicated load with an undef merge value has no unpredicated definition of
; its destination, so it has to be preceded by a lifetime start. Otherwise the
; destination is live from the kernel entry. The chained load from the generic
; address resolution takes over the destination of the first one and needs no
; lifetime start of its own.

; CHECK-LABEL: .kernel "test_undef_merge"
; CHECK: lifetime.start [[DST:[^ ]+]]
; CHECK-NEXT: ({{P[0-9]+}}) lsc_load.ugm (M1, {{[0-9]+}}) [[DST]]:d32
; CHECK-NOT: lifetime.start
; CHECK: ({{P[0-9]+}}) lsc_load.slm (M1, {{[0-9]+}}) [[DST]]:d32

define spir_kernel void @test_undef_merge(ptr addrspace(1) %out, ptr addrspace(4) %gp, i32 %sel, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %lid = zext i16 %localIdX to i64
  %addr = getelementptr i32, ptr addrspace(4) %gp, i64 %lid
  %isLocal = icmp eq i32 %sel, 2
  %isGlobal = xor i1 %isLocal, true
  %global.addr = addrspacecast ptr addrspace(4) %addr to ptr addrspace(1)
  %globalLoad = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %global.addr, i64 4, i1 %isGlobal, i32 poison)
  %local.addr = addrspacecast ptr addrspace(4) %addr to ptr addrspace(3)
  %localLoad = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p3.i32(ptr addrspace(3) %local.addr, i64 4, i1 %isLocal, i32 %globalLoad)
  %out.addr = getelementptr i32, ptr addrspace(1) %out, i64 %lid
  store i32 %localLoad, ptr addrspace(1) %out.addr, align 4
  ret void
}

; A defined merge value is copied into the destination, which is a full
; definition, so no lifetime start is needed.

; CHECK-LABEL: .kernel "test_defined_merge"
; CHECK-NOT: lifetime.start
; CHECK: mov (M1, {{[0-9]+}}) [[RES:[^ ]+]](0,0)<1> 0x0:d
; CHECK-NEXT: ({{P[0-9]+}}) lsc_load.ugm (M1, {{[0-9]+}}) [[RES]]:d32

define spir_kernel void @test_defined_merge(ptr addrspace(1) %out, ptr addrspace(1) %in, i32 %sel, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %lid = zext i16 %localIdX to i64
  %addr = getelementptr i32, ptr addrspace(1) %in, i64 %lid
  %p = icmp eq i32 %sel, 0
  %res = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %addr, i64 4, i1 %p, i32 0)
  %out.addr = getelementptr i32, ptr addrspace(1) %out, i64 %lid
  store i32 %res, ptr addrspace(1) %out.addr, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32)
declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p3.i32(ptr addrspace(3), i64, i1, i32)

!IGCMetadata = !{!0}
!igc.functions = !{!20, !23}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3, !12, !13}
!2 = !{!"FuncMDMap[0]", ptr @test_undef_merge}
!3 = !{!"FuncMDValue[0]", !4, !8}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6, !6, !6}
!6 = !{!"argAllocMDListVec[0]", !7, !11, !11}
!7 = !{!"type", i32 0}
!8 = !{!"implicitArgInfoList", !9, !10, !14}
!9 = !{!"implicitArgInfoListVec[0]", !{!"argId", i32 0}}
!10 = !{!"implicitArgInfoListVec[1]", !{!"argId", i32 1}}
!11 = !{!"extensionType", i32 -1}
!12 = !{!"FuncMDMap[1]", ptr @test_defined_merge}
!13 = !{!"FuncMDValue[1]", !4, !8}
!14 = !{!"implicitArgInfoListVec[2]", !{!"argId", i32 8}}
!20 = !{ptr @test_undef_merge, !21}
!21 = !{!22}
!22 = !{!"function_type", i32 0}
!23 = !{ptr @test_defined_merge, !21}
