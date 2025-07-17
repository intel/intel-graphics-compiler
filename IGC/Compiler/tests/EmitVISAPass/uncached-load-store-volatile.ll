;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-14-plus

; RUN: igc_opt -ocl -platformdg2 -igc-emit-visa -regkey DumpVISAASMToConsole < %s | FileCheck %s
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Test checks if volatile load/store instructions emit uncached LSC intructions

define spir_kernel void @test(i32 addrspace(1)* %dst, i32 %bindlessOffset) {
entry:
  ; CHECK: lsc_load.ugm.uc.uc {{.*}}
  ; CHECK: lsc_store.ugm.uc.uc {{.*}}
  %0 = load volatile i32, i32 addrspace(1)* %dst
  store volatile i32 %0, i32 addrspace(1)* %dst

  ; CHECK: lsc_load.ugm.uc.uc {{.*}}
  ; CHECK: lsc_store.ugm.uc.uc {{.*}}
  %1 = inttoptr i32 %bindlessOffset to float addrspace(2490368)*
  %2 = call float @llvm.genx.GenISA.ldraw.indexed.f32.p2490368f32(float addrspace(2490368)* %1, i32 %0, i32 4, i1 true)
  call void @llvm.genx.GenISA.storeraw.indexed.p2490368f32.f32(float addrspace(2490368)* %1, i32 4, float %2, i32 4, i1 true)

  ; COM: Checks below can be enabled when PredicatedLoad and PredicatedStore class will
  ; COM: start supporting volatile instructions.
  ; COM: lsc_load.ugm.uc.uc {{.*}}
  ; COM: lsc_store.ugm.uc.uc {{.*}}
  ; COM: %3 = call i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)* %dst, i64 4, i1 true, i32 4)
  ; COM: call void @llvm.genx.GenISA.PredicatedStore.p1i32.i32(i32 addrspace(1)* %dst, i32 1, i64 2, i1 true)
  ret void
}

declare float @llvm.genx.GenISA.ldraw.indexed.f32.p2490368f32(float addrspace(2490368)*, i32, i32, i1) #0
declare void @llvm.genx.GenISA.storeraw.indexed.p2490368f32.f32(float addrspace(2490368)*, i32, float, i32, i1) #1
; declare i32 @llvm.genx.GenISA.PredicatedLoad.i32.p1i32.i32(i32 addrspace(1)*, i64, i1, i32)
; declare void @llvm.genx.GenISA.PredicatedStore.p1i32.i32(i32 addrspace(1)*, i32, i64, i1)

attributes #0 = { argmemonly nounwind readonly willreturn }
attributes #1 = { argmemonly nounwind writeonly }

!IGCMetadata = !{!0}
!igc.functions = !{!21}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, i32)* @test}
!3 = !{!"FuncMDValue[0]", !4, !17}
!4 = !{!"resAllocMD", !5}
!5 = !{!"argAllocMDList", !6, !10, !11, !14, !15, !16}
!6 = !{!"argAllocMDListVec[0]", !7, !8, !9}
!7 = !{!"type", i32 0}
!8 = !{!"extensionType", i32 -1}
!9 = !{!"indexType", i32 -1}
!10 = !{!"argAllocMDListVec[1]", !7, !8, !9}
!11 = !{!"argAllocMDListVec[2]", !12, !8, !13}
!12 = !{!"type", i32 1}
!13 = !{!"indexType", i32 0}
!14 = !{!"argAllocMDListVec[3]", !7, !8, !9}
!15 = !{!"argAllocMDListVec[4]", !7, !8, !9}
!16 = !{!"argAllocMDListVec[5]", !7, !8, !9}
!17 = !{!"m_OpenCLArgTypeQualifiers", !18, !19, !20}
!18 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!19 = !{!"m_OpenCLArgTypeQualifiersVec[1]", !""}
!20 = !{!"m_OpenCLArgTypeQualifiersVec[2]", !""}
!21 = !{void (i32 addrspace(1)*, i32)* @test, !22}
!22 = !{!23}
!23 = !{!"function_type", i32 0}
