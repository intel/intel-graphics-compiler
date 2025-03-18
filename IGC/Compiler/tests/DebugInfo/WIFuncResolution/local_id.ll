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
;   int x = get_local_id(0);
;   int y = get_local_id(1);
;   int z = get_local_id(2);
;   dst[0] = x + y + z;
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
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: %z, metadata [[Z_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[Z_LOC:![0-9]*]]
; CHECK: [[Z_V:%[A-z0-9.]*]] = {{.*}}, !dbg [[ZC_LOC:![0-9]*]]
; CHECK: store i32 [[Z_V]], i32* %z, align 4, !dbg [[Z_LOC]]

define spir_kernel void @test_wi(i32 addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i32 %bufferOffset) #0 !dbg !18 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %x = alloca i32, align 4
  %y = alloca i32, align 4
  %z = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !27, metadata !DIExpression()), !dbg !28
  call void @llvm.dbg.declare(metadata i32* %x, metadata !29, metadata !DIExpression()), !dbg !31
  %call.i = call spir_func i32 @__builtin_IB_get_local_id_x(), !dbg !32
  store i32 %call.i, i32* %x, align 4, !dbg !31
  call void @llvm.dbg.declare(metadata i32* %y, metadata !33, metadata !DIExpression()), !dbg !34
  %call1.i = call spir_func i32 @__builtin_IB_get_local_id_y(), !dbg !35
  store i32 %call1.i, i32* %y, align 4, !dbg !34
  call void @llvm.dbg.declare(metadata i32* %z, metadata !36, metadata !DIExpression()), !dbg !37
  %call2.i = call spir_func i32 @__builtin_IB_get_local_id_z(), !dbg !38
  store i32 %call2.i, i32* %z, align 4, !dbg !37
  %0 = load i32, i32* %x, align 4, !dbg !39
  %1 = load i32, i32* %y, align 4, !dbg !40
  %add = add nsw i32 %0, %1, !dbg !41
  %2 = load i32, i32* %z, align 4, !dbg !42
  %add5 = add nsw i32 %add, %2, !dbg !43
  %3 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !44
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %3, i64 0, !dbg !44
  store i32 %add5, i32 addrspace(1)* %arrayidx, align 4, !dbg !45
  ret void, !dbg !46
}

; Check MD:
;
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "local_id.ll", directory: "/")
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
;
; CHECK-DAG: [[Z_MD]] = !DILocalVariable(name: "z", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[Z_LOC]] = !DILocation(line: 5, column: 7, scope: [[SCOPE]])
; CHECK-DAG: [[ZC_LOC]] = !DILocation(line: 5, column: 11, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_local_id_x() local_unnamed_addr #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_local_id_y() local_unnamed_addr #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_local_id_z() local_unnamed_addr #2

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
!6 = !{void (i32 addrspace(1)*, <8 x i32>, <8 x i32>, i16, i16, i16, i8*, i32)* @test_wi, !7}
!7 = !{!8, !9}
!8 = !{!"function_type", i32 0}
!9 = !{!"implicit_arg_desc", !10, !11, !12, !13, !14, !15, !16}
!10 = !{i32 0}
!11 = !{i32 1}
!12 = !{i32 8}
!13 = !{i32 9}
!14 = !{i32 10}
!15 = !{i32 13}
!16 = !{i32 15, !17}
!17 = !{!"explicit_arg_num", i32 0}
!18 = distinct !DISubprogram(name: "test_wi", scope: null, file: !19, line: 1, type: !20, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!19 = !DIFile(filename: "local_id.ll", directory: "/")
!20 = !DISubroutineType(types: !21)
!21 = !{!22, !23}
!22 = !DIBasicType(name: "int", size: 4)
!23 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !24, size: 64)
!24 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint", file: !25, baseType: !26)
!25 = !DIFile(filename: "opencl-c-base.h", directory: "/")
!26 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!27 = !DILocalVariable(name: "dst", arg: 1, scope: !18, file: !19, line: 1, type: !23)
!28 = !DILocation(line: 1, column: 38, scope: !18)
!29 = !DILocalVariable(name: "x", scope: !18, file: !19, line: 3, type: !30)
!30 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!31 = !DILocation(line: 3, column: 7, scope: !18)
!32 = !DILocation(line: 3, column: 12, scope: !18)
!33 = !DILocalVariable(name: "y", scope: !18, file: !19, line: 4, type: !30)
!34 = !DILocation(line: 4, column: 7, scope: !18)
!35 = !DILocation(line: 4, column: 11, scope: !18)
!36 = !DILocalVariable(name: "z", scope: !18, file: !19, line: 5, type: !30)
!37 = !DILocation(line: 5, column: 7, scope: !18)
!38 = !DILocation(line: 5, column: 11, scope: !18)
!39 = !DILocation(line: 6, column: 12, scope: !18)
!40 = !DILocation(line: 6, column: 16, scope: !18)
!41 = !DILocation(line: 6, column: 14, scope: !18)
!42 = !DILocation(line: 6, column: 20, scope: !18)
!43 = !DILocation(line: 6, column: 18, scope: !18)
!44 = !DILocation(line: 6, column: 3, scope: !18)
!45 = !DILocation(line: 6, column: 10, scope: !18)
!46 = !DILocation(line: 7, column: 1, scope: !18)
