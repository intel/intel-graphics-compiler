;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
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
;   int x  = get_global_offset(0);
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
; CHECK: [[X_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[XC_LOC:![0-9]*]]
; CHECK: store i32 [[X_V]], i32* %x, align 4, !dbg [[X_LOC]]

define spir_kernel void @test_wi(i32 addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %bufferOffset) #0 !dbg !15 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %x = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !24, metadata !DIExpression()), !dbg !25
  call void @llvm.dbg.declare(metadata i32* %x, metadata !26, metadata !DIExpression()), !dbg !28
  %call.i = call spir_func i32 @__builtin_IB_get_global_offset(i32 0), !dbg !29
  store i32 %call.i, i32* %x, align 4, !dbg !28
  %0 = load i32, i32* %x, align 4, !dbg !30
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !31
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %1, i64 0, !dbg !31
  store i32 %0, i32 addrspace(1)* %arrayidx, align 4, !dbg !32
  ret void, !dbg !33
}

; Check MD:
;
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "global_off.ll", directory: "/")
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
declare spir_func i32 @__builtin_IB_get_global_offset(i32) local_unnamed_addr #2

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
!6 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8*, i32)* @test_wi, !7}
!7 = !{!8, !9}
!8 = !{!"function_type", i32 0}
!9 = !{!"implicit_arg_desc", !10, !11, !12, !13}
!10 = !{i32 0}
!11 = !{i32 1}
!12 = !{i32 13}
!13 = !{i32 15, !14}
!14 = !{!"explicit_arg_num", i32 0}
!15 = distinct !DISubprogram(name: "test_wi", scope: null, file: !16, line: 1, type: !17, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!16 = !DIFile(filename: "global_off.ll", directory: "/")
!17 = !DISubroutineType(types: !18)
!18 = !{!19, !20}
!19 = !DIBasicType(name: "int", size: 4)
!20 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !21, size: 64)
!21 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint", file: !22, baseType: !23)
!22 = !DIFile(filename: "opencl-c-base.h", directory: "/")
!23 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!24 = !DILocalVariable(name: "dst", arg: 1, scope: !15, file: !16, line: 1, type: !20)
!25 = !DILocation(line: 1, column: 38, scope: !15)
!26 = !DILocalVariable(name: "x", scope: !15, file: !16, line: 3, type: !27)
!27 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!28 = !DILocation(line: 3, column: 7, scope: !15)
!29 = !DILocation(line: 3, column: 12, scope: !15)
!30 = !DILocation(line: 4, column: 12, scope: !15)
!31 = !DILocation(line: 4, column: 3, scope: !15)
!32 = !DILocation(line: 4, column: 10, scope: !15)
!33 = !DILocation(line: 5, column: 1, scope: !15)
