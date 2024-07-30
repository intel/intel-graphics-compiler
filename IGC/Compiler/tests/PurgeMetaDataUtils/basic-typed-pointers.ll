;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt -igc-purgeMetaDataUtils-import -S < %s | FileCheck %s
; ------------------------------------------------
; PurgeMetaDataUtils
; ------------------------------------------------
;
; Test was produced from this ocl kernel
;
; __kernel void bar(__global int *dst, int src)
; {
;     dst[0] = src;
;
; }
;
; __kernel void foo(__global int *srcA)
; {
;     int src = srcA[1];
;     bar(srcA, src);
; }
;
; ------------------------------------------------

; Check that non-kernel functions without usage are properly purged with their MD
;
; CHECK-NOT: @bar
; CHECK: declare void @llvm.dbg.declare
;

; Function Attrs: noinline nounwind optnone
define internal spir_func void @bar(i32 addrspace(1)* %dst, i32 %src) #0 !dbg !22 {
entry:
  call void @llvm.genx.GenISA.CatchAllDebugLine(), !dbg !29
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %src.addr = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !30, metadata !DIExpression()), !dbg !31
  store i32 %src, i32* %src.addr, align 4
  call void @llvm.dbg.declare(metadata i32* %src.addr, metadata !32, metadata !DIExpression()), !dbg !33
  %0 = load i32, i32* %src.addr, align 4, !dbg !34
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !35
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %1, i64 0, !dbg !35
  store i32 %0, i32 addrspace(1)* %arrayidx, align 4, !dbg !36
  ret void, !dbg !37
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: noinline nounwind optnone
define spir_kernel void @foo(i32 addrspace(1)* %srcA) #0 !dbg !38 {
entry:
  %dst.addr.i = alloca i32 addrspace(1)*, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr.i, metadata !41, metadata !DIExpression()), !dbg !43
  %src.addr.i = alloca i32, align 4
  call void @llvm.dbg.declare(metadata i32* %src.addr.i, metadata !45, metadata !DIExpression()), !dbg !46
  call void @llvm.genx.GenISA.CatchAllDebugLine(), !dbg !47
  %srcA.addr = alloca i32 addrspace(1)*, align 8
  %src = alloca i32, align 4
  store i32 addrspace(1)* %srcA, i32 addrspace(1)** %srcA.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %srcA.addr, metadata !48, metadata !DIExpression()), !dbg !49
  call void @llvm.dbg.declare(metadata i32* %src, metadata !50, metadata !DIExpression()), !dbg !51
  %0 = load i32 addrspace(1)*, i32 addrspace(1)** %srcA.addr, align 8, !dbg !52
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 1, !dbg !52
  %1 = load i32, i32 addrspace(1)* %arrayidx, align 4, !dbg !52
  store i32 %1, i32* %src, align 4, !dbg !51
  %2 = load i32 addrspace(1)*, i32 addrspace(1)** %srcA.addr, align 8, !dbg !53
  %3 = load i32, i32* %src, align 4, !dbg !54
  %4 = bitcast i32 addrspace(1)** %dst.addr.i to i8*, !dbg !55
  call void @llvm.lifetime.start.p0i8(i64 8, i8* %4), !dbg !55
  %5 = bitcast i32* %src.addr.i to i8*, !dbg !55
  call void @llvm.lifetime.start.p0i8(i64 4, i8* %5), !dbg !55
  call void @llvm.genx.GenISA.CatchAllDebugLine() #2, !dbg !55
  store i32 addrspace(1)* %2, i32 addrspace(1)** %dst.addr.i, align 8
  store i32 %3, i32* %src.addr.i, align 4
  %6 = load i32, i32* %src.addr.i, align 4, !dbg !56
  %7 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr.i, align 8, !dbg !57
  store i32 %6, i32 addrspace(1)* %7, align 4, !dbg !58
  %8 = bitcast i32 addrspace(1)** %dst.addr.i to i8*, !dbg !59
  call void @llvm.lifetime.end.p0i8(i64 8, i8* %8), !dbg !59
  %9 = bitcast i32* %src.addr.i to i8*, !dbg !59
  call void @llvm.lifetime.end.p0i8(i64 4, i8* %9), !dbg !59
  ret void, !dbg !60
}

; MD checks
;
; check that inlined MD is preserved
; CHECK-NOT: distinct !DISubprogram(name: "bar"
; CHECK-NOT: DILocation(line: 1, column: 33
; CHECK-NOT: DILocation(line: 1, column: 42
; CHECK-NOT: DILocation(line: 3, column: 14
; CHECK-NOT: DILocation(line: 3, column: 5
; CHECK-NOT: DILocation(line: 3, column: 12
; CHECK-NOT: DILocation(line: 5, column: 1
; CHECK: [[SCOPE_FOO:![0-9]*]] = distinct !DISubprogram(name: "foo"
; CHECK: [[SCOPE_BAR:![0-9]*]] = distinct !DISubprogram(name: "bar"
; inline loc
; CHECK-COUNT-1: DILocation(line: 1, column: 33
; CHECK-SAME: scope: [[SCOPE_BAR]], inlinedAt: [[INL_LOC:![0-9]*]])
; CHECK-DAG: [[INL_LOC]] = distinct !DILocation(line: 10, column: 5, scope: [[SCOPE_FOO]])
; CHECK-COUNT-1: DILocation(line: 1, column: 42
; CHECK-SAME: scope: [[SCOPE_BAR]], inlinedAt: [[INL_LOC]])
; CHECK-COUNT-1: DILocation(line: 3, column: 14
; CHECK-SAME: scope: [[SCOPE_BAR]], inlinedAt: [[INL_LOC]])
; CHECK-COUNT-1: DILocation(line: 3, column: 5
; CHECK-SAME: scope: [[SCOPE_BAR]], inlinedAt: [[INL_LOC]])
; CHECK-COUNT-1: DILocation(line: 3, column: 12
; CHECK-SAME: scope: [[SCOPE_BAR]], inlinedAt: [[INL_LOC]])
; CHECK-COUNT-1: DILocation(line: 5, column: 1
; CHECK-SAME: scope: [[SCOPE_BAR]], inlinedAt: [[INL_LOC]])

; Function Attrs: nounwind
declare void @llvm.genx.GenISA.CatchAllDebugLine() #2

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture) #3

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture) #3

attributes #0 = { noinline nounwind optnone "less-precise-fpmad"="true" }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { nounwind }
attributes #3 = { argmemonly nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!IGCMetadata = !{!6}
!igc.functions = !{!14, !17}
!opencl.ocl.version = !{!20, !20, !20, !20, !20}
!opencl.spir.version = !{!20, !20, !20, !20, !20}
!llvm.ident = !{!21, !21, !21, !21, !21}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "dir")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{!"ModuleMD", !7}
!7 = !{!"FuncMD", !8, !9, !12, !13}
!8 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, i32)* @bar}
!9 = !{!"FuncMDValue[0]", !10, !11}
!10 = !{!"funcArgs"}
!11 = !{!"functionType", !"UserFunction"}
!12 = !{!"FuncMDMap[1]", void (i32 addrspace(1)*)* @foo}
!13 = !{!"FuncMDValue[1]", !10, !11}
!14 = !{void (i32 addrspace(1)*, i32)* @bar, !15}
!15 = !{!16}
!16 = !{!"function_type", i32 2}
!17 = !{void (i32 addrspace(1)*)* @foo, !18}
!18 = !{!19}
!19 = !{!"function_type", i32 0}
!20 = !{i32 2, i32 0}
!21 = !{!"clang version 10.0.0"}
!22 = distinct !DISubprogram(name: "bar", scope: null, file: !23, line: 1, type: !24, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!23 = !DIFile(filename: "PurgeMetaDataUtils.ll", directory: "dir")
!24 = !DISubroutineType(types: !25)
!25 = !{!26, !27, !28}
!26 = !DIBasicType(name: "int", size: 4)
!27 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !28, size: 64)
!28 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!29 = !DILocation(line: 1, scope: !22)
!30 = !DILocalVariable(name: "dst", arg: 1, scope: !22, file: !23, line: 1, type: !27)
!31 = !DILocation(line: 1, column: 33, scope: !22)
!32 = !DILocalVariable(name: "src", arg: 2, scope: !22, file: !23, line: 1, type: !28)
!33 = !DILocation(line: 1, column: 42, scope: !22)
!34 = !DILocation(line: 3, column: 14, scope: !22)
!35 = !DILocation(line: 3, column: 5, scope: !22)
!36 = !DILocation(line: 3, column: 12, scope: !22)
!37 = !DILocation(line: 5, column: 1, scope: !22)
!38 = distinct !DISubprogram(name: "foo", scope: null, file: !23, line: 7, type: !39, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!39 = !DISubroutineType(types: !40)
!40 = !{!26, !27}
!41 = !DILocalVariable(name: "dst", arg: 1, scope: !42, file: !23, line: 1, type: !27)
!42 = distinct !DISubprogram(name: "bar", scope: null, file: !23, line: 1, type: !24, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!43 = !DILocation(line: 1, column: 33, scope: !42, inlinedAt: !44)
!44 = distinct !DILocation(line: 10, column: 5, scope: !38)
!45 = !DILocalVariable(name: "src", arg: 2, scope: !42, file: !23, line: 1, type: !28)
!46 = !DILocation(line: 1, column: 42, scope: !42, inlinedAt: !44)
!47 = !DILocation(line: 7, scope: !38)
!48 = !DILocalVariable(name: "srcA", arg: 1, scope: !38, file: !23, line: 7, type: !27)
!49 = !DILocation(line: 7, column: 33, scope: !38)
!50 = !DILocalVariable(name: "src", scope: !38, file: !23, line: 9, type: !28)
!51 = !DILocation(line: 9, column: 9, scope: !38)
!52 = !DILocation(line: 9, column: 15, scope: !38)
!53 = !DILocation(line: 10, column: 9, scope: !38)
!54 = !DILocation(line: 10, column: 15, scope: !38)
!55 = !DILocation(line: 1, scope: !42, inlinedAt: !44)
!56 = !DILocation(line: 3, column: 14, scope: !42, inlinedAt: !44)
!57 = !DILocation(line: 3, column: 5, scope: !42, inlinedAt: !44)
!58 = !DILocation(line: 3, column: 12, scope: !42, inlinedAt: !44)
!59 = !DILocation(line: 5, column: 1, scope: !42, inlinedAt: !44)
!60 = !DILocation(line: 11, column: 1, scope: !38)
