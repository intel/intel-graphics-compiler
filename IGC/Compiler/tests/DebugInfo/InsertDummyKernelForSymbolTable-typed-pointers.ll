;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-insert-dummy-kernel-for-symbol-table -S < %s | FileCheck %s
; ------------------------------------------------
; InsertDummyKernelForSymbolTable
; ------------------------------------------------
; This test checks that InsertDummyKernelForSymbolTable pass follows
; 'How to Update Debug Info' llvm guideline.
;
; This pass adds dummy kernel so no MD should be changed.
; ------------------------------------------------

; CHECK: @b {{.*}} !dbg [[GB_MDE:![0-9]*]]
;
; CHECK: define spir_kernel void @bar
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]

; CHECK: @llvm.dbg.declare(metadata i32* %{{.*}}, metadata [[A_MD:![0-9]*]], metadata !DIExpression()), !dbg [[A_LOC:![0-9]*]]
; CHECK: @llvm.dbg.declare(metadata i32* %{{.*}}, metadata [[SID_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SID_LOC:![0-9]*]]
; CHECK: [[CALL_V:%[A-z0-9]*]] = call spir_func i32 {{.*}} !dbg [[SIDCALL_LOC:![0-9]*]]
; CHECK: store i32 [[CALL_V]], {{.*}} !dbg [[SID_LOC]]

@b = common addrspace(1) global i32 0, align 4, !dbg !0

; Function Attrs: noinline nounwind
define spir_kernel void @bar(i32 %a) #0 !dbg !25 {
entry:
  %a.addr = alloca i32, align 4
  %subid = alloca i32, align 4
  %sum = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  call void @llvm.dbg.declare(metadata i32* %a.addr, metadata !30, metadata !DIExpression()), !dbg !31
  call void @llvm.dbg.declare(metadata i32* %subid, metadata !32, metadata !DIExpression()), !dbg !33
  %call = call spir_func i32 @__builtin_spirv_BuiltInSubgroupId() #2, !dbg !34
  store i32 %call, i32* %subid, align 4, !dbg !33
  call void @llvm.dbg.declare(metadata i32* %sum, metadata !35, metadata !DIExpression()), !dbg !36
  %0 = load i32, i32* %subid, align 4, !dbg !37
  %1 = load i32, i32* %a.addr, align 4, !dbg !38
  %add = add nsw i32 %0, %1, !dbg !39
  store i32 %add, i32* %sum, align 4, !dbg !36
  ret void, !dbg !40
}

; CHECK-DAG: [[FILE1:![0-9]*]] = !DIFile(filename: "<stdin>", directory: "/")
; CHECK-DAG: [[SCOPE1:![0-9]*]] = distinct !DICompileUnit(language: DW_LANG_C99, file: [[FILE1]], producer: "spirv"
; CHECK-DAG: [[GB_MDE]] = !DIGlobalVariableExpression(var: [[GB_MD:![0-9]*]], expr: !DIExpression())
; CHECK-DAG: [[GB_MD]] = distinct !DIGlobalVariable(name: "b", scope: [[SCOPE1]], file: [[FILE1]], line: 2, type: {{.*}}, isLocal: true, isDefinition: true)
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "InsertDummyKernelForSymbolTable.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "bar", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[A_MD]] = !DILocalVariable(name: "a", arg: 1, scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[A_LOC]] = !DILocation(line: 1, column: 23, scope: [[SCOPE]])
; CHECK-DAG: [[SID_MD]] = !DILocalVariable(name: "subid", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[SID_LOC]] = !DILocation(line: 3, column: 9, scope: [[SCOPE]])
; CHECK-DAG: [[SIDCALL_LOC]] = !DILocation(line: 3, column: 15, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: nounwind readnone
declare spir_func i32 @__builtin_spirv_BuiltInSubgroupId() #2

attributes #0 = { noinline nounwind }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { nounwind readnone }

!llvm.dbg.cu = !{!2}
!llvm.module.flags = !{!6, !7}
!IGCMetadata = !{!8}
!igc.functions = !{!22}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "b", scope: !2, file: !3, line: 2, type: !5, isLocal: true, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4)
!3 = !DIFile(filename: "<stdin>", directory: "/")
!4 = !{}
!5 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!6 = !{i32 2, !"Dwarf Version", i32 4}
!7 = !{i32 2, !"Debug Info Version", i32 3}
!8 = !{!"ModuleMD", !9, !19}
!9 = !{!"FuncMD", !10, !11}
!10 = !{!"FuncMDMap[0]", void (i32)* @bar}
!11 = !{!"FuncMDValue[0]", !12, !13, !17, !18}
!12 = !{!"localOffsets"}
!13 = !{!"workGroupWalkOrder", !14, !15, !16}
!14 = !{!"dim0", i32 0}
!15 = !{!"dim1", i32 1}
!16 = !{!"dim2", i32 2}
!17 = !{!"funcArgs"}
!18 = !{!"functionType", !"KernelFunction"}
!19 = !{!"inlineProgramScopeOffsets", !20, !21}
!20 = !{!"inlineProgramScopeOffsetsMap[0]", i32 addrspace(1)* @b}
!21 = !{!"inlineProgramScopeOffsetsValue[0]", i32 0}
!22 = !{void (i32)* @bar, !23}
!23 = !{!24}
!24 = !{!"function_type", i32 0}
!25 = distinct !DISubprogram(name: "bar", scope: null, file: !26, line: 1, type: !27, flags: DIFlagPrototyped, unit: !2, templateParams: !4, retainedNodes: !4)
!26 = !DIFile(filename: "InsertDummyKernelForSymbolTable.ll", directory: "/")
!27 = !DISubroutineType(types: !28)
!28 = !{!29, !5}
!29 = !DIBasicType(name: "int", size: 4)
!30 = !DILocalVariable(name: "a", arg: 1, scope: !25, file: !26, line: 1, type: !5)
!31 = !DILocation(line: 1, column: 23, scope: !25)
!32 = !DILocalVariable(name: "subid", scope: !25, file: !26, line: 3, type: !5)
!33 = !DILocation(line: 3, column: 9, scope: !25)
!34 = !DILocation(line: 3, column: 15, scope: !25)
!35 = !DILocalVariable(name: "sum", scope: !25, file: !26, line: 4, type: !5)
!36 = !DILocation(line: 4, column: 9, scope: !25)
!37 = !DILocation(line: 4, column: 15, scope: !25)
!38 = !DILocation(line: 4, column: 23, scope: !25)
!39 = !DILocation(line: 4, column: 21, scope: !25)
!40 = !DILocation(line: 5, column: 1, scope: !25)
