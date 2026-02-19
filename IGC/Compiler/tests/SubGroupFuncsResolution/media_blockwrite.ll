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
; __kernel void test_bwrite(write_only image2d_t dst, int2 coord, int src)
; {
;     intel_sub_group_block_write(dst, coord, src );
; }
;
; ------------------------------------------------

; Debug-info related check
; CHECK-NOT: WARNING
; CHECK: CheckModuleDebugify: PASS

%opencl.image2d_t.write_only = type opaque

; Function Attrs: convergent noinline nounwind optnone
define spir_kernel void @test_bwrite(%opencl.image2d_t.write_only addrspace(1)* %dst, <2 x i32> %coord, i32 %src) #0 {
; CHECK-LABEL: @test_bwrite(
; CHECK:  entry:
; CHECK:    [[DST_ADDR:%.*]] = alloca [[OPENCL_IMAGE2D_T_WRITE_ONLY:%.*]] addrspace(1)*, align 8
; CHECK:    [[COORD_ADDR:%.*]] = alloca <2 x i32>, align 8
; CHECK:    [[SRC_ADDR:%.*]] = alloca i32, align 4
; CHECK:    [[TMP1:%.*]] = load <2 x i32>, <2 x i32>* [[COORD_ADDR]], align 8
; CHECK:    [[TMP2:%.*]] = load i32, i32* [[SRC_ADDR]], align 4
; CHECK:    [[XOFFSET:%.*]] = extractelement <2 x i32> [[TMP1]], i32 0
; CHECK:    [[YOFFSET:%.*]] = extractelement <2 x i32> [[TMP1]], i32 1
; CHECK:    call void @llvm.genx.GenISA.simdMediaBlockWrite.i32.i32(i32 1, i32 [[XOFFSET]], i32 [[YOFFSET]], i32 1, i32 [[TMP2]])
; CHECK:    ret void
;
; CHECK:  declare void @llvm.genx.GenISA.simdMediaBlockWrite.i32.i32(i32, i32, i32, i32, i32) [[ATTR:#.*]]
; CHECK:  attributes [[ATTR]] = { {{.*convergent.*}} }
;
entry:
  %dst.addr = alloca %opencl.image2d_t.write_only addrspace(1)*, align 8
  %coord.addr = alloca <2 x i32>, align 8
  %src.addr = alloca i32, align 4
  store %opencl.image2d_t.write_only addrspace(1)* %dst, %opencl.image2d_t.write_only addrspace(1)** %dst.addr, align 8
  store <2 x i32> %coord, <2 x i32>* %coord.addr, align 8
  store i32 %src, i32* %src.addr, align 4
  %0 = load %opencl.image2d_t.write_only addrspace(1)*, %opencl.image2d_t.write_only addrspace(1)** %dst.addr, align 8
  %1 = load <2 x i32>, <2 x i32>* %coord.addr, align 8
  %2 = load i32, i32* %src.addr, align 4
  %ImageArgVal = ptrtoint %opencl.image2d_t.write_only addrspace(1)* %0 to i64
  %conv.i = trunc i64 %ImageArgVal to i32
  call spir_func void @__builtin_IB_simd_media_block_write_1(i32 %conv.i, <2 x i32> %1, i32 %2)
  ret void
}

; Function Attrs: convergent noinline nounwind optnone
declare spir_func void @__builtin_IB_simd_media_block_write_1(i32, <2 x i32>, i32) local_unnamed_addr #0

attributes #0 = { convergent noinline nounwind optnone }

!igc.functions = !{!3}
!IGCMetadata = !{!7}

!3 = !{void (%opencl.image2d_t.write_only addrspace(1)*, <2 x i32>, i32)* @test_bwrite, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"sub_group_size", i32 16}
!7 = !{!"ModuleMD", !8}
!8 = !{!"FuncMD", !9, !10}
!9 = !{!"FuncMDMap[0]", void (%opencl.image2d_t.write_only addrspace(1)*, <2 x i32>, i32)* @test_bwrite}
!10 = !{!"FuncMDValue[0]", !11}
!11 = !{!"resAllocMD", !12}
!12 = !{!"argAllocMDList", !13, !17, !21}
!13 = !{!"argAllocMDListVec[0]", !14, !15, !16}
!14 = !{!"type", i32 1}
!15 = !{!"extensionType", i32 0}
!16 = !{!"indexType", i32 1}
!17 = !{!"argAllocMDListVec[1]", !18, !19, !20}
!18 = !{!"type", i32 0}
!19 = !{!"extensionType", i32 -1}
!20 = !{!"indexType", i32 -1}
!21 = !{!"argAllocMDListVec[2]", !18, !19, !20}
