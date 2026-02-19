;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-promote-resources-to-direct-addrspace -S < %s | FileCheck %s
; ------------------------------------------------
; PromoteResourceToDirectAS : sampler promotion part
; ------------------------------------------------
; This test checks that PromoteResourceToDirectAS pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @test_indirect
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK: [[ACAST_V:%[0-9A-z]*]] = addrspacecast
; CHECK-SAME: !dbg [[ACAST_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata {{.*}} addrspace(2293760)* [[ACAST_V]]
; CHECK-SAME: metadata [[ACAST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ACAST_LOC]]
;
; CHECK: !dbg [[CALL_LOC:[0-9]*]]
; CHECK: ret void

define spir_kernel void @test_indirect(i32 %a, i32 %b, i32* %ptr) !dbg !42 {
  %1 = addrspacecast i32* %ptr to float addrspace(2293760)*, !dbg !47
  call void @llvm.dbg.value(metadata float addrspace(2293760)* %1, metadata !45, metadata !DIExpression()), !dbg !47
  call void @llvm.genx.GenISA.typedwrite.p2293760f32(float addrspace(2293760)* %1, i32 1, i32 2, i32 3, i32 4, float 5.000000e+00, float 6.000000e+00, float 7.000000e+00, float 8.000000e+00), !dbg !48
  ret void, !dbg !49
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "samplertodir.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_indirect", linkageName: "test_indirect", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[ACAST_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 1, type: !46)
; CHECK-DAG: [[ACAST_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])

declare void @llvm.genx.GenISA.typedwrite.p2293760f32(float addrspace(2293760)*, i32, i32, i32, i32, float, float, float, float)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!IGCMetadata = !{!0}
!igc.functions = !{!33}
!llvm.dbg.cu = !{!36}
!llvm.debugify = !{!39, !40}
!llvm.module.flags = !{!41}

!0 = !{!"ModuleMD", !1, !2, !3, !4, !5, !6, !7}
!1 = !{!"EnableTextureIndirection", i1 true}
!2 = !{!"EnableSamplerIndirection", i1 true}
!3 = !{!"samplerStateStride", i32 0}
!4 = !{!"samplerStateOffset", i32 0}
!5 = !{!"textureStateStride", i32 0}
!6 = !{!"textureStateOffset", i32 0}
!7 = !{!"FuncMD", !8, !9}
!8 = !{!"FuncMDMap[0]", void (i32, i32, i32*)* @test_indirect}
!9 = !{!"FuncMDValue[0]", !10}
!10 = !{!"resAllocMD", !11, !12, !13, !14}
!11 = !{!"uavsNumType", i32 2}
!12 = !{!"srvsNumType", i32 1}
!13 = !{!"samplersNumType", i32 0}
!14 = !{!"argAllocMDList", !15, !19, !22, !26, !27, !28, !29, !30, !31, !32}
!15 = !{!"argAllocMDListVec[0]", !16, !17, !18}
!16 = !{!"type", i32 2}
!17 = !{!"extensionType", i32 0}
!18 = !{!"indexType", i32 0}
!19 = !{!"argAllocMDListVec[1]", !20, !17, !21}
!20 = !{!"type", i32 1}
!21 = !{!"indexType", i32 1}
!22 = !{!"argAllocMDListVec[2]", !23, !24, !25}
!23 = !{!"type", i32 4}
!24 = !{!"extensionType", i32 -1}
!25 = !{!"indexType", i32 5}
!26 = !{!"argAllocMDListVec[3]", !23, !24, !25}
!27 = !{!"argAllocMDListVec[4]", !23, !24, !25}
!28 = !{!"argAllocMDListVec[5]", !23, !24, !25}
!29 = !{!"argAllocMDListVec[6]", !23, !24, !25}
!30 = !{!"argAllocMDListVec[7]", !23, !24, !25}
!31 = !{!"argAllocMDListVec[8]", !23, !24, !25}
!32 = !{!"argAllocMDListVec[9]", !20, !24, !18}
!33 = !{void (i32, i32, i32*)* @test_indirect, !34}
!34 = !{!35}
!35 = !{!"function_type", i32 0}
!36 = distinct !DICompileUnit(language: DW_LANG_C, file: !37, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !38)
!37 = !DIFile(filename: "samplertodir.ll", directory: "/")
!38 = !{}
!39 = !{i32 3}
!40 = !{i32 1}
!41 = !{i32 2, !"Debug Info Version", i32 3}
!42 = distinct !DISubprogram(name: "test_indirect", linkageName: "test_indirect", scope: null, file: !37, line: 1, type: !43, scopeLine: 1, unit: !36, retainedNodes: !44)
!43 = !DISubroutineType(types: !38)
!44 = !{!45}
!45 = !DILocalVariable(name: "1", scope: !42, file: !37, line: 1, type: !46)
!46 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!47 = !DILocation(line: 1, column: 1, scope: !42)
!48 = !DILocation(line: 2, column: 1, scope: !42)
!49 = !DILocation(line: 3, column: 1, scope: !42)
