;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
;
; RUN: igc_opt -platformbmg -igc-emit-visa -simd-mode 16 -regkey DumpVISAASMToConsole -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; The mask selects second.high, first.low, second.low, first.high. This checks
; a non-BDPAS permutation and all four valid source indices.
;
; CHECK-LABEL: .kernel "test"
; CHECK: mov (M1, 16) [[DST:[A-Za-z0-9_]+]](0,0)<1> [[SECOND:[A-Za-z0-9_]+]](0,1)<2;1,0>
; CHECK: mov (M1, 16) [[DST]](0,16)<1> [[FIRST:[A-Za-z0-9_]+]](0,0)<2;1,0>
; CHECK: mov (M1, 16) [[DST]](0,32)<1> [[SECOND]](0,0)<2;1,0>
; CHECK: mov (M1, 16) [[DST]](0,48)<1> [[FIRST]](0,1)<2;1,0>
; An immediate source is materialized once, then each selected byte is
; broadcast. This also checks a mixed uniform/varying input pair.
; CHECK: mov (M1_NM, 1) {{[A-Za-z0-9_]+}}(0,0)<1> 0x1122:w
; CHECK: mov (M1, 16) [[MIXED_DST:[A-Za-z0-9_]+]](0,0)<1> [[IMM_BYTES:[A-Za-z0-9_]+]](0,1)<0;1,0>
; CHECK: mov (M1, 16) [[MIXED_DST]](0,16)<1> [[SECOND]](0,0)<2;1,0>
; CHECK: mov (M1, 16) [[MIXED_DST]](0,32)<1> [[IMM_BYTES]](0,0)<0;1,0>
; CHECK: mov (M1, 16) [[MIXED_DST]](0,48)<1> [[SECOND]](0,1)<2;1,0>
; Uniform inputs produce a uniform result with one no-mask write per byte.
; CHECK: mov (M1_NM, 1) uniform_swizzled(0,0)<1> [[UNIFORM_SECOND:[A-Za-z0-9_]+]](0,0)<0;1,0>
; CHECK: mov (M1_NM, 1) uniform_swizzled(0,1)<1> [[UNIFORM_FIRST:[A-Za-z0-9_]+]](0,1)<0;1,0>
; CHECK: mov (M1_NM, 1) uniform_swizzled(0,2)<1> [[UNIFORM_SECOND]](0,1)<0;1,0>
; CHECK: mov (M1_NM, 1) uniform_swizzled(0,3)<1> [[UNIFORM_FIRST]](0,0)<0;1,0>
; A same-block load is read from its d16u32 response with byte stride 4. No
; narrow load-result copy is needed.
; CHECK: lsc_load.ugm (M1, 16) [[SAME_WIDE:[A-Za-z0-9_]+]]:d16u32
; CHECK-NOT: mov (M1, 16) same_load(0,0)<1>
; CHECK: mov (M1, 16) same_swizzled(0,0)<1> [[SAME_BYTES:[A-Za-z0-9_]+]](0,0)<4;1,0>
; CHECK: mov (M1, 16) same_swizzled(0,16)<1> [[SECOND]](0,0)<2;1,0>
; CHECK: mov (M1, 16) same_swizzled(0,32)<1> [[SAME_BYTES]](0,1)<4;1,0>
; CHECK: mov (M1, 16) same_swizzled(0,48)<1> [[SECOND]](0,1)<2;1,0>
; Two direct loads with the BDPAS mask still use the general masked path when
; the result has a non-BDPAS consumer.
; CHECK: mov (M1, 16) generic_swizzled(0,0)<1> [[GENERIC_FIRST:[A-Za-z0-9_]+]](0,0)<4;1,0>
; CHECK: mov (M1, 16) generic_swizzled(0,16)<1> [[GENERIC_SECOND:[A-Za-z0-9_]+]](0,0)<4;1,0>
; CHECK: mov (M1, 16) generic_swizzled(0,32)<1> [[GENERIC_FIRST]](0,1)<4;1,0>
; CHECK: mov (M1, 16) generic_swizzled(0,48)<1> [[GENERIC_SECOND]](0,1)<4;1,0>
; A cross-block use cannot read the block-local d16u32 response. Retain the
; narrow load-result copy and swizzle from it with byte stride 2.
; CHECK: lsc_load.ugm (M1, 16) {{[A-Za-z0-9_]+}}:d16u32
; CHECK-NEXT: mov (M1, 16) cross_load(0,0)<1> {{[A-Za-z0-9_]+}}(0,0)<2;1,0>
; CHECK: _test_001_cross_use:
; CHECK: mov (M1, 16) cross_swizzled(0,0)<1> cross_load_0v(0,0)<2;1,0>
; CHECK: mov (M1, 16) cross_swizzled(0,16)<1> [[SECOND]](0,0)<2;1,0>
; CHECK: mov (M1, 16) cross_swizzled(0,32)<1> cross_load_0v(0,1)<2;1,0>
; CHECK: mov (M1, 16) cross_swizzled(0,48)<1> [[SECOND]](0,1)<2;1,0>
define spir_kernel void @test(i32 addrspace(1)* %src, i16 addrspace(1)* %dst, i32 %bufferOffset, i32 %bufferOffset1) {
entry:
  %localIdX = call i16 @llvm.genx.GenISA.simdLaneId()
  %first = add i16 %localIdX, 4386
  %second = xor i16 %localIdX, 13124
  %swizzled = call <4 x i8> @llvm.genx.GenISA.byte.swizzle(i16 %first, i16 %second, <4 x i32> <i32 3, i32 0, i32 2, i32 1>)
  %packed = bitcast <4 x i8> %swizzled to i32

  %id = zext i16 %localIdX to i64
  %offset = shl i64 %id, 1
  %dst.offset = getelementptr i16, i16 addrspace(1)* %dst, i64 %offset
  %dst.scalar = bitcast i16 addrspace(1)* %dst.offset to i32 addrspace(1)*
  store i32 %packed, i32 addrspace(1)* %dst.scalar, align 4

  %mixed.swizzled = call <4 x i8> @llvm.genx.GenISA.byte.swizzle(i16 4386, i16 %second, <4 x i32> <i32 1, i32 2, i32 0, i32 3>)
  %mixed.packed = bitcast <4 x i8> %mixed.swizzled to i32
  %mixed.dst = getelementptr i32, i32 addrspace(1)* %dst.scalar, i64 16
  store i32 %mixed.packed, i32 addrspace(1)* %mixed.dst, align 4

  %uniform.first = trunc i32 %bufferOffset to i16
  %uniform.second = trunc i32 %bufferOffset1 to i16
  %uniform.swizzled = call <4 x i8> @llvm.genx.GenISA.byte.swizzle(i16 %uniform.first, i16 %uniform.second, <4 x i32> <i32 2, i32 1, i32 3, i32 0>)
  %uniform.packed = bitcast <4 x i8> %uniform.swizzled to i32
  store i32 %uniform.packed, i32 addrspace(1)* %src, align 4

  %same.load = load i16, i16 addrspace(1)* %dst.offset, align 2
  %same.swizzled = call <4 x i8> @llvm.genx.GenISA.byte.swizzle(i16 %same.load, i16 %second, <4 x i32> <i32 0, i32 2, i32 1, i32 3>)
  %same.packed = bitcast <4 x i8> %same.swizzled to i32
  %same.dst = getelementptr i32, i32 addrspace(1)* %mixed.dst, i64 16
  store i32 %same.packed, i32 addrspace(1)* %same.dst, align 4

  %generic.first.ptr = getelementptr i16, i16 addrspace(1)* %dst.offset, i64 2
  %generic.second.ptr = getelementptr i16, i16 addrspace(1)* %dst.offset, i64 3
  %generic.first = load i16, i16 addrspace(1)* %generic.first.ptr, align 2
  %generic.second = load i16, i16 addrspace(1)* %generic.second.ptr, align 2
  %generic.swizzled = call <4 x i8> @llvm.genx.GenISA.byte.swizzle(i16 %generic.first, i16 %generic.second, <4 x i32> <i32 0, i32 2, i32 1, i32 3>)
  %generic.packed = bitcast <4 x i8> %generic.swizzled to i32
  %generic.dst = getelementptr i32, i32 addrspace(1)* %same.dst, i64 16
  store i32 %generic.packed, i32 addrspace(1)* %generic.dst, align 4

  %cross.load = load i16, i16 addrspace(1)* %dst.offset, align 2
  br label %cross.use

cross.use:
  %cross.swizzled = call <4 x i8> @llvm.genx.GenISA.byte.swizzle(i16 %cross.load, i16 %second, <4 x i32> <i32 0, i32 2, i32 1, i32 3>)
  %cross.packed = bitcast <4 x i8> %cross.swizzled to i32
  %cross.dst = getelementptr i32, i32 addrspace(1)* %generic.dst, i64 16
  store i32 %cross.packed, i32 addrspace(1)* %cross.dst, align 4
  ret void
}

declare <4 x i8> @llvm.genx.GenISA.byte.swizzle(i16, i16, <4 x i32>)
declare i16 @llvm.genx.GenISA.simdLaneId()

!igc.functions = !{!0}
!0 = !{void (i32 addrspace(1)*, i16 addrspace(1)*, i32, i32)* @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", void (i32 addrspace(1)*, i16 addrspace(1)*, i32, i32)* @test}
!6 = !{!"FuncMDValue[0]", !7}
!7 = !{!"resAllocMD", !8}
!8 = !{!"argAllocMDList", !9, !10, !11, !12}
!9 = !{!"argAllocMDListVec[0]", !13, !14, !15}
!10 = !{!"argAllocMDListVec[1]", !13, !14, !15}
!11 = !{!"argAllocMDListVec[2]", !13, !14, !15}
!12 = !{!"argAllocMDListVec[3]", !13, !14, !15}
!13 = !{!"type", i32 0}
!14 = !{!"extensionType", i32 -1}
!15 = !{!"indexType", i32 -1}
!IGCMetadata = !{!3}
