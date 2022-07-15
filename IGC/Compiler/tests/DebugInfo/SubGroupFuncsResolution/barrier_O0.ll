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
; __kernel void test_bar(__global uint* dst, int src)
; {
;    sub_group_barrier(CLK_LOCAL_MEM_FENCE);
;    dst[0] = src;
; }
;
; ------------------------------------------------

; Sanity check:
; CHECK: @test_bar{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: dbg.declare{{.*}} addrspace
; CHECK-SAME: metadata [[DST_MD:![0-9]*]]
; CHECK-SAME: !dbg [[DST_LOC:![0-9]*]]
; CHECK: dbg.declare{{.*}} i32
; CHECK-SAME: metadata [[SRC_MD:![0-9]*]]
; CHECK-SAME: !dbg [[SRC_LOC:![0-9]*]]
;
; Check that call line is not lost
; CHECK: call {{.*}}, !dbg [[CALL_LOC:![0-9]*]]

; CHECK: store i32 {{.*}}, i32 addrspace{{.*}}, !dbg [[DST_STORE_LOC:![0-9]*]]

define spir_kernel void @test_bar(i32 addrspace(1)* %dst, i32 %src) #0 !dbg !9 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %src.addr = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !19, metadata !DIExpression()), !dbg !20
  store i32 %src, i32* %src.addr, align 4
  call void @llvm.dbg.declare(metadata i32* %src.addr, metadata !21, metadata !DIExpression()), !dbg !22
  call spir_func void @__builtin_IB_sub_group_barrier(), !dbg !23
  %0 = load i32, i32* %src.addr, align 4, !dbg !24
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !25
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %1, i64 0, !dbg !25
  store i32 %0, i32 addrspace(1)* %arrayidx, align 4, !dbg !26
  ret void, !dbg !27
}

; Check MD:
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "barrier_O0.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_bar", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[DST_LOC]] = !DILocation(line: 1, column: 39, scope: [[SCOPE]])
; CHECK-DAG: [[DST_MD]] = !DILocalVariable(name: "dst", arg: 1, scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[SRC_LOC]] = !DILocation(line: 1, column: 48, scope: [[SCOPE]])
; CHECK-DAG: [[SRC_MD]] = !DILocalVariable(name: "src", arg: 2, scope: [[SCOPE]], file: [[FILE]], line: 1

; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 3, column: 4, scope: [[SCOPE]])
; CHECK-DAG: [[DST_STORE_LOC]] = !DILocation(line: 4, column: 11, scope: [[SCOPE]])

declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

declare spir_func void @__builtin_IB_sub_group_barrier() local_unnamed_addr #0

attributes #0 = { convergent noinline nounwind optnone }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!IGCMetadata = !{!6}
!igc.functions = !{}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "/")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{!"ModuleMD", !7}
!7 = !{!"compOpt", !8}
!8 = !{!"OptDisable", i1 true}
!9 = distinct !DISubprogram(name: "test_bar", scope: null, file: !10, line: 1, type: !11, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!10 = !DIFile(filename: "barrier_O0.ll", directory: "/")
!11 = !DISubroutineType(types: !12)
!12 = !{!13, !14, !18}
!13 = !DIBasicType(name: "int", size: 4)
!14 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !15, size: 64)
!15 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint", file: !16, baseType: !17)
!16 = !DIFile(filename: "opencl-c-base.h", directory: "/")
!17 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!18 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!19 = !DILocalVariable(name: "dst", arg: 1, scope: !9, file: !10, line: 1, type: !14)
!20 = !DILocation(line: 1, column: 39, scope: !9)
!21 = !DILocalVariable(name: "src", arg: 2, scope: !9, file: !10, line: 1, type: !18)
!22 = !DILocation(line: 1, column: 48, scope: !9)
!23 = !DILocation(line: 3, column: 4, scope: !9)
!24 = !DILocation(line: 4, column: 13, scope: !9)
!25 = !DILocation(line: 4, column: 4, scope: !9)
!26 = !DILocation(line: 4, column: 11, scope: !9)
!27 = !DILocation(line: 5, column: 1, scope: !9)
