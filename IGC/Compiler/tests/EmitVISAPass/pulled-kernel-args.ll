;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; REQUIRES: regkeys
;
; RUN: igc_opt --opaque-pointers -ocl -platformbmg -igc-emit-visa %s -regkey DumpVISAASMToConsole,EnablePassInlineData=1 | FileCheck %s --check-prefixes=CHECK,INLINE32,BMG
; RUN: igc_opt --opaque-pointers -ocl -platformCri -igc-emit-visa %s -regkey DumpVISAASMToConsole,EnablePassInlineData=1,EnableEfficient64b=1 | FileCheck %s --check-prefixes=CHECK,INLINE64,CRI
; RUN: igc_opt --opaque-pointers -ocl -platformbmg -igc-emit-visa %s -regkey DumpVISAASMToConsole,EnablePassInlineData=1,EnableGlobalStateBuffer=0 | FileCheck %s --check-prefixes=CHECK,INLINE32,NORELOC
; ------------------------------------------------
; EmitVISAPass
; ------------------------------------------------

; Test that pulled kernel arguments (FuncMD pulledArgs) get no GRF input and that
; GenISA_getKernelArgPtr is lowered to the cross-thread offset relocation (emitted once,
; at the kernel start) plus the memory offset of the argument. Without the relocation
; (with EnableGlobalStateBuffer=0) the memory offset is used directly.
;
; The input is the IR produced by PullKernelArguments: %out, %a and %v have 44 B fit in the 2048 B
; push budget, %big (4000 B) does not, so %big and all arguments after it (%b, %p) are pulled.
; Pulled arguments are placed after the last pushed argument in declaration order.
; Memory offset = payload offset - inline data size.
;
; Payload offsets are the same on both platforms: %out 0, %a 8, %v 32, %big 64, %b 4064,
; %p 4072 (end 4080, cross-thread data size 4096). Only the inline data size differs:
; - BMG, 32 B: memory offsets 0x20, 0xFC0, 0xFC8, PerThreadPayloadOffset 4096 - 32 = 4064;
;   %v is after inline data, so it starts in the next GRF (input offset 128).
; - CRI, 64 B: memory offsets 0x0, 0xFA0, 0xFA8, PerThreadPayloadOffset 4096 - 64 = 4032;
;   %v is in inline data (input offset 96).

%struct.Big = type { [1000 x i32] }

define spir_kernel void @pulled(ptr addrspace(1) %out, i32 %a, <8 x i32> %v, ptr byval(%struct.Big) align 4 %big, i32 %b, ptr addrspace(1) %p, <8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
  %big_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 3)
  %big_1_ptr = getelementptr inbounds i8, ptr addrspace(5) %big_pulled, i64 4
  %big_1 = load i32, ptr addrspace(5) %big_1_ptr, align 4
  %b_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 4)
  %b_val = load i32, ptr addrspace(5) %b_pulled, align 4
  %p_pulled = call ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32 5)
  %p_val = load ptr addrspace(1), ptr addrspace(5) %p_pulled, align 8
  %v7 = extractelement <8 x i32> %v, i64 7
  %t0 = add i32 %a, %v7
  %t1 = add i32 %t0, %b_val
  %t2 = add i32 %t1, %big_1
  store i32 %t2, ptr addrspace(1) %p_val, align 4
  store i32 %t2, ptr addrspace(1) %out, align 4
  ret void
}

; CHECK-LABEL: .kernel "pulled"
; CHECK:            .input out offset=64 size=8
; CHECK-NEXT:       .input a offset=72 size=4
; INLINE32-NEXT:    .input v offset=128 size=32
; INLINE64-NEXT:    .input v offset=96 size=32
; CHECK-NOT:        .input
; INLINE32:         .kernel_attr PerThreadPayloadOffset=4064
; INLINE64:         .kernel_attr PerThreadPayloadOffset=4032

; The relocation is emitted once, at the kernel start
; BMG:              faddr __INTEL_PATCH_CROSS_THREAD_OFFSET_OFF_R0 crossThreadOffset(0,0)<1>
; CRI:              faddr __INTEL_PATCH_CROSS_THREAD_OFFSET_OFF_R0 crossThreadOffset(0,0)<1>
; CHECK-NOT:        faddr

; BMG-NEXT:         add (M1_NM, 1) big_pulled(0,0)<1> crossThreadOffset(0,0)<0;1,0> 0x20:ud
; CRI-NEXT:         add (M1_NM, 1) big_pulled(0,0)<1> crossThreadOffset(0,0)<0;1,0> 0x0:ud
; NORELOC:          mov (M1_NM, 1) big_pulled(0,0)<1> 0x20:ud
; CHECK:            lsc_load.ugm (M1_NM, 1)  big_1:d32t  arg[big_1_ptr]:a32

; BMG-NEXT:         add (M1_NM, 1) b_pulled(0,0)<1> crossThreadOffset(0,0)<0;1,0> 0xfc0:ud
; CRI-NEXT:         add (M1_NM, 1) b_pulled(0,0)<1> crossThreadOffset(0,0)<0;1,0> 0xfa0:ud
; NORELOC-NEXT:     mov (M1_NM, 1) b_pulled(0,0)<1> 0xfc0:ud
; CHECK-NEXT:       lsc_load.ugm (M1_NM, 1)  b_val:d32t  arg[b_pulled]:a32

; BMG-NEXT:         add (M1_NM, 1) p_pulled(0,0)<1> crossThreadOffset(0,0)<0;1,0> 0xfc8:ud
; CRI-NEXT:         add (M1_NM, 1) p_pulled(0,0)<1> crossThreadOffset(0,0)<0;1,0> 0xfa8:ud
; NORELOC-NEXT:     mov (M1_NM, 1) p_pulled(0,0)<1> 0xfc8:ud
; CHECK-NEXT:       lsc_load.ugm (M1_NM, 1)  p_val:d64t  arg[p_pulled]:a32
; CHECK-NOT:        faddr
; CHECK:            ret


define spir_kernel void @not_pulled(ptr addrspace(1) %out, i32 %a, <8 x i32> %r0, <8 x i32> %payloadHeader) {
entry:
  store i32 %a, ptr addrspace(1) %out, align 4
  ret void
}

; Kernel without pulled arguments: no relocation, no attribute
; CHECK-LABEL: .kernel "not_pulled"
; CHECK-NOT:        PerThreadPayloadOffset
; CHECK-NOT:        faddr
; CHECK:            ret

declare ptr addrspace(5) @llvm.genx.GenISA.getKernelArgPtr.p5(i32)

!igc.functions = !{!0, !1}
!IGCMetadata = !{!3}

!0 = !{ptr @pulled, !2}
!1 = !{ptr @not_pulled, !2}
!2 = !{!4}
!4 = !{!"function_type", i32 0}
!3 = !{!"ModuleMD", !5}
!5 = !{!"FuncMD", !6, !7, !8, !9}
!6 = !{!"FuncMDMap[0]", ptr @pulled}
!7 = !{!"FuncMDValue[0]", !10, !11, !30, !40}
!8 = !{!"FuncMDMap[1]", ptr @not_pulled}
!9 = !{!"FuncMDValue[1]", !10, !12, !30}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"resAllocMD", !13}
!12 = !{!"resAllocMD", !14}
!13 = !{!"argAllocMDList", !20, !21, !22, !23, !24, !25, !26, !27}
!14 = !{!"argAllocMDList", !20, !21, !22, !23}
!15 = !{!"type", i32 0}
!16 = !{!"extensionType", i32 -1}
!17 = !{!"indexType", i32 -1}
!20 = !{!"argAllocMDListVec[0]", !15, !16, !17}
!21 = !{!"argAllocMDListVec[1]", !15, !16, !17}
!22 = !{!"argAllocMDListVec[2]", !15, !16, !17}
!23 = !{!"argAllocMDListVec[3]", !15, !16, !17}
!24 = !{!"argAllocMDListVec[4]", !15, !16, !17}
!25 = !{!"argAllocMDListVec[5]", !15, !16, !17}
!26 = !{!"argAllocMDListVec[6]", !15, !16, !17}
!27 = !{!"argAllocMDListVec[7]", !15, !16, !17}
!30 = !{!"implicitArgInfoList", !31, !33}
!31 = !{!"implicitArgInfoListVec[0]", !32}
!32 = !{!"argId", i32 0}
!33 = !{!"implicitArgInfoListVec[1]", !34}
!34 = !{!"argId", i32 1}
!40 = !{!"pulledArgs", !41, !42, !43}
!41 = !{!"pulledArgsVec[0]", i32 3}
!42 = !{!"pulledArgsVec[1]", i32 4}
!43 = !{!"pulledArgsVec[2]", i32 5}
