;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; Run vISA EmitPass for .ll input derived from codegen.ll obtained
; for the following OCL-C kernel
;    __kernel void test_fn( __global char2 *srcValues, __global uint *offsets, __global char *destBuffer, uint alignmentOffset )
;    {
;        int tid = get_local_id( 0 );
;        vstore2( srcValues[ tid ], offsets[ tid ], destBuffer + alignmentOffset );
;    }

; The test checks if we follow the IND0 restriction, that the surface base address must be aligned to data size.

; REQUIRES: llvm-14-plus, regkeys

; RUN: igc_opt --opaque-pointers -platformCri -simd-mode 32 -igc-emit-visa %s -regkey DumpVISAASMToConsole -regkey EnableEfficient64b | FileCheck %s
; CHECK:      .kernel "test_fn"
; CHECK:      lsc_load.ugm (M1, 32)  {{.*}}:d16u32  flat({{.*}})[0x2*{{.*}}]:a64
; CHECK:      lsc_store.ugm (M1, 32)  flat[{{.*}}]:a64  {{.*}}:d16u32

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind
define spir_kernel void @test_fn(<2 x i8> addrspace(1)* %srcValues, i32 addrspace(1)* %offsets, i8 addrspace(1)* %destBuffer, i32 %alignmentOffset, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ, i8* %privateBase, i8 addrspace(1)* %indirectDataPointer, i8 addrspace(1)* %scratchPointer, i32 %bufferOffset, i32 %bufferOffset1, i32 %bufferOffset2) #0 {
entry:
  %idxprom = zext i16 %localIdX to i64
  %0 = ptrtoint <2 x i8> addrspace(1)* %srcValues to i64
  %1 = shl nuw nsw i64 %idxprom, 1
  %2 = add i64 %1, %0
  %3 = inttoptr i64 %2 to i16 addrspace(1)*
  %4 = load i16, i16 addrspace(1)* %3, align 2
  %5 = shl nuw nsw i64 %idxprom, 2
  %6 = ptrtoint i32 addrspace(1)* %offsets to i64
  %7 = add i64 %5, %6
  %8 = inttoptr i64 %7 to i32 addrspace(1)*
  %9 = load i32, i32 addrspace(1)* %8, align 4
  %conv3 = zext i32 %9 to i64
  %idx.ext = zext i32 %alignmentOffset to i64
  %10 = ptrtoint i8 addrspace(1)* %destBuffer to i64
  %11 = add i64 %10, %idx.ext
  %mul.i = shl nuw nsw i64 %conv3, 1
  %12 = add i64 %11, %mul.i
  %13 = inttoptr i64 %12 to i16 addrspace(1)*
  ; Note: the alignment of the store instruction is 1, data size alignment is 2.
  ; Even though this is not a vector datatype, we can't generate base offset due to WA.
  store i16 %4, i16 addrspace(1)* %13, align 1
  ret void
}

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture) #1

; Function Attrs: convergent
declare spir_func void @__builtin_IB_memcpy_private_to_global(i8 addrspace(1)*, i8*, i32, i32) local_unnamed_addr #2

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture) #1

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_local_id_x() local_unnamed_addr #3

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_local_id_y() local_unnamed_addr #3

; Function Attrs: convergent nounwind readnone
declare spir_func i32 @__builtin_IB_get_local_id_z() local_unnamed_addr #3

; Function Attrs: nounwind
declare void @llvm.assume(i1) #4

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p1i8.p0i8.i32(i8 addrspace(1)* nocapture writeonly, i8* nocapture readonly, i32, i1 immarg) #1

attributes #0 = { convergent nounwind "less-precise-fpmad"="true" "null-pointer-is-valid"="true" }
attributes #1 = { argmemonly nounwind }
attributes #2 = { convergent "correctly-rounded-divide-sqrt-fp-math"="false" "denorms-are-zero"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { convergent nounwind readnone "correctly-rounded-divide-sqrt-fp-math"="false" "denorms-are-zero"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind }

!IGCMetadata = !{!0}
!igc.functions = !{!395}

!0 = !{!"ModuleMD", !90}
!90 = !{!"FuncMD", !91, !92}
!91 = !{!"FuncMDMap[0]", void (<2 x i8> addrspace(1)*, i32 addrspace(1)*, i8 addrspace(1)*, i32, <8 x i32>, <8 x i32>, i16, i16, i16, i8*, i8 addrspace(1)*, i8 addrspace(1)*, i32, i32, i32)* @test_fn}
!92 = !{!"FuncMDValue[0]", !99, !126, !186}
!99 = !{!"functionType", !"KernelFunction"}
!126 = !{!"resAllocMD", !127, !128, !129, !130, !149}
!127 = !{!"uavsNumType", i32 0}
!128 = !{!"srvsNumType", i32 0}
!129 = !{!"samplersNumType", i32 0}
!130 = !{!"argAllocMDList", !131, !135, !136, !137, !138, !139, !140, !141, !142, !143, !144, !145, !146, !147, !148}
!131 = !{!"argAllocMDListVec[0]", !132, !133, !134}
!132 = !{!"type", i32 0}
!133 = !{!"extensionType", i32 -1}
!134 = !{!"indexType", i32 -1}
!135 = !{!"argAllocMDListVec[1]", !132, !133, !134}
!136 = !{!"argAllocMDListVec[2]", !132, !133, !134}
!137 = !{!"argAllocMDListVec[3]", !132, !133, !134}
!138 = !{!"argAllocMDListVec[4]", !132, !133, !134}
!139 = !{!"argAllocMDListVec[5]", !132, !133, !134}
!140 = !{!"argAllocMDListVec[6]", !132, !133, !134}
!141 = !{!"argAllocMDListVec[7]", !132, !133, !134}
!142 = !{!"argAllocMDListVec[8]", !132, !133, !134}
!143 = !{!"argAllocMDListVec[9]", !132, !133, !134}
!144 = !{!"argAllocMDListVec[10]", !132, !133, !134}
!145 = !{!"argAllocMDListVec[11]", !132, !133, !134}
!146 = !{!"argAllocMDListVec[12]", !132, !133, !134}
!147 = !{!"argAllocMDListVec[13]", !132, !133, !134}
!148 = !{!"argAllocMDListVec[14]", !132, !133, !134}
!149 = !{!"inlineSamplersMD"}
!186 = !{!"m_OpenCLArgTypeQualifiers", !187, !188, !189, !190}
!187 = !{!"m_OpenCLArgTypeQualifiersVec[0]", !""}
!188 = !{!"m_OpenCLArgTypeQualifiersVec[1]", !""}
!189 = !{!"m_OpenCLArgTypeQualifiersVec[2]", !""}
!190 = !{!"m_OpenCLArgTypeQualifiersVec[3]", !""}
!395 = !{void (<2 x i8> addrspace(1)*, i32 addrspace(1)*, i8 addrspace(1)*, i32, <8 x i32>, <8 x i32>, i16, i16, i16, i8*, i8 addrspace(1)*, i8 addrspace(1)*, i32, i32, i32)* @test_fn, !396}
!396 = !{!397, !398}
!397 = !{!"function_type", i32 0}
!398 = !{!"implicit_arg_desc", !399, !400, !401, !402, !403, !404, !405, !406, !407, !409, !411}
!399 = !{i32 0}
!400 = !{i32 1}
!401 = !{i32 8}
!402 = !{i32 9}
!403 = !{i32 10}
!404 = !{i32 13}
!405 = !{i32 56}
!406 = !{i32 57}
!407 = !{i32 15, !408}
!408 = !{!"explicit_arg_num", i32 0}
!409 = !{i32 15, !410}
!410 = !{!"explicit_arg_num", i32 1}
!411 = !{i32 15, !412}
!412 = !{!"explicit_arg_num", i32 2}
