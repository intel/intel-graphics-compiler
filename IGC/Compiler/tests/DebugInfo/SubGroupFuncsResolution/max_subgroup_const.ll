;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-sub-group-func-resolution -S < %s | FileCheck %s
; ------------------------------------------------
; SubGroupFuncsResolution
; ------------------------------------------------
; This test checks that SubGroupFuncsResolution pass follows
; 'How to Update Debug Info' llvm guideline.
;
; And was reduced from ocl test kernel:
;
; __kernel void test_sg_max(global int *dst)
; {
;     int max_size = get_max_sub_group_size();
;     dst[0] = max_size;
; }
;
; ------------------------------------------------
;
; get_max_sub_group_size transforms to const

; Sanity check:
; CHECK: @test_sg_max{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: dbg.declare{{.*}} addrspace
; CHECK-SAME: metadata [[DST_MD:![0-9]*]]
; CHECK-SAME: !dbg [[DST_LOC:![0-9]*]]

; CHECK: dbg.value(metadata i32 [[CONST_SIMD:[0-9]*]]
; CHECK-SAME: metadata [[MAX_MD:![0-9]*]]
; CHECK-SAME: !dbg [[MAX_LOC:![0-9]*]]

; CHECK: store i32 [[CONST_SIMD]], i32 addrspace{{.*}}, !dbg [[DST_STORE_LOC:![0-9]*]]

define spir_kernel void @test_sg_max(i32 addrspace(1)* %dst) #0 !dbg !13 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !20, metadata !DIExpression()), !dbg !21
  %call.i = call spir_func i32 @__builtin_IB_get_simd_size(), !dbg !22
  call void @llvm.dbg.value(metadata i32 %call.i, metadata !23, metadata !DIExpression()), !dbg !24
  %0 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !25
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 0, !dbg !25
  store i32 %call.i, i32 addrspace(1)* %arrayidx, align 4, !dbg !26
  ret void, !dbg !27
}

; Check MD:
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "max_subgroup_const.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_sg_max", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[DST_LOC]] = !DILocation(line: 1, column: 42, scope: [[SCOPE]])
; CHECK-DAG: [[DST_MD]] = !DILocalVariable(name: "dst", arg: 1, scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[MAX_LOC]] = !DILocation(line: 3, column: 9, scope: [[SCOPE]])
; CHECK-DAG: [[MAX_MD]] = !DILocalVariable(name: "max_size", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: !{!"sub_group_size", i32 [[CONST_SIMD]]}
; CHECK-DAG: [[DST_STORE_LOC]] =  !DILocation(line: 4, column: 12, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

; Function Attrs: convergent
declare spir_func i32 @__builtin_IB_get_simd_size() local_unnamed_addr #2

attributes #0 = { convergent noinline nounwind optnone }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { convergent }

!llvm.dbg.cu = !{!0}
!IGCMetadata = !{!3}
!igc.functions = !{!6}
!llvm.module.flags = !{!10, !11, !12}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "/")
!2 = !{}
!3 = !{!"ModuleMD", !4}
!4 = !{!"msInfo", !5}
!5 = !{!"SubgroupSize", i32 16}
!6 = !{void (i32 addrspace(1)*)* @test_sg_max, !7}
!7 = !{!8, !9}
!8 = !{!"function_type", i32 0}
!9 = !{!"sub_group_size", i32 16}
!10 = !{i32 2, !"Dwarf Version", i32 4}
!11 = !{i32 2, !"Debug Info Version", i32 3}
!12 = !{i32 1, !"wchar_size", i32 4}
!13 = distinct !DISubprogram(name: "test_sg_max", scope: null, file: !14, line: 1, type: !15, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!14 = !DIFile(filename: "max_subgroup_const.ll", directory: "/")
!15 = !DISubroutineType(types: !16)
!16 = !{!17, !18}
!17 = !DIBasicType(name: "int", size: 4)
!18 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !19, size: 64)
!19 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!20 = !DILocalVariable(name: "dst", arg: 1, scope: !13, file: !14, line: 1, type: !18)
!21 = !DILocation(line: 1, column: 42, scope: !13)
!22 = !DILocation(line: 3, column: 12, scope: !13)
!23 = !DILocalVariable(name: "max_size", scope: !13, file: !14, line: 3, type: !19)
!24 = !DILocation(line: 3, column: 9, scope: !13)
!25 = !DILocation(line: 4, column: 5, scope: !13)
!26 = !DILocation(line: 4, column: 12, scope: !13)
!27 = !DILocation(line: 5, column: 1, scope: !13)
