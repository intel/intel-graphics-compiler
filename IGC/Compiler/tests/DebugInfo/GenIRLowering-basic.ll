;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-gen-ir-lowering -S < %s | FileCheck %s
; ------------------------------------------------
; GenIRLowering
; ------------------------------------------------
; This test checks that GenIRLowering pass follows
; 'How to Update Debug Info' llvm guideline.
;
; And was reduced from ocl test kernel:
;
;  __kernel void test_lowering( __global float *src1, __global float *src2, __global float *dest )
;  {
;     size_t i = get_global_id(0);
;     float a = src1[i];
;     float b = src2[i];
;     float c = (a > b) ? a : b;
;     float con_a = 5.0;
;     float con_b = 10.0;
;     float con_c = (con_b > con_a) ? con_b : con_a;
;     float d = (c < con_c) ? c : con_c;
;     float e = (d > 0.5) ? d : 0.5;
;     float f = (e > 0.5) ? e : 0.5;
;     float g = (f < 10.0) ? f : 10.0;
;     float h = (g < 1.0) ? g : 1.0;
;     float j = (h > 0.0) ? h : 0.0;
;     dest[i] = j;
;  }
;
; ------------------------------------------------

; COM: Sanity checks - DebugInfo is not lost or changed on non-affected IR:
; Metadata:
; CHECK-DAG: [[SUBPROGRAM:![0-9]+]] = distinct !DISubprogram(name: "test_lowering"
; CHECK-DAG: [[SRC1:![0-9]+]] = !DILocalVariable(name: "src1"
; CHECK-DAG: [[SRC2:![0-9]+]] = !DILocalVariable(name: "src2", arg: 2
; CHECK-DAG: [[DEST:![0-9]+]] = !DILocalVariable(name: "dest", arg: 3
; CHECK-DAG: [[LINE0:![0-9]+]] = !DILocation(line: 0, scope: [[SUBPROGRAM]])
; CHECK-DAG: [[LINE4:![0-9]+]] = !DILocation(line: 4, column: 14, scope: [[SUBPROGRAM]])
; Ir:
; CHECK-DAG: void @test_lowering{{.*}} #0 !dbg [[SUBPROGRAM]]
; CHECK-DAG: [[DBG_VALUE_CALL:dbg.value\(metadata]] [[ADDR_SPACE:float addrspace\(1\)\*]] %src1,  metadata [[SRC1]], {{.*}} !dbg [[LINE0]]
; CHECK-DAG: [[DBG_VALUE_CALL]] [[ADDR_SPACE]] %src2, metadata [[SRC2]], {{.*}} !dbg !29
; CHECK-DAG: [[DBG_VALUE_CALL]] [[ADDR_SPACE]] %dest, metadata [[DEST]], {{.*}} !dbg !29
; CHECK-DAG: {{%[0-9]*}} = load {{.*}} %arrayidx, align 4, !dbg [[LINE4]]
define spir_kernel void @test_lowering(float addrspace(1)* %src1, float addrspace(1)* %src2, float addrspace(1)* %dest, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %enqueuedLocalSize, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 !dbg !21 {
entry:
  %scalar27 = extractelement <8 x i32> %payloadHeader, i32 0
  %scalar24 = extractelement <3 x i32> %enqueuedLocalSize, i32 0
  %scalar17 = extractelement <8 x i32> %r0, i32 1
  call void @llvm.dbg.value(metadata float addrspace(1)* %src1, metadata !28, metadata !DIExpression()), !dbg !29
  call void @llvm.dbg.value(metadata float addrspace(1)* %src2, metadata !30, metadata !DIExpression()), !dbg !29
  call void @llvm.dbg.value(metadata float addrspace(1)* %dest, metadata !31, metadata !DIExpression()), !dbg !29
  %v.0.i.i.i = zext i16 %localIdX to i64
  %mul.i.i = mul i32 %scalar24, %scalar17
  %conv.i.i = zext i32 %mul.i.i to i64
  %add.i.i = add nuw nsw i64 %v.0.i.i.i, %conv.i.i
  %conv4.i.i = zext i32 %scalar27 to i64
  %add5.i.i = add nuw nsw i64 %add.i.i, %conv4.i.i
  call void @llvm.dbg.value(metadata i64 %add5.i.i, metadata !32, metadata !DIExpression()), !dbg !29
  %arrayidx = getelementptr inbounds float, float addrspace(1)* %src1, i64 %add5.i.i, !dbg !36
  %0 = load float, float addrspace(1)* %arrayidx, align 4, !dbg !36
  call void @llvm.dbg.value(metadata float %0, metadata !37, metadata !DIExpression()), !dbg !29
  %arrayidx1 = getelementptr inbounds float, float addrspace(1)* %src2, i64 %add5.i.i, !dbg !38
  %1 = load float, float addrspace(1)* %arrayidx1, align 4, !dbg !38
  call void @llvm.dbg.value(metadata float %1, metadata !39, metadata !DIExpression()), !dbg !29

; Testcase 1:
; Select replaced by maxnum call
; Cmp is preserved though its value is not used and its debuginfo
; may be lost on other passes.
; Info is preserved on maxnum and dependent dbg.values call
;
; MetaData:
; CHECK-DAG: [[LINE6C17:![0-9]+]] = !DILocation(line: 6, column: 17, scope: [[SUBPROGRAM]])
; CHECK-DAG: [[LINE6C14:![0-9]+]] = !DILocation(line: 6, column: 14, scope: [[SUBPROGRAM]])
; CHECK-DAG: [[C:![0-9]+]] = !DILocalVariable(name: "c"
; Ir:
; CHECK-DAG: %cmp = fcmp ogt float %0, %1, !dbg [[LINE6C17]]
; CHECK-DAG: [[V0:%[0-9]*]] = [[CALL_MAXN:call float @llvm.maxnum.f32\(]]{{.*}} !dbg [[LINE6C14]]
; CHECK-DAG: [[DBG_VALUE_CALL]] float [[V0]], metadata [[C]]

  %cmp = fcmp ogt float %0, %1, !dbg !40
  %. = select i1 %cmp, float %0, float %1, !dbg !41
  call void @llvm.dbg.value(metadata float %., metadata !42, metadata !DIExpression()), !dbg !29
  call void @llvm.dbg.value(metadata float 5.000000e+00, metadata !43, metadata !DIExpression()), !dbg !29

; Testcase 2:
; maxnum call with constants is folded, and its line info is discarded
; but dbg.value call is preserved
; CHECK-DAG: [[CON_C:![0-9]+]] = !DILocalVariable(name: "con_c"
; CHECK-DAG: [[DBG_VALUE_CALL]] float 1.000000e+01, metadata [[CON_C]]

  %con_c = call float @llvm.maxnum.f32(float 10.0, float 5.0), !dbg !44
  call void @llvm.dbg.value(metadata float %con_c, metadata !45, metadata !DIExpression()), !dbg !29

; Testcase 3:
; Nested clamp with same values is folded and correct line info is preserved
; MetaData:
; CHECK-DAG: [[LINE10:![0-9]+]] = !DILocation(line: 10, column: 14, scope: [[SUBPROGRAM]])
; CHECK-DAG: [[LINE11:![0-9]+]] = !DILocation(line: 11, column: 14, scope: [[SUBPROGRAM]])
; CHECK-DAG: [[D:![0-9]+]] = !DILocalVariable(name: "d"
; CHECK-DAG: [[E:![0-9]+]] = !DILocalVariable(name: "e"
; CHECK-DAG: [[G:![0-9]+]] = !DILocalVariable(name: "g"
; Ir:
; CHECK-DAG: [[V_D:%[0-9]+]] = [[CALL_MINN:call float @llvm.minnum.f32\(]]{{.*}} !dbg [[LINE10]]
; CHECK-DAG: [[DBG_VALUE_CALL]] float [[V_D]], metadata [[D]]
; CHECK-DAG: [[V_EG:%[0-9]+]] = [[CALL_MAXN]]{{.*}}, !dbg [[LINE11]]
; CHECK-DAG: [[DBG_VALUE_CALL]] float [[V_EG]], metadata [[E]]
; CHECK-DAG: [[DBG_VALUE_CALL]] float [[V_EG]], metadata [[G]]

  %cmp7 = fcmp olt float %., %con_c, !dbg !46
  %cond11 = select i1 %cmp7, float %., float %con_c, !dbg !47
  call void @llvm.dbg.value(metadata float %cond11, metadata !48, metadata !DIExpression()), !dbg !29
  %cmp12 = fcmp ogt float %cond11, 5.000000e-01, !dbg !49
  %cond26 = select i1 %cmp12, float %cond11, float 5.000000e-01, !dbg !50
  call void @llvm.dbg.value(metadata float %cond26, metadata !51, metadata !DIExpression()), !dbg !29
  %cmp13 = fcmp ogt float %cond26, 5.000000e-01, !dbg !52
  %cond27 = select i1 %cmp13, float %cond26, float 5.000000e-01, !dbg !53
  call void @llvm.dbg.value(metadata float %cond27, metadata !54, metadata !DIExpression()), !dbg !29
  %cmp30 = fcmp olt float %cond27, %con_c, !dbg !55
  %cond27. = select i1 %cmp30, float %cond27, float %con_c, !dbg !56
  call void @llvm.dbg.value(metadata float %cond27., metadata !57, metadata !DIExpression()), !dbg !29
  %cmp39 = fcmp olt float %cond27., 1.000000e+00, !dbg !58
  %cond45 = select i1 %cmp39, float %cond27., float 1.000000e+00, !dbg !59
  call void @llvm.dbg.value(metadata float %cond45, metadata !60, metadata !DIExpression()), !dbg !29

; Testcase 4:
; Clamp with [1,0] is folded to fsat
; MetaData:
; CHECK-DAG: [[LINE15:![0-9]+]] = !DILocation(line: 15, column: 14, scope: [[SUBPROGRAM]])
; Ir:
; CHECK-DAG: call float @llvm.genx.GenISA.fsat.f32(float [[V_EG]]), !dbg [[LINE15]]
  %cmp48 = fcmp ogt float %cond45, 0.000000e+00, !dbg !61
  %cond45. = select i1 %cmp48, float %cond45, float 0.000000e+00, !dbg !62
  call void @llvm.dbg.value(metadata float %cond45., metadata !63, metadata !DIExpression()), !dbg !29
  %arrayidx56 = getelementptr inbounds float, float addrspace(1)* %dest, i64 %add5.i.i, !dbg !64
  store float %cond45., float addrspace(1)* %arrayidx56, align 4, !dbg !65
  ret void, !dbg !66
}

; Function Attrs: nounwind readnone speculatable
declare float @llvm.maxnum.f32(float, float) #1

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

attributes #0 = { alwaysinline convergent nounwind "no-nans-fp-math"="true" }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!IGCMetadata = !{!6}
!igc.functions = !{!9}
!opencl.ocl.version = !{!19, !19, !19, !19, !19}
!opencl.spir.version = !{!19, !19, !19, !19, !19}
!llvm.ident = !{!20, !20, !20, !20, !20}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "/somepath1/.")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{!"ModuleMD", !7}
!7 = !{!"compOpt", !8}
!8 = !{!"FiniteMathOnly", i1 true}
!9 = !{void (float addrspace(1)*, float addrspace(1)*, float addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i16, i16, i16)* @test_lowering, !10}
!10 = !{!11, !12}
!11 = !{!"function_type", i32 0}
!12 = !{!"implicit_arg_desc", !13, !14, !15, !16, !17, !18}
!13 = !{i32 0}
!14 = !{i32 1}
!15 = !{i32 7}
!16 = !{i32 8}
!17 = !{i32 9}
!18 = !{i32 10}
!19 = !{i32 2, i32 0}
!20 = !{!"clang version 10.0.0"}
!21 = distinct !DISubprogram(name: "test_lowering", scope: null, file: !22, line: 1, type: !23, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!22 = !DIFile(filename: "1", directory: "/somepath2/")
!23 = !DISubroutineType(types: !24)
!24 = !{!25, !26, !26, !26}
!25 = !DIBasicType(name: "int", size: 4)
!26 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !27, size: 64)
!27 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!28 = !DILocalVariable(name: "src1", arg: 1, scope: !21, file: !22, line: 1, type: !26)
!29 = !DILocation(line: 0, scope: !21)
!30 = !DILocalVariable(name: "src2", arg: 2, scope: !21, file: !22, line: 1, type: !26)
!31 = !DILocalVariable(name: "dest", arg: 3, scope: !21, file: !22, line: 1, type: !26)
!32 = !DILocalVariable(name: "i", scope: !21, file: !22, line: 3, type: !33)
!33 = !DIDerivedType(tag: DW_TAG_typedef, name: "size_t", file: !34, baseType: !35)
!34 = !DIFile(filename: "filename.h", directory: "/somepath3/.")
!35 = !DIBasicType(name: "long unsigned int", size: 64, encoding: DW_ATE_unsigned)
!36 = !DILocation(line: 4, column: 14, scope: !21)
!37 = !DILocalVariable(name: "a", scope: !21, file: !22, line: 4, type: !27)
!38 = !DILocation(line: 5, column: 14, scope: !21)
!39 = !DILocalVariable(name: "b", scope: !21, file: !22, line: 5, type: !27)
!40 = !DILocation(line: 6, column: 17, scope: !21)
!41 = !DILocation(line: 6, column: 14, scope: !21)
!42 = !DILocalVariable(name: "c", scope: !21, file: !22, line: 6, type: !27)
!43 = !DILocalVariable(name: "con_a", scope: !21, file: !22, line: 7, type: !27)
!44 = !DILocation(line: 9, column:14, scope: !21)
!45 = !DILocalVariable(name: "con_c", scope: !21, file: !22, line: 9, type: !27)
!46 = !DILocation(line: 10, column: 17, scope: !21)
!47 = !DILocation(line: 10, column: 14, scope: !21)
!48 = !DILocalVariable(name: "d", scope: !21, file: !22, line: 10, type: !27)
!49 = !DILocation(line: 11, column: 17, scope: !21)
!50 = !DILocation(line: 11, column: 14, scope: !21)
!51 = !DILocalVariable(name: "e", scope: !21, file: !22, line: 11, type: !27)
!52 = !DILocation(line: 12, column: 17, scope: !21)
!53 = !DILocation(line: 12, column: 14, scope: !21)
!54 = !DILocalVariable(name: "f", scope: !21, file: !22, line: 12, type: !27)
!55 = !DILocation(line: 13, column: 17, scope: !21)
!56 = !DILocation(line: 13, column: 14, scope: !21)
!57 = !DILocalVariable(name: "g", scope: !21, file: !22, line: 13, type: !27)
!58 = !DILocation(line: 14, column: 17, scope: !21)
!59 = !DILocation(line: 14, column: 14, scope: !21)
!60 = !DILocalVariable(name: "h", scope: !21, file: !22, line: 14, type: !27)
!61 = !DILocation(line: 15, column: 17, scope: !21)
!62 = !DILocation(line: 15, column: 14, scope: !21)
!63 = !DILocalVariable(name: "j", scope: !21, file: !22, line: 15, type: !27)
!64 = !DILocation(line: 16, column: 4, scope: !21)
!65 = !DILocation(line: 16, column: 12, scope: !21)
!66 = !DILocation(line: 17, column: 1, scope: !21)
