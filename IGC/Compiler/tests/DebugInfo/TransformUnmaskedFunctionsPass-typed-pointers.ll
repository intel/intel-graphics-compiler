;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --transform-unmasked -S < %s | FileCheck %s
; ------------------------------------------------
; TransformUnmaskedFunctionsPass
; ------------------------------------------------
; This test checks that TransformUnmaskedFunctionsPass pass follows
; 'How to Update Debug Info' llvm guideline.
;
; And was reduced from ocl test kernel
; int foo(int src, int r)
; {
;   uchar l = convert_uchar(src);
;   bool a = l < 10;
;   bool b = r > 20;
;   bool c = a || b;
;   return c ? l : r;
; }
;
; __kernel void test_const(__global uint* dst, int s1, int s2)
; {
;   dst[0] = foo(s1, s2);
; }
;
; ------------------------------------------------

; CHECK: define {{.*}} @foo
; CHECK-SAME: !dbg [[FOO_SCOPE:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare(metadata i32* %
; CHECK-SAME: metadata [[SRC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SRC_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare(metadata i32* %
; CHECK-SAME: metadata [[R_MD:![0-9]*]], metadata !DIExpression()), !dbg [[R_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare(metadata i8* %
; CHECK-SAME: metadata [[L_MD:![0-9]*]], metadata !DIExpression()), !dbg [[L_LOC:![0-9]*]]
;
; CHECK: call spir_func i8 @__builtin_spirv_OpSConvert{{.*}} !dbg [[OPS_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare(metadata i8* %
; CHECK-SAME: metadata [[A_MD:![0-9]*]], metadata !DIExpression()), !dbg [[A_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare(metadata i8* %
; CHECK-SAME: metadata [[B_MD:![0-9]*]], metadata !DIExpression()), !dbg [[B_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare(metadata i8* %
; CHECK-SAME: metadata [[C_MD:![0-9]*]], metadata !DIExpression()), !dbg [[C_LOC:![0-9]*]]

define spir_func i32 @foo(i32 %src, i32 %r) #0 !dbg !5 {
entry:
  %src.addr = alloca i32, align 4
  %r.addr = alloca i32, align 4
  %l = alloca i8, align 1
  %a = alloca i8, align 1
  %b = alloca i8, align 1
  %c = alloca i8, align 1
  store i32 %src, i32* %src.addr, align 4
  call void @llvm.dbg.declare(metadata i32* %src.addr, metadata !10, metadata !DIExpression()), !dbg !11
  store i32 %r, i32* %r.addr, align 4
  call void @llvm.dbg.declare(metadata i32* %r.addr, metadata !12, metadata !DIExpression()), !dbg !13
  call void @llvm.dbg.declare(metadata i8* %l, metadata !14, metadata !DIExpression()), !dbg !18
  %0 = load i32, i32* %src.addr, align 4, !dbg !19
  %call = call spir_func i8 @__builtin_spirv_OpSConvert_i8_i32(i32 %0) #2, !dbg !20
  store i8 %call, i8* %l, align 1, !dbg !18
  call void @llvm.dbg.declare(metadata i8* %a, metadata !21, metadata !DIExpression()), !dbg !23
  %1 = load i8, i8* %l, align 1, !dbg !24
  %conv = call spir_func i32 @__builtin_spirv_OpUConvert_i32_i8(i8 %1) #2, !dbg !24
  %cmp = icmp slt i32 %conv, 10, !dbg !25
  %frombool = select i1 %cmp, i8 1, i8 0
  store i8 %frombool, i8* %a, align 1, !dbg !23
  call void @llvm.dbg.declare(metadata i8* %b, metadata !26, metadata !DIExpression()), !dbg !27
  %2 = load i32, i32* %r.addr, align 4, !dbg !28
  %cmp2 = icmp sgt i32 %2, 20, !dbg !29
  %frombool4 = select i1 %cmp2, i8 1, i8 0
  store i8 %frombool4, i8* %b, align 1, !dbg !27
  call void @llvm.dbg.declare(metadata i8* %c, metadata !30, metadata !DIExpression()), !dbg !31
  %3 = load i8, i8* %a, align 1, !dbg !32
  %tobool = icmp ne i8 %3, 0
  br i1 %tobool, label %lor.end, label %lor.rhs, !dbg !33

lor.rhs:                                          ; preds = %entry
  %4 = load i8, i8* %b, align 1, !dbg !34
  %tobool6 = icmp ne i8 %4, 0
  %i1promo = zext i1 %tobool6 to i8
  br label %lor.end, !dbg !33

lor.end:                                          ; preds = %lor.rhs, %entry
  %5 = phi i8 [ 1, %entry ], [ %i1promo, %lor.rhs ]
  %i1trunc = trunc i8 %5 to i1
  %frombool8 = select i1 %i1trunc, i8 1, i8 0
  store i8 %frombool8, i8* %c, align 1, !dbg !31
  %6 = load i8, i8* %c, align 1, !dbg !35
  %tobool9 = icmp ne i8 %6, 0
  br i1 %tobool9, label %cond.true, label %cond.false, !dbg !35

cond.true:                                        ; preds = %lor.end
  %7 = load i8, i8* %l, align 1, !dbg !36
  %conv11 = call spir_func i32 @__builtin_spirv_OpUConvert_i32_i8(i8 %7) #2, !dbg !36
  br label %cond.end, !dbg !35

cond.false:                                       ; preds = %lor.end
  %8 = load i32, i32* %r.addr, align 4, !dbg !37
  br label %cond.end, !dbg !35

cond.end:                                         ; preds = %cond.false, %cond.true
  %cond = phi i32 [ %conv11, %cond.true ], [ %8, %cond.false ], !dbg !35
  ret i32 %cond, !dbg !38
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "TransformUnmaskedFunctionsPass", directory: "/")
; CHECK-DAG: [[FOO_SCOPE]] = distinct !DISubprogram(name: "foo", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[SRC_MD]] = !DILocalVariable(name: "src", arg: 1, scope: [[FOO_SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[SRC_LOC]] = !DILocation(line: 1, column: 13, scope: [[FOO_SCOPE]])
; CHECK-DAG: [[R_MD]] = !DILocalVariable(name: "r", arg: 2, scope: [[FOO_SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[R_LOC]] = !DILocation(line: 1, column: 22, scope: [[FOO_SCOPE]])
; CHECK-DAG: [[L_MD]] = !DILocalVariable(name: "l", scope: [[FOO_SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[L_LOC]] = !DILocation(line: 3, column: 9, scope: [[FOO_SCOPE]])
; CHECK-DAG: [[A_MD]] = !DILocalVariable(name: "a", scope: [[FOO_SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[A_LOC]] = !DILocation(line: 4, column: 8, scope: [[FOO_SCOPE]])
; CHECK-DAG: [[OPS_LOC]] = !DILocation(line: 3, column: 13, scope: [[FOO_SCOPE]])
; CHECK-DAG: [[B_MD]] = !DILocalVariable(name: "b", scope: [[FOO_SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[B_LOC]] = !DILocation(line: 5, column: 8, scope: [[FOO_SCOPE]])
; CHECK-DAG: [[C_MD]] = !DILocalVariable(name: "c", scope: [[FOO_SCOPE]], file: [[FILE]], line: 6
; CHECK-DAG: [[C_LOC]] = !DILocation(line: 6, column: 8, scope: [[FOO_SCOPE]])

declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

declare spir_func i8 @__builtin_spirv_OpSConvert_i8_i32(i32) #2

declare spir_func i32 @__builtin_spirv_OpUConvert_i32_i8(i8) #2

define spir_kernel void @test_const(i32 addrspace(1)* %dst, i32 %s1, i32 %s2) #3 !dbg !39 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %s1.addr = alloca i32, align 4
  %s2.addr = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !46, metadata !DIExpression()), !dbg !47
  store i32 %s1, i32* %s1.addr, align 4
  call void @llvm.dbg.declare(metadata i32* %s1.addr, metadata !48, metadata !DIExpression()), !dbg !49
  store i32 %s2, i32* %s2.addr, align 4
  call void @llvm.dbg.declare(metadata i32* %s2.addr, metadata !50, metadata !DIExpression()), !dbg !51
  %0 = load i32, i32* %s1.addr, align 4, !dbg !52
  %1 = load i32, i32* %s2.addr, align 4, !dbg !53
  %call = call spir_func i32 @foo(i32 %0, i32 %1) #3, !dbg !54
  %2 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !55
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %2, i64 0, !dbg !55
  store i32 %call, i32 addrspace(1)* %arrayidx, align 4, !dbg !56
  ret void, !dbg !57
}

attributes #0 = { alwaysinline nounwind "sycl-unmasked" }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { nounwind }
attributes #3 = { noinline nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4}
!igc.functions = !{}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "/")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = distinct !DISubprogram(name: "foo", scope: null, file: !6, line: 1, type: !7, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!6 = !DIFile(filename: "TransformUnmaskedFunctionsPass", directory: "/")
!7 = !DISubroutineType(types: !8)
!8 = !{!9, !9, !9}
!9 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!10 = !DILocalVariable(name: "src", arg: 1, scope: !5, file: !6, line: 1, type: !9)
!11 = !DILocation(line: 1, column: 13, scope: !5)
!12 = !DILocalVariable(name: "r", arg: 2, scope: !5, file: !6, line: 1, type: !9)
!13 = !DILocation(line: 1, column: 22, scope: !5)
!14 = !DILocalVariable(name: "l", scope: !5, file: !6, line: 3, type: !15)
!15 = !DIDerivedType(tag: DW_TAG_typedef, name: "uchar", file: !16, baseType: !17)
!16 = !DIFile(filename: "header.h", directory: "/")
!17 = !DIBasicType(name: "unsigned char", size: 8, encoding: DW_ATE_unsigned_char)
!18 = !DILocation(line: 3, column: 9, scope: !5)
!19 = !DILocation(line: 3, column: 27, scope: !5)
!20 = !DILocation(line: 3, column: 13, scope: !5)
!21 = !DILocalVariable(name: "a", scope: !5, file: !6, line: 4, type: !22)
!22 = !DIBasicType(name: "bool", size: 8, encoding: DW_ATE_boolean)
!23 = !DILocation(line: 4, column: 8, scope: !5)
!24 = !DILocation(line: 4, column: 12, scope: !5)
!25 = !DILocation(line: 4, column: 14, scope: !5)
!26 = !DILocalVariable(name: "b", scope: !5, file: !6, line: 5, type: !22)
!27 = !DILocation(line: 5, column: 8, scope: !5)
!28 = !DILocation(line: 5, column: 12, scope: !5)
!29 = !DILocation(line: 5, column: 14, scope: !5)
!30 = !DILocalVariable(name: "c", scope: !5, file: !6, line: 6, type: !22)
!31 = !DILocation(line: 6, column: 8, scope: !5)
!32 = !DILocation(line: 6, column: 12, scope: !5)
!33 = !DILocation(line: 6, column: 14, scope: !5)
!34 = !DILocation(line: 6, column: 17, scope: !5)
!35 = !DILocation(line: 7, column: 10, scope: !5)
!36 = !DILocation(line: 7, column: 14, scope: !5)
!37 = !DILocation(line: 7, column: 18, scope: !5)
!38 = !DILocation(line: 7, column: 3, scope: !5)
!39 = distinct !DISubprogram(name: "test_const", scope: null, file: !6, line: 10, type: !40, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!40 = !DISubroutineType(types: !41)
!41 = !{!42, !43, !9, !9}
!42 = !DIBasicType(name: "int", size: 4)
!43 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !44, size: 64)
!44 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint", file: !16, baseType: !45)
!45 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!46 = !DILocalVariable(name: "dst", arg: 1, scope: !39, file: !6, line: 10, type: !43)
!47 = !DILocation(line: 10, column: 41, scope: !39)
!48 = !DILocalVariable(name: "s1", arg: 2, scope: !39, file: !6, line: 10, type: !9)
!49 = !DILocation(line: 10, column: 50, scope: !39)
!50 = !DILocalVariable(name: "s2", arg: 3, scope: !39, file: !6, line: 10, type: !9)
!51 = !DILocation(line: 10, column: 58, scope: !39)
!52 = !DILocation(line: 12, column: 16, scope: !39)
!53 = !DILocation(line: 12, column: 20, scope: !39)
!54 = !DILocation(line: 12, column: 12, scope: !39)
!55 = !DILocation(line: 12, column: 3, scope: !39)
!56 = !DILocation(line: 12, column: 10, scope: !39)
!57 = !DILocation(line: 13, column: 1, scope: !39)
