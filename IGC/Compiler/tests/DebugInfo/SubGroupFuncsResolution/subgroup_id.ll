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
; __kernel void test_sub(global int *dst)
; {
;     int sub_id = get_sub_group_local_id();
;     dst[0] = sub_id;
; }
;
; ------------------------------------------------
;
; get_sub_group_local_id transformation

; Sanity check:
; CHECK: @test_sub{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: dbg.declare{{.*}} addrspace
; CHECK-SAME: metadata [[DST_MD:![0-9]*]]
; CHECK-SAME: !dbg [[DST_LOC:![0-9]*]]
; CHECK: dbg.declare{{.*}} i32*
; CHECK-SAME: metadata [[SUBID_MD:![0-9]*]]
; CHECK-SAME: !dbg [[SUBID_LOC:![0-9]*]]

; Check that call line is not lost
; CHECK-DAG: store i32 [[SUBID_V:%[A-z0-9.]*]], {{.*}}, !dbg [[SUBID_LOC]]
; CHECK-DAG: [[SUBID_V]] = {{.*}}, !dbg [[CALL_LOC:![0-9]*]]

; CHECK: store i32 {{.*}}, i32 addrspace{{.*}}, !dbg [[DST_STORE_LOC:![0-9]*]]


define spir_kernel void @test_sub(i32 addrspace(1)* %dst) #0 !dbg !6 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %sub_id = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !13, metadata !DIExpression()), !dbg !14
  call void @llvm.dbg.declare(metadata i32* %sub_id, metadata !15, metadata !DIExpression()), !dbg !16
  %call.i = call spir_func i32 @__builtin_IB_get_simd_id(), !dbg !17
  store i32 %call.i, i32* %sub_id, align 4, !dbg !16
  %0 = load i32, i32* %sub_id, align 4, !dbg !18
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !19
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %1, i64 0, !dbg !19
  store i32 %0, i32 addrspace(1)* %arrayidx, align 4, !dbg !20
  ret void, !dbg !21
}

; Check MD:
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "subgroup_id.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_sub", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[DST_LOC]] = !DILocation(line: 1, column: 36, scope: [[SCOPE]])
; CHECK-DAG: [[DST_MD]] = !DILocalVariable(name: "dst", arg: 1, scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[SUBID_LOC]] = !DILocation(line: 3, column: 9, scope: [[SCOPE]])
; CHECK-DAG: [[SUBID_MD]] = !DILocalVariable(name: "sub_id", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 3, column: 12, scope: [[SCOPE]])
; CHECK-DAG: [[DST_STORE_LOC]] =  !DILocation(line: 4, column: 12, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: convergent
declare spir_func i32 @__builtin_IB_get_simd_id() local_unnamed_addr #2

attributes #0 = { convergent noinline nounwind optnone }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { convergent }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!igc.functions = !{}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "/")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = distinct !DISubprogram(name: "test_sub", scope: null, file: !7, line: 1, type: !8, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!7 = !DIFile(filename: "subgroup_id.ll", directory: "/")
!8 = !DISubroutineType(types: !9)
!9 = !{!10, !11}
!10 = !DIBasicType(name: "int", size: 4)
!11 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !12, size: 64)
!12 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!13 = !DILocalVariable(name: "dst", arg: 1, scope: !6, file: !7, line: 1, type: !11)
!14 = !DILocation(line: 1, column: 36, scope: !6)
!15 = !DILocalVariable(name: "sub_id", scope: !6, file: !7, line: 3, type: !12)
!16 = !DILocation(line: 3, column: 9, scope: !6)
!17 = !DILocation(line: 3, column: 12, scope: !6)
!18 = !DILocation(line: 4, column: 14, scope: !6)
!19 = !DILocation(line: 4, column: 5, scope: !6)
!20 = !DILocation(line: 4, column: 12, scope: !6)
!21 = !DILocation(line: 5, column: 1, scope: !6)
