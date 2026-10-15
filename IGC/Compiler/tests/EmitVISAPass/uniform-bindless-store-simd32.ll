;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers -ocl -platformdg2 -igc-emit-visa -simd-mode 32 %s -regkey DumpVISAASMToConsole | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Verifies that a uniform bindless store in a SIMD32 kernel is emitted once,
; not once per SIMD16 instance. Non-uniform stores still need both instances.

; CHECK-LABEL: .kernel "test"

; CHECK:     lsc_store.ugm (M1_NM, 1) {{.*}}:d32t
; CHECK-NOT: lsc_store.ugm (M5_NM, 1)
; CHECK:     lsc_store.ugm (M1_NM, 1) {{.*}}:d32x2t
; CHECK-NOT: lsc_store.ugm (M5_NM, 1)
; CHECK:     lsc_store.ugm (M1, 16) {{.*}}:d32
; CHECK:     lsc_store.ugm (M5, 16) {{.*}}:d32
; CHECK:     ret

define spir_kernel void @test(ptr addrspace(1) %out, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i32 %bufferOffset, i32 %bindlessOffset) {
entry:
  %res = inttoptr i32 %bindlessOffset to ptr addrspace(2490368)
  call void @llvm.genx.GenISA.storeraw.indexed.p2490368.i32(ptr addrspace(2490368) %res, i32 %bufferOffset, i32 1, i32 4, i1 false)
  %off1 = add i32 %bufferOffset, 16
  call void @llvm.genx.GenISA.storerawvector.indexed.p2490368.v2i32(ptr addrspace(2490368) %res, i32 %off1, <2 x i32> <i32 2, i32 3>, i32 8, i1 false)
  %lid = zext i16 %localIdX to i32
  %lidOff = shl i32 %lid, 2
  %off2 = add i32 %bufferOffset, %lidOff
  call void @llvm.genx.GenISA.storeraw.indexed.p2490368.i32(ptr addrspace(2490368) %res, i32 %off2, i32 4, i32 4, i1 false)
  ret void
}

declare void @llvm.genx.GenISA.storeraw.indexed.p2490368.i32(ptr addrspace(2490368), i32, i32, i32, i1)
declare void @llvm.genx.GenISA.storerawvector.indexed.p2490368.v2i32(ptr addrspace(2490368), i32, <2 x i32>, i32, i1)

!igc.functions = !{!0}
!IGCMetadata = !{!3}

!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4, !6}
!4 = !{!"compOpt", !5, !26}
!5 = !{!"UseBindlessMode", i1 true}
!6 = !{!"FuncMD", !7, !8}
!7 = !{!"FuncMDMap[0]", ptr @test}
!8 = !{!"FuncMDValue[0]", !9, !10, !31}
!9 = !{!"functionType", !"KernelFunction"}
!10 = !{!"resAllocMD", !11, !12, !13, !14}
!11 = !{!"uavsNumType", i32 1}
!12 = !{!"srvsNumType", i32 0}
!13 = !{!"samplersNumType", i32 0}
!14 = !{!"argAllocMDList", !15, !20, !22, !23, !24, !25}
!15 = !{!"argAllocMDListVec[0]", !16, !17, !18}
!16 = !{!"type", i32 1}
!17 = !{!"extensionType", i32 -1}
!18 = !{!"indexType", i32 0}
!19 = !{!"type", i32 0}
!20 = !{!"argAllocMDListVec[1]", !19, !17, !21}
!21 = !{!"indexType", i32 -1}
!22 = !{!"argAllocMDListVec[2]", !19, !17, !21}
!23 = !{!"argAllocMDListVec[3]", !19, !17, !21}
!24 = !{!"argAllocMDListVec[4]", !19, !17, !21}
!25 = !{!"argAllocMDListVec[5]", !19, !17, !21}
!26 = !{!"UseLegacyBindlessMode", i1 false}
!31 = !{!"implicitArgInfoList", !32, !34, !36, !38, !41}
!32 = !{!"implicitArgInfoListVec[0]", !33}
!33 = !{!"argId", i32 0}
!34 = !{!"implicitArgInfoListVec[1]", !35}
!35 = !{!"argId", i32 1}
!36 = !{!"implicitArgInfoListVec[2]", !37}
!37 = !{!"argId", i32 8}
!38 = !{!"implicitArgInfoListVec[3]", !39, !40}
!39 = !{!"argId", i32 15}
!40 = !{!"explicitArgNum", i32 0}
!41 = !{!"implicitArgInfoListVec[4]", !42, !40}
!42 = !{!"argId", i32 54}
