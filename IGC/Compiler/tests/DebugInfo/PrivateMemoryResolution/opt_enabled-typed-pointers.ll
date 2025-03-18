;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --typed-pointers --igc-private-mem-resolution -S < %s | FileCheck %s
; ------------------------------------------------
; PrivateMemoryResolution
; ------------------------------------------------
; This test checks that PrivateMemoryResolution pass follows
; 'How to Update Debug Info' llvm guideline.
;
; And was reduced from ocl test kernel:
;
; int test_add(unsigned int a, unsigned int b)
; {
;   int c = a + b;
;   return c;
; }
;
; __kernel void test_pmem(__global unsigned int* dst, __global unsigned int* src)
; {
;   int aa = src[0];
;   int bb = src[1];
;   int cc = test_add(aa, bb);
;   dst[0] = cc;
;   int dd = src[2];
;   int ee = src[3];
;   dst[1] = ee;
; }
;
; ------------------------------------------------



; CHECK: define{{.*}} @test_pmem{{.*}} !dbg [[KSCOPE:![0-9]*]]
; CHECK: @llvm.dbg.declare(metadata i32 addrspace(1)** [[DST_V:%[A-z0-9.]*]], metadata [[DST_MD:![0-9]*]], metadata !DIExpression()), !dbg [[DST_LOC:![0-9]*]]
; CHECK: store i32 addrspace(1)* %dst, i32 addrspace(1)** [[DST_V]], align 8, !dbg [[DST_LOC]]
; CHECK: @llvm.dbg.declare(metadata i32 addrspace(1)** [[SRC_V:%[A-z0-9.]*]], metadata [[SRC_MD:![0-9]*]], metadata !DIExpression()), !dbg [[SRC_LOC:![0-9]*]]
; CHECK: store i32 addrspace(1)* %src, i32 addrspace(1)** [[SRC_V]], align 8, !dbg [[SRC_LOC]]
; CHECK: @llvm.dbg.declare(metadata i32* [[AA_V:%[A-z0-9.]*]], metadata [[AA_MD:![0-9]*]], metadata !DIExpression()), !dbg [[AA_LOC:![0-9]*]]
; CHECK: store i32 {{.*}}, i32* [[AA_V]], align 4, !dbg [[AA_LOC]]

define spir_kernel void @test_pmem(i32 addrspace(1)* %dst, i32 addrspace(1)* %src, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %bufferOffset, i32 %bufferOffset1) #0 !dbg !33 {
entry:
  call void @llvm.genx.GenISA.CatchAllDebugLine(), !dbg !40
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %src.addr = alloca i32 addrspace(1)*, align 8
  %aa = alloca i32, align 4
  %bb = alloca i32, align 4
  %cc = alloca i32, align 4
  %dd = alloca i32, align 4
  %ee = alloca i32, align 4
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !41, metadata !DIExpression()), !dbg !42
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8, !dbg !42
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %src.addr, metadata !43, metadata !DIExpression()), !dbg !44
  store i32 addrspace(1)* %src, i32 addrspace(1)** %src.addr, align 8, !dbg !44
  call void @llvm.dbg.declare(metadata i32* %aa, metadata !45, metadata !DIExpression()), !dbg !47
  %0 = load i32 addrspace(1)*, i32 addrspace(1)** %src.addr, align 8, !dbg !48
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %0, i64 0, !dbg !48
  %1 = load i32, i32 addrspace(1)* %arrayidx, align 4, !dbg !48
  store i32 %1, i32* %aa, align 4, !dbg !47
  call void @llvm.dbg.declare(metadata i32* %bb, metadata !49, metadata !DIExpression()), !dbg !50
  %2 = load i32 addrspace(1)*, i32 addrspace(1)** %src.addr, align 8, !dbg !51
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %2, i64 1, !dbg !51
  %3 = load i32, i32 addrspace(1)* %arrayidx1, align 4, !dbg !51
  store i32 %3, i32* %bb, align 4, !dbg !50
  call void @llvm.dbg.declare(metadata i32* %cc, metadata !52, metadata !DIExpression()), !dbg !53
  %4 = load i32, i32* %aa, align 4, !dbg !54
  %5 = load i32, i32* %bb, align 4, !dbg !55
  %call = call spir_func i32 @test_add(i32 %4, i32 %5) #3, !dbg !56
  store i32 %call, i32* %cc, align 4, !dbg !53
  %6 = load i32, i32* %cc, align 4, !dbg !57
  %7 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !58
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %7, i64 0, !dbg !58
  store i32 %6, i32 addrspace(1)* %arrayidx2, align 4, !dbg !59
  call void @llvm.dbg.declare(metadata i32* %dd, metadata !60, metadata !DIExpression()), !dbg !61
  %8 = load i32 addrspace(1)*, i32 addrspace(1)** %src.addr, align 8, !dbg !62
  %arrayidx3 = getelementptr inbounds i32, i32 addrspace(1)* %8, i64 2, !dbg !62
  %9 = load i32, i32 addrspace(1)* %arrayidx3, align 4, !dbg !62
  store i32 %9, i32* %dd, align 4, !dbg !61
  call void @llvm.dbg.declare(metadata i32* %ee, metadata !63, metadata !DIExpression()), !dbg !64
  %10 = load i32 addrspace(1)*, i32 addrspace(1)** %src.addr, align 8, !dbg !65
  %arrayidx4 = getelementptr inbounds i32, i32 addrspace(1)* %10, i64 3, !dbg !65
  %11 = load i32, i32 addrspace(1)* %arrayidx4, align 4, !dbg !65
  store i32 %11, i32* %ee, align 4, !dbg !64
  %12 = load i32, i32* %ee, align 4, !dbg !66
  %13 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !67
  %arrayidx5 = getelementptr inbounds i32, i32 addrspace(1)* %13, i64 1, !dbg !67
  store i32 %12, i32 addrspace(1)* %arrayidx5, align 4, !dbg !68
  ret void, !dbg !69
}

; CHECK: define{{.*}} @test_add{{.*}} !dbg [[FSCOPE:![0-9]*]]
; CHECK: @llvm.dbg.declare(metadata i32* [[A_V:%[A-z0-9.]*]], metadata [[A_MD:![0-9]*]], metadata !DIExpression()), !dbg [[A_LOC:![0-9]*]]
; CHECK-SAME: !StorageOffset [[A_OFFSET:![0-9]*]],  !StorageSize [[B_OFFSET:![0-9]*]]
; CHECK: store i32 {{.*}}, i32* [[A_V]], align 4, !dbg [[A_LOC]]
; CHECK: @llvm.dbg.declare(metadata i32* [[B_V:%[A-z0-9.]*]], metadata [[B_MD:![0-9]*]], metadata !DIExpression()), !dbg [[B_LOC:![0-9]*]]
; CHECK-SAME: !StorageOffset [[B_OFFSET]],  !StorageSize [[B_OFFSET]]
; CHECK: store i32 {{.*}}, i32* [[B_V]], align 4, !dbg [[B_LOC]]

; Function Attrs: convergent noinline nounwind optnone
define internal spir_func i32 @test_add(i32 %a, i32 %b) #1 !dbg !70 {
entry:
  %a.addr = alloca i32, align 4
  %b.addr = alloca i32, align 4
  %c = alloca i32, align 4
  call void @llvm.dbg.declare(metadata i32* %a.addr, metadata !73, metadata !DIExpression()), !dbg !74
  store i32 %a, i32* %a.addr, align 4, !dbg !74
  call void @llvm.dbg.declare(metadata i32* %b.addr, metadata !75, metadata !DIExpression()), !dbg !76
  store i32 %b, i32* %b.addr, align 4, !dbg !76
  call void @llvm.dbg.declare(metadata i32* %c, metadata !77, metadata !DIExpression()), !dbg !78
  %0 = load i32, i32* %a.addr, align 4, !dbg !79
  %1 = load i32, i32* %b.addr, align 4, !dbg !80
  %add = add i32 %0, %1, !dbg !81
  store i32 %add, i32* %c, align 4, !dbg !78
  %2 = load i32, i32* %c, align 4, !dbg !82
  ret i32 %2, !dbg !83
}

; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "opt_enabled.ll", directory: "/")
; CHECK-DAG: [[KSCOPE]] = distinct !DISubprogram(name: "test_pmem", scope: null, file: [[FILE]], line: 8
; CHECK-DAG: [[DST_MD]] = !DILocalVariable(name: "dst", arg: 1, scope: [[KSCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[DST_LOC]] = !DILocation(line: 8, column: 48, scope: [[KSCOPE]])
; CHECK-DAG: [[SRC_MD]] = !DILocalVariable(name: "src", arg: 2, scope: [[KSCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[SRC_LOC]] =  !DILocation(line: 8, column: 76, scope: [[KSCOPE]])
; CHECK-DAG: [[AA_MD]] = !DILocalVariable(name: "aa", scope: [[KSCOPE]], file: [[FILE]], line: 10
; CHECK-DAG: [[AA_LOC]] = !DILocation(line: 10, column: 7, scope: [[KSCOPE]])
; CHECK-DAG: [[FSCOPE]] = distinct !DISubprogram(name: "test_add", scope: null, file: [[FILE]], line: 2
; CHECK-DAG: [[A_MD]] = !DILocalVariable(name: "a", arg: 1, scope: [[FSCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[A_LOC]] = !DILocation(line: 2, column: 27, scope: [[FSCOPE]])
; CHECK-DAG: [[A_OFFSET]] = !{i32 0}
; CHECK-DAG: [[B_MD]] = !DILocalVariable(name: "b", arg: 2, scope: [[FSCOPE]], file: [[FILE]], line: 2
; CHECK-DAG: [[B_LOC]] = !DILocation(line: 2, column: 43, scope: [[FSCOPE]])
; CHECK-DAG: [[B_OFFSET]] = !{i32 4}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #2

; Function Attrs: nounwind
declare void @llvm.genx.GenISA.CatchAllDebugLine() #3

attributes #0 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" }
attributes #1 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" "visaStackCall" }
attributes #2 = { nounwind readnone speculatable }
attributes #3 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!IGCMetadata = !{!6}
!igc.functions = !{!17, !20}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "/")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{!"ModuleMD", !7, !10, !16, !13}
!7 = !{!"compOpt", !8, !9}
!8 = !{!"OptDisable", i1 false}
!9 = !{!"UseScratchSpacePrivateMemory", i1 true}
!10 = !{!"FuncMD", !11, !12, !14, !15}
!11 = !{!"FuncMDMap[0]", i32 (i32, i32)* @test_add}
!12 = !{!"FuncMDValue[0]", !13}
!13 = !{!"privateMemoryPerWI", i32 0}
!14 = !{!"FuncMDMap[1]", void (i32 addrspace(1)*, i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8*, i32, i32)* @test_pmem}
!15 = !{!"FuncMDValue[1]", !13}
!16 = !{!"MinNOSPushConstantSize", i32 0}
!17 = !{i32 (i32, i32)* @test_add, !18}
!18 = !{!19}
!19 = !{!"function_type", i32 2}
!20 = !{void (i32 addrspace(1)*, i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8*, i32, i32)* @test_pmem, !21}
!21 = !{!22, !23}
!22 = !{!"function_type", i32 0}
!23 = !{!"implicit_arg_desc", !24, !25, !26, !27, !29}
!24 = !{i32 0}
!25 = !{i32 1}
!26 = !{i32 13}
!27 = !{i32 15, !28}
!28 = !{!"explicit_arg_num", i32 0}
!29 = !{i32 15, !30}
!30 = !{!"explicit_arg_num", i32 1}
!33 = distinct !DISubprogram(name: "test_pmem", scope: null, file: !34, line: 8, type: !35, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!34 = !DIFile(filename: "opt_enabled.ll", directory: "/")
!35 = !DISubroutineType(types: !36)
!36 = !{!37, !38, !38}
!37 = !DIBasicType(name: "int", size: 4)
!38 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !39, size: 64)
!39 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!40 = !DILocation(line: 8, scope: !33)
!41 = !DILocalVariable(name: "dst", arg: 1, scope: !33, file: !34, line: 8, type: !38)
!42 = !DILocation(line: 8, column: 48, scope: !33)
!43 = !DILocalVariable(name: "src", arg: 2, scope: !33, file: !34, line: 8, type: !38)
!44 = !DILocation(line: 8, column: 76, scope: !33)
!45 = !DILocalVariable(name: "aa", scope: !33, file: !34, line: 10, type: !46)
!46 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!47 = !DILocation(line: 10, column: 7, scope: !33)
!48 = !DILocation(line: 10, column: 12, scope: !33)
!49 = !DILocalVariable(name: "bb", scope: !33, file: !34, line: 11, type: !46)
!50 = !DILocation(line: 11, column: 7, scope: !33)
!51 = !DILocation(line: 11, column: 12, scope: !33)
!52 = !DILocalVariable(name: "cc", scope: !33, file: !34, line: 12, type: !46)
!53 = !DILocation(line: 12, column: 7, scope: !33)
!54 = !DILocation(line: 12, column: 21, scope: !33)
!55 = !DILocation(line: 12, column: 25, scope: !33)
!56 = !DILocation(line: 12, column: 12, scope: !33)
!57 = !DILocation(line: 13, column: 12, scope: !33)
!58 = !DILocation(line: 13, column: 3, scope: !33)
!59 = !DILocation(line: 13, column: 10, scope: !33)
!60 = !DILocalVariable(name: "dd", scope: !33, file: !34, line: 14, type: !46)
!61 = !DILocation(line: 14, column: 7, scope: !33)
!62 = !DILocation(line: 14, column: 12, scope: !33)
!63 = !DILocalVariable(name: "ee", scope: !33, file: !34, line: 15, type: !46)
!64 = !DILocation(line: 15, column: 7, scope: !33)
!65 = !DILocation(line: 15, column: 12, scope: !33)
!66 = !DILocation(line: 16, column: 12, scope: !33)
!67 = !DILocation(line: 16, column: 3, scope: !33)
!68 = !DILocation(line: 16, column: 10, scope: !33)
!69 = !DILocation(line: 17, column: 1, scope: !33)
!70 = distinct !DISubprogram(name: "test_add", scope: null, file: !34, line: 2, type: !71, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!71 = !DISubroutineType(types: !72)
!72 = !{!46, !39, !39}
!73 = !DILocalVariable(name: "a", arg: 1, scope: !70, file: !34, line: 2, type: !39)
!74 = !DILocation(line: 2, column: 27, scope: !70)
!75 = !DILocalVariable(name: "b", arg: 2, scope: !70, file: !34, line: 2, type: !39)
!76 = !DILocation(line: 2, column: 43, scope: !70)
!77 = !DILocalVariable(name: "c", scope: !70, file: !34, line: 4, type: !46)
!78 = !DILocation(line: 4, column: 7, scope: !70)
!79 = !DILocation(line: 4, column: 11, scope: !70)
!80 = !DILocation(line: 4, column: 15, scope: !70)
!81 = !DILocation(line: 4, column: 13, scope: !70)
!82 = !DILocation(line: 5, column: 10, scope: !70)
!83 = !DILocation(line: 5, column: 3, scope: !70)
