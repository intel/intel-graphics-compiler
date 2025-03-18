;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: opaque-ptr-fix, llvm-14-plus
; RUN: igc_opt --opaque-pointers -igc-stateless-to-stateful-resolution -S < %s | FileCheck %s
; ------------------------------------------------
; StatelessToStateful
; ------------------------------------------------
; This test checks that StatelessToStateful pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Debug MD for this test was created with debugify pass.
; ------------------------------------------------

; CHECK: define spir_kernel void @func_b{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: [[VAL2_V:%[A-z0-9]*]] = load {{.*}}, !dbg [[VAL2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[VAL2_V]], metadata [[VAL2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[VAL2_LOC]]
; CHECK: store i32{{.*}}, !dbg [[STR1_LOC:![0-9]*]]

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @func_b(i32 %n, i32 addrspace(1)* %r, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i8 addrspace(1)* %s2, i8 addrspace(1)* %s3, i32 %s4, i32 %s5, i32 %bufferOffset) #0 !dbg !97 {
entry:
  %0 = getelementptr i32, i32 addrspace(1)* %r, i32 16, !dbg !104
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %0, metadata !100, metadata !DIExpression()), !dbg !104
  %1 = load i32, i32 addrspace(1)* %0, !dbg !105
  call void @llvm.dbg.value(metadata i32 %1, metadata !102, metadata !DIExpression()), !dbg !105
  store i32 %n, i32 addrspace(1)* %0, !dbg !106
  ret void, !dbg !107
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "StatelessToStatefull.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "func_b", linkageName: "func_b", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[VAL2_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[VAL2_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STR1_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])


; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" }
attributes #1 = { nounwind readnone speculatable }

!IGCMetadata = !{!0}
!igc.functions = !{!78}
!llvm.dbg.cu = !{!91}
!llvm.debugify = !{!94, !95}
!llvm.module.flags = !{!96}

!0 = !{!"ModuleMD", !1, !2, !6, !69, !65}
!1 = !{!"isPrecise", i1 false}
!2 = !{!"compOpt", !3, !4, !5}
!3 = !{!"DenormsAreZero", i1 false}
!4 = !{!"CorrectlyRoundedDivSqrt", i1 false}
!5 = !{!"OptDisable", i1 false}
!6 = !{!"FuncMD", !7, !8}
!7 = !{!"FuncMDMap[0]", void (i32, i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8*, i8 addrspace(1)*, i8 addrspace(1)*, i32, i32, i32)* @func_b}
!8 = !{!"FuncMDValue[0]", !9, !10, !14, !15, !16, !17, !32, !56, !57, !58, !59, !60, !61, !62, !63, !64, !65, !66, !67, !68}
!9 = !{!"localOffsets"}
!10 = !{!"workGroupWalkOrder", !11, !12, !13}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 0}
!13 = !{!"dim2", i32 0}
!14 = !{!"funcArgs"}
!15 = !{!"functionType", !"KernelFunction"}
!16 = !{!"callableShaderType", !"NumberOfCallableShaderTypes"}
!17 = !{!"rtInfo", !18, !19, !20, !21, !22, !23, !24, !25, !26, !27, !28}
!18 = !{!"hasTraceRayPayload", i1 false}
!19 = !{!"hasHitAttributes", i1 false}
!20 = !{!"hasCallableData", i1 false}
!21 = !{!"ShaderStackSize", i32 0}
!22 = !{!"IsCallStackHandler", i1 false}
!23 = !{!"ShaderName", !""}
!24 = !{!"NOSSize", i32 0}
!25 = !{!"Entries"}
!26 = !{!"SpillUnions"}
!27 = !{!"CustomHitAttrSizeInBytes", i32 0}
!28 = !{!"Types", !29, !30, !31}
!29 = !{!"FrameStartTys"}
!30 = !{!"ArgumentTys"}
!31 = !{!"FullFrameTys"}
!32 = !{!"resAllocMD", !33, !34, !35, !36, !55}
!33 = !{!"uavsNumType", i32 4}
!34 = !{!"srvsNumType", i32 0}
!35 = !{!"samplersNumType", i32 0}
!36 = !{!"argAllocMDList", !37, !41, !44, !45, !46, !48, !50, !52, !53, !54}
!37 = !{!"argAllocMDListVec[0]", !38, !39, !40}
!38 = !{!"type", i32 0}
!39 = !{!"extensionType", i32 -1}
!40 = !{!"indexType", i32 -1}
!41 = !{!"argAllocMDListVec[1]", !42, !39, !43}
!42 = !{!"type", i32 1}
!43 = !{!"indexType", i32 0}
!44 = !{!"argAllocMDListVec[2]", !38, !39, !40}
!45 = !{!"argAllocMDListVec[3]", !38, !39, !40}
!46 = !{!"argAllocMDListVec[4]", !42, !39, !47}
!47 = !{!"indexType", i32 1}
!48 = !{!"argAllocMDListVec[5]", !42, !39, !49}
!49 = !{!"indexType", i32 2}
!50 = !{!"argAllocMDListVec[6]", !42, !39, !51}
!51 = !{!"indexType", i32 3}
!52 = !{!"argAllocMDListVec[7]", !38, !39, !40}
!53 = !{!"argAllocMDListVec[8]", !38, !39, !40}
!54 = !{!"argAllocMDListVec[9]", !38, !39, !40}
!55 = !{!"inlineSamplersMD"}
!56 = !{!"maxByteOffsets"}
!57 = !{!"IsInitializer", i1 false}
!58 = !{!"IsFinalizer", i1 false}
!59 = !{!"CompiledSubGroupsNumber", i32 0}
!60 = !{!"isCloned", i1 false}
!61 = !{!"hasInlineVmeSamplers", i1 false}
!62 = !{!"localSize", i32 0}
!63 = !{!"localIDPresent", i1 false}
!64 = !{!"groupIDPresent", i1 false}
!65 = !{!"privateMemoryPerWI", i32 0}
!66 = !{!"globalIDPresent", i1 false}
!67 = !{!"isUniqueEntry", i1 false}
!68 = !{!"UserAnnotations"}
!69 = !{!"pushInfo", !70, !71, !74, !75, !76, !77}
!70 = !{!"pushableAddresses"}
!71 = !{!"dynamicBufferInfo", !72, !73}
!72 = !{!"firstIndex", i32 0}
!73 = !{!"numOffsets", i32 0}
!74 = !{!"MaxNumberOfPushedBuffers", i32 0}
!75 = !{!"inlineConstantBufferSlot", i32 -1}
!76 = !{!"inlineConstantBufferOffset", i32 -1}
!77 = !{!"inlineConstantBufferGRFOffset", i32 -1}
!78 = !{void (i32, i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8*, i8 addrspace(1)*, i8 addrspace(1)*, i32, i32, i32)* @func_b, !79}
!79 = !{!80, !81}
!80 = !{!"function_type", i32 0}
!81 = !{!"implicit_arg_desc", !82, !83, !84, !85, !86, !87, !88, !89}
!82 = !{i32 0}
!83 = !{i32 1}
!84 = !{i32 13}
!85 = !{i32 41}
!86 = !{i32 42}
!87 = !{i32 43}
!88 = !{i32 44}
!89 = !{i32 15, !90}
!90 = !{!"explicit_arg_num", i32 1}
!91 = distinct !DICompileUnit(language: DW_LANG_C, file: !92, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !93)
!92 = !DIFile(filename: "StatelessToStatefull.ll", directory: "/")
!93 = !{}
!94 = !{i32 4}
!95 = !{i32 2}
!96 = !{i32 2, !"Debug Info Version", i32 3}
!97 = distinct !DISubprogram(name: "func_b", linkageName: "func_b", scope: null, file: !92, line: 1, type: !98, scopeLine: 1, unit: !91, retainedNodes: !99)
!98 = !DISubroutineType(types: !93)
!99 = !{!100, !102}
!100 = !DILocalVariable(name: "1", scope: !97, file: !92, line: 1, type: !101)
!101 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!102 = !DILocalVariable(name: "2", scope: !97, file: !92, line: 2, type: !103)
!103 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!104 = !DILocation(line: 1, column: 1, scope: !97)
!105 = !DILocation(line: 2, column: 1, scope: !97)
!106 = !DILocation(line: 3, column: 1, scope: !97)
!107 = !DILocation(line: 4, column: 1, scope: !97)
