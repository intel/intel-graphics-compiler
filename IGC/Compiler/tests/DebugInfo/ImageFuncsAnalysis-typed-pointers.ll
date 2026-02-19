;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022-2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers --igc-image-func-analysis -S < %s | FileCheck %s
; ------------------------------------------------
; ImageFuncsAnalysis
; ------------------------------------------------
; This test checks that ImageFuncsAnalysis pass follows
; 'How to Update Debug Info' llvm guideline.
;
; And was reduced from ocl test kernel:
;
; __kernel void sample_kernel( read_only image3d_t input, __global int *dst )
; {
;    int width = get_image_width( input );
;    int height = get_image_height( input );
;    int depth = get_image_depth( input );
;
;    int channelType = get_image_channel_data_type( input );
;    int channelOrder = get_image_channel_order( input );
;    dst[0] = width + height + depth + channelOrder + channelType;
; }
;
; This is an analysis pass, check that debug MD is not modified
; ------------------------------------------------

; Check IR:
;
; CHECK: define spir_kernel void @sample_kernel
; CHECK-SAME: !dbg [[SCOPE:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: image3d_t{{.*}}, metadata [[INPUT_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[INPUT_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: addrspace{{.*}}, metadata [[DST_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[DST_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: %width, metadata [[WIDTH_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[WIDTH_LOC:![0-9]*]]
; CHECK-DAG: store i32 [[WIDTH_V:%[A-z0-9.]*]], i32* %width, align 4, !dbg [[WIDTH_LOC]]
; CHECK-DAG: [[WIDTH_V]] = {{.*}} !dbg [[WIDTHC_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: %height, metadata [[HEIGHT_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[HEIGHT_LOC:![0-9]*]]
; CHECK-DAG: store i32 [[HEIGHT_V:%[A-z0-9.]*]], i32* %height, align 4, !dbg [[HEIGHT_LOC]]
; CHECK-DAG: [[HEIGHT_V]] = {{.*}} !dbg [[HEIGHTC_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: %depth, metadata [[DEPTH_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[DEPTH_LOC:![0-9]*]]
; CHECK-DAG: store i32 [[DEPTH_V:%[A-z0-9.]*]], i32* %depth, align 4, !dbg [[DEPTH_LOC]]
; CHECK-DAG: [[DEPTH_V]] = {{.*}} !dbg [[DEPTHC_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: %channelType, metadata [[CTYPE_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[CTYPE_LOC:![0-9]*]]
; CHECK-DAG: store i32 [[CTYPE_V:%[A-z0-9.]*]], i32* %channelType, align 4, !dbg [[CTYPE_LOC]]
; CHECK-DAG: [[CTYPE_V]] = {{.*}} !dbg [[CTYPEC_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: %channelOrder, metadata [[CORD_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[CORD_LOC:![0-9]*]]
; CHECK-DAG: store i32 [[CORD_V:%[A-z0-9.]*]], i32* %channelOrder, align 4, !dbg [[CORD_LOC]]
; CHECK-DAG: [[CORD_V]] = {{.*}} !dbg [[CORDC_LOC:![0-9]*]]

%opencl.image3d_t.read_only = type opaque

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @sample_kernel(%opencl.image3d_t.read_only addrspace(1)* %input, i32 addrspace(1)* %dst) #0 !dbg !12 {
entry:
  %input.addr = alloca %opencl.image3d_t.read_only addrspace(1)*, align 8
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %width = alloca i32, align 4
  %height = alloca i32, align 4
  %depth = alloca i32, align 4
  %channelType = alloca i32, align 4
  %channelOrder = alloca i32, align 4
  store %opencl.image3d_t.read_only addrspace(1)* %input, %opencl.image3d_t.read_only addrspace(1)** %input.addr, align 8
  call void @llvm.dbg.declare(metadata %opencl.image3d_t.read_only addrspace(1)** %input.addr, metadata !21, metadata !DIExpression()), !dbg !22
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !23, metadata !DIExpression()), !dbg !24
  call void @llvm.dbg.declare(metadata i32* %width, metadata !25, metadata !DIExpression()), !dbg !26
  %0 = load %opencl.image3d_t.read_only addrspace(1)*, %opencl.image3d_t.read_only addrspace(1)** %input.addr, align 8, !dbg !27
  %ImageArgVal = ptrtoint %opencl.image3d_t.read_only addrspace(1)* %0 to i64
  %conv.i.i = trunc i64 %ImageArgVal to i32
  %call.old = call spir_func i32 @__builtin_IB_get_image_width(i32 %conv.i.i), !dbg !28
  store i32 %call.old, i32* %width, align 4, !dbg !26
  call void @llvm.dbg.declare(metadata i32* %height, metadata !29, metadata !DIExpression()), !dbg !30
  %1 = load %opencl.image3d_t.read_only addrspace(1)*, %opencl.image3d_t.read_only addrspace(1)** %input.addr, align 8, !dbg !31
  %ImageArgVal1 = ptrtoint %opencl.image3d_t.read_only addrspace(1)* %1 to i64
  %conv.i.i5 = trunc i64 %ImageArgVal1 to i32
  %call1.old = call spir_func i32 @__builtin_IB_get_image_height(i32 %conv.i.i5), !dbg !32
  store i32 %call1.old, i32* %height, align 4, !dbg !30
  call void @llvm.dbg.declare(metadata i32* %depth, metadata !33, metadata !DIExpression()), !dbg !34
  %2 = load %opencl.image3d_t.read_only addrspace(1)*, %opencl.image3d_t.read_only addrspace(1)** %input.addr, align 8, !dbg !35
  %ImageArgVal2 = ptrtoint %opencl.image3d_t.read_only addrspace(1)* %2 to i64
  %conv.i.i12 = trunc i64 %ImageArgVal2 to i32
  %call2.old = call spir_func i32 @__builtin_IB_get_image_depth(i32 %conv.i.i12), !dbg !36
  store i32 %call2.old, i32* %depth, align 4, !dbg !34
  call void @llvm.dbg.declare(metadata i32* %channelType, metadata !37, metadata !DIExpression()), !dbg !38
  %3 = load %opencl.image3d_t.read_only addrspace(1)*, %opencl.image3d_t.read_only addrspace(1)** %input.addr, align 8, !dbg !39
  %ImageArgVal3 = ptrtoint %opencl.image3d_t.read_only addrspace(1)* %3 to i64
  %conv.i = trunc i64 %ImageArgVal3 to i32
  %call3.old = call spir_func i32 @__builtin_IB_get_image_channel_data_type(i32 %conv.i), !dbg !40
  store i32 %call3.old, i32* %channelType, align 4, !dbg !38
  call void @llvm.dbg.declare(metadata i32* %channelOrder, metadata !41, metadata !DIExpression()), !dbg !42
  %4 = load %opencl.image3d_t.read_only addrspace(1)*, %opencl.image3d_t.read_only addrspace(1)** %input.addr, align 8, !dbg !43
  %ImageArgVal4 = ptrtoint %opencl.image3d_t.read_only addrspace(1)* %4 to i64
  %conv.i19 = trunc i64 %ImageArgVal4 to i32
  %call4.old = call spir_func i32 @__builtin_IB_get_image_channel_order(i32 %conv.i19), !dbg !44
  store i32 %call4.old, i32* %channelOrder, align 4, !dbg !42
  %5 = load i32, i32* %width, align 4, !dbg !45
  %6 = load i32, i32* %height, align 4, !dbg !46
  %add = add nsw i32 %5, %6, !dbg !47
  %7 = load i32, i32* %depth, align 4, !dbg !48
  %add5 = add nsw i32 %add, %7, !dbg !49
  %8 = load i32, i32* %channelOrder, align 4, !dbg !50
  %add6 = add nsw i32 %add5, %8, !dbg !51
  %9 = load i32, i32* %channelType, align 4, !dbg !52
  %add7 = add nsw i32 %add6, %9, !dbg !53
  %10 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !54
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %10, i64 0, !dbg !54
  store i32 %add7, i32 addrspace(1)* %arrayidx, align 4, !dbg !55
  ret void, !dbg !56
}

; Check MD:
;
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "ImageFuncsAnalysis.ll", directory: "/")
; CHECK-DAG: [[SCOPE:![0-9]*]] = distinct !DISubprogram(name: "sample_kernel", scope: null, file: [[FILE]], line: 1
; CHECK-DAG: [[INPUT_MD]] = !DILocalVariable(name: "input", arg: 1, scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[INPUT_LOC]] = !DILocation(line: 1, column: 50, scope: [[SCOPE]])
; CHECK-DAG: [[DST_MD]] = !DILocalVariable(name: "dst", arg: 2, scope: [[SCOPE]], file: [[FILE]], line: 1
; CHECK-DAG: [[DST_LOC]] = !DILocation(line: 1, column: 71, scope: [[SCOPE]])
;
; CHECK-DAG: [[WIDTH_MD]] = !DILocalVariable(name: "width", scope: [[SCOPE]], file: [[FILE]], line: 3
; CHECK-DAG: [[WIDTH_LOC]] = !DILocation(line: 3, column: 8, scope: [[SCOPE]])
; CHECK-DAG: [[WIDTHC_LOC]] = !DILocation(line: 3, column: 16, scope: [[SCOPE]])
;
; CHECK-DAG: [[HEIGHT_MD]] = !DILocalVariable(name: "height", scope: [[SCOPE]], file: [[FILE]], line: 4
; CHECK-DAG: [[HEIGHT_LOC]] = !DILocation(line: 4, column: 8, scope: [[SCOPE]])
; CHECK-DAG: [[HEIGHTC_LOC]] = !DILocation(line: 4, column: 17, scope: [[SCOPE]])
;
; CHECK-DAG: [[DEPTH_MD]] = !DILocalVariable(name: "depth", scope: [[SCOPE]], file: [[FILE]], line: 5
; CHECK-DAG: [[DEPTH_LOC]] = !DILocation(line: 5, column: 8, scope: [[SCOPE]])
; CHECK-DAG: [[DEPTHC_LOC]] = !DILocation(line: 5, column: 16, scope: [[SCOPE]])
;
; CHECK-DAG: [[CTYPE_MD]] = !DILocalVariable(name: "channelType", scope: [[SCOPE]], file: [[FILE]], line: 7
; CHECK-DAG: [[CTYPE_LOC]] = !DILocation(line: 7, column: 8, scope: [[SCOPE]])
; CHECK-DAG: [[CTYPEC_LOC]] = !DILocation(line: 7, column: 22, scope: [[SCOPE]])
;
; CHECK-DAG: [[CORD_MD]] = !DILocalVariable(name: "channelOrder", scope: [[SCOPE]], file: [[FILE]], line: 8
; CHECK-DAG: [[CORD_LOC]] = !DILocation(line: 8, column: 8, scope: [[SCOPE]])
; CHECK-DAG: [[CORDC_LOC]] = !DILocation(line: 8, column: 23, scope: [[SCOPE]])

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_image_channel_data_type(i32) local_unnamed_addr #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_image_channel_order(i32) local_unnamed_addr #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_image_width(i32) local_unnamed_addr #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_image_height(i32) local_unnamed_addr #2

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_image_depth(i32) local_unnamed_addr #2

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
!6 = !{void (%opencl.image3d_t.read_only addrspace(1)*, i32 addrspace(1)*)* @sample_kernel, !7}
!7 = !{!8, !9}
!8 = !{!"function_type", i32 0}
!9 = !{!"implicit_arg_desc", !10, !11}
!10 = !{i32 0}
!11 = !{i32 1}
!12 = distinct !DISubprogram(name: "sample_kernel", scope: null, file: !13, line: 1, type: !14, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!13 = !DIFile(filename: "ImageFuncsAnalysis.ll", directory: "/")
!14 = !DISubroutineType(types: !15)
!15 = !{!16, !17, !19}
!16 = !DIBasicType(name: "int", size: 4)
!17 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !18, size: 64)
!18 = !DICompositeType(tag: DW_TAG_structure_type, name: "opencl_image3d_ro_t", file: !1, flags: DIFlagFwdDecl, elements: !2)
!19 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !20, size: 64)
!20 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!21 = !DILocalVariable(name: "input", arg: 1, scope: !12, file: !13, line: 1, type: !17)
!22 = !DILocation(line: 1, column: 50, scope: !12)
!23 = !DILocalVariable(name: "dst", arg: 2, scope: !12, file: !13, line: 1, type: !19)
!24 = !DILocation(line: 1, column: 71, scope: !12)
!25 = !DILocalVariable(name: "width", scope: !12, file: !13, line: 3, type: !20)
!26 = !DILocation(line: 3, column: 8, scope: !12)
!27 = !DILocation(line: 3, column: 33, scope: !12)
!28 = !DILocation(line: 3, column: 16, scope: !12)
!29 = !DILocalVariable(name: "height", scope: !12, file: !13, line: 4, type: !20)
!30 = !DILocation(line: 4, column: 8, scope: !12)
!31 = !DILocation(line: 4, column: 35, scope: !12)
!32 = !DILocation(line: 4, column: 17, scope: !12)
!33 = !DILocalVariable(name: "depth", scope: !12, file: !13, line: 5, type: !20)
!34 = !DILocation(line: 5, column: 8, scope: !12)
!35 = !DILocation(line: 5, column: 33, scope: !12)
!36 = !DILocation(line: 5, column: 16, scope: !12)
!37 = !DILocalVariable(name: "channelType", scope: !12, file: !13, line: 7, type: !20)
!38 = !DILocation(line: 7, column: 8, scope: !12)
!39 = !DILocation(line: 7, column: 51, scope: !12)
!40 = !DILocation(line: 7, column: 22, scope: !12)
!41 = !DILocalVariable(name: "channelOrder", scope: !12, file: !13, line: 8, type: !20)
!42 = !DILocation(line: 8, column: 8, scope: !12)
!43 = !DILocation(line: 8, column: 48, scope: !12)
!44 = !DILocation(line: 8, column: 23, scope: !12)
!45 = !DILocation(line: 9, column: 13, scope: !12)
!46 = !DILocation(line: 9, column: 21, scope: !12)
!47 = !DILocation(line: 9, column: 19, scope: !12)
!48 = !DILocation(line: 9, column: 30, scope: !12)
!49 = !DILocation(line: 9, column: 28, scope: !12)
!50 = !DILocation(line: 9, column: 38, scope: !12)
!51 = !DILocation(line: 9, column: 36, scope: !12)
!52 = !DILocation(line: 9, column: 53, scope: !12)
!53 = !DILocation(line: 9, column: 51, scope: !12)
!54 = !DILocation(line: 9, column: 4, scope: !12)
!55 = !DILocation(line: 9, column: 11, scope: !12)
!56 = !DILocation(line: 11, column: 1, scope: !12)

; The following metadata are needed to recognize functions using image/sampler arguments:
!IGCMetadata = !{!57}
!57 = !{!"ModuleMD", !58}
!58 = !{!"FuncMD", !59, !60}
!59 = !{!"FuncMDMap[0]", void (%opencl.image3d_t.read_only addrspace(1)*, i32 addrspace(1)*)* @sample_kernel}
!60 = !{!"FuncMDValue[0]", !61}
!61 = !{!"m_OpenCLArgBaseTypes", !62, !63}
!62 = !{!"m_OpenCLArgBaseTypesVec[0]", !"image3d_t"}
!63 = !{!"m_OpenCLArgBaseTypesVec[1]", !"int*"}
