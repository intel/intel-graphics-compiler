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
; get_max_sub_group_size transform to intrinsic call

; Sanity check:
; CHECK: @test_sg_max{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: dbg.declare{{.*}} addrspace
; CHECK-SAME: metadata [[DST_MD:![0-9]*]]
; CHECK-SAME: !dbg [[DST_LOC:![0-9]*]]
; CHECK: dbg.declare{{.*}} i32*
; CHECK-SAME: metadata [[MAX_MD:![0-9]*]]
; CHECK-SAME: !dbg [[MAX_LOC:![0-9]*]]

; Check that call line is not lost
; CHECK-DAG: store i32 [[MAX_V:%[A-z0-9.]*]], {{.*}}, !dbg [[MAX_LOC]]
; CHECK-DAG: [[MAX_V]] = {{.*}}, !dbg [[CALL_LOC:![0-9]*]]

; CHECK: store i32 {{.*}}, i32 addrspace{{.*}}, !dbg [[DST_STORE_LOC:![0-9]*]]

define spir_kernel void @test_sg_max(i32 addrspace(1)* %dst) #0 !dbg !12 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %max_size = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !19, metadata !DIExpression()), !dbg !20
  call void @llvm.dbg.declare(metadata i32* %max_size, metadata !21, metadata !DIExpression()), !dbg !23
  %call.i = call spir_func i32 @__builtin_IB_get_simd_size(), !dbg !22
  store i32 %call.i, i32* %max_size, align 4, !dbg !23
  %0 = load i32, i32* %max_size, align 4, !dbg !24
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !25
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %1, i64 0, !dbg !25
  store i32 %0, i32 addrspace(1)* %arrayidx, align 4, !dbg !26
  ret void, !dbg !27
}

; Check MD:
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "max_subgroup_intr.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_sg_max", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[DST_LOC]] = !DILocation(line: 1, column: 42, scope: [[SCOPE]])
; CHECK-DAG: [[DST_MD]] = !DILocalVariable(name: "dst", arg: 1, scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[MAX_LOC]] = !DILocation(line: 3, column: 9, scope: [[SCOPE]])
; CHECK-DAG: [[MAX_MD]] = !DILocalVariable(name: "max_size", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 3, column: 12, scope: [[SCOPE]])
; CHECK-DAG: [[DST_STORE_LOC]] =  !DILocation(line: 4, column: 12, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: convergent
declare spir_func i32 @__builtin_IB_get_simd_size() local_unnamed_addr #2

attributes #0 = { convergent noinline nounwind optnone }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { convergent }

!llvm.dbg.cu = !{!0}
!IGCMetadata = !{!3}
!igc.functions = !{!6}
!llvm.module.flags = !{!9, !10, !11}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "/")
!2 = !{}
!3 = !{!"ModuleMD", !4}
!4 = !{!"msInfo", !5}
!5 = !{!"SubgroupSize", i32 0}
!6 = !{void (i32 addrspace(1)*)* @test_sg_max, !7}
!7 = !{!8}
!8 = !{!"function_type", i32 0}
!9 = !{i32 2, !"Dwarf Version", i32 4}
!10 = !{i32 2, !"Debug Info Version", i32 3}
!11 = !{i32 1, !"wchar_size", i32 4}
!12 = distinct !DISubprogram(name: "test_sg_max", scope: null, file: !13, line: 1, type: !14, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!13 = !DIFile(filename: "max_subgroup_intr.ll", directory: "/")
!14 = !DISubroutineType(types: !15)
!15 = !{!16, !17}
!16 = !DIBasicType(name: "int", size: 4)
!17 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !18, size: 64)
!18 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!19 = !DILocalVariable(name: "dst", arg: 1, scope: !12, file: !13, line: 1, type: !17)
!20 = !DILocation(line: 1, column: 42, scope: !12)
!21 = !DILocalVariable(name: "max_size", scope: !12, file: !13, line: 3, type: !18)
!22 = !DILocation(line: 3, column: 12, scope: !12)
!23 = !DILocation(line: 3, column: 9, scope: !12)
!24 = !DILocation(line: 4, column: 14, scope: !12)
!25 = !DILocation(line: 4, column: 5, scope: !12)
!26 = !DILocation(line: 4, column: 12, scope: !12)
!27 = !DILocation(line: 5, column: 1, scope: !12)
