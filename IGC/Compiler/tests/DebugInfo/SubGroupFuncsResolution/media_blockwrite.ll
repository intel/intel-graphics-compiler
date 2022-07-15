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
; __kernel void test_bwrite(write_only image2d_t dst, int2 coord, int src)
; {
;     intel_sub_group_block_write(dst, coord, src );
; }
;
; ------------------------------------------------

; Sanity check:
; CHECK: @test_bwrite{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: dbg.declare{{.*}} addrspace
; CHECK-SAME: metadata [[DST_MD:![0-9]*]]
; CHECK-SAME: !dbg [[DST_LOC:![0-9]*]]
; CHECK: dbg.declare{{.*}} i32*
; CHECK-SAME: metadata [[SRC_MD:![0-9]*]]
; CHECK-SAME: !dbg [[SRC_LOC:![0-9]*]]
;
; Check that call line is not lost
; CHECK: [[SRC_V:%[0-9]*]] = load {{.*}} %src.addr
; CHECK: call {{.*}}void
; CHECK-SAME: [[SRC_V]]
; CHECK-SAME: !dbg [[CALL_LOC:![0-9]*]]

%opencl.image2d_t.write_only = type opaque

define spir_kernel void @test_bwrite(%opencl.image2d_t.write_only addrspace(1)* %dst, <2 x i32> %coord, i32 %src) #0 !dbg !25 {
entry:
  %dst.addr = alloca %opencl.image2d_t.write_only addrspace(1)*, align 8
  %coord.addr = alloca <2 x i32>, align 8
  %src.addr = alloca i32, align 4
  store %opencl.image2d_t.write_only addrspace(1)* %dst, %opencl.image2d_t.write_only addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata %opencl.image2d_t.write_only addrspace(1)** %dst.addr, metadata !38, metadata !DIExpression()), !dbg !39
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  call void @llvm.dbg.declare(metadata <2 x i32>* %coord.addr, metadata !40, metadata !DIExpression()), !dbg !41
  store i32 %src, i32* %src.addr, align 4
  call void @llvm.dbg.declare(metadata i32* %src.addr, metadata !42, metadata !DIExpression()), !dbg !43
  %0 = load %opencl.image2d_t.write_only addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)** %dst.addr, align 8, !dbg !44
  %1 = load <2 x i32>, <2 x i32>* %coord.addr, align 8, !dbg !45
  %2 = load i32, i32* %src.addr, align 4, !dbg !46
  %ImageArgVal = ptrtoint %opencl.image2d_t.write_only addrspace(1)* %0 to i64
  %conv.i = trunc i64 %ImageArgVal to i32
  call spir_func void @__builtin_IB_simd_media_block_write_1(i32 %conv.i, <2 x i32> %1, i32 %2) #2, !dbg !47
  ret void, !dbg !48
}

; Check MD:
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "media_blockwrite.ll", directory: "/")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_bwrite", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[DST_LOC]] = !DILocation(line: 1, column: 48, scope: [[SCOPE]])
; CHECK-DAG: [[DST_MD]] = !DILocalVariable(name: "dst", arg: 1, scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[SRC_LOC]] = !DILocation(line: 1, column: 69, scope: [[SCOPE]])
; CHECK-DAG: [[SRC_MD]] = !DILocalVariable(name: "src", arg: 3, scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 3, column: 4, scope: [[SCOPE]])

declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

declare spir_func void @__builtin_IB_simd_media_block_write_1(i32, <2 x i32>, i32) local_unnamed_addr #0

attributes #0 = { convergent noinline nounwind optnone }
attributes #1 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!igc.functions = !{!6}
!IGCMetadata = !{!10}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "/")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{void (%opencl.image2d_t.write_only addrspace(1)*, <2 x i32>, i32)* @test_bwrite, !7}
!7 = !{!8, !9}
!8 = !{!"function_type", i32 0}
!9 = !{!"sub_group_size", i32 16}
!10 = !{!"ModuleMD", !11}
!11 = !{!"FuncMD", !12, !13}
!12 = !{!"FuncMDMap[0]", void (%opencl.image2d_t.write_only addrspace(1)*, <2 x i32>, i32)* @test_bwrite}
!13 = !{!"FuncMDValue[0]", !14}
!14 = !{!"resAllocMD", !15}
!15 = !{!"argAllocMDList", !16, !20, !24}
!16 = !{!"argAllocMDListVec[0]", !17, !18, !19}
!17 = !{!"type", i32 1}
!18 = !{!"extensionType", i32 0}
!19 = !{!"indexType", i32 1}
!20 = !{!"argAllocMDListVec[1]", !21, !22, !23}
!21 = !{!"type", i32 0}
!22 = !{!"extensionType", i32 -1}
!23 = !{!"indexType", i32 -1}
!24 = !{!"argAllocMDListVec[2]", !21, !22, !23}
!25 = distinct !DISubprogram(name: "test_bwrite", scope: null, file: !26, line: 1, type: !27, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!26 = !DIFile(filename: "media_blockwrite.ll", directory: "/")
!27 = !DISubroutineType(types: !28)
!28 = !{!29, !30, !32, !35}
!29 = !DIBasicType(name: "int", size: 4)
!30 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !31, size: 64)
!31 = !DICompositeType(tag: DW_TAG_structure_type, name: "opencl_image2d_wo_t", file: !1, flags: DIFlagFwdDecl, elements: !2)
!32 = !DIDerivedType(tag: DW_TAG_typedef, name: "int2", file: !33, baseType: !34)
!33 = !DIFile(filename: "opencl-c-base.h", directory: "/")
!34 = !DICompositeType(tag: DW_TAG_array_type, baseType: !35, size: 64, flags: DIFlagVector, elements: !36)
!35 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!36 = !{!37}
!37 = !DISubrange(count: 2)
!38 = !DILocalVariable(name: "dst", arg: 1, scope: !25, file: !26, line: 1, type: !30)
!39 = !DILocation(line: 1, column: 48, scope: !25)
!40 = !DILocalVariable(name: "coord", arg: 2, scope: !25, file: !26, line: 1, type: !32)
!41 = !DILocation(line: 1, column: 58, scope: !25)
!42 = !DILocalVariable(name: "src", arg: 3, scope: !25, file: !26, line: 1, type: !35)
!43 = !DILocation(line: 1, column: 69, scope: !25)
!44 = !DILocation(line: 3, column: 33, scope: !25)
!45 = !DILocation(line: 3, column: 38, scope: !25)
!46 = !DILocation(line: 3, column: 45, scope: !25)
!47 = !DILocation(line: 3, column: 4, scope: !25)
!48 = !DILocation(line: 4, column: 1, scope: !25)
