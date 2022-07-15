;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-resolve-inline-locals -S < %s | FileCheck %s
; ------------------------------------------------
; InlineLocalsResolution
; ------------------------------------------------
; This test checks that InlineLocalsResolution pass follows
; 'How to Update Debug Info' llvm guideline.
;
; Reduced from OCL kernel, with changed global variable @a addrspace:
;
; static __global int* a;
; static __global int* b;
; __kernel void test_inline(global int * table)
; {
;  int wg = work_group_any(table[0]);
;  a[0] = wg;
;  b[0] = wg;
; }
;
; ------------------------------------------------

; CHECK: void @test_inline{{.*}}!dbg [[SCOPE:![0-9]*]]
; Value debug info made from global variable dbg
; CHECK: void @llvm.dbg.declare(metadata i32 addrspace(3)* addrspace(3)* @a, metadata [[A_MD:![0-9]*]], metadata !DIExpression()), !dbg [[FIRST_LOC:![0-9]*]]
; CHECK: call void @llvm.genx.GenISA.CatchAllDebugLine(), !dbg [[FIRST_LOC]]
; CHECK: [[ALLOCTB_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[ALLOCTB_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 addrspace(1)** [[ALLOCTB_V]], metadata [[ALLOCTB_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ALLOCTB_LOC]]
; CHECK: [[ALLOCWG_V:%[A-z0-9]*]] = {{.*}}, !dbg [[ALLOCWG_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32* [[ALLOCWG_V]], metadata [[ALLOCWG_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ALLOCWG_LOC]]
; CHECK: store{{.*}}, !dbg [[STORE1_LOC:![0-9]*]]
; CHECK: [[LOAD1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[LOAD1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 addrspace(1)* [[LOAD1_V]], metadata [[LOAD1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD1_LOC]]
; CHECK: [[GEP1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[GEP1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 addrspace(1)* [[GEP1_V]], metadata [[GEP1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[GEP1_LOC]]
; CHECK: [[LOAD2_V:%[A-z0-9]*]] = {{.*}}, !dbg [[LOAD2_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 [[LOAD2_V]], metadata [[LOAD2_MD:![0-9]*]], metadata !DIExpression()), !dbg [[LOAD2_LOC]]
; CHECK: [[ICMP1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[ICMP1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i1 [[ICMP1_V]], metadata [[ICMP1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[ICMP1_LOC]]
; CHECK: [[CALL1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[CALL1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i8 addrspace(3)* [[CALL1_V]], metadata [[CALL1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CALL1_LOC]]
; CHECK: [[BCAST1_V:%[A-z0-9]*]] = {{.*}}, !dbg [[BCAST1_LOC:![0-9]*]]
; CHECK: void @llvm.dbg.value(metadata i32 addrspace(3)* [[BCAST1_V]], metadata [[BCAST1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[BCAST1_LOC]]
; CHECK: store{{.*}}, !dbg [[STORE2_LOC:![0-9]*]]

@a = internal addrspace(3) global i32 addrspace(3)* null, align 8, !dbg !0
@b = internal addrspace(1) global i32 addrspace(1)* null, align 8, !dbg !6

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_inline(i32 addrspace(1)* %table, <8 x i32> %r0, <8 x i32> %payloadHeader, i8 addrspace(1)* %globalBase, i8* %privateBase, i32 %bufferOffset) #0 !dbg !107 {
entry:
  call void @llvm.genx.GenISA.CatchAllDebugLine(), !dbg !133
  %table.addr = alloca i32 addrspace(1)*, align 8, !dbg !134
  call void @llvm.dbg.value(metadata i32 addrspace(1)** %table.addr, metadata !110, metadata !DIExpression()), !dbg !134
  %wg = alloca i32, align 4, !dbg !135
  call void @llvm.dbg.value(metadata i32* %wg, metadata !112, metadata !DIExpression()), !dbg !135
  store i32 addrspace(1)* %table, i32 addrspace(1)** %table.addr, align 8, !dbg !136
  %0 = load i32 addrspace(1)*, i32 addrspace(1)** %table.addr, align 8, !dbg !137
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %0, metadata !113, metadata !DIExpression()), !dbg !137
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 0, !dbg !138
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %arrayidx, metadata !114, metadata !DIExpression()), !dbg !138
  %1 = load i32, i32 addrspace(1)* %arrayidx, align 4, !dbg !139
  call void @llvm.dbg.value(metadata i32 %1, metadata !115, metadata !DIExpression()), !dbg !139
  %2 = icmp ne i32 %1, 0, !dbg !140
  call void @llvm.dbg.value(metadata i1 %2, metadata !117, metadata !DIExpression()), !dbg !140
  %call.i = call spir_func i8 addrspace(3)* @__builtin_IB_AllocLocalMemPool(i1 zeroext false, i32 1, i32 4) #5, !dbg !141
  call void @llvm.dbg.value(metadata i8 addrspace(3)* %call.i, metadata !119, metadata !DIExpression()), !dbg !141
  %3 = bitcast i8 addrspace(3)* %call.i to i32 addrspace(3)*, !dbg !142
  call void @llvm.dbg.value(metadata i32 addrspace(3)* %3, metadata !120, metadata !DIExpression()), !dbg !142
  store i32 0, i32 addrspace(3)* %3, align 4, !dbg !143, !tbaa !144
  call spir_func void @__builtin_IB_memfence(i1 zeroext true, i1 zeroext true, i1 zeroext false, i1 zeroext false, i1 zeroext false, i1 zeroext false, i1 zeroext true) #5, !dbg !148
  call spir_func void @__builtin_IB_thread_group_barrier() #5, !dbg !149
  %conv.i = zext i1 %2 to i32, !dbg !150
  call void @llvm.dbg.value(metadata i32 %conv.i, metadata !121, metadata !DIExpression()), !dbg !150
  %PtrDstToInt = ptrtoint i32 addrspace(3)* %3 to i32, !dbg !151
  call void @llvm.dbg.value(metadata i32 %PtrDstToInt, metadata !122, metadata !DIExpression()), !dbg !151
  %call.i.i1 = call i32 @llvm.genx.GenISA.intatomicraw.i32.p3i32.i32(i32 addrspace(3)* %3, i32 %PtrDstToInt, i32 %conv.i, i32 9), !dbg !152
  call void @llvm.dbg.value(metadata i32 %call.i.i1, metadata !123, metadata !DIExpression()), !dbg !152
  call spir_func void @__builtin_IB_memfence(i1 zeroext true, i1 zeroext true, i1 zeroext false, i1 zeroext false, i1 zeroext false, i1 zeroext false, i1 zeroext true) #5, !dbg !153
  call spir_func void @__builtin_IB_thread_group_barrier() #5, !dbg !154
  %4 = load i32, i32 addrspace(3)* %3, align 4, !dbg !155, !tbaa !144
  call void @llvm.dbg.value(metadata i32 %4, metadata !124, metadata !DIExpression()), !dbg !155
  %tobool4.i = icmp ne i32 %4, 0, !dbg !156
  call void @llvm.dbg.value(metadata i1 %tobool4.i, metadata !125, metadata !DIExpression()), !dbg !156
  %call.old = select i1 %tobool4.i, i32 1, i32 0, !dbg !157
  call void @llvm.dbg.value(metadata i32 %call.old, metadata !126, metadata !DIExpression()), !dbg !157
  store i32 %call.old, i32* %wg, align 4, !dbg !158
  %5 = load i32, i32* %wg, align 4, !dbg !159
  call void @llvm.dbg.value(metadata i32 %5, metadata !127, metadata !DIExpression()), !dbg !159
  %6 = load i32 addrspace(3)*, i32 addrspace(3)* addrspace(3)* @a, align 8, !dbg !160
  call void @llvm.dbg.value(metadata i32 addrspace(3)* %6, metadata !128, metadata !DIExpression()), !dbg !160
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(3)* %6, i64 0, !dbg !161
  call void @llvm.dbg.value(metadata i32 addrspace(3)* %arrayidx1, metadata !129, metadata !DIExpression()), !dbg !161
  store i32 %5, i32 addrspace(3)* %arrayidx1, align 4, !dbg !162
  %7 = load i32, i32* %wg, align 4, !dbg !163
  call void @llvm.dbg.value(metadata i32 %7, metadata !130, metadata !DIExpression()), !dbg !163
  %8 = load i32 addrspace(1)*, i32 addrspace(1)* addrspace(1)* @b, align 8, !dbg !164
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %8, metadata !131, metadata !DIExpression()), !dbg !164
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %8, i64 0, !dbg !165
  call void @llvm.dbg.value(metadata i32 addrspace(1)* %arrayidx2, metadata !132, metadata !DIExpression()), !dbg !165
  store i32 %7, i32 addrspace(1)* %arrayidx2, align 4, !dbg !166
  ret void, !dbg !167
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "basic.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_inline", linkageName: "test_inline", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[A_MD]] = !DILocalVariable(name: "a", scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[FIRST_LOC]] = !DILocation(line: 1, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ALLOCTB_MD]] = !DILocalVariable(name: "1", scope: [[SCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[ALLOCTB_LOC]] = !DILocation(line: 2, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ALLOCWG_MD]] = !DILocalVariable(name: "2", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[ALLOCWG_LOC]] = !DILocation(line: 3, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE1_LOC]] = !DILocation(line: 4, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD1_MD]] = !DILocalVariable(name: "3", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[LOAD1_LOC]] = !DILocation(line: 5, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[GEP1_MD]] = !DILocalVariable(name: "4", scope: [[SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[GEP1_LOC]] = !DILocation(line: 6, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[LOAD2_MD]] = !DILocalVariable(name: "5", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[LOAD2_LOC]] = !DILocation(line: 7, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[ICMP1_MD]] = !DILocalVariable(name: "6", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[ICMP1_LOC]] = !DILocation(line: 8, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[CALL1_MD]] = !DILocalVariable(name: "7", scope: [[SCOPE]], file: [[FILE]], line: 9
; CHECK-DAG: [[CALL1_LOC]] = !DILocation(line: 9, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[BCAST1_MD]] = !DILocalVariable(name: "8", scope: [[SCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[BCAST1_LOC]] = !DILocation(line: 10, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[STORE2_LOC]] = !DILocation(line: 11, column: 1, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: convergent
declare spir_func i8 addrspace(3)* @__builtin_IB_AllocLocalMemPool(i1 zeroext, i32, i32) local_unnamed_addr #2

; Function Attrs: convergent
declare spir_func void @__builtin_IB_memfence(i1 zeroext, i1 zeroext, i1 zeroext, i1 zeroext, i1 zeroext, i1 zeroext, i1 zeroext) local_unnamed_addr #2

; Function Attrs: convergent
declare spir_func void @__builtin_IB_thread_group_barrier() local_unnamed_addr #2

; Function Attrs: convergent
declare spir_func i32 @__builtin_IB_atomic_or_local_i32(i32 addrspace(3)*, i32) local_unnamed_addr #2

; Function Attrs: nounwind
declare void @llvm.genx.GenISA.CatchAllDebugLine() #3

; Function Attrs: argmemonly nounwind
declare i32 @llvm.genx.GenISA.intatomicraw.i32.p3i32.i32(i32 addrspace(3)*, i32, i32, i32) #4

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { convergent "correctly-rounded-divide-sqrt-fp-math"="false" "denorms-are-zero"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }
attributes #4 = { argmemonly nounwind }
attributes #5 = { convergent nounwind }

!llvm.module.flags = !{!10, !11, !12}
!IGCMetadata = !{!13}
!igc.functions = !{!93}
!opencl.ocl.version = !{!103, !103, !103, !103, !103}
!opencl.spir.version = !{!103, !103, !103, !103, !103}
!llvm.dbg.cu = !{!104}
!llvm.debugify = !{!105, !106}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "a", scope: !2, file: !3, line: 1, type: !8, isLocal: true, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, globals: !5)
!3 = !DIFile(filename: "basic.ll", directory: "/")
!4 = !{}
!5 = !{!0, !6}
!6 = !DIGlobalVariableExpression(var: !7, expr: !DIExpression())
!7 = distinct !DIGlobalVariable(name: "b", scope: !2, file: !3, line: 2, type: !8, isLocal: true, isDefinition: true)
!8 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !9, size: 64)
!9 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!10 = !{i32 2, !"Dwarf Version", i32 4}
!11 = !{i32 2, !"Debug Info Version", i32 3}
!12 = !{i32 1, !"wchar_size", i32 4}
!13 = !{!"ModuleMD", !14, !15, !17, !51, !78, !79, !80, !82, !83, !84, !85, !86, !87, !89, !90, !91, !92}
!14 = !{!"isPrecise", i1 false}
!15 = !{!"compOpt", !16}
!16 = !{!"OptDisable", i1 true}
!17 = !{!"FuncMD", !18, !19}
!18 = distinct !{!"FuncMDMap[0]", void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8 addrspace(1)*, i8*, i32)* @test_inline}
!19 = !{!"FuncMDValue[0]", !20, !21, !25, !26, !27, !28, !34, !35, !36, !37, !38, !39, !40, !41, !42, !43, !44, !45, !46, !47, !48, !49, !50}
!20 = !{!"localOffsets"}
!21 = !{!"workGroupWalkOrder", !22, !23, !24}
!22 = !{!"dim0", i32 0}
!23 = !{!"dim1", i32 0}
!24 = !{!"dim2", i32 0}
!25 = !{!"funcArgs"}
!26 = !{!"functionType", !"KernelFunction"}
!27 = !{!"callableShaderType", !"NumberOfCallableShaderTypes"}
!28 = !{!"resAllocMD", !29, !30, !31, !32, !33}
!29 = !{!"uavsNumType", i32 0}
!30 = !{!"srvsNumType", i32 0}
!31 = !{!"samplersNumType", i32 0}
!32 = !{!"argAllocMDList"}
!33 = !{!"inlineSamplersMD"}
!34 = !{!"maxByteOffsets"}
!35 = !{!"IsInitializer", i1 false}
!36 = !{!"IsFinalizer", i1 false}
!37 = !{!"CompiledSubGroupsNumber", i32 0}
!38 = !{!"isCloned", i1 false}
!39 = !{!"localSize", i32 0}
!40 = !{!"localIDPresent", i1 false}
!41 = !{!"groupIDPresent", i1 false}
!42 = !{!"globalIDPresent", i1 false}
!43 = !{!"isUniqueEntry", i1 false}
!44 = !{!"UserAnnotations"}
!45 = !{!"m_OpenCLArgAddressSpaces"}
!46 = !{!"m_OpenCLArgAccessQualifiers"}
!47 = !{!"m_OpenCLArgTypes"}
!48 = !{!"m_OpenCLArgBaseTypes"}
!49 = !{!"m_OpenCLArgTypeQualifiers"}
!50 = !{!"m_OpenCLArgNames"}
!51 = !{!"pushInfo", !52, !53, !56, !57, !58, !59, !60, !61, !62, !63, !75, !76, !77}
!52 = !{!"pushableAddresses"}
!53 = !{!"dynamicBufferInfo", !54, !55}
!54 = !{!"firstIndex", i32 0}
!55 = !{!"numOffsets", i32 0}
!56 = !{!"MaxNumberOfPushedBuffers", i32 0}
!57 = !{!"inlineConstantBufferSlot", i32 -1}
!58 = !{!"inlineConstantBufferOffset", i32 -1}
!59 = !{!"inlineConstantBufferGRFOffset", i32 -1}
!60 = !{!"constants"}
!61 = !{!"inputs"}
!62 = !{!"constantReg"}
!63 = !{!"simplePushInfoArr", !64, !72, !73, !74}
!64 = !{!"simplePushInfoArrVec[0]", !65, !66, !67, !68, !69, !70, !71}
!65 = !{!"cbIdx", i32 0}
!66 = !{!"pushableAddressGrfOffset", i32 -1}
!67 = !{!"pushableOffsetGrfOffset", i32 -1}
!68 = !{!"offset", i32 0}
!69 = !{!"size", i32 0}
!70 = !{!"isStateless", i1 false}
!71 = !{!"simplePushLoads"}
!72 = !{!"simplePushInfoArrVec[1]", !65, !66, !67, !68, !69, !70, !71}
!73 = !{!"simplePushInfoArrVec[2]", !65, !66, !67, !68, !69, !70, !71}
!74 = !{!"simplePushInfoArrVec[3]", !65, !66, !67, !68, !69, !70, !71}
!75 = !{!"simplePushBufferUsed", i32 0}
!76 = !{!"pushAnalysisWIInfos"}
!77 = !{!"inlineRTGlobalPtrOffset", i32 0}
!78 = !{!"inlineDynConstants"}
!79 = !{!"inlineDynTextures"}
!80 = !{!"immConstant", !81}
!81 = !{!"data"}
!82 = !{!"inlineConstantBuffers"}
!83 = !{!"inlineGlobalBuffers"}
!84 = !{!"GlobalPointerProgramBinaryInfos"}
!85 = !{!"ConstantPointerProgramBinaryInfos"}
!86 = !{!"inlineProgramScopeOffsets"}
!87 = !{!"shaderData", !88}
!88 = !{!"numReplicas", i32 0}
!89 = !{!"UseBindlessImage", i1 false}
!90 = !{!"enableRangeReduce", i1 false}
!91 = !{!"statefullResourcesNotAliased", i1 false}
!92 = !{!"disableMixMode", i1 false}
!93 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8 addrspace(1)*, i8*, i32)* @test_inline, !94}
!94 = !{!95, !96}
!95 = !{!"function_type", i32 0}
!96 = !{!"implicit_arg_desc", !97, !98, !99, !100, !101}
!97 = !{i32 0}
!98 = !{i32 1}
!99 = !{i32 11}
!100 = !{i32 12}
!101 = !{i32 14, !102}
!102 = !{!"explicit_arg_num", i32 0}
!103 = !{i32 2, i32 0}
!104 = distinct !DICompileUnit(language: DW_LANG_C, file: !3, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !4)
!105 = !{i32 31}
!106 = !{i32 20}
!107 = distinct !DISubprogram(name: "test_inline", linkageName: "test_inline", scope: null, file: !3, line: 1, type: !108, scopeLine: 1, unit: !104, retainedNodes: !109)
!108 = !DISubroutineType(types: !4)
!109 = !{!110, !112, !113, !114, !115, !117, !119, !120, !121, !122, !123, !124, !125, !126, !127, !128, !129, !130, !131, !132}
!110 = !DILocalVariable(name: "1", scope: !107, file: !3, line: 2, type: !111)
!111 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!112 = !DILocalVariable(name: "2", scope: !107, file: !3, line: 3, type: !111)
!113 = !DILocalVariable(name: "3", scope: !107, file: !3, line: 5, type: !111)
!114 = !DILocalVariable(name: "4", scope: !107, file: !3, line: 6, type: !111)
!115 = !DILocalVariable(name: "5", scope: !107, file: !3, line: 7, type: !116)
!116 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!117 = !DILocalVariable(name: "6", scope: !107, file: !3, line: 8, type: !118)
!118 = !DIBasicType(name: "ty8", size: 8, encoding: DW_ATE_unsigned)
!119 = !DILocalVariable(name: "7", scope: !107, file: !3, line: 9, type: !111)
!120 = !DILocalVariable(name: "8", scope: !107, file: !3, line: 10, type: !111)
!121 = !DILocalVariable(name: "9", scope: !107, file: !3, line: 14, type: !116)
!122 = !DILocalVariable(name: "10", scope: !107, file: !3, line: 15, type: !116)
!123 = !DILocalVariable(name: "11", scope: !107, file: !3, line: 16, type: !116)
!124 = !DILocalVariable(name: "12", scope: !107, file: !3, line: 19, type: !116)
!125 = !DILocalVariable(name: "13", scope: !107, file: !3, line: 20, type: !118)
!126 = !DILocalVariable(name: "14", scope: !107, file: !3, line: 21, type: !116)
!127 = !DILocalVariable(name: "15", scope: !107, file: !3, line: 23, type: !116)
!128 = !DILocalVariable(name: "16", scope: !107, file: !3, line: 24, type: !111)
!129 = !DILocalVariable(name: "17", scope: !107, file: !3, line: 25, type: !111)
!130 = !DILocalVariable(name: "18", scope: !107, file: !3, line: 27, type: !116)
!131 = !DILocalVariable(name: "19", scope: !107, file: !3, line: 28, type: !111)
!132 = !DILocalVariable(name: "20", scope: !107, file: !3, line: 29, type: !111)
!133 = !DILocation(line: 1, column: 1, scope: !107)
!134 = !DILocation(line: 2, column: 1, scope: !107)
!135 = !DILocation(line: 3, column: 1, scope: !107)
!136 = !DILocation(line: 4, column: 1, scope: !107)
!137 = !DILocation(line: 5, column: 1, scope: !107)
!138 = !DILocation(line: 6, column: 1, scope: !107)
!139 = !DILocation(line: 7, column: 1, scope: !107)
!140 = !DILocation(line: 8, column: 1, scope: !107)
!141 = !DILocation(line: 9, column: 1, scope: !107)
!142 = !DILocation(line: 10, column: 1, scope: !107)
!143 = !DILocation(line: 11, column: 1, scope: !107)
!144 = !{!145, !145, i64 0}
!145 = !{!"int", !146, i64 0}
!146 = !{!"omnipotent char", !147, i64 0}
!147 = !{!"Simple C/C++ TBAA"}
!148 = !DILocation(line: 12, column: 1, scope: !107)
!149 = !DILocation(line: 13, column: 1, scope: !107)
!150 = !DILocation(line: 14, column: 1, scope: !107)
!151 = !DILocation(line: 15, column: 1, scope: !107)
!152 = !DILocation(line: 16, column: 1, scope: !107)
!153 = !DILocation(line: 17, column: 1, scope: !107)
!154 = !DILocation(line: 18, column: 1, scope: !107)
!155 = !DILocation(line: 19, column: 1, scope: !107)
!156 = !DILocation(line: 20, column: 1, scope: !107)
!157 = !DILocation(line: 21, column: 1, scope: !107)
!158 = !DILocation(line: 22, column: 1, scope: !107)
!159 = !DILocation(line: 23, column: 1, scope: !107)
!160 = !DILocation(line: 24, column: 1, scope: !107)
!161 = !DILocation(line: 25, column: 1, scope: !107)
!162 = !DILocation(line: 26, column: 1, scope: !107)
!163 = !DILocation(line: 27, column: 1, scope: !107)
!164 = !DILocation(line: 28, column: 1, scope: !107)
!165 = !DILocation(line: 29, column: 1, scope: !107)
!166 = !DILocation(line: 30, column: 1, scope: !107)
!167 = !DILocation(line: 31, column: 1, scope: !107)
