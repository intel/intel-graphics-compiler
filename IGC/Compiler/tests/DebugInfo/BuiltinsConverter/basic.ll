;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: igc_opt --igc-conv-ocl-to-common -S < %s | FileCheck %s
; ------------------------------------------------
; BuiltinsConverter
; ------------------------------------------------
; This test checks that BuiltinsConverter pass follows
; 'How to Update Debug Info' llvm guideline.
;
; And was reduced from ocl test kernel:
;
; __kernel void test_read_imagef(global float4 *dst, read_only image2d_t img, int2 coord)
; {
;     dst[0] = read_imagef (img, coord);
; }
;
; __kernel void test_read_imagei(global int4 *dst, read_only image2d_t img, int2 coord)
; {
;     dst[0] = read_imagei (img, coord);
; }
;
; __kernel void test_read_imageui(global uint4 *dst, read_only image2d_t img, int2 coord)
; {
;     dst[0] = read_imageui (img, coord);
; }
;
; __kernel void test_write_imagef(global float4 *src, write_only image2d_t img, int2 coord)
; {
;     write_imagef (img, coord, src[0]);
; }
;
; __kernel void test_write_imagei(global int4 *src, write_only image2d_t img, int2 coord)
; {
;     write_imagei (img, coord, src[0]);
; }
;
; __kernel void test_write_imageui(global uint4 *src, write_only image2d_t img, int2 coord)
; {
;     write_imageui (img, coord, src[0]);
; }
;
; ------------------------------------------------

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"

%opencl.image2d_t.read_only = type opaque
%opencl.image2d_t.write_only = type opaque

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_read_imagef(<4 x float> addrspace(1)* %dst, %opencl.image2d_t.read_only addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %bufferOffset) #0 !dbg !56 {
entry:
  call void @llvm.genx.GenISA.CatchAllDebugLine(), !dbg !75
  %dst.addr = alloca <4 x float> addrspace(1)*, align 8
  %img.addr = alloca %opencl.image2d_t.read_only addrspace(1)*, align 8
  %coord.addr = alloca <2 x i32>, align 8
  store <4 x float> addrspace(1)* %dst, <4 x float> addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata <4 x float> addrspace(1)** %dst.addr, metadata !76, metadata !DIExpression()), !dbg !77
  store %opencl.image2d_t.read_only addrspace(1)* %img, %opencl.image2d_t.read_only addrspace(1)** %img.addr, align 8
  call void @llvm.dbg.declare(metadata %opencl.image2d_t.read_only addrspace(1)** %img.addr, metadata !78, metadata !DIExpression()), !dbg !79
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  call void @llvm.dbg.declare(metadata <2 x i32>* %coord.addr, metadata !80, metadata !DIExpression()), !dbg !81
  %0 = load %opencl.image2d_t.read_only addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)** %img.addr, align 8, !dbg !82
  %1 = load <2 x i32>, <2 x i32>* %coord.addr, align 8, !dbg !83
  %ImageArgVal = ptrtoint %opencl.image2d_t.read_only addrspace(1)* %0 to i64
  %2 = shufflevector <2 x i32> %1, <2 x i32> undef, <4 x i32> <i32 0, i32 1, i32 0, i32 0>
  %conv.i = trunc i64 %ImageArgVal to i32, !dbg !84
  ;
  ; CHECK: call <4 x float> @llvm.genx.GenISA.ldptr.v4f32.p196608f32{{.*}}, !dbg [[IMGFR_LOC:![0-9]*]]
  ;
  %call21.i = call spir_func <4 x float> @__builtin_IB_OCL_2d_ld(i32 %conv.i, <2 x i32> %1, i32 0) #4, !dbg !84
  %astype.i54.i = bitcast <4 x float> %call21.i to <4 x i32>, !dbg !84
  %and.i55.i = and <4 x i32> %astype.i54.i, <i32 -2139095041, i32 -2139095041, i32 -2139095041, i32 -2139095041>, !dbg !84
  %cmp.i56.i = icmp eq <4 x i32> %and.i55.i, %astype.i54.i, !dbg !84
  %and5.i57.i = and <4 x i32> %astype.i54.i, <i32 -2147483648, i32 -2147483648, i32 -2147483648, i32 -2147483648>, !dbg !84
  %3 = select <4 x i1> %cmp.i56.i, <4 x i32> %and5.i57.i, <4 x i32> %astype.i54.i, !dbg !84
  %4 = bitcast <4 x i32> %3 to <4 x float>, !dbg !84
  %5 = load <4 x float> addrspace(1)*, <4 x float> addrspace(1)** %dst.addr, align 8, !dbg !85
  %arrayidx = getelementptr inbounds <4 x float>, <4 x float> addrspace(1)* %5, i64 0, !dbg !85
  store <4 x float> %4, <4 x float> addrspace(1)* %arrayidx, align 16, !dbg !86
  ret void, !dbg !87
}

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_read_imagei(<4 x i32> addrspace(1)* %dst, %opencl.image2d_t.read_only addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %bufferOffset) #0 !dbg !88 {
entry:
  call void @llvm.genx.GenISA.CatchAllDebugLine(), !dbg !94
  %dst.addr = alloca <4 x i32> addrspace(1)*, align 8
  %img.addr = alloca %opencl.image2d_t.read_only addrspace(1)*, align 8
  %coord.addr = alloca <2 x i32>, align 8
  store <4 x i32> addrspace(1)* %dst, <4 x i32> addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata <4 x i32> addrspace(1)** %dst.addr, metadata !95, metadata !DIExpression()), !dbg !96
  store %opencl.image2d_t.read_only addrspace(1)* %img, %opencl.image2d_t.read_only addrspace(1)** %img.addr, align 8
  call void @llvm.dbg.declare(metadata %opencl.image2d_t.read_only addrspace(1)** %img.addr, metadata !97, metadata !DIExpression()), !dbg !98
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  call void @llvm.dbg.declare(metadata <2 x i32>* %coord.addr, metadata !99, metadata !DIExpression()), !dbg !100
  %0 = load %opencl.image2d_t.read_only addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)** %img.addr, align 8, !dbg !101
  %1 = load <2 x i32>, <2 x i32>* %coord.addr, align 8, !dbg !102
  %ImageArgVal = ptrtoint %opencl.image2d_t.read_only addrspace(1)* %0 to i64
  %2 = shufflevector <2 x i32> %1, <2 x i32> undef, <4 x i32> <i32 0, i32 1, i32 0, i32 0>
  %conv.i = trunc i64 %ImageArgVal to i32, !dbg !103
  ;
  ; CHECK: call <4 x i32> @llvm.genx.GenISA.ldptr.v4i32.p196608f32{{.*}}, !dbg [[IMGIR_LOC:![0-9]*]]
  ;
  %call15.i = call spir_func <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %conv.i, <2 x i32> %1, i32 0) #4, !dbg !103
  %3 = load <4 x i32> addrspace(1)*, <4 x i32> addrspace(1)** %dst.addr, align 8, !dbg !104
  %arrayidx = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %3, i64 0, !dbg !104
  store <4 x i32> %call15.i, <4 x i32> addrspace(1)* %arrayidx, align 16, !dbg !105
  ret void, !dbg !106
}

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_read_imageui(<4 x i32> addrspace(1)* %dst, %opencl.image2d_t.read_only addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %bufferOffset) #0 !dbg !107 {
entry:
  call void @llvm.genx.GenISA.CatchAllDebugLine(), !dbg !115
  %dst.addr = alloca <4 x i32> addrspace(1)*, align 8
  %img.addr = alloca %opencl.image2d_t.read_only addrspace(1)*, align 8
  %coord.addr = alloca <2 x i32>, align 8
  store <4 x i32> addrspace(1)* %dst, <4 x i32> addrspace(1)** %dst.addr, align 8
  call void @llvm.dbg.declare(metadata <4 x i32> addrspace(1)** %dst.addr, metadata !116, metadata !DIExpression()), !dbg !117
  store %opencl.image2d_t.read_only addrspace(1)* %img, %opencl.image2d_t.read_only addrspace(1)** %img.addr, align 8
  call void @llvm.dbg.declare(metadata %opencl.image2d_t.read_only addrspace(1)** %img.addr, metadata !118, metadata !DIExpression()), !dbg !119
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  call void @llvm.dbg.declare(metadata <2 x i32>* %coord.addr, metadata !120, metadata !DIExpression()), !dbg !121
  %0 = load %opencl.image2d_t.read_only addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)** %img.addr, align 8, !dbg !122
  %1 = load <2 x i32>, <2 x i32>* %coord.addr, align 8, !dbg !123
  %ImageArgVal = ptrtoint %opencl.image2d_t.read_only addrspace(1)* %0 to i64
  %2 = shufflevector <2 x i32> %1, <2 x i32> undef, <4 x i32> <i32 0, i32 1, i32 0, i32 0>
  %conv.i = trunc i64 %ImageArgVal to i32, !dbg !124
  ;
  ; CHECK: call <4 x i32> @llvm.genx.GenISA.ldptr.v4i32.p196608f32{{.*}}, !dbg [[IMGUIR_LOC:![0-9]*]]
  ;
  %call15.i = call spir_func <4 x i32> @__builtin_IB_OCL_2d_ldui(i32 %conv.i, <2 x i32> %1, i32 0) #4, !dbg !124
  %3 = load <4 x i32> addrspace(1)*, <4 x i32> addrspace(1)** %dst.addr, align 8, !dbg !125
  %arrayidx = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %3, i64 0, !dbg !125
  store <4 x i32> %call15.i, <4 x i32> addrspace(1)* %arrayidx, align 16, !dbg !126
  ret void, !dbg !127
}

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_write_imagef(<4 x float> addrspace(1)* %src, %opencl.image2d_t.write_only addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %bufferOffset) #0 !dbg !128 {
entry:
  call void @llvm.genx.GenISA.CatchAllDebugLine(), !dbg !133
  %src.addr = alloca <4 x float> addrspace(1)*, align 8
  %img.addr = alloca %opencl.image2d_t.write_only addrspace(1)*, align 8
  %coord.addr = alloca <2 x i32>, align 8
  store <4 x float> addrspace(1)* %src, <4 x float> addrspace(1)** %src.addr, align 8
  call void @llvm.dbg.declare(metadata <4 x float> addrspace(1)** %src.addr, metadata !134, metadata !DIExpression()), !dbg !135
  store %opencl.image2d_t.write_only addrspace(1)* %img, %opencl.image2d_t.write_only addrspace(1)** %img.addr, align 8
  call void @llvm.dbg.declare(metadata %opencl.image2d_t.write_only addrspace(1)** %img.addr, metadata !136, metadata !DIExpression()), !dbg !137
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  call void @llvm.dbg.declare(metadata <2 x i32>* %coord.addr, metadata !138, metadata !DIExpression()), !dbg !139
  %0 = load %opencl.image2d_t.write_only addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)** %img.addr, align 8, !dbg !140
  %1 = load <2 x i32>, <2 x i32>* %coord.addr, align 8, !dbg !141
  %2 = load <4 x float> addrspace(1)*, <4 x float> addrspace(1)** %src.addr, align 8, !dbg !142
  %arrayidx = getelementptr inbounds <4 x float>, <4 x float> addrspace(1)* %2, i64 0, !dbg !142
  %3 = load <4 x float>, <4 x float> addrspace(1)* %arrayidx, align 16, !dbg !142
  %ImageArgVal = ptrtoint %opencl.image2d_t.write_only addrspace(1)* %0 to i64
  %4 = shufflevector <2 x i32> %1, <2 x i32> undef, <4 x i32> <i32 0, i32 1, i32 0, i32 0>
  %astype.i = bitcast <4 x float> %3 to <4 x i32>, !dbg !143
  %conv.i.i.i = trunc i64 %ImageArgVal to i32, !dbg !143
  ;
  ; CHECK: call void @llvm.genx.GenISA.typedwrite.p196608f32{{.*}}, !dbg [[IMGFW_LOC:![0-9]*]]
  ;
  call spir_func void @__builtin_IB_write_2d_ui(i32 %conv.i.i.i, <2 x i32> %1, <4 x i32> %astype.i, i32 0) #4, !dbg !143
  ret void, !dbg !144
}

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_write_imagei(<4 x i32> addrspace(1)* %src, %opencl.image2d_t.write_only addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %bufferOffset) #0 !dbg !145 {
entry:
  call void @llvm.genx.GenISA.CatchAllDebugLine(), !dbg !148
  %src.addr = alloca <4 x i32> addrspace(1)*, align 8
  %img.addr = alloca %opencl.image2d_t.write_only addrspace(1)*, align 8
  %coord.addr = alloca <2 x i32>, align 8
  store <4 x i32> addrspace(1)* %src, <4 x i32> addrspace(1)** %src.addr, align 8
  call void @llvm.dbg.declare(metadata <4 x i32> addrspace(1)** %src.addr, metadata !149, metadata !DIExpression()), !dbg !150
  store %opencl.image2d_t.write_only addrspace(1)* %img, %opencl.image2d_t.write_only addrspace(1)** %img.addr, align 8
  call void @llvm.dbg.declare(metadata %opencl.image2d_t.write_only addrspace(1)** %img.addr, metadata !151, metadata !DIExpression()), !dbg !152
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  call void @llvm.dbg.declare(metadata <2 x i32>* %coord.addr, metadata !153, metadata !DIExpression()), !dbg !154
  %0 = load %opencl.image2d_t.write_only addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)** %img.addr, align 8, !dbg !155
  %1 = load <2 x i32>, <2 x i32>* %coord.addr, align 8, !dbg !156
  %2 = load <4 x i32> addrspace(1)*, <4 x i32> addrspace(1)** %src.addr, align 8, !dbg !157
  %arrayidx = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %2, i64 0, !dbg !157
  %3 = load <4 x i32>, <4 x i32> addrspace(1)* %arrayidx, align 16, !dbg !157
  %ImageArgVal = ptrtoint %opencl.image2d_t.write_only addrspace(1)* %0 to i64
  %4 = shufflevector <2 x i32> %1, <2 x i32> undef, <4 x i32> <i32 0, i32 1, i32 0, i32 0>
  %conv.i.i = trunc i64 %ImageArgVal to i32, !dbg !158
  ;
  ; CHECK: call void @llvm.genx.GenISA.typedwrite.p196608f32{{.*}}, !dbg [[IMGIW_LOC:![0-9]*]]
  ;
  call spir_func void @__builtin_IB_write_2d_ui(i32 %conv.i.i, <2 x i32> %1, <4 x i32> %3, i32 0) #4, !dbg !158
  ret void, !dbg !159
}

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_write_imageui(<4 x i32> addrspace(1)* %src, %opencl.image2d_t.write_only addrspace(1)* %img, <2 x i32> %coord, <8 x i32> %r0, <8 x i32> %payloadHeader, i8* %privateBase, i32 %bufferOffset) #0 !dbg !160 {
entry:
  call void @llvm.genx.GenISA.CatchAllDebugLine(), !dbg !163
  %src.addr = alloca <4 x i32> addrspace(1)*, align 8
  %img.addr = alloca %opencl.image2d_t.write_only addrspace(1)*, align 8
  %coord.addr = alloca <2 x i32>, align 8
  store <4 x i32> addrspace(1)* %src, <4 x i32> addrspace(1)** %src.addr, align 8
  call void @llvm.dbg.declare(metadata <4 x i32> addrspace(1)** %src.addr, metadata !164, metadata !DIExpression()), !dbg !165
  store %opencl.image2d_t.write_only addrspace(1)* %img, %opencl.image2d_t.write_only addrspace(1)** %img.addr, align 8
  call void @llvm.dbg.declare(metadata %opencl.image2d_t.write_only addrspace(1)** %img.addr, metadata !166, metadata !DIExpression()), !dbg !167
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  call void @llvm.dbg.declare(metadata <2 x i32>* %coord.addr, metadata !168, metadata !DIExpression()), !dbg !169
  %0 = load %opencl.image2d_t.write_only addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)** %img.addr, align 8, !dbg !170
  %1 = load <2 x i32>, <2 x i32>* %coord.addr, align 8, !dbg !171
  %2 = load <4 x i32> addrspace(1)*, <4 x i32> addrspace(1)** %src.addr, align 8, !dbg !172
  %arrayidx = getelementptr inbounds <4 x i32>, <4 x i32> addrspace(1)* %2, i64 0, !dbg !172
  %3 = load <4 x i32>, <4 x i32> addrspace(1)* %arrayidx, align 16, !dbg !172
  %ImageArgVal = ptrtoint %opencl.image2d_t.write_only addrspace(1)* %0 to i64
  %4 = shufflevector <2 x i32> %1, <2 x i32> undef, <4 x i32> <i32 0, i32 1, i32 0, i32 0>
  %conv.i.i = trunc i64 %ImageArgVal to i32, !dbg !173
  ;
  ; CHECK: call void @llvm.genx.GenISA.typedwrite.p196608f32{{.*}}, !dbg [[IMGUIW_LOC:![0-9]*]]
  ;
  call spir_func void @__builtin_IB_write_2d_ui(i32 %conv.i.i, <2 x i32> %1, <4 x i32> %3, i32 0) #4, !dbg !173
  ret void, !dbg !174
}

; CHECK-DAG: [[IMGFR_LOC]] = !DILocation(line: 4, column: 14, scope: !56)
; CHECK-DAG: [[IMGIR_LOC]] = !DILocation(line: 9, column: 14, scope: !88)
; CHECK-DAG: [[IMGUIR_LOC]] = !DILocation(line: 14, column: 14, scope: !107)
; CHECK-DAG: [[IMGFW_LOC]] =  !DILocation(line: 19, column: 5, scope: !128)
; CHECK-DAG: [[IMGIW_LOC]] =  !DILocation(line: 24, column: 5, scope: !145)
; CHECK-DAG: [[IMGUIW_LOC]] = !DILocation(line: 29, column: 5, scope: !160)

; Function Attrs: convergent
declare spir_func <4 x i32> @__builtin_IB_OCL_2d_ldui(i32, <2 x i32>, i32) local_unnamed_addr #2

; Function Attrs: convergent
declare spir_func <4 x float> @__builtin_IB_OCL_2d_ld(i32, <2 x i32>, i32) local_unnamed_addr #2

; Function Attrs: convergent
declare spir_func void @__builtin_IB_write_2d_ui(i32, <2 x i32>, <4 x i32>, i32) local_unnamed_addr #2

; Function Attrs: nounwind
declare void @llvm.genx.GenISA.CatchAllDebugLine() #3

attributes #0 = { convergent noinline nounwind optnone "less-precise-fpmad"="true" }
attributes #1 = { nounwind readnone speculatable }
attributes #2 = { convergent "correctly-rounded-divide-sqrt-fp-math"="false" "denorms-are-zero"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }
attributes #4 = { convergent nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4, !5}
!IGCMetadata = !{!6}
!igc.functions = !{!40, !49, !50, !51, !52, !53}
!opencl.ocl.version = !{!54, !54, !54, !54, !54}
!opencl.spir.version = !{!54, !54, !54, !54, !54}
!llvm.ident = !{!55, !55, !55, !55, !55}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "spirv", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "<stdin>", directory: "dir")
!2 = !{}
!3 = !{i32 2, !"Dwarf Version", i32 4}
!4 = !{i32 2, !"Debug Info Version", i32 3}
!5 = !{i32 1, !"wchar_size", i32 4}
!6 = !{!"ModuleMD", !7}
!7 = !{!"FuncMD", !8, !9, !30, !31, !32, !33, !34, !35, !36, !37, !38, !39}
!8 = distinct !{!"FuncMDMap[0]", void (<4 x float> addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_read_imagef}
!9 = !{!"FuncMDValue[0]", !10, !11, !12}
!10 = !{!"funcArgs"}
!11 = !{!"functionType", !"KernelFunction"}
!12 = !{!"resAllocMD", !13}
!13 = !{!"argAllocMDList", !14, !18, !21, !24, !25, !27, !28, !29}
!14 = !{!"argAllocMDListVec[0]", !15, !16, !17}
!15 = !{!"type", i32 1}
!16 = !{!"extensionType", i32 -1}
!17 = !{!"indexType", i32 0}
!18 = !{!"argAllocMDListVec[1]", !19, !20, !17}
!19 = !{!"type", i32 2}
!20 = !{!"extensionType", i32 0}
!21 = !{!"argAllocMDListVec[2]", !22, !16, !23}
!22 = !{!"type", i32 0}
!23 = !{!"indexType", i32 -1}
!24 = !{!"argAllocMDListVec[3]", !22, !16, !23}
!25 = !{!"argAllocMDListVec[4]", !15, !16, !26}
!26 = !{!"indexType", i32 1}
!27 = !{!"argAllocMDListVec[5]", !22, !16, !23}
!28 = !{!"argAllocMDListVec[6]", !22, !16, !23}
!29 = !{!"argAllocMDListVec[7]", !22, !16, !23}
!30 = distinct !{!"FuncMDMap[1]", void (<4 x i32> addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_read_imagei}
!31 = !{!"FuncMDValue[1]", !10, !11, !12}
!32 = distinct !{!"FuncMDMap[2]", void (<4 x i32> addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_read_imageui}
!33 = !{!"FuncMDValue[2]", !10, !11, !12}
!34 = distinct !{!"FuncMDMap[3]", void (<4 x float> addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_write_imagef}
!35 = !{!"FuncMDValue[3]", !10, !11, !12}
!36 = distinct !{!"FuncMDMap[4]", void (<4 x i32> addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_write_imagei}
!37 = !{!"FuncMDValue[4]", !10, !11, !12}
!38 = distinct !{!"FuncMDMap[5]", void (<4 x i32> addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_write_imageui}
!39 = !{!"FuncMDValue[5]", !10, !11, !12}
!40 = !{void (<4 x float> addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_read_imagef, !41}
!41 = !{!42, !43}
!42 = !{!"function_type", i32 0}
!43 = !{!"implicit_arg_desc", !44, !45, !46, !47}
!44 = !{i32 0}
!45 = !{i32 1}
!46 = !{i32 12}
!47 = !{i32 14, !48}
!48 = !{!"explicit_arg_num", i32 0}
!49 = !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_read_imagei, !41}
!50 = !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_read_imageui, !41}
!51 = !{void (<4 x float> addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_write_imagef, !41}
!52 = !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_write_imagei, !41}
!53 = !{void (<4 x i32> addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)*, <2 x i32>, <8 x i32>, <8 x i32>, i8*, i32)* @test_write_imageui, !41}
!54 = !{i32 2, i32 0}
!55 = !{!"clang version 10.0.0"}
!56 = distinct !DISubprogram(name: "test_read_imagef", scope: null, file: !57, line: 2, type: !58, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!57 = !DIFile(filename: "1", directory: "dir")
!58 = !DISubroutineType(types: !59)
!59 = !{!60, !61, !68, !70}
!60 = !DIBasicType(name: "int", size: 4)
!61 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !62, size: 64)
!62 = !DIDerivedType(tag: DW_TAG_typedef, name: "float4", file: !63, baseType: !64)
!63 = !DIFile(filename: "opencl-c-base.h", directory: "dir")
!64 = !DICompositeType(tag: DW_TAG_array_type, baseType: !65, size: 128, flags: DIFlagVector, elements: !66)
!65 = !DIBasicType(name: "float", size: 32, encoding: DW_ATE_float)
!66 = !{!67}
!67 = !DISubrange(count: 4)
!68 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !69, size: 64)
!69 = !DICompositeType(tag: DW_TAG_structure_type, name: "opencl_image2d_ro_t", file: !1, flags: DIFlagFwdDecl, elements: !2)
!70 = !DIDerivedType(tag: DW_TAG_typedef, name: "int2", file: !63, baseType: !71)
!71 = !DICompositeType(tag: DW_TAG_array_type, baseType: !72, size: 64, flags: DIFlagVector, elements: !73)
!72 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!73 = !{!74}
!74 = !DISubrange(count: 2)
!75 = !DILocation(line: 2, scope: !56)
!76 = !DILocalVariable(name: "dst", arg: 1, scope: !56, file: !57, line: 2, type: !61)
!77 = !DILocation(line: 2, column: 47, scope: !56)
!78 = !DILocalVariable(name: "img", arg: 2, scope: !56, file: !57, line: 2, type: !68)
!79 = !DILocation(line: 2, column: 72, scope: !56)
!80 = !DILocalVariable(name: "coord", arg: 3, scope: !56, file: !57, line: 2, type: !70)
!81 = !DILocation(line: 2, column: 82, scope: !56)
!82 = !DILocation(line: 4, column: 27, scope: !56)
!83 = !DILocation(line: 4, column: 32, scope: !56)
!84 = !DILocation(line: 4, column: 14, scope: !56)
!85 = !DILocation(line: 4, column: 5, scope: !56)
!86 = !DILocation(line: 4, column: 12, scope: !56)
!87 = !DILocation(line: 5, column: 1, scope: !56)
!88 = distinct !DISubprogram(name: "test_read_imagei", scope: null, file: !57, line: 7, type: !89, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!89 = !DISubroutineType(types: !90)
!90 = !{!60, !91, !68, !70}
!91 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !92, size: 64)
!92 = !DIDerivedType(tag: DW_TAG_typedef, name: "int4", file: !63, baseType: !93)
!93 = !DICompositeType(tag: DW_TAG_array_type, baseType: !72, size: 128, flags: DIFlagVector, elements: !66)
!94 = !DILocation(line: 7, scope: !88)
!95 = !DILocalVariable(name: "dst", arg: 1, scope: !88, file: !57, line: 7, type: !91)
!96 = !DILocation(line: 7, column: 45, scope: !88)
!97 = !DILocalVariable(name: "img", arg: 2, scope: !88, file: !57, line: 7, type: !68)
!98 = !DILocation(line: 7, column: 70, scope: !88)
!99 = !DILocalVariable(name: "coord", arg: 3, scope: !88, file: !57, line: 7, type: !70)
!100 = !DILocation(line: 7, column: 80, scope: !88)
!101 = !DILocation(line: 9, column: 27, scope: !88)
!102 = !DILocation(line: 9, column: 32, scope: !88)
!103 = !DILocation(line: 9, column: 14, scope: !88)
!104 = !DILocation(line: 9, column: 5, scope: !88)
!105 = !DILocation(line: 9, column: 12, scope: !88)
!106 = !DILocation(line: 10, column: 1, scope: !88)
!107 = distinct !DISubprogram(name: "test_read_imageui", scope: null, file: !57, line: 12, type: !108, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!108 = !DISubroutineType(types: !109)
!109 = !{!60, !110, !68, !70}
!110 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !111, size: 64)
!111 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint4", file: !63, baseType: !112)
!112 = !DICompositeType(tag: DW_TAG_array_type, baseType: !113, flags: DIFlagVector, elements: !66)
!113 = !DIDerivedType(tag: DW_TAG_typedef, name: "uint", file: !63, baseType: !114)
!114 = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
!115 = !DILocation(line: 12, scope: !107)
!116 = !DILocalVariable(name: "dst", arg: 1, scope: !107, file: !57, line: 12, type: !110)
!117 = !DILocation(line: 12, column: 47, scope: !107)
!118 = !DILocalVariable(name: "img", arg: 2, scope: !107, file: !57, line: 12, type: !68)
!119 = !DILocation(line: 12, column: 72, scope: !107)
!120 = !DILocalVariable(name: "coord", arg: 3, scope: !107, file: !57, line: 12, type: !70)
!121 = !DILocation(line: 12, column: 82, scope: !107)
!122 = !DILocation(line: 14, column: 28, scope: !107)
!123 = !DILocation(line: 14, column: 33, scope: !107)
!124 = !DILocation(line: 14, column: 14, scope: !107)
!125 = !DILocation(line: 14, column: 5, scope: !107)
!126 = !DILocation(line: 14, column: 12, scope: !107)
!127 = !DILocation(line: 15, column: 1, scope: !107)
!128 = distinct !DISubprogram(name: "test_write_imagef", scope: null, file: !57, line: 17, type: !129, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!129 = !DISubroutineType(types: !130)
!130 = !{!60, !61, !131, !70}
!131 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !132, size: 64)
!132 = !DICompositeType(tag: DW_TAG_structure_type, name: "opencl_image2d_wo_t", file: !1, flags: DIFlagFwdDecl, elements: !2)
!133 = !DILocation(line: 17, scope: !128)
!134 = !DILocalVariable(name: "src", arg: 1, scope: !128, file: !57, line: 17, type: !61)
!135 = !DILocation(line: 17, column: 48, scope: !128)
!136 = !DILocalVariable(name: "img", arg: 2, scope: !128, file: !57, line: 17, type: !131)
!137 = !DILocation(line: 17, column: 74, scope: !128)
!138 = !DILocalVariable(name: "coord", arg: 3, scope: !128, file: !57, line: 17, type: !70)
!139 = !DILocation(line: 17, column: 84, scope: !128)
!140 = !DILocation(line: 19, column: 19, scope: !128)
!141 = !DILocation(line: 19, column: 24, scope: !128)
!142 = !DILocation(line: 19, column: 31, scope: !128)
!143 = !DILocation(line: 19, column: 5, scope: !128)
!144 = !DILocation(line: 20, column: 1, scope: !128)
!145 = distinct !DISubprogram(name: "test_write_imagei", scope: null, file: !57, line: 22, type: !146, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!146 = !DISubroutineType(types: !147)
!147 = !{!60, !91, !131, !70}
!148 = !DILocation(line: 22, scope: !145)
!149 = !DILocalVariable(name: "src", arg: 1, scope: !145, file: !57, line: 22, type: !91)
!150 = !DILocation(line: 22, column: 46, scope: !145)
!151 = !DILocalVariable(name: "img", arg: 2, scope: !145, file: !57, line: 22, type: !131)
!152 = !DILocation(line: 22, column: 72, scope: !145)
!153 = !DILocalVariable(name: "coord", arg: 3, scope: !145, file: !57, line: 22, type: !70)
!154 = !DILocation(line: 22, column: 82, scope: !145)
!155 = !DILocation(line: 24, column: 19, scope: !145)
!156 = !DILocation(line: 24, column: 24, scope: !145)
!157 = !DILocation(line: 24, column: 31, scope: !145)
!158 = !DILocation(line: 24, column: 5, scope: !145)
!159 = !DILocation(line: 25, column: 1, scope: !145)
!160 = distinct !DISubprogram(name: "test_write_imageui", scope: null, file: !57, line: 27, type: !161, flags: DIFlagPrototyped, unit: !0, templateParams: !2, retainedNodes: !2)
!161 = !DISubroutineType(types: !162)
!162 = !{!60, !110, !131, !70}
!163 = !DILocation(line: 27, scope: !160)
!164 = !DILocalVariable(name: "src", arg: 1, scope: !160, file: !57, line: 27, type: !110)
!165 = !DILocation(line: 27, column: 48, scope: !160)
!166 = !DILocalVariable(name: "img", arg: 2, scope: !160, file: !57, line: 27, type: !131)
!167 = !DILocation(line: 27, column: 74, scope: !160)
!168 = !DILocalVariable(name: "coord", arg: 3, scope: !160, file: !57, line: 27, type: !70)
!169 = !DILocation(line: 27, column: 84, scope: !160)
!170 = !DILocation(line: 29, column: 20, scope: !160)
!171 = !DILocation(line: 29, column: 25, scope: !160)
!172 = !DILocation(line: 29, column: 32, scope: !160)
!173 = !DILocation(line: 29, column: 5, scope: !160)
!174 = !DILocation(line: 30, column: 1, scope: !160)
