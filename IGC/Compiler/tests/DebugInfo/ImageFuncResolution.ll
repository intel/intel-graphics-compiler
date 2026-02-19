;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-image-func-resolution -S < %s | FileCheck %s
; ------------------------------------------------
; ImageFuncResolution
; ------------------------------------------------
; This test checks that ImageFuncResolution pass follows
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
; CHECK-DAG: @llvm.dbg.value(metadata i32 [[WIDTH_V]], metadata [[WIDTH_MD]], metadata !DIExpression()), !dbg [[WIDTHC_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: %height, metadata [[HEIGHT_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[HEIGHT_LOC:![0-9]*]]
; CHECK-DAG: store i32 [[HEIGHT_V:%[A-z0-9.]*]], i32* %height, align 4, !dbg [[HEIGHT_LOC]]
; CHECK-DAG: @llvm.dbg.value(metadata i32 [[HEIGHT_V]], metadata [[HEIGHT_MD]], metadata !DIExpression()), !dbg [[HEIGHTC_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: %depth, metadata [[DEPTH_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[DEPTH_LOC:![0-9]*]]
; CHECK-DAG: store i32 [[DEPTH_V:%[A-z0-9.]*]], i32* %depth, align 4, !dbg [[DEPTH_LOC]]
; CHECK-DAG: @llvm.dbg.value(metadata i32 [[DEPTH_V]], metadata [[DEPTH_MD]], metadata !DIExpression()), !dbg [[DEPTHC_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: %channelType, metadata [[CTYPE_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[CTYPE_LOC:![0-9]*]]
; CHECK-DAG: store i32 [[CTYPE_V:%[A-z0-9.]*]], i32* %channelType, align 4, !dbg [[CTYPE_LOC]]
; CHECK-DAG: @llvm.dbg.value(metadata i32 [[CTYPE_V]], metadata [[CTYPE_MD]], metadata !DIExpression()), !dbg [[CTYPEC_LOC:![0-9]*]]
;
; CHECK: call void @llvm.dbg.declare
; CHECK-SAME: %channelOrder, metadata [[CORD_MD:![0-9]*]], metadata !DIExpression()
; CHECK-SAME: !dbg [[CORD_LOC:![0-9]*]]
; CHECK-DAG: store i32 [[CORD_V:%[A-z0-9.]*]], i32* %channelOrder, align 4, !dbg [[CORD_LOC]]
; CHECK-DAG: @llvm.dbg.value(metadata i32 [[CORD_V]], metadata [[CORD_MD]], metadata !DIExpression()), !dbg [[CORDC_LOC:![0-9]*]]

%opencl.image3d_t.read_only = type opaque

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @sample_kernel(%opencl.image3d_t.read_only addrspace(1)* %input, i32 addrspace(1)* %dst, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %imageHeigt, i32 %imageWidth, i32 %imageDepth, i32 %imageDataType, i32 %imageOrder, i32 %bufferOffset) #0 !dbg !21 {
entry:
  %input.addr = alloca %opencl.image3d_t.read_only addrspace(1)*, align 8
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %width = alloca i32, align 4
  %height = alloca i32, align 4
  %depth = alloca i32, align 4
  %channelType = alloca i32, align 4
  %channelOrder = alloca i32, align 4
  store %opencl.image3d_t.read_only addrspace(1)* %input, %opencl.image3d_t.read_only addrspace(1)** %input.addr, align 8
  call void @llvm.dbg.declare(metadata %opencl.image3d_t.read_only addrspace(1)** %input.addr, metadata !30, metadata !DIExpression()), !dbg !31
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %dst.addr, metadata !32, metadata !DIExpression()), !dbg !33
  call void @llvm.dbg.declare(metadata i32* %width, metadata !34, metadata !DIExpression()), !dbg !35
  %0 = load %opencl.image3d_t.read_only addrspace(1)*, %opencl.image3d_t.read_only addrspace(1)** %input.addr, align 8, !dbg !36
  %ImageArgVal = ptrtoint %opencl.image3d_t.read_only addrspace(1)* %0 to i64
  %conv.i.i = trunc i64 %ImageArgVal to i32
  %call.old = call spir_func i32 @__builtin_IB_get_image_width(i32 %conv.i.i), !dbg !37
  call void @llvm.dbg.value(metadata i32 %call.old, metadata !34, metadata !DIExpression()), !dbg !37
  store i32 %call.old, i32* %width, align 4, !dbg !35
  call void @llvm.dbg.declare(metadata i32* %height, metadata !38, metadata !DIExpression()), !dbg !39
  %1 = load %opencl.image3d_t.read_only addrspace(1)*, %opencl.image3d_t.read_only addrspace(1)** %input.addr, align 8, !dbg !40
  %ImageArgVal1 = ptrtoint %opencl.image3d_t.read_only addrspace(1)* %1 to i64
  %conv.i.i5 = trunc i64 %ImageArgVal1 to i32
  %call1.old = call spir_func i32 @__builtin_IB_get_image_height(i32 %conv.i.i5), !dbg !41
  call void @llvm.dbg.value(metadata i32 %call1.old, metadata !38, metadata !DIExpression()), !dbg !41
  store i32 %call1.old, i32* %height, align 4, !dbg !39
  call void @llvm.dbg.declare(metadata i32* %depth, metadata !42, metadata !DIExpression()), !dbg !43
  %2 = load %opencl.image3d_t.read_only addrspace(1)*, %opencl.image3d_t.read_only addrspace(1)** %input.addr, align 8, !dbg !44
  %ImageArgVal2 = ptrtoint %opencl.image3d_t.read_only addrspace(1)* %2 to i64
  %conv.i.i12 = trunc i64 %ImageArgVal2 to i32
  %call2.old = call spir_func i32 @__builtin_IB_get_image_depth(i32 %conv.i.i12), !dbg !45
  call void @llvm.dbg.value(metadata i32 %call2.old, metadata !42, metadata !DIExpression()), !dbg !45
  store i32 %call2.old, i32* %depth, align 4, !dbg !43
  call void @llvm.dbg.declare(metadata i32* %channelType, metadata !46, metadata !DIExpression()), !dbg !47
  %3 = load %opencl.image3d_t.read_only addrspace(1)*, %opencl.image3d_t.read_only addrspace(1)** %input.addr, align 8, !dbg !48
  %ImageArgVal3 = ptrtoint %opencl.image3d_t.read_only addrspace(1)* %3 to i64
  %conv.i = trunc i64 %ImageArgVal3 to i32
  %call3.old = call spir_func i32 @__builtin_IB_get_image_channel_data_type(i32 %conv.i), !dbg !49
  call void @llvm.dbg.value(metadata i32 %call3.old, metadata !46, metadata !DIExpression()), !dbg !49
  store i32 %call3.old, i32* %channelType, align 4, !dbg !47
  call void @llvm.dbg.declare(metadata i32* %channelOrder, metadata !50, metadata !DIExpression()), !dbg !51
  %4 = load %opencl.image3d_t.read_only addrspace(1)*, %opencl.image3d_t.read_only addrspace(1)** %input.addr, align 8, !dbg !52
  %ImageArgVal4 = ptrtoint %opencl.image3d_t.read_only addrspace(1)* %4 to i64
  %conv.i19 = trunc i64 %ImageArgVal4 to i32
  %call4.old = call spir_func i32 @__builtin_IB_get_image_channel_order(i32 %conv.i19), !dbg !53
  call void @llvm.dbg.value(metadata i32 %call4.old, metadata !50, metadata !DIExpression()), !dbg !53
  store i32 %call4.old, i32* %channelOrder, align 4, !dbg !51
  %5 = load i32, i32* %width, align 4, !dbg !54
  %6 = load i32, i32* %height, align 4, !dbg !55
  %add = add nsw i32 %5, %6, !dbg !56
  %7 = load i32, i32* %depth, align 4, !dbg !57
  %add5 = add nsw i32 %add, %7, !dbg !58
  %8 = load i32, i32* %channelOrder, align 4, !dbg !59
  %add6 = add nsw i32 %add5, %8, !dbg !60
  %9 = load i32, i32* %channelType, align 4, !dbg !61
  %add7 = add nsw i32 %add6, %9, !dbg !62
  %10 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8, !dbg !63
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %10, i64 0, !dbg !63
  store i32 %add7, i32 addrspace(1)* %arrayidx, align 4, !dbg !64
  ret void, !dbg !65
}

; Check MD:
;
; CHECK-DAG: [[FILE:![0-9]*]] = !DIFile(filename: "ImageFuncResolution.ll", directory: "/")
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
declare void @llvm.dbg.value(metadata, metadata, metadata) #1

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

attributes #0 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" }
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
!6 = !{void (%opencl.image3d_t.read_only addrspace(1)*, i32 addrspace(1)*, <8 x i32>, <8 x i32>, i8*, i32, i32, i32, i32, i32, i32)* @sample_kernel, !7}
!7 = !{!8, !9}
!8 = !{!"function_type", i32 0}
!9 = !{!"implicit_arg_desc", !10, !11, !12, !13, !15, !16, !17, !18, !19}
!10 = !{i32 0}
!11 = !{i32 1}
!12 = !{i32 13}
!13 = !{i32 21, !14}
!14 = !{!"explicit_arg_num", i32 0}
!15 = !{i32 22, !14}
!16 = !{i32 23, !14}
!17 = !{i32 25, !14}
!18 = !{i32 26, !14}
!19 = !{i32 15, !20}
!20 = !{!"explicit_arg_num", i32 1}
!21 = distinct !DISubprogram(name: "sample_kernel", scope: null, file: !22, line: 1, type: !23, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!22 = !DIFile(filename: "ImageFuncResolution.ll", directory: "/")
!23 = !DISubroutineType(types: !24)
!24 = !{!25, !26, !28}
!25 = !DIBasicType(name: "int", size: 4)
!26 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !27, size: 64)
!27 = !DICompositeType(tag: DW_TAG_structure_type, name: "opencl_image3d_ro_t", file: !1, flags: DIFlagFwdDecl, elements: !2)
!28 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !29, size: 64)
!29 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!30 = !DILocalVariable(name: "input", arg: 1, scope: !21, file: !22, line: 1, type: !26)
!31 = !DILocation(line: 1, column: 50, scope: !21)
!32 = !DILocalVariable(name: "dst", arg: 2, scope: !21, file: !22, line: 1, type: !28)
!33 = !DILocation(line: 1, column: 71, scope: !21)
!34 = !DILocalVariable(name: "width", scope: !21, file: !22, line: 3, type: !29)
!35 = !DILocation(line: 3, column: 8, scope: !21)
!36 = !DILocation(line: 3, column: 33, scope: !21)
!37 = !DILocation(line: 3, column: 16, scope: !21)
!38 = !DILocalVariable(name: "height", scope: !21, file: !22, line: 4, type: !29)
!39 = !DILocation(line: 4, column: 8, scope: !21)
!40 = !DILocation(line: 4, column: 35, scope: !21)
!41 = !DILocation(line: 4, column: 17, scope: !21)
!42 = !DILocalVariable(name: "depth", scope: !21, file: !22, line: 5, type: !29)
!43 = !DILocation(line: 5, column: 8, scope: !21)
!44 = !DILocation(line: 5, column: 33, scope: !21)
!45 = !DILocation(line: 5, column: 16, scope: !21)
!46 = !DILocalVariable(name: "channelType", scope: !21, file: !22, line: 7, type: !29)
!47 = !DILocation(line: 7, column: 8, scope: !21)
!48 = !DILocation(line: 7, column: 51, scope: !21)
!49 = !DILocation(line: 7, column: 22, scope: !21)
!50 = !DILocalVariable(name: "channelOrder", scope: !21, file: !22, line: 8, type: !29)
!51 = !DILocation(line: 8, column: 8, scope: !21)
!52 = !DILocation(line: 8, column: 48, scope: !21)
!53 = !DILocation(line: 8, column: 23, scope: !21)
!54 = !DILocation(line: 9, column: 13, scope: !21)
!55 = !DILocation(line: 9, column: 21, scope: !21)
!56 = !DILocation(line: 9, column: 19, scope: !21)
!57 = !DILocation(line: 9, column: 30, scope: !21)
!58 = !DILocation(line: 9, column: 28, scope: !21)
!59 = !DILocation(line: 9, column: 38, scope: !21)
!60 = !DILocation(line: 9, column: 36, scope: !21)
!61 = !DILocation(line: 9, column: 53, scope: !21)
!62 = !DILocation(line: 9, column: 51, scope: !21)
!63 = !DILocation(line: 9, column: 4, scope: !21)
!64 = !DILocation(line: 9, column: 11, scope: !21)
!65 = !DILocation(line: 11, column: 1, scope: !21)
