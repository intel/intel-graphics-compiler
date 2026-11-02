;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================


; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers -platformpvc -igc-emit-visa %s -simd-mode 32 -regkey DumpVISAASMToConsole | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; DeSSA coalesces both predicated loads with the PHI, so each load has to write
; the PHI register. The merge value must not take the destination over: the
; store reads the PHI register, which nothing else writes.

; CHECK-LABEL: _test_kernel_001_then:
; CHECK:       ({{P[0-9]+}}) lsc_load{{.*}} [[RES:[^ ]+]]:d32
; CHECK-LABEL: _test_kernel_002_else:
; CHECK:       ({{P[0-9]+}}) lsc_load{{.*}} [[RES]]:d32
; CHECK-LABEL: _test_kernel_003_end:
; CHECK:       lsc_store{{.*}} [[RES]]:d32

define spir_kernel void @test_kernel(ptr addrspace(1) %a, ptr addrspace(1) %b, ptr addrspace(1) %output, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX) {
entry:
  %tid = zext i16 %localIdX to i64
  %offset = shl i64 %tid, 2
  %a.base = ptrtoint ptr addrspace(1) %a to i64
  %a.off = add i64 %offset, %a.base
  %a.addr = inttoptr i64 %a.off to ptr addrspace(1)
  %b.base = ptrtoint ptr addrspace(1) %b to i64
  %b.off = add i64 %offset, %b.base
  %b.addr = inttoptr i64 %b.off to ptr addrspace(1)
  %out.base = ptrtoint ptr addrspace(1) %output to i64
  %out.off = add i64 %offset, %out.base
  %out.addr = inttoptr i64 %out.off to ptr addrspace(1)
  %pa = icmp ult i16 %localIdX, 4
  %pb = icmp ult i16 %localIdX, 8
  %cmp = icmp ult i16 %localIdX, 2
  br i1 %cmp, label %then, label %else

then:
  %merge.t = load i32, ptr addrspace(1) %b.addr, align 4
  %load.t = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %a.addr, i64 4, i1 %pa, i32 %merge.t)
  br label %end

else:
  %merge.e = load i32, ptr addrspace(1) %a.addr, align 4
  %load.e = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1) %b.addr, i64 4, i1 %pb, i32 %merge.e)
  br label %end

end:
  %res = phi i32 [ %load.t, %then ], [ %load.e, %else ]
  store i32 %res, ptr addrspace(1) %out.addr, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1.i32(ptr addrspace(1), i64, i1, i32)

!IGCMetadata = !{!0}
!igc.functions = !{!14}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", ptr @test_kernel}
!3 = !{!"FuncMDValue[0]", !4, !10}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6, !9, !9}
!6 = !{!"argAllocMDListVec[0]", !7, !8, !8}
!7 = !{!"type", i32 0}
!8 = !{!"extensionType", i32 -1}
!9 = !{!"argAllocMDListVec[1]", !7, !8, !8}
!10 = !{!"implicitArgInfoList", !11, !12, !13}
!11 = !{!"implicitArgInfoListVec[0]", !{!"argId", i32 0}}
!12 = !{!"implicitArgInfoListVec[1]", !{!"argId", i32 1}}
!13 = !{!"implicitArgInfoListVec[2]", !{!"argId", i32 8}}
!14 = !{ptr @test_kernel, !15}
!15 = !{!16}
!16 = !{!"function_type", i32 0}
