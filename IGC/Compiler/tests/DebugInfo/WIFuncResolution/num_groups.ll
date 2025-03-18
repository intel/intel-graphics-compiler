;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --typed-pointers -igc-wi-func-resolution -S < %s | FileCheck %s
; ------------------------------------------------
; WIFuncResolution
; ------------------------------------------------
; This test checks that WIFuncResolution pass follows
; 'How to Update Debug Info' llvm guideline.
;
; And was reduced from ocl test kernel:
;
; __kernel void test_wi(__global uint* dst)
; {
;   int x  = get_num_groups(0);
;   dst[0] = x;
; }
;
; ------------------------------------------------

; Check IR:
;
; CHECK: define spir_kernel void @test_wi
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: addrspace{{.*}}, metadata [[DST_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[DST_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: %x, metadata [[X_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[X_LOC:![0-9]*]]
; CHECK-DAG: store i32 [[X_V:%[A-z0-9.]*]], i32* %x, align 4, !dbg [[X_LOC]]
; CHECK-DAG: [[X_V]] = {{.*}}, !dbg [[XC_LOC:![0-9]*]]

define spir_kernel void @test_wi(i32 addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %numWorkGroups, i8* %privateBase, i32 %bufferOffset) #0 !dbg !16 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %x = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !25, metadata !DIExpression()), !dbg !26
  call void @llvm.dbg.declare(metadata i32* %x, metadata !27, metadata !DIExpression()), !dbg !29
  %call.i = call spir_func i32 @__builtin_IB_get_num_groups(i32 0), !dbg !30
  store i32 %call.i, i32* %x, align 4, !dbg !29
  %0 = load i32, i32* %x, align 4, !dbg !31
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !32
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %1, i64 0, !dbg !32
  store i32 %0, i32 addrspace(1)* %arrayidx, align 4, !dbg !33
  ret void, !dbg !34
}

; Check MD:
;
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "num_groups.ll", directory: "/")
; CHECK-DAG: [[SCOPE:![0-9]*]] = distinct !DISubprogram(name: "test_wi", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[DST_MD]] = !DILocalVariable(name: "dst", arg: 1, scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[DST_LOC]] = !DILocation(line: 1, column: 38, scope: [[SCOPE]])
;
; CHECK-DAG: [[X_MD]] = !DILocalVariable(name: "x", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[X_LOC]] = !DILocation(line: 3, column: 7, scope: [[SCOPE]])
; CHECK-DAG: [[XC_LOC]] = !DILocation(line: 3, column: 12, scope: [[SCOPE]])
;

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_num_groups(i32) local_unnamed_addr #2

attributes #0 = { convergent noinline nounwind optnone }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { convergent nounwind readnone }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!igc.functions = !{!6}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "/")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, i8*, i32)* @test_wi, !7}
!7 = !{!8, !9}
!8 = !{!"function_type", i32 0}
!9 = !{!"implicit_arg_desc", !10, !11, !12, !13, !14}
!10 = !{i32 0}
!11 = !{i32 1}
!12 = !{i32 4}
!13 = !{i32 13}
!14 = !{i32 15, !15}
!15 = !{!"explicit_arg_num", i32 0}
!16 = distinct !DISubprogram(name: "test_wi", scope: null, file: !17, line: 1, type: !18, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!17 = !DIFile(filename: "num_groups.ll", directory: "/")
!18 = !DISubroutineType(types: !19)
!19 = !{!20, !21}
!20 = !DIBasicType(name: "int", size: 4)
!21 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !22, size: 64)
!22 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint", file: !23, baseType: !24)
!23 = !DIFile(filename: "opencl-c-base.h", directory: "/")
!24 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!25 = !DILocalVariable(name: "dst", arg: 1, scope: !16, file: !17, line: 1, type: !21)
!26 = !DILocation(line: 1, column: 38, scope: !16)
!27 = !DILocalVariable(name: "x", scope: !16, file: !17, line: 3, type: !28)
!28 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!29 = !DILocation(line: 3, column: 7, scope: !16)
!30 = !DILocation(line: 3, column: 12, scope: !16)
!31 = !DILocation(line: 4, column: 12, scope: !16)
!32 = !DILocation(line: 4, column: 3, scope: !16)
!33 = !DILocation(line: 4, column: 10, scope: !16)
!34 = !DILocation(line: 5, column: 1, scope: !16)
