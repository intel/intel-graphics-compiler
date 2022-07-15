;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-codeassumption -S < %s | FileCheck %s
; ------------------------------------------------
; CodeAssumption
; ------------------------------------------------
; This test checks that CodeAssumption pass follows
; 'How to Update Debug Info' llvm guideline.
;
; And was reduced from ocl test kernel:
;
; __kernel void bar(int a)
; {
;     int subid = get_sub_group_id();
;     int sum = subid + a;
; }
;
; Uniform part
;

; Check that builtin call is properly updated
;
; CHECK: @llvm.dbg.declare(metadata i32* %subid, metadata [[SUBID_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SUBID_LOC:![0-9]*]]
; CHECK: @__builtin_spirv_BuiltInSubgroupId() {{.*}} !dbg [[CALL_LOC:![0-9]*]]
; CHECK: [[SUBID_V:%[0-9A-z]*]] = call i32 @llvm.genx.GenISA{{.*}}, !dbg [[CALL_LOC]]
; CHECK: store i32 [[SUBID_V]], {{.*}}, !dbg [[SUBID_LOC]]
;

define spir_kernel void @bar(i32 %a) #0 !dbg !19 {
entry:
  %a.addr = alloca i32, align 4
  %subid = alloca i32, align 4
  %sum = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  call void @llvm.dbg.declare(metadata i32* %a.addr, metadata !25, metadata !DIExpression()), !dbg !26
  call void @llvm.dbg.declare(metadata i32* %subid, metadata !27, metadata !DIExpression()), !dbg !28
  %call = call spir_func i32 @__builtin_spirv_BuiltInSubgroupId() #2, !dbg !29
  store i32 %call, i32* %subid, align 4, !dbg !28
  call void @llvm.dbg.declare(metadata i32* %sum, metadata !30, metadata !DIExpression()), !dbg !31
  %0 = load i32, i32* %subid, align 4, !dbg !32
  %1 = load i32, i32* %a.addr, align 4, !dbg !33
  %add = add nsw i32 %0, %1, !dbg !34
  store i32 %add, i32* %sum, align 4, !dbg !31
  ret void, !dbg !35
}

;
; TestMD
;
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "uniform.ll", directory: "/")
; CHECK-DAG: [[SCOPE:![0-9]*]] = distinct !DISubprogram(name: "bar", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[SUBID_MD]] = !DILocalVariable(name: "subid", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[SUBID_LOC]] = !DILocation(line: 3, column: 9, scope: [[SCOPE]])
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 3, column: 15, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: nounwind readnone
declare spir_func i32 @__builtin_spirv_BuiltInSubgroupId() #2

attributes #0 = { noinline nounwind }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { nounwind readnone }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4}
!IGCMetadata = !{!5}
!igc.functions = !{!16}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "/")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{!"ModuleMD", !6}
!6 = !{!"FuncMD", !7, !8}
!7 = !{!"FuncMDMap[0]", void (i32)* @bar}
!8 = !{!"FuncMDValue[0]", !9, !10, !14, !15}
!9 = !{!"localOffsets"}
!10 = !{!"workGroupWalkOrder", !11, !12, !13}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 1}
!13 = !{!"dim2", i32 2}
!14 = !{!"funcArgs"}
!15 = !{!"functionType", !"KernelFunction"}
!16 = !{void (i32)* @bar, !17}
!17 = !{!18}
!18 = !{!"function_type", i32 0}
!19 = distinct !DISubprogram(name: "bar", scope: null, file: !20, line: 1, type: !21, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!20 = !DIFile(filename: "uniform.ll", directory: "/")
!21 = !DISubroutineType(types: !22)
!22 = !{!23, !24}
!23 = !DIBasicType(name: "int", size: 4)
!24 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!25 = !DILocalVariable(name: "a", arg: 1, scope: !19, file: !20, line: 1, type: !24)
!26 = !DILocation(line: 1, column: 23, scope: !19)
!27 = !DILocalVariable(name: "subid", scope: !19, file: !20, line: 3, type: !24)
!28 = !DILocation(line: 3, column: 9, scope: !19)
!29 = !DILocation(line: 3, column: 15, scope: !19)
!30 = !DILocalVariable(name: "sum", scope: !19, file: !20, line: 4, type: !24)
!31 = !DILocation(line: 4, column: 9, scope: !19)
!32 = !DILocation(line: 4, column: 15, scope: !19)
!33 = !DILocation(line: 4, column: 23, scope: !19)
!34 = !DILocation(line: 4, column: 21, scope: !19)
!35 = !DILocation(line: 5, column: 1, scope: !19)
