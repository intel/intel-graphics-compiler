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
;
; __kernel void test_reduce(__global uint* dst, int src)
; {
;    int re = sub_group_reduce_add(src);
;    dst[0] = re;
; }
;
; ------------------------------------------------

; Sanity check:
; CHECK: @test_reduce{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: dbg.declare{{.*}} addrspace
; CHECK-SAME: metadata [[DST_MD:![0-9]*]]
; CHECK-SAME: !dbg [[DST_LOC:![0-9]*]]
; CHECK: dbg.declare{{.*}} i32
; CHECK-SAME: metadata [[SRC_MD:![0-9]*]]
; CHECK-SAME: !dbg [[SRC_LOC:![0-9]*]]
; CHECK: dbg.declare{{.*}} i32*
; CHECK-SAME: metadata [[REDUCE_MD:![0-9]*]]
; CHECK-SAME: !dbg [[REDUCE_LOC:![0-9]*]]
;
; Check that call line is not lost
; CHECK-DAG: store i32 [[REDUCE_V:%[A-z0-9.]*]], {{.*}}, !dbg [[REDUCE_LOC]]
; CHECK-DAG: [[REDUCE_V]] = {{.*}}, !dbg [[CALL_LOC:![0-9]*]]

; CHECK: store i32 {{.*}}, i32 addrspace{{.*}}, !dbg [[DST_STORE_LOC:![0-9]*]]

define spir_kernel void @test_reduce(i32 addrspace(1)* %dst, i32 %src) #0 !dbg !10 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %src.addr = alloca i32, align 4
  %re = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !20, metadata !DIExpression()), !dbg !21
  store i32 %src, i32* %src.addr, align 4
  call void @llvm.dbg.declare(metadata i32* %src.addr, metadata !22, metadata !DIExpression()), !dbg !23
  call void @llvm.dbg.declare(metadata i32* %re, metadata !24, metadata !DIExpression()), !dbg !25
  %0 = load i32, i32* %src.addr, align 4, !dbg !26
  %call82.i = call spir_func i32 @__builtin_IB_sub_group_reduce_IAdd_i32(i32 %0), !dbg !27
  store i32 %call82.i, i32* %re, align 4, !dbg !25
  %1 = load i32, i32* %re, align 4, !dbg !28
  %2 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !29
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %2, i64 0, !dbg !29
  store i32 %1, i32 addrspace(1)* %arrayidx, align 4, !dbg !30
  ret void, !dbg !31
}

; Check MD:
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "reduce.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_reduce", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[DST_LOC]] = !DILocation(line: 1, column: 42, scope: [[SCOPE]])
; CHECK-DAG: [[DST_MD]] = !DILocalVariable(name: "dst", arg: 1, scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[SRC_LOC]] = !DILocation(line: 1, column: 51, scope: [[SCOPE]])
; CHECK-DAG: [[SRC_MD]] = !DILocalVariable(name: "src", arg: 2, scope: [[SCOPE]], file: [[FILE]], line: 1

; CHECK-DAG: [[REDUCE_LOC]] = !DILocation(line: 3, column: 8, scope: [[SCOPE]])
; CHECK-DAG: [[REDUCE_MD]] = !DILocalVariable(name: "re", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 3, column: 13, scope: [[SCOPE]])
; CHECK-DAG: [[DST_STORE_LOC]] =  !DILocation(line: 4, column: 11, scope: [[SCOPE]])

declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

declare spir_func i32 @__builtin_IB_sub_group_reduce_IAdd_i32(i32) local_unnamed_addr #0

attributes #0 = { convergent noinline nounwind optnone }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!igc.functions = !{!6}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "/")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{void (i32 addrspace(1)*, i32)* @test_reduce, !7}
!7 = !{!8, !9}
!8 = !{!"function_type", i32 0}
!9 = !{!"sub_group_size", i32 16}
!10 = distinct !DISubprogram(name: "test_reduce", scope: null, file: !11, line: 1, type: !12, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!11 = !DIFile(filename: "reduce.ll", directory: "/")
!12 = !DISubroutineType(types: !13)
!13 = !{!14, !15, !19}
!14 = !DIBasicType(name: "int", size: 4)
!15 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !16, size: 64)
!16 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint", file: !17, baseType: !18)
!17 = !DIFile(filename: "opencl-c-base.h", directory: "/")
!18 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!19 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!20 = !DILocalVariable(name: "dst", arg: 1, scope: !10, file: !11, line: 1, type: !15)
!21 = !DILocation(line: 1, column: 42, scope: !10)
!22 = !DILocalVariable(name: "src", arg: 2, scope: !10, file: !11, line: 1, type: !19)
!23 = !DILocation(line: 1, column: 51, scope: !10)
!24 = !DILocalVariable(name: "re", scope: !10, file: !11, line: 3, type: !19)
!25 = !DILocation(line: 3, column: 8, scope: !10)
!26 = !DILocation(line: 3, column: 34, scope: !10)
!27 = !DILocation(line: 3, column: 13, scope: !10)
!28 = !DILocation(line: 4, column: 13, scope: !10)
!29 = !DILocation(line: 4, column: 4, scope: !10)
!30 = !DILocation(line: 4, column: 11, scope: !10)
!31 = !DILocation(line: 5, column: 1, scope: !10)
