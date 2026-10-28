;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; Test how ResourceLoopAnalysis treats the igc.resource.loop.prepared metadata that ResourceLoopDecodeSink puts on the
; sends of a run it has prepared: two sends followed by their decode code form a group of two sends, which
; FuseResourceLoopMinSize=3 would discard. A tagged group is kept and emitted as one resource loop. The same IR without
; the tag gives two loops.
;
; FIXME: make this test work without shader type
; REQUIRES: regkeys, shader-types
; UNSUPPORTED: llvm-17-plus

; RUN: igc_opt --typed-pointers -platformbmg -LSC-Cache-Hints -igc-emit-visa -simd-mode 16 -inputrt -regkey DumpVISAASMToConsole -S %s 2>&1 | FileCheck %s --check-prefix=TAGGED
; RUN: sed 's/, !igc.resource.loop.prepared ![0-9]*//' %s | igc_opt --typed-pointers -platformbmg -LSC-Cache-Hints -igc-emit-visa -simd-mode 16 -inputrt -regkey DumpVISAASMToConsole -S 2>&1 | FileCheck %s --check-prefix=UNTAGGED

@ThreadGroupSize_X = constant i32 64
@ThreadGroupSize_Y = constant i32 1
@ThreadGroupSize_Z = constant i32 1

define spir_kernel void @test_prepared_run(i32 %src1, i32 %val, i32 addrspace(1)* %dst) {
entry:
; COM: tagged: the group of two sends is kept and both loads sit in one resource loop
; TAGGED-LABEL: _main_0:
; TAGGED: _test_prepared_run_001__opt_resource_loop:
; TAGGED: lsc_load{{.*}} a:d32
; TAGGED-NEXT: lsc_load{{.*}} b:d32
; TAGGED-NEXT: goto {{.*}} _test_prepared_run_001__opt_resource_loop
; TAGGED-NOT: _test_prepared_run_002__opt_resource_loop

; COM: untagged: FuseResourceLoopMinSize=3 discards the group of two, so each load has its own loop
; UNTAGGED-LABEL: _main_0:
; UNTAGGED: _test_prepared_run_001__opt_resource_loop:
; UNTAGGED: lsc_load{{.*}} a:d32
; UNTAGGED-NEXT: goto {{.*}} _test_prepared_run_001__opt_resource_loop
; UNTAGGED: _test_prepared_run_002__opt_resource_loop:
; UNTAGGED: lsc_load{{.*}} b:d32
; UNTAGGED-NEXT: goto {{.*}} _test_prepared_run_002__opt_resource_loop
  %svn = call i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32 17)
  %nonuniform = zext i16 %svn to i32
  %res = inttoptr i32 %nonuniform to <4 x float> addrspace(2621440)*
  %off = add i32 %nonuniform, 4
  %a = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)* %res, i32 %nonuniform, i32 4, i1 false), !igc.resource.loop.prepared !50
  %b = call i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)* %res, i32 %off, i32 4, i1 false), !igc.resource.loop.prepared !50
  %a.dec = and i32 %a, 65535
  %b.dec = shl i32 %b, 2
  %s = add i32 %a.dec, %b.dec
  store i32 %s, i32 addrspace(1)* %dst, align 4
  ret void
}

declare i32 @llvm.genx.GenISA.ldraw.indexed.i32.p2621440v4f32(<4 x float> addrspace(2621440)*, i32, i32, i1) #4
declare i16 @llvm.genx.GenISA.DCL.SystemValue.i16(i32) #1

attributes #1 = { nounwind readnone }
attributes #4 = { argmemonly nounwind readonly }

!50 = !{}

!IGCMetadata = !{!0}
!igc.functions = !{!21}

!0 = !{!"ModuleMD", !1}
!1 = !{!"FuncMD", !2, !3}
!2 = !{!"FuncMDMap[0]", void (i32, i32, i32 addrspace(1)*)* @test_prepared_run}
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
!21 = !{void (i32, i32, i32 addrspace(1)*)* @test_prepared_run, !22}
!22 = !{!23}
!23 = !{!"function_type", i32 0}

