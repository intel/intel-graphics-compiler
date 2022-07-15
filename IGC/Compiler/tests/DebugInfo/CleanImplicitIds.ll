;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-clear-implicit-ids -S < %s | FileCheck %s
; ------------------------------------------------
; CleanImplicitIds
; ------------------------------------------------
; This test checks that CleanImplicitIds pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Note: extractelement currently dont have lineinfo in igc
; ------------------------------------------------

; CHECK: define spir_kernel void @__func_b_block_invoke
;
; CHECK: [[GID0_V:%.*]] = extractelement {{.*}} %globalId.i
; CHECK: [[GID1_V:%.*]] = extractelement {{.*}} %globalId.i
; CHECK: [[GID2_V:%.*]] = extractelement {{.*}} %globalId.i
;
; CHECK: @llvm.dbg.value(metadata i64 [[GID0_V]], metadata [[GID0_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GID_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i64 [[GID1_V]], metadata [[GID1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GID_LOC]]
; CHECK: @llvm.dbg.value(metadata i64 [[GID2_V]], metadata [[GID2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GID_LOC]]
;
; CHECK: [[LID0_V:%.*]] = extractelement {{.*}} %globalId1.i
; CHECK: [[LID1_V:%.*]] = extractelement {{.*}} %globalId1.i
; CHECK: [[LID2_V:%.*]] = extractelement {{.*}} %globalId1.i
;
; CHECK: @llvm.dbg.value(metadata i64 [[LID0_V]], metadata [[LID0_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GID_LOC]]
; CHECK: @llvm.dbg.value(metadata i64 [[LID1_V]], metadata [[LID1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GID_LOC]]
; CHECK: @llvm.dbg.value(metadata i64 [[LID2_V]], metadata [[LID2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GID_LOC]]
;
; CHECK: [[GRID0_V:%.*]] = extractelement {{.*}} %globalId2.i
; CHECK: [[GRID1_V:%.*]] = extractelement {{.*}} %globalId2.i
; CHECK: [[GRID2_V:%.*]] = extractelement {{.*}} %globalId2.i
;
; CHECK: @llvm.dbg.value(metadata i64 [[GRID0_V]], metadata [[GRID0_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GID_LOC]]
; CHECK: @llvm.dbg.value(metadata i64 [[GRID1_V]], metadata [[GRID1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GID_LOC]]
; CHECK: @llvm.dbg.value(metadata i64 [[GRID2_V]], metadata [[GRID2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GID_LOC]]
;
; CHECK: [[GIDI0_V:%.*]] = extractelement {{.*}} %globalId.i
;
; CHECK: @llvm.dbg.value(metadata i64 [[GID0_V]], metadata [[GIDI0_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GIDI_LOC:![0-9]*]]
; CHECK: @llvm.dbg.value(metadata i64 [[GID1_V]], metadata [[GIDI1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GIDI_LOC]]
; CHECK: @llvm.dbg.value(metadata i64 [[GID2_V]], metadata [[GIDI2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GIDI_LOC]]
; CHECK: @llvm.dbg.value(metadata i64 [[LID0_V]], metadata [[LIDI0_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GIDI_LOC]]
; CHECK: @llvm.dbg.value(metadata i64 [[LID1_V]], metadata [[LIDI1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GIDI_LOC]]
; CHECK: @llvm.dbg.value(metadata i64 [[LID2_V]], metadata [[LIDI2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GIDI_LOC]]
; CHECK: @llvm.dbg.value(metadata i64 [[GRID0_V]], metadata [[GRIDI0_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GIDI_LOC]]
; CHECK: @llvm.dbg.value(metadata i64 [[GRID1_V]], metadata [[GRIDI1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GIDI_LOC]]
; CHECK: @llvm.dbg.value(metadata i64 [[GRID2_V]], metadata [[GRIDI2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GIDI_LOC]]
;
; CHECK: store i64 [[GIDI0_V]]
; CHECK-SAME: !dbg [[STORE_LOC:![0-9]*]]

define spir_kernel void @__func_b_block_invoke(i8 addrspace(4)* %a) {
entry:
  %0 = alloca i64, !dbg !17
  %globalId.i = call <3 x i64> @_Z26__intel_GlobalInvocationIdv() #0
  %__ocl_dbg_gid0.i = extractelement <3 x i64> %globalId.i, i64 0, !implicitGlobalID !35
  %__ocl_dbg_gid1.i = extractelement <3 x i64> %globalId.i, i64 1, !implicitGlobalID !35
  %__ocl_dbg_gid2.i = extractelement <3 x i64> %globalId.i, i64 2, !implicitGlobalID !35
  call void @llvm.dbg.value(metadata i64 %__ocl_dbg_gid0.i, metadata !36, metadata !DIExpression()), !dbg !38
  call void @llvm.dbg.value(metadata i64 %__ocl_dbg_gid1.i, metadata !39, metadata !DIExpression()), !dbg !38
  call void @llvm.dbg.value(metadata i64 %__ocl_dbg_gid2.i, metadata !40, metadata !DIExpression()), !dbg !38
  %globalId1.i = call <3 x i64> @_Z25__intel_LocalInvocationIdv() #0
  %__ocl_dbg_lid0.i = extractelement <3 x i64> %globalId1.i, i64 0, !implicitGlobalID !35
  %__ocl_dbg_lid1.i = extractelement <3 x i64> %globalId1.i, i64 1, !implicitGlobalID !35
  %__ocl_dbg_lid2.i = extractelement <3 x i64> %globalId1.i, i64 2, !implicitGlobalID !35
  call void @llvm.dbg.value(metadata i64 %__ocl_dbg_lid0.i, metadata !41, metadata !DIExpression()), !dbg !38
  call void @llvm.dbg.value(metadata i64 %__ocl_dbg_lid1.i, metadata !42, metadata !DIExpression()), !dbg !38
  call void @llvm.dbg.value(metadata i64 %__ocl_dbg_lid2.i, metadata !43, metadata !DIExpression()), !dbg !38
  %globalId2.i = call <3 x i64> @_Z19__intel_WorkgroupIdv() #0
  %__ocl_dbg_grid0.i = extractelement <3 x i64> %globalId2.i, i64 0, !implicitGlobalID !35
  %__ocl_dbg_grid1.i = extractelement <3 x i64> %globalId2.i, i64 1, !implicitGlobalID !35
  %__ocl_dbg_grid2.i = extractelement <3 x i64> %globalId2.i, i64 2, !implicitGlobalID !35
  call void @llvm.dbg.value(metadata i64 %__ocl_dbg_grid0.i, metadata !44, metadata !DIExpression()), !dbg !38
  call void @llvm.dbg.value(metadata i64 %__ocl_dbg_grid1.i, metadata !45, metadata !DIExpression()), !dbg !38
  call void @llvm.dbg.value(metadata i64 %__ocl_dbg_grid2.i, metadata !46, metadata !DIExpression()), !dbg !38
  %globalId.i.i = call <3 x i64> @_Z26__intel_GlobalInvocationIdv() #0
  %__ocl_dbg_gid0.i.i = extractelement <3 x i64> %globalId.i.i, i64 0, !implicitGlobalID !35
  %__ocl_dbg_gid1.i.i = extractelement <3 x i64> %globalId.i.i, i64 1, !implicitGlobalID !35
  %__ocl_dbg_gid2.i.i = extractelement <3 x i64> %globalId.i.i, i64 2, !implicitGlobalID !35
  call void @llvm.dbg.value(metadata i64 %__ocl_dbg_gid0.i.i, metadata !47, metadata !DIExpression()), !dbg !48
  call void @llvm.dbg.value(metadata i64 %__ocl_dbg_gid1.i.i, metadata !49, metadata !DIExpression()), !dbg !48
  call void @llvm.dbg.value(metadata i64 %__ocl_dbg_gid2.i.i, metadata !50, metadata !DIExpression()), !dbg !48
  %globalId1.i.i = call <3 x i64> @_Z25__intel_LocalInvocationIdv() #0
  %__ocl_dbg_lid0.i.i = extractelement <3 x i64> %globalId1.i.i, i64 0, !implicitGlobalID !35
  %__ocl_dbg_lid1.i.i = extractelement <3 x i64> %globalId1.i.i, i64 1, !implicitGlobalID !35
  %__ocl_dbg_lid2.i.i = extractelement <3 x i64> %globalId1.i.i, i64 2, !implicitGlobalID !35
  call void @llvm.dbg.value(metadata i64 %__ocl_dbg_lid0.i.i, metadata !51, metadata !DIExpression()), !dbg !48
  call void @llvm.dbg.value(metadata i64 %__ocl_dbg_lid1.i.i, metadata !52, metadata !DIExpression()), !dbg !48
  call void @llvm.dbg.value(metadata i64 %__ocl_dbg_lid2.i.i, metadata !53, metadata !DIExpression()), !dbg !48
  %globalId2.i.i = call <3 x i64> @_Z19__intel_WorkgroupIdv() #0
  %__ocl_dbg_grid0.i.i = extractelement <3 x i64> %globalId2.i.i, i64 0, !implicitGlobalID !35
  %__ocl_dbg_grid1.i.i = extractelement <3 x i64> %globalId2.i.i, i64 1, !implicitGlobalID !35
  %__ocl_dbg_grid2.i.i = extractelement <3 x i64> %globalId2.i.i, i64 2, !implicitGlobalID !35
  call void @llvm.dbg.value(metadata i64 %__ocl_dbg_grid0.i.i, metadata !54, metadata !DIExpression()), !dbg !48
  call void @llvm.dbg.value(metadata i64 %__ocl_dbg_grid1.i.i, metadata !55, metadata !DIExpression()), !dbg !48
  call void @llvm.dbg.value(metadata i64 %__ocl_dbg_grid2.i.i, metadata !56, metadata !DIExpression()), !dbg !48
  store i64 %__ocl_dbg_gid0.i.i, i64* %0, !dbg !57
  %cmp.gid.i.i = icmp uge i64 %__ocl_dbg_gid1.i.i, 13, !dbg !58
  call void @llvm.assume(i1 %cmp.gid.i.i) #0, !dbg !59
  ret void, !dbg !60
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "CleanImplicitIds.ll", directory: "/")
; CHECK-DAG: [[SCOPE_B:![0-9]*]] = distinct !DISubprogram(name: "func_b", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[SCOPE_BLOCK:![0-9]*]] = distinct !DISubprogram(name: "__func_b_block_invoke", scope: null, file: [[FILE]], line: 5
; CHECK-DAG: [[SCOPE_I:![0-9]*]] = distinct !DILexicalBlock(scope: [[SCOPE_BLOCK]], file: [[FILE]], line: 5, column: 76)
; CHECK-DAG: [[INLINE1_LOC:![0-9]*]] = distinct !DILocation(line: 5, column: 3, scope: [[SCOPE_B]])
; CHECK-DAG: [[INLINE2_LOC:![0-9]*]] = distinct !DILocation(line: 5, column: 77, scope: [[SCOPE_I]], inlinedAt: [[INLINE1_LOC]])
; CHECK-DAG: [[GID0_MD]] = !DILocalVariable(name: "__ocl_dbg_gid0", scope: [[SCOPE_BLOCK]], line: 1
; CHECK-DAG: [[GID_LOC]] =  !DILocation(line: 5, scope: [[SCOPE_BLOCK]], inlinedAt: [[INLINE1_LOC]])
; CHECK-DAG: [[GID1_MD]] = !DILocalVariable(name: "__ocl_dbg_gid1", scope: [[SCOPE_BLOCK]], line: 1
; CHECK-DAG: [[GID2_MD]] = !DILocalVariable(name: "__ocl_dbg_gid2", scope: [[SCOPE_BLOCK]], line: 1
; CHECK-DAG: [[LID0_MD]] = !DILocalVariable(name: "__ocl_dbg_lid0", scope: [[SCOPE_BLOCK]], line: 1
; CHECK-DAG: [[LID1_MD]] = !DILocalVariable(name: "__ocl_dbg_lid1", scope: [[SCOPE_BLOCK]], line: 1
; CHECK-DAG: [[LID2_MD]] = !DILocalVariable(name: "__ocl_dbg_lid2", scope: [[SCOPE_BLOCK]], line: 1
; CHECK-DAG: [[GRID0_MD]] = !DILocalVariable(name: "__ocl_dbg_grid0", scope: [[SCOPE_BLOCK]], line: 1
; CHECK-DAG: [[GRID1_MD]] = !DILocalVariable(name: "__ocl_dbg_grid1", scope: [[SCOPE_BLOCK]], line: 1
; CHECK-DAG: [[GRID2_MD]] = !DILocalVariable(name: "__ocl_dbg_grid2", scope: [[SCOPE_BLOCK]], line: 1
; CHECK-DAG: [[GIDI_LOC]] =  !DILocation(line: 1, scope: [[SCOPE_B]], inlinedAt: [[INLINE2_LOC]])
; CHECK_DAG: [[GIDI0_MD]] = !DILocalVariable(name: "__ocl_dbg_gid0", scope: [[SCOPE_B]], line: 1
; CHECK_DAG: [[GIDI1_MD]] = !DILocalVariable(name: "__ocl_dbg_gid1", scope: [[SCOPE_B]], line: 1
; CHECK_DAG: [[GIDI2_MD]] = !DILocalVariable(name: "__ocl_dbg_gid2", scope: [[SCOPE_B]], line: 1
; CHECK_DAG: [[LIDI0_MD]] = !DILocalVariable(name: "__ocl_dbg_lid0", scope: [[SCOPE_B]], line: 1
; CHECK_DAG: [[LIDI1_MD]] = !DILocalVariable(name: "__ocl_dbg_lid1", scope: [[SCOPE_B]], line: 1
; CHECK_DAG: [[LIDI2_MD]] = !DILocalVariable(name: "__ocl_dbg_lid2", scope: [[SCOPE_B]], line: 1
; CHECK_DAG: [[GRIDI0_MD]] = !DILocalVariable(name: "__ocl_dbg_grid0", scope: [[SCOPE_B]], line: 1
; CHECK_DAG: [[GRIDI1_MD]] = !DILocalVariable(name: "__ocl_dbg_grid1", scope: [[SCOPE_B]], line: 1
; CHECK_DAG: [[GRIDI2_MD]] = !DILocalVariable(name: "__ocl_dbg_grid2", scope: [[SCOPE_B]], line: 1
; CHECK_DAG: [[STORE_LOC]] = !DILocation(line: 3, column: 23, scope: [[SCOPE_B]], inlinedAt: [[INLINE2_LOC]])


; Function Attrs: nounwind
declare void @llvm.assume(i1) #2

; Function Attrs: nounwind readnone
declare <3 x i64> @_Z26__intel_GlobalInvocationIdv() #3

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

; Function Attrs: nounwind readnone
declare <3 x i64> @_Z25__intel_LocalInvocationIdv() #3

; Function Attrs: nounwind readnone
declare <3 x i64> @_Z19__intel_WorkgroupIdv() #3

attributes #0 = { nounwind }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { nounwind }
attributes #3 = { nounwind readnone }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4}
!IGCMetadata = !{!5}
!igc.functions = !{}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "/")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{!"ModuleMD", !6}
!6 = !{!"FuncMD", !7, !8}
!7 = !{!"FuncMDMap[0]", void (i8 addrspace(4)*)* @__func_b_block_invoke}
!8 = !{!"FuncMDValue[0]", !9, !10, !11, !12, !15, !16}
!9 = !{!"m_OpenCLArgAddressSpaces"}
!10 = !{!"m_OpenCLArgAccessQualifiers"}
!11 = !{!"m_OpenCLArgTypes"}
!12 = !{!"m_OpenCLArgBaseTypes", !13, !14}
!13 = !{!"m_OpenCLArgBaseTypesVec[0]", !"int*"}
!14 = !{!"m_OpenCLArgBaseTypesVec[1]", !"int"}
!15 = !{!"m_OpenCLArgTypeQualifiers"}
!16 = !{!"m_OpenCLArgNames"}
!17 = !DILocation(line: 3, column: 34, scope: !18, inlinedAt: !28)
!18 = distinct !DISubprogram(name: "func_b", scope: null, file: !19, line: 1, type: !20, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!19 = !DIFile(filename: "CleanImplicitIds.ll", directory: "/")
!20 = !DISubroutineType(types: !21)
!21 = !{!22, !23, !26}
!22 = !DIBasicType(name: "int", size: 4)
!23 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint", file: !24, baseType: !25)
!24 = !DIFile(filename: "header.h", directory: "/")
!25 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!26 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !27, size: 64)
!27 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!28 = distinct !DILocation(line: 5, column: 77, scope: !29, inlinedAt: !34)
!29 = distinct !DILexicalBlock(scope: !30, file: !19, line: 5, column: 76)
!30 = distinct !DISubprogram(name: "__func_b_block_invoke", scope: null, file: !19, line: 5, type: !31, scopeLine: 1, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!31 = !DISubroutineType(types: !32)
!32 = !{!22, !33}
!33 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !22, size: 64)
!34 = distinct !DILocation(line: 5, column: 3, scope: !18)
!35 = !{null}
!36 = !DILocalVariable(name: "__ocl_dbg_gid0", scope: !30, line: 1, type: !37)
!37 = !DIBasicType(name: "long long", size: 64, encoding: DW_ATE_signed)
!38 = !DILocation(line: 5, scope: !30, inlinedAt: !34)
!39 = !DILocalVariable(name: "__ocl_dbg_gid1", scope: !30, line: 1, type: !37)
!40 = !DILocalVariable(name: "__ocl_dbg_gid2", scope: !30, line: 1, type: !37)
!41 = !DILocalVariable(name: "__ocl_dbg_lid0", scope: !30, line: 1, type: !37)
!42 = !DILocalVariable(name: "__ocl_dbg_lid1", scope: !30, line: 1, type: !37)
!43 = !DILocalVariable(name: "__ocl_dbg_lid2", scope: !30, line: 1, type: !37)
!44 = !DILocalVariable(name: "__ocl_dbg_grid0", scope: !30, line: 1, type: !37)
!45 = !DILocalVariable(name: "__ocl_dbg_grid1", scope: !30, line: 1, type: !37)
!46 = !DILocalVariable(name: "__ocl_dbg_grid2", scope: !30, line: 1, type: !37)
!47 = !DILocalVariable(name: "__ocl_dbg_gid0", scope: !18, line: 1, type: !37)
!48 = !DILocation(line: 1, scope: !18, inlinedAt: !28)
!49 = !DILocalVariable(name: "__ocl_dbg_gid1", scope: !18, line: 1, type: !37)
!50 = !DILocalVariable(name: "__ocl_dbg_gid2", scope: !18, line: 1, type: !37)
!51 = !DILocalVariable(name: "__ocl_dbg_lid0", scope: !18, line: 1, type: !37)
!52 = !DILocalVariable(name: "__ocl_dbg_lid1", scope: !18, line: 1, type: !37)
!53 = !DILocalVariable(name: "__ocl_dbg_lid2", scope: !18, line: 1, type: !37)
!54 = !DILocalVariable(name: "__ocl_dbg_grid0", scope: !18, line: 1, type: !37)
!55 = !DILocalVariable(name: "__ocl_dbg_grid1", scope: !18, line: 1, type: !37)
!56 = !DILocalVariable(name: "__ocl_dbg_grid2", scope: !18, line: 1, type: !37)
!57 = !DILocation(line: 3, column: 23, scope: !18, inlinedAt: !28)
!58 = !DILocation(line: 5, column: 75, scope: !18, inlinedAt: !28)
!59 = !DILocation(line: 5, column: 3, scope: !18, inlinedAt: !28)
!60 = !DILocation(line: 5, column: 3, scope: !18)
