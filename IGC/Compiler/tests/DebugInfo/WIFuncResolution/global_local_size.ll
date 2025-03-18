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
;   int x  = get_global_size(0);
;   int y = get_local_size(0);
;   dst[0] = x + y;
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
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: %y, metadata [[Y_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[Y_LOC:![0-9]*]]
; CHECK: [[Y_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[YC_LOC:![0-9]*]]
; CHECK: store i32 [[Y_V]], i32* %y, align 4, !dbg [[Y_LOC]]

define spir_kernel void @test_wi(i32 addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, <3 x i32> %globalSize, <3 x i32> %localSize, i8* %privateBase, i32 %bufferOffset) #0 !dbg !17 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %x = alloca i32, align 4
  %y = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !26, metadata !DIExpression()), !dbg !27
  call void @llvm.dbg.declare(metadata i32* %x, metadata !28, metadata !DIExpression()), !dbg !30
  %call.i = call spir_func i32 @__builtin_IB_get_global_size(i32 0), !dbg !31
  store i32 %call.i, i32* %x, align 4, !dbg !30
  call void @llvm.dbg.declare(metadata i32* %y, metadata !32, metadata !DIExpression()), !dbg !33
  %call.i2 = call spir_func i32 @__builtin_IB_get_local_size(i32 0), !dbg !34
  store i32 %call.i2, i32* %y, align 4, !dbg !33
  %0 = load i32, i32* %x, align 4, !dbg !35
  %1 = load i32, i32* %y, align 4, !dbg !36
  %add = add nsw i32 %0, %1, !dbg !37
  %2 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !38
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %2, i64 0, !dbg !38
  store i32 %add, i32 addrspace(1)* %arrayidx, align 4, !dbg !39
  ret void, !dbg !40
}

; Check MD:
;
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "global_local_size.ll", directory: "/")
; CHECK-DAG: [[SCOPE:![0-9]*]] = distinct !DISubprogram(name: "test_wi", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[DST_MD]] = !DILocalVariable(name: "dst", arg: 1, scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[DST_LOC]] = !DILocation(line: 1, column: 38, scope: [[SCOPE]])
;
; CHECK-DAG: [[X_MD]] = !DILocalVariable(name: "x", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[X_LOC]] = !DILocation(line: 3, column: 7, scope: [[SCOPE]])
; CHECK-DAG: [[XC_LOC]] = !DILocation(line: 3, column: 12, scope: [[SCOPE]])
;
; CHECK-DAG: [[Y_MD]] = !DILocalVariable(name: "y", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[Y_LOC]] = !DILocation(line: 4, column: 7, scope: [[SCOPE]])
; CHECK-DAG: [[YC_LOC]] = !DILocation(line: 4, column: 11, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_local_size(i32) local_unnamed_addr #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_global_size(i32) local_unnamed_addr #2

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
!6 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, <3 x i32>, <3 x i32>, i8*, i32)* @test_wi, !7}
!7 = !{!8, !9}
!8 = !{!"function_type", i32 0}
!9 = !{!"implicit_arg_desc", !10, !11, !12, !13, !14, !15}
!10 = !{i32 0}
!11 = !{i32 1}
!12 = !{i32 5}
!13 = !{i32 6}
!14 = !{i32 13}
!15 = !{i32 15, !16}
!16 = !{!"explicit_arg_num", i32 0}
!17 = distinct !DISubprogram(name: "test_wi", scope: null, file: !18, line: 1, type: !19, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!18 = !DIFile(filename: "global_local_size.ll", directory: "/")
!19 = !DISubroutineType(types: !20)
!20 = !{!21, !22}
!21 = !DIBasicType(name: "int", size: 4)
!22 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !23, size: 64)
!23 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint", file: !24, baseType: !25)
!24 = !DIFile(filename: "opencl-c-base.h", directory: "/")
!25 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!26 = !DILocalVariable(name: "dst", arg: 1, scope: !17, file: !18, line: 1, type: !22)
!27 = !DILocation(line: 1, column: 38, scope: !17)
!28 = !DILocalVariable(name: "x", scope: !17, file: !18, line: 3, type: !29)
!29 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!30 = !DILocation(line: 3, column: 7, scope: !17)
!31 = !DILocation(line: 3, column: 12, scope: !17)
!32 = !DILocalVariable(name: "y", scope: !17, file: !18, line: 4, type: !29)
!33 = !DILocation(line: 4, column: 7, scope: !17)
!34 = !DILocation(line: 4, column: 11, scope: !17)
!35 = !DILocation(line: 5, column: 12, scope: !17)
!36 = !DILocation(line: 5, column: 16, scope: !17)
!37 = !DILocation(line: 5, column: 14, scope: !17)
!38 = !DILocation(line: 5, column: 3, scope: !17)
!39 = !DILocation(line: 5, column: 10, scope: !17)
!40 = !DILocation(line: 6, column: 1, scope: !17)
