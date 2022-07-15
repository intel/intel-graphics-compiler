;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-named-barriers-resolution -S < %s | FileCheck %s
; ------------------------------------------------
; NamedBarriersResolution
; ------------------------------------------------
; This test checks that NamedBarriersResolution pass follows
; 'How to Update Debug Info' llvm guideline.
;
; And was reduced from ocl test kernel:
; __kernel void foo(__global unsigned char *in)
; {
;   __local NamedBarrier_t* a;
;   a = named_barrier_init(1);
;   work_group_named_barrier(a, CLK_LOCAL_MEM_FENCE );
;   work_group_named_barrier(a, memory_scope_work_group, CLK_LOCAL_MEM_FENCE );
; }
;
; ------------------------------------------------

%struct.__namedBarrier = type { i32, i32, i32 }

; Function Attrs: nounwind
declare spir_func %struct.__namedBarrier addrspace(3)* @_Z18named_barrier_initi(i32) #0

; Function Attrs: nounwind
declare spir_func void @_Z24work_group_named_barrierPU3AS314__namedBarrierj(%struct.__namedBarrier addrspace(3)*, i32) #0

; Function Attrs: nounwind
declare spir_func void @_Z24work_group_named_barrierPU3AS314__namedBarrierj12memory_scope(%struct.__namedBarrier addrspace(3)*, i32, i32) #0

; CHECK: define spir_kernel void @foo
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare(metadata i8
; CHECK-SAME:  metadata [[IN_MD:![0-9]*]], metadata !DIExpression()), !dbg [[IN_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare(metadata %struct
; CHECK-SAME:  metadata [[A_MD:![0-9]*]], metadata !DIExpression()), !dbg [[A_LOC:![0-9]*]]
;
; CHECK: [[INIT_V:%[A-z0-9]*]] = call{{.*}} %struct
; CHECK-SAME: !dbg [[INIT_LOC:![0-9]*]]
;
; CHECK: store {{.*}} [[INIT_V]]
; CHECK-SAME: !dbg [[STORE_LOC:![0-9]*]]
;
; CHECK: call {{.*}}%struct
; CHECK-SAME: !dbg [[BAR1_LOC:![0-9]*]]
;
; CHECK: call {{.*}}%struct
; CHECK-SAME: !dbg [[BAR2_LOC:![0-9]*]]


; Function Attrs: noinline nounwind
define spir_kernel void @foo(i8 addrspace(1)* %in) #1 !dbg !5 {
entry:
  %in.addr = alloca i8 addrspace(1)*, align 8
  %a = alloca %struct.__namedBarrier addrspace(3)*, align 8
  store i8 addrspace(1)* %in, i8 addrspace(1)** %in.addr, align 8
  call void @llvm.dbg.declare(metadata i8 addrspace(1)** %in.addr, metadata !12, metadata !DIExpression()), !dbg !13
  call void @llvm.dbg.declare(metadata %struct.__namedBarrier addrspace(3)** %a, metadata !14, metadata !DIExpression()), !dbg !25
  %call = call spir_func %struct.__namedBarrier addrspace(3)* @_Z18named_barrier_initi(i32 1) #0, !dbg !26
  store %struct.__namedBarrier addrspace(3)* %call, %struct.__namedBarrier addrspace(3)** %a, align 8, !dbg !27
  %0 = load %struct.__namedBarrier addrspace(3)*, %struct.__namedBarrier addrspace(3)** %a, align 8, !dbg !28
  call spir_func void @_Z24work_group_named_barrierPU3AS314__namedBarrierj(%struct.__namedBarrier addrspace(3)* %0, i32 1) #0, !dbg !29
  %1 = load %struct.__namedBarrier addrspace(3)*, %struct.__namedBarrier addrspace(3)** %a, align 8, !dbg !30
  call spir_func void @_Z24work_group_named_barrierPU3AS314__namedBarrierj12memory_scope(%struct.__namedBarrier addrspace(3)* %1, i32 1, i32 1) #0, !dbg !31
  ret void, !dbg !32
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "NamedBarriersResolution.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "foo", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[IN_MD]] = !DILocalVariable(name: "in", arg: 1, scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[IN_LOC]] = !DILocation(line: 1, column: 43, scope: [[SCOPE]])
; CHECK-DAG: [[A_MD]] = !DILocalVariable(name: "a", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[A_LOC]] = !DILocation(line: 3, column: 27, scope: [[SCOPE]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 4, column: 5, scope: [[SCOPE]])
; CHECK-DAG: [[INIT_LOC]] = !DILocation(line: 4, column: 7, scope: [[SCOPE]])
; CHECK-DAG: [[BAR1_LOC]] = !DILocation(line: 5, column: 3, scope: [[SCOPE]])
; CHECK-DAG: [[BAR2_LOC]] = !DILocation(line: 6, column: 3, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #2

attributes #0 = { nounwind }
attributes #1 = { noinline nounwind }
attributes #2 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4}
!igc.functions = !{}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "/")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = distinct !DISubprogram(name: "foo", scope: null, file: !6, line: 1, type: !7, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!6 = !DIFile(filename: "NamedBarriersResolution.ll", directory: "/")
!7 = !DISubroutineType(types: !8)
!8 = !{!9, !10}
!9 = !DIBasicType(name: "int", size: 4)
!10 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !11, size: 64)
!11 = !DIBasicType(name: "unsigned char", size: 8, encoding: DW_ATE_unsigned_char)
!12 = !DILocalVariable(name: "in", arg: 1, scope: !5, file: !6, line: 1, type: !10)
!13 = !DILocation(line: 1, column: 43, scope: !5)
!14 = !DILocalVariable(name: "a", scope: !5, file: !6, line: 3, type: !15)
!15 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !16, size: 64)
!16 = !DIDerivedType(tag: DW_TAG_typedef, name: "NamedBarrier_t", file: !17, baseType: !18)
!17 = !DIFile(filename: "header.h", directory: "/")
!18 = !DIDerivedType(tag: DW_TAG_typedef, name: "__namedBarrier", file: !17, baseType: !19)
!19 = !DICompositeType(tag: DW_TAG_structure_type, file: !17, line: 9276, size: 96, elements: !20)
!20 = !{!21, !23, !24}
!21 = !DIDerivedType(tag: DW_TAG_member, name: "count", file: !17, line: 9278, baseType: !22, size: 32)
!22 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!23 = !DIDerivedType(tag: DW_TAG_member, name: "orig_count", file: !17, line: 9279, baseType: !22, size: 32, offset: 32)
!24 = !DIDerivedType(tag: DW_TAG_member, name: "inc", file: !17, line: 9280, baseType: !22, size: 32, offset: 64)
!25 = !DILocation(line: 3, column: 27, scope: !5)
!26 = !DILocation(line: 4, column: 7, scope: !5)
!27 = !DILocation(line: 4, column: 5, scope: !5)
!28 = !DILocation(line: 5, column: 28, scope: !5)
!29 = !DILocation(line: 5, column: 3, scope: !5)
!30 = !DILocation(line: 6, column: 28, scope: !5)
!31 = !DILocation(line: 6, column: 3, scope: !5)
!32 = !DILocation(line: 7, column: 1, scope: !5)
