;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -platformpvc -igc-emit-visa %s -regkey DumpVISAASMToConsole | FileCheck %s

; A dynamic extractelement/insertelement index may be out of bounds (poison),
; but the indirect GRF access it lowers to must stay inside the vector.

; Non-power-of-2 element count: clamp with an unsigned min.
; CHECK-LABEL: .kernel "min_clamp"
; CHECK: min (M1_NM, 1) [[IDX:[A-Za-z0-9_]+]](0,0)<1> idx_0v(0,0)<0;1,0> 0x2:uw
; CHECK-NEXT: mul (M1_NM, 1) [[OFF:[A-Za-z0-9_]+]](0,0)<1> [[IDX]](0,0)<0;1,0> 0x4:uw
; CHECK-NEXT: addr_add (M1_NM, 1) {{A[0-9]+}}(0)<1> &v3 [[OFF]](0,0)<0;1,0>
define spir_kernel void @min_clamp(ptr addrspace(1) %buffer, i32 %idx) {
  %v3 = load <3 x i32>, ptr addrspace(1) %buffer, align 16
  %ext = extractelement <3 x i32> %v3, i32 %idx
  store i32 %ext, ptr addrspace(1) %buffer, align 4
  ret void
}

; Power-of-2 element count: mask.
; CHECK-LABEL: .kernel "and_mask"
; CHECK: and (M1_NM, 1) [[IDX:[A-Za-z0-9_]+]](0,0)<1> idx_0v(0,0)<0;1,0> 0x3:uw
; CHECK-NEXT: mul (M1_NM, 1) [[OFF:[A-Za-z0-9_]+]](0,0)<1> [[IDX]](0,0)<0;1,0> 0x4:uw
; CHECK-NEXT: addr_add (M1_NM, 1) {{A[0-9]+}}(0)<1> &ins [[OFF]](0,0)<0;1,0>
define spir_kernel void @and_mask(ptr addrspace(1) %buffer, i32 %idx, i32 %val) {
  %v4 = load <4 x i32>, ptr addrspace(1) %buffer, align 16
  %ins = insertelement <4 x i32> %v4, i32 %val, i32 %idx
  store <4 x i32> %ins, ptr addrspace(1) %buffer, align 16
  ret void
}

; Poison-generating flags prove nothing about the emitted arithmetic.
; CHECK-LABEL: .kernel "nuw_flag"
; CHECK: min (M1_NM, 1) [[IDX:[A-Za-z0-9_]+]](0,0)<1> {{[A-Za-z0-9_]+}}(0,0)<0;1,0> 0x2:uw
; CHECK-NEXT: mul (M1_NM, 1) {{[A-Za-z0-9_]+}}(0,0)<1> [[IDX]](0,0)<0;1,0> 0x4:uw
define spir_kernel void @nuw_flag(ptr addrspace(1) %buffer, i32 %idx) {
  %v3 = load <3 x i32>, ptr addrspace(1) %buffer, align 16
  %nuw.idx = sub nuw i32 2, %idx
  %ext = extractelement <3 x i32> %v3, i32 %nuw.idx
  store i32 %ext, ptr addrspace(1) %buffer, align 4
  ret void
}

; A narrow index is compared as unsigned, so a negative value is clamped too.
; CHECK-LABEL: .kernel "narrow_index"
; CHECK: .decl [[UW:[A-Za-z0-9_]+]] v_type=G type=uw num_elts=1 align=word alias=<narrow_idx, 0>
; CHECK: min (M1_NM, 1) [[IDX:[A-Za-z0-9_]+]](0,0)<1> [[UW]](0,0)<0;1,0> 0x2:uw
; CHECK-NEXT: mul (M1_NM, 1) {{[A-Za-z0-9_]+}}(0,0)<1> [[IDX]](0,0)<0;1,0> 0x4:uw
define spir_kernel void @narrow_index(ptr addrspace(1) %buffer, i32 %idx) {
  %v3 = load <3 x i32>, ptr addrspace(1) %buffer, align 16
  %narrow.idx = trunc i32 %idx to i16
  %ext = extractelement <3 x i32> %v3, i16 %narrow.idx
  store i32 %ext, ptr addrspace(1) %buffer, align 4
  ret void
}

; An index guarded by a dominating bounds check is used unclamped.
; CHECK-LABEL: .kernel "guarded"
; CHECK: _guarded_001_in_bounds:
; CHECK-NEXT: mul (M1_NM, 1) {{[A-Za-z0-9_]+}}(0,0)<1> {{idx_[0-9]+v}}(0,0)<0;1,0> 0x4:uw
define spir_kernel void @guarded(ptr addrspace(1) %buffer, i32 %idx) {
entry:
  %v3 = load <3 x i32>, ptr addrspace(1) %buffer, align 16
  %cmp = icmp ult i32 %idx, 3
  br i1 %cmp, label %in.bounds, label %exit

in.bounds:
  %ext = extractelement <3 x i32> %v3, i32 %idx
  store i32 %ext, ptr addrspace(1) %buffer, align 4
  br label %exit

exit:
  ret void
}

; An index bounded only by its known bits, not by a constant range, is used unclamped.
; CHECK-LABEL: .kernel "known_bits"
; CHECK-NOT: and (M1_NM, 1)
; CHECK: addr_add (M1_NM, 1) {{A[0-9]+}}(0)<1> &v8
define spir_kernel void @known_bits(ptr addrspace(1) %buffer, i32 %dim) {
  %v8 = load <8 x i32>, ptr addrspace(1) %buffer, align 32
  %cond = icmp eq i32 %dim, 0
  %sel = select i1 %cond, i8 1, i8 5
  %zext = zext i8 %sel to i32
  %idx = add i32 2, %zext
  %ext = extractelement <8 x i32> %v8, i32 %idx
  store i32 %ext, ptr addrspace(1) %buffer, align 4
  ret void
}

!igc.functions = !{!1, !2, !3, !4, !5, !6}

!1 = !{ptr @min_clamp, !10}
!2 = !{ptr @and_mask, !10}
!3 = !{ptr @nuw_flag, !10}
!4 = !{ptr @narrow_index, !10}
!5 = !{ptr @guarded, !10}
!6 = !{ptr @known_bits, !10}
!10 = !{!11}
!11 = !{!"function_type", i32 0}

!IGCMetadata = !{!20}
!20 = !{!"ModuleMD", !21}
!21 = !{!"FuncMD", !30, !31, !32, !33, !34, !35, !36, !37, !38, !39, !51, !52}
!30 = !{!"FuncMDMap[0]", ptr @min_clamp}
!31 = !{!"FuncMDValue[0]", !40, !41}
!32 = !{!"FuncMDMap[1]", ptr @and_mask}
!33 = !{!"FuncMDValue[1]", !40, !41}
!34 = !{!"FuncMDMap[2]", ptr @nuw_flag}
!35 = !{!"FuncMDValue[2]", !40, !41}
!36 = !{!"FuncMDMap[3]", ptr @narrow_index}
!37 = !{!"FuncMDValue[3]", !40, !41}
!38 = !{!"FuncMDMap[4]", ptr @guarded}
!39 = !{!"FuncMDValue[4]", !40, !41}
!51 = !{!"FuncMDMap[5]", ptr @known_bits}
!52 = !{!"FuncMDValue[5]", !40, !41}
!40 = !{!"functionType", !"KernelFunction"}
!41 = !{!"resAllocMD", !42, !43, !44, !45, !47}
!42 = !{!"uavsNumType", i32 1}
!43 = !{!"srvsNumType", i32 0}
!44 = !{!"samplersNumType", i32 0}
!45 = !{!"argAllocMDList", !46}
!46 = !{!"argAllocMDListVec[0]", !48, !49, !50}
!47 = !{!"inlineSamplersMD"}
!48 = !{!"type", i32 1}
!49 = !{!"extensionType", i32 -1}
!50 = !{!"indexType", i32 0}
