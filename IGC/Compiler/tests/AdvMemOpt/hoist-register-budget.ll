;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: llvm-14-plus, regkeys
;
; RUN: igc_opt --opaque-pointers -igc-advmemopt -S --platformbmg \
; RUN:   --regkey=ForceOCLSIMDWidth=32 < %s | FileCheck %s

; Test: AdvMemOpt::hoistBudgetInRegisters() and the headroom check in
; hoistInst().
;
; Every kernel is the same loop with the same hoistable uniform load %l1; they
; differ only in what else is live across the lead block %bb.
;
; Sizes are calibrated: bmg gives 128 GRFs of 64 bytes each and the pass reserves
; an eighth, so occupancy of 112 registers or more refuses the hoist. If either
; platform number changes, one CHECK of each pair fails rather than the pair
; quietly agreeing.

; --- Lane-varying pressure -------------------------------------------------
;
; @headroom has nothing else live, so %l1 moves up.
;
; @saturated keeps four <64 x i32> values live from %entry through to %end. At
; SIMD32 each one is 8 KB, the whole register file on its own, so the lead block
; has no headroom left and the hoist is refused. They are indexed by the local
; ID to keep them lane-varying.

define spir_kernel void @headroom(i32 %a, ptr addrspace(1) %b, ptr addrspace(1) %b2) {
entry:
  %c0 = icmp slt i32 %a, 13
  br i1 %c0, label %bb, label %end

bb:
  %iv = phi i32 [ 0, %entry ], [ %ivnext, %bb1 ]
  %l0 = load i32, ptr addrspace(1) %b, align 4
  br label %bb1

bb1:
  %l1 = load i32, ptr addrspace(1) %b2, align 4
  %ivnext = add i32 %iv, %l0
  %t = add i32 %l1, %ivnext
  %cc = icmp slt i32 %ivnext, %a
  br i1 %cc, label %bb, label %end

end:
  %r = phi i32 [ %a, %entry ], [ %t, %bb1 ]
  store i32 %r, ptr addrspace(1) %b, align 4
  ret void
}

define spir_kernel void @saturated(i32 %a, ptr addrspace(1) %b, ptr addrspace(1) %b2, ptr addrspace(1) %vin,
                                   ptr addrspace(1) %vout) {
entry:
  %lid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %p0 = getelementptr inbounds <64 x i32>, ptr addrspace(1) %vin, i32 %lid
  %v0 = load <64 x i32>, ptr addrspace(1) %p0, align 4
  %i1 = add i32 %lid, 1
  %p1 = getelementptr inbounds <64 x i32>, ptr addrspace(1) %vin, i32 %i1
  %v1 = load <64 x i32>, ptr addrspace(1) %p1, align 4
  %i2 = add i32 %lid, 2
  %p2 = getelementptr inbounds <64 x i32>, ptr addrspace(1) %vin, i32 %i2
  %v2 = load <64 x i32>, ptr addrspace(1) %p2, align 4
  %i3 = add i32 %lid, 3
  %p3 = getelementptr inbounds <64 x i32>, ptr addrspace(1) %vin, i32 %i3
  %v3 = load <64 x i32>, ptr addrspace(1) %p3, align 4
  %c0 = icmp slt i32 %a, 13
  br i1 %c0, label %bb, label %end

bb:
  %iv = phi i32 [ 0, %entry ], [ %ivnext, %bb1 ]
  %l0 = load i32, ptr addrspace(1) %b, align 4
  br label %bb1

bb1:
  %l1 = load i32, ptr addrspace(1) %b2, align 4
  %ivnext = add i32 %iv, %l0
  %t = add i32 %l1, %ivnext
  %cc = icmp slt i32 %ivnext, %a
  br i1 %cc, label %bb, label %end

end:
  %r = phi i32 [ %a, %entry ], [ %t, %bb1 ]
  store i32 %r, ptr addrspace(1) %b, align 4
  %s0 = add <64 x i32> %v0, %v1
  %s1 = add <64 x i32> %v2, %v3
  %s2 = add <64 x i32> %s0, %s1
  store <64 x i32> %s2, ptr addrspace(1) %vout, align 4
  ret void
}

; --- Uniform pressure ------------------------------------------------------
;
; A crowd of small uniform values is what a byte estimate gets wrong: 40 uniform
; i32 loads at 4 bytes each sum and round to 3 registers, where the allocator
; gives each its own, which is 40.
;
; Both kernels carry the same <44 x i32> lane-varying ballast: 5632 bytes at
; SIMD32, so 88 registers under either reading, plus 6 for the scalars live
; across the lead.
;
; @uniform_headroom is the ballast alone, 94 of 112, so the hoist is allowed --
; here so a failure in @uniform_crowd cannot be blamed on the ballast.
;
; @uniform_crowd adds the 40 loads: 134 registers counted per value, so refused;
; summed in bytes it is 92 and the load would go through.

define spir_kernel void @uniform_headroom(i32 %a, ptr addrspace(1) %b, ptr addrspace(1) %b2,
                                          ptr addrspace(1) %vin, ptr addrspace(1) %vout) {
entry:
  %lid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %p0 = getelementptr inbounds <44 x i32>, ptr addrspace(1) %vin, i32 %lid
  %v0 = load <44 x i32>, ptr addrspace(1) %p0, align 4
  %c0 = icmp slt i32 %a, 13
  br i1 %c0, label %bb, label %end

bb:
  %iv = phi i32 [ 0, %entry ], [ %ivnext, %bb1 ]
  %l0 = load i32, ptr addrspace(1) %b, align 4
  br label %bb1

bb1:
  %l1 = load i32, ptr addrspace(1) %b2, align 4
  %ivnext = add i32 %iv, %l0
  %t = add i32 %l1, %ivnext
  %cc = icmp slt i32 %ivnext, %a
  br i1 %cc, label %bb, label %end

end:
  %r = phi i32 [ %a, %entry ], [ %t, %bb1 ]
  store i32 %r, ptr addrspace(1) %b, align 4
  store <44 x i32> %v0, ptr addrspace(1) %vout, align 4
  ret void
}

define spir_kernel void @uniform_crowd(i32 %a, ptr addrspace(1) %b, ptr addrspace(1) %b2,
                                       ptr addrspace(1) %vin, ptr addrspace(1) %vout,
                                       ptr addrspace(1) %u) {
entry:
  %lid = call i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32 17)
  %p0 = getelementptr inbounds <44 x i32>, ptr addrspace(1) %vin, i32 %lid
  %v0 = load <44 x i32>, ptr addrspace(1) %p0, align 4
  %u0 = load i32, ptr addrspace(1) %u, align 4
  %u1 = load i32, ptr addrspace(1) %u, align 4
  %u2 = load i32, ptr addrspace(1) %u, align 4
  %u3 = load i32, ptr addrspace(1) %u, align 4
  %u4 = load i32, ptr addrspace(1) %u, align 4
  %u5 = load i32, ptr addrspace(1) %u, align 4
  %u6 = load i32, ptr addrspace(1) %u, align 4
  %u7 = load i32, ptr addrspace(1) %u, align 4
  %u8 = load i32, ptr addrspace(1) %u, align 4
  %u9 = load i32, ptr addrspace(1) %u, align 4
  %u10 = load i32, ptr addrspace(1) %u, align 4
  %u11 = load i32, ptr addrspace(1) %u, align 4
  %u12 = load i32, ptr addrspace(1) %u, align 4
  %u13 = load i32, ptr addrspace(1) %u, align 4
  %u14 = load i32, ptr addrspace(1) %u, align 4
  %u15 = load i32, ptr addrspace(1) %u, align 4
  %u16 = load i32, ptr addrspace(1) %u, align 4
  %u17 = load i32, ptr addrspace(1) %u, align 4
  %u18 = load i32, ptr addrspace(1) %u, align 4
  %u19 = load i32, ptr addrspace(1) %u, align 4
  %u20 = load i32, ptr addrspace(1) %u, align 4
  %u21 = load i32, ptr addrspace(1) %u, align 4
  %u22 = load i32, ptr addrspace(1) %u, align 4
  %u23 = load i32, ptr addrspace(1) %u, align 4
  %u24 = load i32, ptr addrspace(1) %u, align 4
  %u25 = load i32, ptr addrspace(1) %u, align 4
  %u26 = load i32, ptr addrspace(1) %u, align 4
  %u27 = load i32, ptr addrspace(1) %u, align 4
  %u28 = load i32, ptr addrspace(1) %u, align 4
  %u29 = load i32, ptr addrspace(1) %u, align 4
  %u30 = load i32, ptr addrspace(1) %u, align 4
  %u31 = load i32, ptr addrspace(1) %u, align 4
  %u32 = load i32, ptr addrspace(1) %u, align 4
  %u33 = load i32, ptr addrspace(1) %u, align 4
  %u34 = load i32, ptr addrspace(1) %u, align 4
  %u35 = load i32, ptr addrspace(1) %u, align 4
  %u36 = load i32, ptr addrspace(1) %u, align 4
  %u37 = load i32, ptr addrspace(1) %u, align 4
  %u38 = load i32, ptr addrspace(1) %u, align 4
  %u39 = load i32, ptr addrspace(1) %u, align 4
  %c0 = icmp slt i32 %a, 13
  br i1 %c0, label %bb, label %end

bb:
  %iv = phi i32 [ 0, %entry ], [ %ivnext, %bb1 ]
  %l0 = load i32, ptr addrspace(1) %b, align 4
  br label %bb1

bb1:
  %l1 = load i32, ptr addrspace(1) %b2, align 4
  %ivnext = add i32 %iv, %l0
  %t = add i32 %l1, %ivnext
  %cc = icmp slt i32 %ivnext, %a
  br i1 %cc, label %bb, label %end

end:
  %r = phi i32 [ %a, %entry ], [ %t, %bb1 ]
  store i32 %r, ptr addrspace(1) %b, align 4
  store <44 x i32> %v0, ptr addrspace(1) %vout, align 4
  call void @sink(i32 %u0, i32 %u1, i32 %u2, i32 %u3, i32 %u4, i32 %u5, i32 %u6, i32 %u7, i32 %u8, i32 %u9, i32 %u10, i32 %u11, i32 %u12, i32 %u13, i32 %u14, i32 %u15, i32 %u16, i32 %u17, i32 %u18, i32 %u19, i32 %u20, i32 %u21, i32 %u22, i32 %u23, i32 %u24, i32 %u25, i32 %u26, i32 %u27, i32 %u28, i32 %u29, i32 %u30, i32 %u31, i32 %u32, i32 %u33, i32 %u34, i32 %u35, i32 %u36, i32 %u37, i32 %u38, i32 %u39)
  ret void
}

; Room to spare: the load joins %l0 in the lead block.
;
; CHECK-LABEL: define spir_kernel void @headroom
; CHECK:       bb:
; CHECK-NEXT:    %iv = phi i32 [ 0, %entry ], [ %ivnext, %bb1 ]
; CHECK-NEXT:    %l0 = load i32, ptr addrspace(1) %b, align 4
; CHECK-NEXT:    %l1 = load i32, ptr addrspace(1) %b2, align 4
; CHECK-NEXT:    br label %bb1

; No room: the same load stays where it was.
;
; CHECK-LABEL: define spir_kernel void @saturated
; CHECK:       bb:
; CHECK-NEXT:    %iv = phi i32 [ 0, %entry ], [ %ivnext, %bb1 ]
; CHECK-NEXT:    %l0 = load i32, ptr addrspace(1) %b, align 4
; CHECK-NEXT:    br label %bb1
;
; CHECK:       bb1:
; CHECK-NEXT:    %l1 = load i32, ptr addrspace(1) %b2, align 4

; The ballast on its own leaves headroom, so this one hoists.
;
; CHECK-LABEL: define spir_kernel void @uniform_headroom
; CHECK:       bb:
; CHECK-NEXT:    %iv = phi i32 [ 0, %entry ], [ %ivnext, %bb1 ]
; CHECK-NEXT:    %l0 = load i32, ptr addrspace(1) %b, align 4
; CHECK-NEXT:    %l1 = load i32, ptr addrspace(1) %b2, align 4
; CHECK-NEXT:    br label %bb1

; The same ballast plus a crowd of uniform values does not.
;
; CHECK-LABEL: define spir_kernel void @uniform_crowd
; CHECK:       bb:
; CHECK-NEXT:    %iv = phi i32 [ 0, %entry ], [ %ivnext, %bb1 ]
; CHECK-NEXT:    %l0 = load i32, ptr addrspace(1) %b, align 4
; CHECK-NEXT:    br label %bb1
;
; CHECK:       bb1:
; CHECK-NEXT:    %l1 = load i32, ptr addrspace(1) %b2, align 4

declare i32 @llvm.genx.GenISA.DCL.SystemValue.i32(i32) #0

declare void @sink(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)

attributes #0 = { nounwind readnone }

!IGCMetadata = !{!0}
!igc.functions = !{!1, !14, !17, !18}

!0 = !{!"ModuleMD", !2}
!1 = !{ptr @headroom, !3}
!2 = !{!"FuncMD", !4, !5, !15, !16, !19, !20, !21, !22}
!3 = !{!6}
!4 = !{!"FuncMDMap[0]", ptr @headroom}
!5 = !{!"FuncMDValue[0]", !7, !8, !9, !10}
!6 = !{!"function_type", i32 0}
!7 = !{!"localOffsets"}
!8 = !{!"workGroupWalkOrder", !11, !12, !13}
!9 = !{!"funcArgs"}
!10 = !{!"functionType", !"KernelFunction"}
!11 = !{!"dim0", i32 0}
!12 = !{!"dim1", i32 1}
!13 = !{!"dim2", i32 2}
!14 = !{ptr @saturated, !3}
!15 = !{!"FuncMDMap[1]", ptr @saturated}
!16 = !{!"FuncMDValue[1]", !7, !8, !9, !10}
!17 = !{ptr @uniform_headroom, !3}
!18 = !{ptr @uniform_crowd, !3}
!19 = !{!"FuncMDMap[2]", ptr @uniform_headroom}
!20 = !{!"FuncMDValue[2]", !7, !8, !9, !10}
!21 = !{!"FuncMDMap[3]", ptr @uniform_crowd}
!22 = !{!"FuncMDValue[3]", !7, !8, !9, !10}
