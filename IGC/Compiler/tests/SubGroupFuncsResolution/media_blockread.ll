;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -enable-debugify --igc-sub-group-func-resolution -S < %s 2>&1 | FileCheck %s
; ------------------------------------------------
; SubGroupFuncsResolution
; ------------------------------------------------
; Was reduced from ocl test kernel:
;
; __kernel void test_bread(__global int* dst,read_only image2d_t src, int2 coord)
; {
;     int b_read = intel_sub_group_block_read( src, coord );
;     dst[0] = b_read;
; }
;
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

%opencl.image2d_t.read_only = type opaque

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_bread(i32 addrspace(1)* %dst, %opencl.image2d_t.read_only addrspace(1)* %src, <2 x i32> %coord) #0 {
; CHECK-LABEL: @test_bread(
; CHECK:  entry:
; CHECK:    [[DST_ADDR:%.*]] = alloca i32 addrspace(1)*, align 8
; CHECK:    [[SRC_ADDR:%.*]] = alloca [[OPENCL_IMAGE2D_T_READ_ONLY:%.*]] addrspace(1)*, align 8
; CHECK:    [[COORD_ADDR:%.*]] = alloca <2 x i32>, align 8
; CHECK:    [[B_READ:%.*]] = alloca i32, align 4
; CHECK:    [[TMP1:%.*]] = load <2 x i32>, <2 x i32>* [[COORD_ADDR]], align 8
; CHECK:    [[XOFFSET:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[YOFFSET:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    [[TMP2:%.*]] = call i32 @llvm.genx.GenISA.simdMediaBlockRead.i32.i32(i32 0, i32 [[XOFFSET]], i32 [[YOFFSET]], i32 0)
; CHECK:    store i32 [[TMP2]], i32* [[B_READ]], align 4
;
; CHECK:  declare i32 @llvm.genx.GenISA.simdMediaBlockRead.i32.i32(i32, i32, i32, i32) [[ATTR:#.*]]
; CHECK:  attributes [[ATTR]] = { {{.*convergent.*}} }
;
entry:
  %dst.addr = alloca i32 addrspace(1)*, align 8
  %src.addr = alloca %opencl.image2d_t.read_only addrspace(1)*, align 8
  %coord.addr = alloca <2 x i32>, align 8
  %b_read = alloca i32, align 4
  store i32 addrspace(1)* %dst, i32 addrspace(1)** %dst.addr, align 8
  store %opencl.image2d_t.read_only addrspace(1)* %src, %opencl.image2d_t.read_only addrspace(1)** %src.addr, align 8
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  %0 = load %opencl.image2d_t.read_only addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)** %src.addr, align 8
  %1 = load <2 x i32>, <2 x i32>* %coord.addr, align 8
  %ImageArgVal = ptrtoint %opencl.image2d_t.read_only addrspace(1)* %0 to i64
  %conv.i = trunc i64 %ImageArgVal to i32
  %call.i = call spir_func i32 @__builtin_IB_simd_media_block_read_1(i32 %conv.i, <2 x i32> %1)
  store i32 %call.i, i32* %b_read, align 4
  %2 = load i32, i32* %b_read, align 4
  %3 = load i32 addrspace(1)*, i32 addrspace(1)** %dst.addr, align 8
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %3, i64 0
  store i32 %2, i32 addrspace(1)* %arrayidx, align 4
  ret void
}

declare spir_func i32 @__builtin_IB_simd_media_block_read_1(i32, <2 x i32>) local_unnamed_addr #0

attributes #0 = { convergent noinline nounwind optnone }

!igc.functions = !{!3}
!IGCMetadata = !{!7}

!3 = !{void (i32 addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)*, <2 x i32>)* @test_bread, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"sub_group_size", i32 16}
!7 = !{!"ModuleMD", !8}
!8 = !{!"FuncMD", !9, !10}
!9 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, %opencl.image2d_t.read_only addrspace(1)*, <2 x i32>)* @test_bread}
!10 = !{!"FuncMDValue[0]", !11}
!11 = !{!"resAllocMD", !12}
!12 = !{!"argAllocMDList", !13, !17, !20}
!13 = !{!"argAllocMDListVec[0]", !14, !15, !16}
!14 = !{!"type", i32 1}
!15 = !{!"extensionType", i32 -1}
!16 = !{!"indexType", i32 0}
!17 = !{!"argAllocMDListVec[1]", !18, !19, !16}
!18 = !{!"type", i32 2}
!19 = !{!"extensionType", i32 0}
!20 = !{!"argAllocMDListVec[2]", !21, !15, !22}
!21 = !{!"type", i32 0}
!22 = !{!"indexType", i32 -1}
