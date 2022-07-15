;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: igc_opt -igc-kernel-function-cloning -S < %s | FileCheck %s
; ------------------------------------------------
; KernelFunctionCloning
; ------------------------------------------------
; This test checks that KernelFunctionCloning pass follows
; 'How to Update Debug Info' llvm guideline.
;
; And was reThis test is reduced from ocl test kernel:
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

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: noinline nounwind
define spir_kernel void @bar(i32 addrspace(1)* %dst, i32 %src) #0 !dbg !18 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %src.addr = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !25, metadata !DIExpression()), !dbg !26
  store i32 %src, i32* %src.addr, align 4
  call void @llvm.dbg.declare(metadata i32* %src.addr, metadata !27, metadata !DIExpression()), !dbg !28
  %0 = load i32, i32* %src.addr, align 4, !dbg !29
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !30
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %1, i64 0, !dbg !30
  store i32 %0, i32 addrspace(1)* %arrayidx, align 4, !dbg !31
  ret void, !dbg !32
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: noinline nounwind
define spir_kernel void @foo(i32 addrspace(1)* %srcA) #0 !dbg !33 {
entry:
  %srcA.addr = alloca i32 addrspace(1)*, align 8
  %src = alloca i32, align 4
  store i32 addrspace(1)* %srcA, i32 addrspace(1)** %srcA.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %srcA.addr, metadata !36, metadata !DIExpression()), !dbg !37
  call void @llvm.dbg.declare(metadata i32* %src, metadata !38, metadata !DIExpression()), !dbg !39
  %0 = load i32 addrspace(1)*, i32 addrspace(1)** %srcA.addr, align 8, !dbg !40
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 1, !dbg !40
  %1 = load i32, i32 addrspace(1)* %arrayidx, align 4, !dbg !40
  store i32 %1, i32* %src, align 4, !dbg !39
  %2 = load i32 addrspace(1)*, i32 addrspace(1)** %srcA.addr, align 8, !dbg !41
  %3 = load i32, i32* %src, align 4, !dbg !42
;
; Check cloned kernel call location
;
; CHECK: call spir_kernel void @bar{{.*}}, !dbg [[CALL_LOC:![0-9]*]]

  call spir_kernel void @bar(i32 addrspace(1)* %2, i32 %3) #0, !dbg !43
  ret void, !dbg !44
}
;
; Check cloned function
;
; CHECK: internal {{.*}} @bar{{.*}} !dbg [[CFUNC_MD:![0-9]*]] {
; CHECK: dbg.declare({{.*}}, metadata [[CDECL_DST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CDECL_DST_LOC:![0-9]*]]
; CHECK: dbg.declare({{.*}}, metadata [[CDECL_SRC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[CDECL_SRC_LOC:![0-9]*]]
; CHECK-NEXT: load {{.*}}, !dbg [[LOAD_SRC_LOC:![0-9]*]]
; CHECK-NEXT: load {{.*}}, !dbg [[LOAD_DST_LOC:![0-9]*]]
; CHECK-NEXT: getelementptr {{.*}}, !dbg [[LOAD_DST_LOC]]
; CHECK-NEXT: store {{.*}}, !dbg [[STORE_LOC:![0-9]*]]

; Checked cloned MD
;
; CHECK-DAG: [[FOO_MD:![0-9]*]] = distinct !DISubprogram(name: "foo"
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 10, column: 5, scope: [[FOO_MD]])
; CHECK-DAG: [[CFUNC_MD]] = distinct !DISubprogram(name: "bar"
; CHECK-DAG: [[CDECL_DST_MD]] = !DILocalVariable(name: "dst", arg: 1, scope: [[CFUNC_MD]], file: [[FILE_MD:![0-9]*]], line: 1, type: [[PTR_T:![0-9]*]])
; CHECK-DAG: [[FILE_MD]] = !DIFile(filename: "1", directory: "/dir")
; CHECK-DAG: [[PTR_T]] = !DIDerivedType(tag: DW_TAG_pointer_type
; CHECK-DAG: [[CDECL_SRC_MD]] = !DILocalVariable(name: "src", arg: 2, scope: [[CFUNC_MD]], file: [[FILE_MD:![0-9]*]], line: 1, type: [[INT_T:![0-9]*]])
; CHECK-DAG: [[INT_T]] = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
; CHECK-DAG: [[LOAD_SRC_LOC]] = !DILocation(line: 3, column: 14, scope: [[CFUNC_MD]])
; CHECK-DAG: [[LOAD_DST_LOC]] = !DILocation(line: 3, column: 5, scope: [[CFUNC_MD]])
; CHECK-DAG: [[STORE_LOC]] = !DILocation(line: 3, column: 12, scope: [[CFUNC_MD]])

attributes #0 = { noinline nounwind }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4}
!IGCMetadata = !{!5}
!igc.functions = !{!14, !17}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "/dir")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{!"ModuleMD", !6}
!6 = !{!"FuncMD", !7, !8, !12, !13}
!7 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, i32)* @bar}
!8 = !{!"FuncMDValue[0]", !9, !10, !11}
!9 = !{!"funcArgs"}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"isCloned", i1 false}
!12 = !{!"FuncMDMap[1]", void (i32 addrspace(1)*)* @foo}
!13 = !{!"FuncMDValue[1]", !9, !10, !11}
!14 = !{void (i32 addrspace(1)*, i32)* @bar, !15}
!15 = !{!16}
!16 = !{!"function_type", i32 0}
!17 = !{void (i32 addrspace(1)*)* @foo, !15}
!18 = distinct !DISubprogram(name: "bar", scope: null, file: !19, line: 1, type: !20, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!19 = !DIFile(filename: "1", directory: "/dir")
!20 = !DISubroutineType(types: !21)
!21 = !{!22, !23, !24}
!22 = !DIBasicType(name: "int", size: 4)
!23 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !24, size: 64)
!24 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!25 = !DILocalVariable(name: "dst", arg: 1, scope: !18, file: !19, line: 1, type: !23)
!26 = !DILocation(line: 1, column: 33, scope: !18)
!27 = !DILocalVariable(name: "src", arg: 2, scope: !18, file: !19, line: 1, type: !24)
!28 = !DILocation(line: 1, column: 42, scope: !18)
!29 = !DILocation(line: 3, column: 14, scope: !18)
!30 = !DILocation(line: 3, column: 5, scope: !18)
!31 = !DILocation(line: 3, column: 12, scope: !18)
!32 = !DILocation(line: 5, column: 1, scope: !18)
!33 = distinct !DISubprogram(name: "foo", scope: null, file: !19, line: 7, type: !34, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!34 = !DISubroutineType(types: !35)
!35 = !{!22, !23}
!36 = !DILocalVariable(name: "srcA", arg: 1, scope: !33, file: !19, line: 7, type: !23)
!37 = !DILocation(line: 7, column: 33, scope: !33)
!38 = !DILocalVariable(name: "src", scope: !33, file: !19, line: 9, type: !24)
!39 = !DILocation(line: 9, column: 9, scope: !33)
!40 = !DILocation(line: 9, column: 15, scope: !33)
!41 = !DILocation(line: 10, column: 9, scope: !33)
!42 = !DILocation(line: 10, column: 15, scope: !33)
!43 = !DILocation(line: 10, column: 5, scope: !33)
!44 = !DILocation(line: 11, column: 1, scope: !33)
