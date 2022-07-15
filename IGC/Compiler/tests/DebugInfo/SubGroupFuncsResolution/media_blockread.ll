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
; __kernel void test_bread(__global int* dst,read_only image2d_t src, int2 coord)
; {
;     int b_read = intel_sub_group_block_read( src, coord );
;     dst[0] = b_read;
; }
;
; ------------------------------------------------

; Sanity check:
; CHECK: @test_bread{{.*}} !dbg [[SCOPE:![0-9]*]]
; CHECK: dbg.declare{{.*}} addrspace
; CHECK-SAME: metadata [[DST_MD:![0-9]*]]
; CHECK-SAME: !dbg [[DST_LOC:![0-9]*]]
; CHECK: dbg.declare{{.*}} %opencl
; CHECK-SAME: metadata [[SRC_MD:![0-9]*]]
; CHECK-SAME: !dbg [[SRC_LOC:![0-9]*]]
; CHECK: dbg.declare{{.*}} i32*
; CHECK-SAME: metadata [[BREAD_MD:![0-9]*]]
; CHECK-SAME: !dbg [[BREAD_LOC:![0-9]*]]
;
; Check that call line is not lost
; CHECK-DAG: store i32 [[BREAD_V:%[A-z0-9.]*]], {{.*}}, !dbg [[BREAD_LOC]]
; CHECK-DAG: [[BREAD_V]] = {{.*}}, !dbg [[CALL_LOC:![0-9]*]]

; CHECK: store i32 {{.*}}, i32 addrspace{{.*}}, !dbg [[DST_STORE_LOC:![0-9]*]]

%opencl.image2d_t.read_only = type opaque

define spir_kernel void @test_bread(i32 addrspace(1)* %dst, %opencl.image2d_t.read_only addrspace(1)* %src, <2 x i32> %coord) #0 !dbg !26 {
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %src.addr = alloca %opencl.image2d_t.read_only addrspace(1)*, align 8
  %coord.addr = alloca <2 x i32>, align 8
  %b_read = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !40, metadata !DIExpression()), !dbg !41
  store %opencl.image2d_t.read_only addrspace(1)* %src, %opencl.image2d_t.read_only addrspace(1)** %src.addr, align 8
  call void @llvm.dbg.declare(metadata %opencl.image2d_t.read_only addrspace(1)** %src.addr, metadata !42, metadata !DIExpression()), !dbg !43
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  call void @llvm.dbg.declare(metadata <2 x i32>* %coord.addr, metadata !44, metadata !DIExpression()), !dbg !45
  call void @llvm.dbg.declare(metadata i32* %b_read, metadata !46, metadata !DIExpression()), !dbg !47
  %0 = load %opencl.image2d_t.read_only addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)** %src.addr, align 8, !dbg !48
  %1 = load <2 x i32>, <2 x i32>* %coord.addr, align 8, !dbg !49
  %ImageArgVal = ptrtoint %opencl.image2d_t.read_only addrspace(1)* %0 to i64
  %conv.i = trunc i64 %ImageArgVal to i32, !dbg !50
  %call.i = call spir_func i32 @__builtin_IB_simd_media_block_read_1(i32 %conv.i, <2 x i32> %1), !dbg !50
  store i32 %call.i, i32* %b_read, align 4, !dbg !47
  %2 = load i32, i32* %b_read, align 4, !dbg !51
  %3 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !52
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %3, i64 0, !dbg !52
  store i32 %2, i32 addrspace(1)* %arrayidx, align 4, !dbg !53
  ret void, !dbg !54
}

; Check MD:
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "media_blockread.ll", directory: "")
; CHECK-DAG: [[SCOPE]] = distinct !DISubprogram(name: "test_bread", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[DST_LOC]] = !DILocation(line: 1, column: 40, scope: [[SCOPE]])
; CHECK-DAG: [[DST_MD]] = !DILocalVariable(name: "dst", arg: 1, scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[SRC_LOC]] = !DILocation(line: 1, column: 64, scope: [[SCOPE]])
; CHECK-DAG: [[SRC_MD]] = !DILocalVariable(name: "src", arg: 2, scope: [[SCOPE]], file: [[FILE]], line: 1

; CHECK-DAG: [[BREAD_LOC]] = !DILocation(line: 3, column: 9, scope: [[SCOPE]])
; CHECK-DAG: [[BREAD_MD]] = !DILocalVariable(name: "b_read", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[CALL_LOC]] = !DILocation(line: 3, column: 18, scope: [[SCOPE]])
; CHECK-DAG: [[DST_STORE_LOC]] =  !DILocation(line: 4, column: 12, scope: [[SCOPE]])

declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

declare spir_func i32 @__builtin_IB_simd_media_block_read_1(i32, <2 x i32>) local_unnamed_addr #0

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
!6 = !{void (i32 addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)*, <2 x i32>)* @test_bread, !7}
!7 = !{!8, !9}
!8 = !{!"function_type", i32 0}
!9 = !{!"sub_group_size", i32 16}
!10 = !{!"ModuleMD", !11}
!11 = !{!"FuncMD", !12, !13}
!12 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)*, <2 x i32>)* @test_bread}
!13 = !{!"FuncMDValue[0]", !14}
!14 = !{!"resAllocMD", !15}
!15 = !{!"argAllocMDList", !16, !20, !23}
!16 = !{!"argAllocMDListVec[0]", !17, !18, !19}
!17 = !{!"type", i32 1}
!18 = !{!"extensionType", i32 -1}
!19 = !{!"indexType", i32 0}
!20 = !{!"argAllocMDListVec[1]", !21, !22, !19}
!21 = !{!"type", i32 2}
!22 = !{!"extensionType", i32 0}
!23 = !{!"argAllocMDListVec[2]", !24, !18, !25}
!24 = !{!"type", i32 0}
!25 = !{!"indexType", i32 -1}
!26 = distinct !DISubprogram(name: "test_bread", scope: null, file: !27, line: 1, type: !28, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!27 = !DIFile(filename: "media_blockread.ll", directory: "")
!28 = !DISubroutineType(types: !29)
!29 = !{!30, !31, !33, !35}
!30 = !DIBasicType(name: "int", size: 4)
!31 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !32, size: 64)
!32 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!33 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !34, size: 64)
!34 = !DICompositeType(tag: DW_TAG_structure_type, name: "opencl_image2d_ro_t", file: !1, flags: DIFlagFwdDecl, elements: !2)
!35 = !DIDerivedType(tag: DW_TAG_typedef, name: "int2", file: !36, baseType: !37)
!36 = !DIFile(filename: "opencl-c-base.h", directory: "/")
!37 = !DICompositeType(tag: DW_TAG_array_type, baseType: !32, size: 64, flags: DIFlagVector, elements: !38)
!38 = !{!39}
!39 = !DISubrange(count: 2)
!40 = !DILocalVariable(name: "dst", arg: 1, scope: !26, file: !27, line: 1, type: !31)
!41 = !DILocation(line: 1, column: 40, scope: !26)
!42 = !DILocalVariable(name: "src", arg: 2, scope: !26, file: !27, line: 1, type: !33)
!43 = !DILocation(line: 1, column: 64, scope: !26)
!44 = !DILocalVariable(name: "coord", arg: 3, scope: !26, file: !27, line: 1, type: !35)
!45 = !DILocation(line: 1, column: 74, scope: !26)
!46 = !DILocalVariable(name: "b_read", scope: !26, file: !27, line: 3, type: !32)
!47 = !DILocation(line: 3, column: 9, scope: !26)
!48 = !DILocation(line: 3, column: 46, scope: !26)
!49 = !DILocation(line: 3, column: 51, scope: !26)
!50 = !DILocation(line: 3, column: 18, scope: !26)
!51 = !DILocation(line: 4, column: 14, scope: !26)
!52 = !DILocation(line: 4, column: 5, scope: !26)
!53 = !DILocation(line: 4, column: 12, scope: !26)
!54 = !DILocation(line: 5, column: 1, scope: !26)
