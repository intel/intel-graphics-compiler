;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; This test was reduced from the following sycl kernel:
; template <typename T>
; void test(T *d, const T *s, size_t count)
; {
;     T *aligned_d = (T *) sycl::min(((((uintptr_t) d) + ALIGNMASK) & (~ALIGNMASK)),
;                                (uintptr_t) (d + count));
;     ulong ad = (uintptr_t) aligned_d;
;     ulong p = (uintptr_t) &d[0];
;     if (p >= ad)
;         d[0] = s[0];
; }
; The issue was with (p >= ad) comparison, where only p was getting its tag cleared.
; That caused the result of the comparison to be wrong.

; REQUIRES: regkeys, llvm-14-plus
; RUN: igc_opt -platformpvc -rev-id B -has-emulated-64-bit-insts -igc-emit-visa -regkey DumpVISAASMToConsole -S %s | FileCheck %s

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

; Function Attrs: convergent nounwind null_pointer_is_valid
define spir_kernel void @test(i64 addrspace(1)* align 8 %0, i64 addrspace(1)* align 8 %1, i64 %2, <8 x i32> %r0, <8 x i32> %payloadHeader, i32 %bufferOffset, i32 %bufferOffset1) #0 {
  %4 = addrspacecast i64 addrspace(1)* %0 to i64 addrspace(4)*
  %5 = ptrtoint i64 addrspace(4)* %4 to i64
  %6 = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p4i64(i64 addrspace(4)* %4)
  %7 = extractvalue { i32, i32 } %6, 0
  %8 = extractvalue { i32, i32 } %6, 1
  %9 = add i64 %5, 127
  %10 = bitcast i64 %9 to <2 x i32>
  %11 = extractelement <2 x i32> %10, i32 0
  %12 = extractelement <2 x i32> %10, i32 1
  %13 = and i32 %11, -128
  %14 = ptrtoint i64 addrspace(1)* %0 to i64
  %15 = shl i64 %2, 3
  %16 = add i64 %15, %14
  %17 = inttoptr i64 %16 to i64 addrspace(1)*
  %18 = addrspacecast i64 addrspace(1)* %17 to i64 addrspace(4)*
  %19 = call { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p4i64(i64 addrspace(4)* %18)
  %20 = extractvalue { i32, i32 } %19, 0
  %21 = extractvalue { i32, i32 } %19, 1
  %22 = icmp eq i32 %12, %21
  %23 = icmp ult i32 %13, %20
  %24 = and i1 %22, %23
  %25 = icmp ult i32 %12, %21
  %26 = or i1 %24, %25
  %27 = select i1 %26, i32 %13, i32 %20
  %28 = select i1 %26, i32 %12, i32 %21
  %29 = icmp eq i32 %28, %8
  %30 = icmp ugt i32 %27, %7
  %31 = and i1 %29, %30
  br label %bug.here

bug.here:
  ; issue was here, with %28 not getting its tag cleared.
  ; CHECK-LABEL: {{.*}}bug_here:
  ; CHECK-NEXT: shl ({{.*}}) [[TMP0:V[0-9]*]]
  ; CHECK-NEXT: asr ({{.*}}) [[TMP0]]
  ; CHECK-NEXT: shl ({{.*}}) [[TMP1:V[0-9]*]]
  ; CHECK-NEXT: asr ({{.*}}) [[TMP1]]
  ; CHECK-NEXT: cmp.gt {{.*}} [[TMP0]]({{.*}})<{{.*}}> [[TMP1]]({{.*}})<{{.*}}>
  %32 = icmp ugt i32 %28, %8
  %33 = or i1 %31, %32
  br i1 %33, label %._crit_edge, label %34

34:                                               ; preds = %3
  %35 = bitcast i64 addrspace(1)* %1 to <2 x i32> addrspace(1)*
  %36 = load <2 x i32>, <2 x i32> addrspace(1)* %35, align 8
  %37 = bitcast i64 addrspace(1)* %0 to <2 x i32> addrspace(1)*
  store <2 x i32> %36, <2 x i32> addrspace(1)* %37, align 8
  br label %38

._crit_edge:                                      ; preds = %3
  br label %38

38:                                               ; preds = %._crit_edge, %34
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i32 3)
  call void @llvm.genx.GenISA.memoryfence(i1 true, i1 true, i1 false, i1 false, i1 false, i1 true, i1 true, i1 false, i32 3)
  ret void
}

; Function Attrs: convergent
declare spir_func void @__builtin_IB_memfence(i1 noundef zeroext, i1 noundef zeroext, i1 noundef zeroext, i1 noundef zeroext, i1 noundef zeroext, i1 noundef zeroext, i1 noundef zeroext, i1 noundef zeroext, i32 noundef) local_unnamed_addr #1

; Function Desc:
; Output:
; Arg 0: Commit Enable
; Arg 1: L3_Flush_RW_Data
; Arg 2: L3_Flush_Constant_Data
; Arg 3: L3_Flush_Texture_Data
; Arg 4: L3_Flush_Instructions
; Arg 5: Fence has global effect
; Arg 6: L1 Invalidate
; Arg 7: L1 Evict
; Arg 8: Fence memory scope
; Function Attrs: convergent nounwind
declare void @llvm.genx.GenISA.memoryfence(i1, i1, i1, i1, i1, i1, i1, i1, i32) #2

; Function Desc:
; Output:
; Arg 0:
; Function Attrs: nounwind readnone
declare { i32, i32 } @llvm.genx.GenISA.ptr.to.pair.p4i64(i64 addrspace(4)*) #3

attributes #0 = { convergent nounwind null_pointer_is_valid "less-precise-fpmad"="true" }
attributes #1 = { convergent "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { convergent nounwind }
attributes #3 = { nounwind readnone }

!spirv.MemoryModel = !{!0}
!spirv.Source = !{!1}
!spirv.Generator = !{!2}
!igc.functions = !{!3}
!IGCMetadata = !{!13}
!printf.strings = !{}

!0 = !{i32 2, i32 2}
!1 = !{i32 4, i32 100000}
!2 = !{i16 6, i16 14}
!3 = !{void (i64 addrspace(1)*, i64 addrspace(1)*, i64, <8 x i32>, <8 x i32>, i32, i32)* @test, !4}
!4 = !{!5, !6}
!5 = !{!"function_type", i32 0}
!6 = !{!"implicit_arg_desc", !7, !8, !9, !11}
!7 = !{i32 0}
!8 = !{i32 1}
!9 = !{i32 15, !10}
!10 = !{!"explicit_arg_num", i32 0}
!11 = !{i32 15, !12}
!12 = !{!"explicit_arg_num", i32 1}
!13 = !{!"ModuleMD", !14}
!14 = !{!"FuncMD", !15, !16}
!15 = !{!"FuncMDMap[0]", void (i64 addrspace(1)*, i64 addrspace(1)*, i64, <8 x i32>, <8 x i32>, i32, i32)* @test}
!16 = !{!"FuncMDValue[0]", !17}
!17 = !{!"resAllocMD", !18, !19, !20, !21, !32}
!18 = !{!"uavsNumType", i32 0}
!19 = !{!"srvsNumType", i32 0}
!20 = !{!"samplersNumType", i32 0}
!21 = !{!"argAllocMDList", !22, !26, !27, !28, !29, !30, !31}
!22 = !{!"argAllocMDListVec[0]", !23, !24, !25}
!23 = !{!"type", i32 0}
!24 = !{!"extensionType", i32 -1}
!25 = !{!"indexType", i32 -1}
!26 = !{!"argAllocMDListVec[1]", !23, !24, !25}
!27 = !{!"argAllocMDListVec[2]", !23, !24, !25}
!28 = !{!"argAllocMDListVec[3]", !23, !24, !25}
!29 = !{!"argAllocMDListVec[4]", !23, !24, !25}
!30 = !{!"argAllocMDListVec[5]", !23, !24, !25}
!31 = !{!"argAllocMDListVec[6]", !23, !24, !25}
!32 = !{!"inlineSamplersMD"}
