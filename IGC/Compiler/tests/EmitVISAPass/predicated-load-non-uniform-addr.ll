;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --opaque-pointers -platformbmg -igc-emit-visa %s -regkey DumpVISAASMToConsole | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------
;
; Non-uniform (per-lane gather) scalar predicated load with a CONSTANT merge value.
; The existing predicated-load-uniform.ll only covers the uniform-address path;
; this covers the non-uniform gather path (emitLSCVectorLoad case #3).
;
; Here the destination is GRF-aligned, so the load is emitted DIRECTLY into the
; merge-initialized destination (needTemp == false): predicate-false lanes correctly
; retain the merge value 0.
;
; NOTE: the defect this guards against lives in the needTemp == true sub-path, taken
; when the destination is aliased at a sub-GRF byte offset by deSSA / variable-reuse
; coalescing. That coalescing runs only in the full compile pipeline, not under
; standalone -igc-emit-visa, so the buggy path cannot be isolated in a minimal LIT
; kernel; validate the defective path end-to-end via the full compile pipeline.

define spir_kernel void @test_nonuniform_addr(ptr addrspace(1) align 4 %in, ptr addrspace(1) align 4 %out, i32 %a, i16 %localIdX) {
entry:
  %lid = zext i16 %localIdX to i64
  %p = icmp slt i32 0, %a
  ; per-lane address => non-uniform gather (emitLSCVectorLoad case #3)
  %addr = getelementptr i32, ptr addrspace(1) %in, i64 %lid

  ; CHECK-LABEL: .kernel "test_nonuniform_addr"
  ; predicate comes from the i1 compare
  ; CHECK: cmp.lt (M1_NM, 32) [[P:P[0-9]+]] 0x0:d
  ; destination is initialized with the constant merge value 0 ...
  ; CHECK: mov (M1, 32) res(0,0)<1> 0x0:d
  ; ... then the predicated gather loads directly into that destination, so
  ; predicate-false lanes keep the merge value (no separate temp copy).
  ; CHECK: ([[P]]) lsc_load.ugm (M1, 32) res:d32 flat[addr]:a64
  %res = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %addr, i64 4, i1 %p, i32 0)

  %out.addr = getelementptr i32, ptr addrspace(1) %out, i64 %lid
  store i32 %res, ptr addrspace(1) %out.addr, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32)

!IGCMetadata = !{!0}
!igc.functions = !{!100}

!0 = !{!"ModuleMD", !2}
!2 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", ptr @test_nonuniform_addr}
!6 = !{!"FuncMDValue[0]", !7, !20}
!7 = !{!"resAllocMD", !8}
!8 = !{!"argAllocMDList", !9, !13, !14, !15}
!9 = !{!"argAllocMDListVec[0]", !10, !11, !12}
!10 = !{!"type", i32 0}
!11 = !{!"extensionType", i32 -1}
!12 = !{!"indexType", i32 -1}
!13 = !{!"argAllocMDListVec[1]", !10, !11, !12}
!14 = !{!"argAllocMDListVec[2]", !10, !11, !12}
!15 = !{!"argAllocMDListVec[3]", !10, !11, !12}
!20 = !{!"implicitArgInfoList", !21}
!21 = !{!"implicitArgInfoListVec[0]", !22}
!22 = !{!"argId", i32 8}
!100 = !{ptr @test_nonuniform_addr, !101}
!101 = !{!102}
!102 = !{!"function_type", i32 0}
