;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys, llvm-14-plus
; RUN: igc_opt --regkey LoopSinkMinSave=1 --regkey ForceLoopSink=1 --regkey LoopSinkEnableVectorShuffle=1 --regkey CodeLoopSinkingMinSize=10 --regkey DumpLoopSink=1 --regkey LoopSinkDumpLevel=2 --regkey PrintToConsole=1 --basic-aa --igc-code-loop-sinking -S %s 2>&1 | FileCheck %s
define void @foo(<16 x i32> addrspace(1)* %in0, <8 x i32> addrspace(1)* noalias %out0, i32 %count, i32 %offsetIn0) {

; Recognizing the shuffle vector pattern can be useful to enable the block load sinking
; In this example the max regpressure increases, so the sinking is reverted

; So checking the dumps instead of the resulting instructions
; 32x "Sinking instruction" in dumps means EE and IEs are sinked

; CHECK: Starting sinking iteration...

; CHECK-COUNT-32: Sinking instruction

entry_preheader:
  %addr_1 = getelementptr <16 x i32>, <16 x i32> addrspace(1)* %in0, i32 %offsetIn0
  %l_1 = load <16 x i32>, <16 x i32> addrspace(1)* %addr_1, align 16

  %e_1 = extractelement <16 x i32> %l_1, i32 0
  %i_1 = insertelement <8 x i32> undef, i32 %e_1, i32 4
  %e_2 = extractelement <16 x i32> %l_1, i32 1
  %i_2 = insertelement <8 x i32> %i_1, i32 %e_2, i32 5
  %e_3 = extractelement <16 x i32> %l_1, i32 2
  %i_3 = insertelement <8 x i32> %i_2, i32 %e_3, i32 6
  %e_4 = extractelement <16 x i32> %l_1, i32 3
  %i_4 = insertelement <8 x i32> %i_3, i32 %e_4, i32 7
  %e_5 = extractelement <16 x i32> %l_1, i32 4
  %i_5 = insertelement <8 x i32> %i_4, i32 %e_5, i32 0
  %e_6 = extractelement <16 x i32> %l_1, i32 5
  %i_6 = insertelement <8 x i32> %i_5, i32 %e_6, i32 1
  %e_7 = extractelement <16 x i32> %l_1, i32 6
  %i_7 = insertelement <8 x i32> %i_6, i32 %e_7, i32 2
  %e_8 = extractelement <16 x i32> %l_1, i32 7
  %i_8 = insertelement <8 x i32> %i_7, i32 %e_8, i32 3

  %e_9 = extractelement <16 x i32> %l_1, i32 8
  %i_9 = insertelement <8 x i32> undef, i32 %e_9, i32 2
  %e_10 = extractelement <16 x i32> %l_1, i32 9
  %i_10 = insertelement <8 x i32> %i_9, i32 %e_10, i32 3
  %e_11 = extractelement <16 x i32> %l_1, i32 10
  %i_11 = insertelement <8 x i32> %i_10, i32 %e_11, i32 0
  %e_12 = extractelement <16 x i32> %l_1, i32 11
  %i_12 = insertelement <8 x i32> %i_11, i32 %e_12, i32 1
  %e_13 = extractelement <16 x i32> %l_1, i32 12
  %i_13 = insertelement <8 x i32> %i_12, i32 %e_13, i32 6
  %e_14 = extractelement <16 x i32> %l_1, i32 13
  %i_14 = insertelement <8 x i32> %i_13, i32 %e_14, i32 7
  %e_15 = extractelement <16 x i32> %l_1, i32 14
  %i_15 = insertelement <8 x i32> %i_14, i32 %e_15, i32 4
  %e_16 = extractelement <16 x i32> %l_1, i32 15
  %i_16 = insertelement <8 x i32> %i_15, i32 %e_16, i32 5

  br label %loop

loop:                                             ; preds = %loop, %entry_preheader
  %index = phi i32 [ 0, %entry_preheader ], [ %inc, %loop ]
  %addr_2 = getelementptr <8 x i32>, <8 x i32> addrspace(1)* %out0, i32 %index
  store <8 x i32> %i_8, <8 x i32> addrspace(1)* %addr_2, align 16
  %addr_3 = getelementptr <8 x i32>, <8 x i32> addrspace(1)* %out0, i32 %index
  store <8 x i32> %i_16, <8 x i32> addrspace(1)* %addr_3, align 16

  %cmptmp = icmp ult i32 %index, %count
  %inc = add i32 %index, 1
  br i1 %cmptmp, label %loop, label %afterloop

afterloop:                                        ; preds = %loop
  ret void
}

!igc.functions = !{}
