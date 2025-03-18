;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: llvm-14-plus
; RUN: igc_opt --opaque-pointers  -igc-opencl-printf-resolution -S  < %s | FileCheck %s
; ------------------------------------------------
; OpenCLPrintfResolution
; ------------------------------------------------
; This test checks that OpenCLPrintfResolution pass follows
; 'How to Update Debug Info' llvm guideline.
;
; And was reduced from ocl test kernel:
; __kernel void test_printf(int src)
; {
;      float4 f = (float4)(1.0f, 2.0f, 3.0f, 4.0f);
;      uchar4 uc = (uchar4)(0xFA, 0xFB, 0xFC, 0xFD);
;      int i = 42;
;      float f1 = 4.2f;
;
;      printf("i, f = %d, %f\n", i, f1);
;      printf("f4 = %2.2v4hlf\n", f);
;      printf("uc = %#v4hhx\n", uc);
;      printf("%s\n", "this is a test string\n");
; }
;
; ------------------------------------------------

; ModuleID = 'OpenCLPrintfResolution.ll'
source_filename = "<stdin>"

@.str = internal unnamed_addr addrspace(2) constant [15 x i8] c"i, f = %d, %f\0A\00", align 1
@.str.1 = internal unnamed_addr addrspace(2) constant [16 x i8] c"f4 = %2.2v4hlf\0A\00", align 1
@.str.2 = internal unnamed_addr addrspace(2) constant [14 x i8] c"uc = %#v4hhx\0A\00", align 1
@.str.3 = internal unnamed_addr addrspace(2) constant [4 x i8] c"%s\0A\00", align 1
@.str.4 = internal unnamed_addr addrspace(2) constant [23 x i8] c"this is a test string\0A\00", align 1

define spir_kernel void @test_printf(i32 %src, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr addrspace(2) %constBase, ptr %privateBase, ptr addrspace(1) %printfBuffer) !dbg !22 {
  %f = alloca <4 x float>, align 16, !dbg !46
  %uc = alloca <4 x i8>, align 4, !dbg !47
  %i = alloca i32, align 4, !dbg !48
  %f1 = alloca float, align 4, !dbg !49
  ;
  ; Sanity check that
  ; variable info is not lost
  ;
  ; CHECK: store <4 x float> {{.*}}, !dbg [[STORE_F_LOC:![0-9]*]]
  ; CHECK-NEXT: declare(metadata ptr %f, metadata [[F_MD:![0-9]*]], metadata !DIExpression()), !dbg [[STORE_F_LOC]]
  ; CHECK-NEXT: store <4 x i8> {{.*}}, !dbg [[STORE_UC_LOC:![0-9]*]]
  ; CHECK-NEXT: declare(metadata ptr %uc, metadata [[UC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[STORE_UC_LOC]]
  ; CHECK-NEXT: store i32 {{.*}}, !dbg [[STORE_I_LOC:![0-9]*]]
  ; CHECK-NEXT: declare(metadata ptr %i, metadata [[I_MD:![0-9]*]], metadata !DIExpression()), !dbg [[STORE_I_LOC]]
  ; CHECK-NEXT: store float {{.*}}, !dbg [[STORE_F1_LOC:![0-9]*]]
  ; CHECK-NEXT: declare(metadata ptr %f1, metadata [[F1_MD:![0-9]*]], metadata !DIExpression()), !dbg [[STORE_F1_LOC]]
  store <4 x float> <float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.000000e+00>, ptr %f, align 16, !dbg !50
  call void @llvm.dbg.declare(metadata ptr %f, metadata !25, metadata !DIExpression()), !dbg !50
  store <4 x i8> <i8 -6, i8 -5, i8 -4, i8 -3>, ptr %uc, align 4, !dbg !51
  call void @llvm.dbg.declare(metadata ptr %uc, metadata !27, metadata !DIExpression()), !dbg !51
  store i32 42, ptr %i, align 4, !dbg !52
  call void @llvm.dbg.declare(metadata ptr %i, metadata !28, metadata !DIExpression()), !dbg !52
  store float 0x4010CCCCC0000000, ptr %f1, align 4, !dbg !53
  call void @llvm.dbg.declare(metadata ptr %f1, metadata !29, metadata !DIExpression()), !dbg !53
  %1 = load i32, ptr %i, align 4, !dbg !54
  call void @llvm.dbg.value(metadata i32 %1, metadata !30, metadata !DIExpression()), !dbg !54
  %2 = load float, ptr %f1, align 4, !dbg !55
  call void @llvm.dbg.value(metadata float %2, metadata !32, metadata !DIExpression()), !dbg !55
  %conv = call spir_func double @__builtin_spirv_OpFConvert_f64_f32(float %2), !dbg !56
  call void @llvm.dbg.value(metadata double %conv, metadata !33, metadata !DIExpression()), !dbg !56

  call void @llvm.dbg.value(metadata ptr addrspace(2) @.str, metadata !34, metadata !DIExpression()), !dbg !57
  ;
  ; Print i, f1
  ; check call location and return value
  ;
  ; CHECK: [[I_V:%[0-9]*]] = load i32, ptr %i
  ; CHECK: store i32 [[I_V]], {{.*}}, !dbg [[PRINT_IF1_LOC:![0-9]*]]
  ; CHECK: store {{double|float}} %{{to_float|conv}}, {{.*}}, !dbg [[PRINT_IF1_LOC:![0-9]*]]
  ; CHECK: [[IF1_RET_V:%.*]] = select i1 {{.*}}, !dbg [[PRINT_IF1_LOC]]
  ; CHECK-NEXT: dbg.value(metadata i32 [[IF1_RET_V]], metadata [[IF1_RET_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PRINT_IF1_LOC]]
  %call = call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @.str, i32 %1, double %conv), !dbg !58
  call void @llvm.dbg.value(metadata i32 %call, metadata !35, metadata !DIExpression()), !dbg !58
  %3 = load <4 x float>, ptr %f, align 16, !dbg !59
  call void @llvm.dbg.value(metadata <4 x float> %3, metadata !36, metadata !DIExpression()), !dbg !59

  call void @llvm.dbg.value(metadata ptr addrspace(2) @.str.1, metadata !38, metadata !DIExpression()), !dbg !60
  ;
  ; Print v4f
  ; check call location and return value
  ;
  ; CHECK: [[F4_V:%[0-9]*]] = load <4 x float>, ptr %f
  ; CHECK: store <4 x float> [[F4_V]], {{.*}}, !dbg [[PRINT_F4_LOC:![0-9]*]]
  ; CHECK: [[F4_RET_V:%.*]] = select i1 {{.*}}, !dbg [[PRINT_F4_LOC]]
  ; CHECK-NEXT: dbg.value(metadata i32 [[F4_RET_V]], metadata [[F4_RET_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PRINT_F4_LOC]]
  %call2 = call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @.str.1, <4 x float> %3), !dbg !61
  call void @llvm.dbg.value(metadata i32 %call2, metadata !39, metadata !DIExpression()), !dbg !61
  %4 = load <4 x i8>, ptr %uc, align 4, !dbg !62
  call void @llvm.dbg.value(metadata <4 x i8> %4, metadata !40, metadata !DIExpression()), !dbg !62

  call void @llvm.dbg.value(metadata ptr addrspace(2) @.str.2, metadata !41, metadata !DIExpression()), !dbg !63
  ;
  ; Print v4i8
  ; check call location and return value
  ;
  ; CHECK: [[I4_V:%[0-9]*]] = load <4 x i8>, ptr %uc
  ; CHECK: store <4 x i8> [[I4_V]], {{.*}}, !dbg [[PRINT_I4_LOC:![0-9]*]]
  ; CHECK: [[I4_RET_V:%.*]] = select i1 {{.*}}, !dbg [[PRINT_I4_LOC]]
  ; CHECK-NEXT: dbg.value(metadata i32 [[I4_RET_V]], metadata [[I4_RET_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PRINT_I4_LOC]]
  %call3 = call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @.str.2, <4 x i8> %4), !dbg !64
  call void @llvm.dbg.value(metadata i32 %call3, metadata !42, metadata !DIExpression()), !dbg !64

  call void @llvm.dbg.value(metadata ptr addrspace(2) @.str.3, metadata !43, metadata !DIExpression()), !dbg !65

  call void @llvm.dbg.value(metadata ptr addrspace(2) @.str.4, metadata !44, metadata !DIExpression()), !dbg !66
  ;
  ; Print v4i8
  ; check call location and return value
  ;
  ; CHECK: [[S_RET_V:%.*]] = select i1 {{.*}}, !dbg [[PRINT_S_LOC:![0-9]*]]
  ; CHECK-NEXT: dbg.value(metadata i32 [[S_RET_V]], metadata [[S_RET_MD:![0-9]*]], metadata !DIExpression()), !dbg [[PRINT_S_LOC]]
  %call4 = call spir_func i32 (ptr addrspace(2), ...) @printf(ptr addrspace(2) @.str.3, ptr addrspace(2) @.str.4), !dbg !67
  call void @llvm.dbg.value(metadata i32 %call4, metadata !45, metadata !DIExpression()), !dbg !67
  ret void, !dbg !68
}
;
; Check MD:
;
; CHECK-DAG: [[STORE_I_LOC]] = !DILocation(line: 7
; CHECK-DAG: [[I_MD]] = !DILocalVariable(name: "3"
; CHECK-DAG: [[STORE_F1_LOC]] = !DILocation(line: 8
; CHECK-DAG: [[F1_MD]] = !DILocalVariable(name: "4"
; CHECK-DAG: [[STORE_F_LOC]] = !DILocation(line: 5
; CHECK-DAG: [[F_MD]] = !DILocalVariable(name: "1"
; CHECK-DAG: [[STORE_UC_LOC]] = !DILocation(line: 6
; CHECK-DAG: [[UC_MD]] = !DILocalVariable(name: "2"

; CHECK-DAG: [[SCOPE:![0-9]*]] = distinct !DISubprogram(name: "test_printf"
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "OpenCLPrintfResolution.ll"
;
; CHECK-DAG: [[PRINT_IF1_LOC]] = !DILocation(line: 13, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[IF1_RET_MD]] = !DILocalVariable(name: "9", scope: [[SCOPE]], file: [[FILE]], line: 13
;
; CHECK-DAG: [[PRINT_F4_LOC]] = !DILocation(line: 16, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[F4_RET_MD]] = !DILocalVariable(name: "12", scope: [[SCOPE]], file: [[FILE]], line: 16
;
; CHECK-DAG: [[PRINT_I4_LOC]] = !DILocation(line: 19, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[I4_RET_MD]] = !DILocalVariable(name: "15", scope: [[SCOPE]], file: [[FILE]], line: 19
;
; CHECK-DAG: [[PRINT_S_LOC]] = !DILocation(line: 22, column: 1, scope: [[SCOPE]])
; CHECK-DAG: [[S_RET_MD]] = !DILocalVariable(name: "18", scope: [[SCOPE]], file: [[FILE]], line: 22
declare spir_func double @__builtin_spirv_OpFConvert_f64_f32(float)

declare spir_func i32 @printf(ptr addrspace(2), ...)

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #0
declare void @llvm.dbg.declare(metadata, metadata, metadata) #0

attributes #0 = { nounwind readnone speculatable }

!igc.functions = !{!0}
!IGCMetadata = !{!9}
!llvm.dbg.cu = !{!16}
!llvm.debugify = !{!19, !20}
!llvm.module.flags = !{!21}

!0 = !{ptr @test_printf, !1}
!1 = !{!2, !3}
!2 = !{!"function_type", i32 0}
!3 = !{!"implicit_arg_desc", !4, !5, !6, !7, !8}
!4 = !{i32 0}
!5 = !{i32 1}
!6 = !{i32 11}
!7 = !{i32 13}
!8 = !{i32 14}
!9 = !{!"ModuleMD", !10}
!10 = !{!"FuncMD", !11, !12}
!11 = distinct !{!"FuncMDMap[0]", ptr @test_printf}
!12 = !{!"FuncMDValue[0]", !13, !14, !15}
!13 = !{!"localOffsets"}
!14 = !{!"funcArgs"}
!15 = !{!"functionType", !"KernelFunction"}
!16 = distinct !DICompileUnit(language: DW_LANG_C, file: !17, producer: "debugify", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, enums: !18)
!17 = !DIFile(filename: "OpenCLPrintfResolution.ll", directory: "/")
!18 = !{}
!19 = !{i32 23}
!20 = !{i32 18}
!21 = !{i32 2, !"Debug Info Version", i32 3}
!22 = distinct !DISubprogram(name: "test_printf", linkageName: "test_printf", scope: null, file: !17, line: 1, type: !23, scopeLine: 1, unit: !16, retainedNodes: !24)
!23 = !DISubroutineType(types: !18)
!24 = !{!25, !27, !28, !29, !30, !32, !33, !34, !35, !36, !38, !39, !40, !41, !42, !43, !44, !45}
!25 = !DILocalVariable(name: "1", scope: !22, file: !17, line: 1, type: !26)
!26 = !DIBasicType(name: "ty64", size: 64, encoding: DW_ATE_unsigned)
!27 = !DILocalVariable(name: "2", scope: !22, file: !17, line: 2, type: !26)
!28 = !DILocalVariable(name: "3", scope: !22, file: !17, line: 3, type: !26)
!29 = !DILocalVariable(name: "4", scope: !22, file: !17, line: 4, type: !26)
!30 = !DILocalVariable(name: "5", scope: !22, file: !17, line: 9, type: !31)
!31 = !DIBasicType(name: "ty32", size: 32, encoding: DW_ATE_unsigned)
!32 = !DILocalVariable(name: "6", scope: !22, file: !17, line: 10, type: !31)
!33 = !DILocalVariable(name: "7", scope: !22, file: !17, line: 11, type: !26)
!34 = !DILocalVariable(name: "8", scope: !22, file: !17, line: 12, type: !26)
!35 = !DILocalVariable(name: "9", scope: !22, file: !17, line: 13, type: !31)
!36 = !DILocalVariable(name: "10", scope: !22, file: !17, line: 14, type: !37)
!37 = !DIBasicType(name: "ty128", size: 128, encoding: DW_ATE_unsigned)
!38 = !DILocalVariable(name: "11", scope: !22, file: !17, line: 15, type: !26)
!39 = !DILocalVariable(name: "12", scope: !22, file: !17, line: 16, type: !31)
!40 = !DILocalVariable(name: "13", scope: !22, file: !17, line: 17, type: !31)
!41 = !DILocalVariable(name: "14", scope: !22, file: !17, line: 18, type: !26)
!42 = !DILocalVariable(name: "15", scope: !22, file: !17, line: 19, type: !31)
!43 = !DILocalVariable(name: "16", scope: !22, file: !17, line: 20, type: !26)
!44 = !DILocalVariable(name: "17", scope: !22, file: !17, line: 21, type: !26)
!45 = !DILocalVariable(name: "18", scope: !22, file: !17, line: 22, type: !31)
!46 = !DILocation(line: 1, column: 1, scope: !22)
!47 = !DILocation(line: 2, column: 1, scope: !22)
!48 = !DILocation(line: 3, column: 1, scope: !22)
!49 = !DILocation(line: 4, column: 1, scope: !22)
!50 = !DILocation(line: 5, column: 1, scope: !22)
!51 = !DILocation(line: 6, column: 1, scope: !22)
!52 = !DILocation(line: 7, column: 1, scope: !22)
!53 = !DILocation(line: 8, column: 1, scope: !22)
!54 = !DILocation(line: 9, column: 1, scope: !22)
!55 = !DILocation(line: 10, column: 1, scope: !22)
!56 = !DILocation(line: 11, column: 1, scope: !22)
!57 = !DILocation(line: 12, column: 1, scope: !22)
!58 = !DILocation(line: 13, column: 1, scope: !22)
!59 = !DILocation(line: 14, column: 1, scope: !22)
!60 = !DILocation(line: 15, column: 1, scope: !22)
!61 = !DILocation(line: 16, column: 1, scope: !22)
!62 = !DILocation(line: 17, column: 1, scope: !22)
!63 = !DILocation(line: 18, column: 1, scope: !22)
!64 = !DILocation(line: 19, column: 1, scope: !22)
!65 = !DILocation(line: 20, column: 1, scope: !22)
!66 = !DILocation(line: 21, column: 1, scope: !22)
!67 = !DILocation(line: 22, column: 1, scope: !22)
!68 = !DILocation(line: 23, column: 1, scope: !22)
