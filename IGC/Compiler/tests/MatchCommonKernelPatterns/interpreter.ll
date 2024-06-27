;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2024 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;

; RUN: igc_opt --match-common-kernel-patterns --print-codegencontext --platformdg2 < %s 2>&1 | FileCheck %s --check-prefix=CHECK-V1
; CHECK-V1: Kernel with forced retry: kernel_interpreter_v1

; RUN: igc_opt --match-common-kernel-patterns --print-codegencontext --platformdg2 < %s 2>&1 | FileCheck %s --check-prefix=CHECK-V2
; CHECK-V2: Kernel with forced retry: kernel_interpreter_v2

define spir_kernel void @kernel_interpreter_v1(i32 addrspace(4)* align 4 %addr1, i32 %op1, <8 x i32> %r0, <8 x i32> %payloadHeader) #0 {
Entry:
  %0 = addrspacecast i32 addrspace(4)* %addr1 to i32 addrspace(1)*
  %load1 = load i32, i32 addrspace(1)* %0, align 4
  %ind1 = add nsw i32 %load1, %op1
  br label %BB1

BB1:                                              ; preds = %Backedge, %Entry
  %phi1 = phi i32 [ %ind1, %Entry ], [ %phi2, %Backedge ]
  %1 = ashr i32 %phi1, 31
  %2 = call i32 addrspace(4)* @llvm.genx.GenISA.pair.to.ptr.p4i32(i32 %phi1, i32 %1)
  %3 = addrspacecast i32 addrspace(4)* %2 to <4 x i32> addrspace(1)*
  %load2 = load <4 x i32>, <4 x i32> addrspace(1)* %3, align 4
  %extrel = extractelement <4 x i32> %load2, i32 0
  %Pivot120 = icmp slt i32 %extrel, 32
  br i1 %Pivot120, label %NodeBlock55, label %NodeBlock117

NodeBlock117:                                     ; preds = %BB1
  %Pivot118 = icmp ult i32 %extrel, 47
  br i1 %Pivot118, label %NodeBlock83, label %NodeBlock115

NodeBlock115:                                     ; preds = %NodeBlock117
  %Pivot116 = icmp ult i32 %extrel, 54
  br i1 %Pivot116, label %NodeBlock95, label %NodeBlock113

NodeBlock113:                                     ; preds = %NodeBlock115
  %Pivot114 = icmp ult i32 %extrel, 58
  br i1 %Pivot114, label %NodeBlock101, label %NodeBlock111

NodeBlock111:                                     ; preds = %NodeBlock113
  %Pivot112 = icmp ult i32 %extrel, 60
  br i1 %Pivot112, label %NodeBlock103, label %NodeBlock109

NodeBlock109:                                     ; preds = %NodeBlock111
  %Pivot110 = icmp ult i32 %extrel, 98
  br i1 %Pivot110, label %LeafBlock105, label %LeafBlock107

LeafBlock107:                                     ; preds = %NodeBlock109
  %SwitchLeaf108 = icmp eq i32 %extrel, 98
  br i1 %SwitchLeaf108, label %Case1, label %LeafBlock107.Default_crit_edge

LeafBlock107.Default_crit_edge:                   ; preds = %LeafBlock107
  br label %Default

LeafBlock105:                                     ; preds = %NodeBlock109
  %SwitchLeaf106 = icmp eq i32 %extrel, 60
  br i1 %SwitchLeaf106, label %Case60, label %LeafBlock105.Default_crit_edge

LeafBlock105.Default_crit_edge:                   ; preds = %LeafBlock105
  br label %Default

NodeBlock103:                                     ; preds = %NodeBlock111
  %Pivot104.not = icmp eq i32 %extrel, 59
  br i1 %Pivot104.not, label %Case59, label %Case58

NodeBlock101:                                     ; preds = %NodeBlock113
  %Pivot102 = icmp ult i32 %extrel, 56
  br i1 %Pivot102, label %NodeBlock97, label %NodeBlock99

NodeBlock99:                                      ; preds = %NodeBlock101
  %Pivot100 = icmp eq i32 %extrel, 56
  br i1 %Pivot100, label %Case56, label %Case57

NodeBlock97:                                      ; preds = %NodeBlock101
  %Pivot98.not = icmp eq i32 %extrel, 55
  br i1 %Pivot98.not, label %Case55, label %Case54

NodeBlock95:                                      ; preds = %NodeBlock115
  %Pivot96 = icmp ult i32 %extrel, 50
  br i1 %Pivot96, label %NodeBlock87, label %NodeBlock93

NodeBlock93:                                      ; preds = %NodeBlock95
  %Pivot94 = icmp ult i32 %extrel, 52
  br i1 %Pivot94, label %NodeBlock89, label %NodeBlock91

NodeBlock91:                                      ; preds = %NodeBlock93
  %Pivot92 = icmp eq i32 %extrel, 52
  br i1 %Pivot92, label %Case52, label %Case53

NodeBlock89:                                      ; preds = %NodeBlock93
  %Pivot90.not = icmp eq i32 %extrel, 51
  br i1 %Pivot90.not, label %Case51, label %Case50

NodeBlock87:                                      ; preds = %NodeBlock95
  %Pivot88 = icmp ult i32 %extrel, 48
  br i1 %Pivot88, label %Case47, label %NodeBlock85

NodeBlock85:                                      ; preds = %NodeBlock87
  %Pivot86 = icmp eq i32 %extrel, 48
  br i1 %Pivot86, label %Case48, label %Case49

NodeBlock83:                                      ; preds = %NodeBlock117
  %Pivot84 = icmp ult i32 %extrel, 39
  br i1 %Pivot84, label %NodeBlock67, label %NodeBlock81

NodeBlock81:                                      ; preds = %NodeBlock83
  %Pivot82 = icmp ult i32 %extrel, 43
  br i1 %Pivot82, label %NodeBlock73, label %NodeBlock79

NodeBlock79:                                      ; preds = %NodeBlock81
  %Pivot80 = icmp ult i32 %extrel, 45
  br i1 %Pivot80, label %NodeBlock75, label %NodeBlock77

NodeBlock77:                                      ; preds = %NodeBlock79
  %Pivot78 = icmp eq i32 %extrel, 45
  br i1 %Pivot78, label %Case45, label %Case46

NodeBlock75:                                      ; preds = %NodeBlock79
  %Pivot76.not = icmp eq i32 %extrel, 44
  br i1 %Pivot76.not, label %Case44, label %Case43

NodeBlock73:                                      ; preds = %NodeBlock81
  %Pivot74 = icmp ult i32 %extrel, 41
  br i1 %Pivot74, label %NodeBlock69, label %NodeBlock71

NodeBlock71:                                      ; preds = %NodeBlock73
  %Pivot72 = icmp eq i32 %extrel, 41
  br i1 %Pivot72, label %Case41, label %Case42

NodeBlock69:                                      ; preds = %NodeBlock73
  %Pivot70.not = icmp eq i32 %extrel, 40
  br i1 %Pivot70.not, label %Case40, label %Case39

NodeBlock67:                                      ; preds = %NodeBlock83
  %Pivot68 = icmp ult i32 %extrel, 35
  br i1 %Pivot68, label %NodeBlock59, label %NodeBlock65

NodeBlock65:                                      ; preds = %NodeBlock67
  %Pivot66 = icmp ult i32 %extrel, 37
  br i1 %Pivot66, label %NodeBlock61, label %NodeBlock63

NodeBlock63:                                      ; preds = %NodeBlock65
  %Pivot64 = icmp eq i32 %extrel, 37
  br i1 %Pivot64, label %Case37, label %Case38

NodeBlock61:                                      ; preds = %NodeBlock65
  %Pivot62.not = icmp eq i32 %extrel, 36
  br i1 %Pivot62.not, label %Case36, label %Case35

NodeBlock59:                                      ; preds = %NodeBlock67
  %Pivot60 = icmp ult i32 %extrel, 33
  br i1 %Pivot60, label %Case32, label %NodeBlock57

NodeBlock57:                                      ; preds = %NodeBlock59
  %Pivot58 = icmp eq i32 %extrel, 33
  br i1 %Pivot58, label %Case33, label %Case34

NodeBlock55:                                      ; preds = %BB1
  %Pivot56 = icmp slt i32 %extrel, 17
  br i1 %Pivot56, label %NodeBlock25, label %NodeBlock53

NodeBlock53:                                      ; preds = %NodeBlock55
  %Pivot54 = icmp ult i32 %extrel, 24
  br i1 %Pivot54, label %NodeBlock37, label %NodeBlock51

NodeBlock51:                                      ; preds = %NodeBlock53
  %Pivot52 = icmp ult i32 %extrel, 28
  br i1 %Pivot52, label %NodeBlock43, label %NodeBlock49

NodeBlock49:                                      ; preds = %NodeBlock51
  %Pivot50 = icmp ult i32 %extrel, 30
  br i1 %Pivot50, label %NodeBlock45, label %NodeBlock47

NodeBlock47:                                      ; preds = %NodeBlock49
  %Pivot48 = icmp eq i32 %extrel, 30
  br i1 %Pivot48, label %Case30, label %Case31

NodeBlock45:                                      ; preds = %NodeBlock49
  %Pivot46.not = icmp eq i32 %extrel, 29
  br i1 %Pivot46.not, label %Case29, label %Case28

NodeBlock43:                                      ; preds = %NodeBlock51
  %Pivot44 = icmp ult i32 %extrel, 26
  br i1 %Pivot44, label %NodeBlock39, label %NodeBlock41

NodeBlock41:                                      ; preds = %NodeBlock43
  %Pivot42 = icmp eq i32 %extrel, 26
  br i1 %Pivot42, label %Case26, label %Case27

NodeBlock39:                                      ; preds = %NodeBlock43
  %Pivot40.not = icmp eq i32 %extrel, 25
  br i1 %Pivot40.not, label %Case25, label %Case24

NodeBlock37:                                      ; preds = %NodeBlock53
  %Pivot38 = icmp ult i32 %extrel, 20
  br i1 %Pivot38, label %NodeBlock29, label %NodeBlock35

NodeBlock35:                                      ; preds = %NodeBlock37
  %Pivot36 = icmp ult i32 %extrel, 22
  br i1 %Pivot36, label %NodeBlock31, label %NodeBlock33

NodeBlock33:                                      ; preds = %NodeBlock35
  %Pivot34 = icmp eq i32 %extrel, 22
  br i1 %Pivot34, label %Case22, label %Case23

NodeBlock31:                                      ; preds = %NodeBlock35
  %Pivot32.not = icmp eq i32 %extrel, 21
  br i1 %Pivot32.not, label %Case21, label %Case20

NodeBlock29:                                      ; preds = %NodeBlock37
  %Pivot30 = icmp ult i32 %extrel, 18
  br i1 %Pivot30, label %Case17, label %NodeBlock27

NodeBlock27:                                      ; preds = %NodeBlock29
  %Pivot28 = icmp eq i32 %extrel, 18
  br i1 %Pivot28, label %Case18, label %Case19

NodeBlock25:                                      ; preds = %NodeBlock55
  %Pivot26 = icmp slt i32 %extrel, 9
  br i1 %Pivot26, label %NodeBlock9, label %NodeBlock23

NodeBlock23:                                      ; preds = %NodeBlock25
  %Pivot24 = icmp ult i32 %extrel, 13
  br i1 %Pivot24, label %NodeBlock15, label %NodeBlock21

NodeBlock21:                                      ; preds = %NodeBlock23
  %Pivot22 = icmp ult i32 %extrel, 15
  br i1 %Pivot22, label %NodeBlock17, label %NodeBlock19

NodeBlock19:                                      ; preds = %NodeBlock21
  %Pivot20 = icmp eq i32 %extrel, 15
  br i1 %Pivot20, label %Case15, label %Case16

NodeBlock17:                                      ; preds = %NodeBlock21
  %Pivot18.not = icmp eq i32 %extrel, 14
  br i1 %Pivot18.not, label %Case14, label %Case13

NodeBlock15:                                      ; preds = %NodeBlock23
  %Pivot16 = icmp ult i32 %extrel, 11
  br i1 %Pivot16, label %NodeBlock11, label %NodeBlock13

NodeBlock13:                                      ; preds = %NodeBlock15
  %Pivot14 = icmp eq i32 %extrel, 11
  br i1 %Pivot14, label %Case11, label %Case12

NodeBlock11:                                      ; preds = %NodeBlock15
  %Pivot12.not = icmp eq i32 %extrel, 10
  br i1 %Pivot12.not, label %Case10, label %Case9

NodeBlock9:                                       ; preds = %NodeBlock25
  %Pivot10 = icmp slt i32 %extrel, 5
  br i1 %Pivot10, label %NodeBlock1, label %NodeBlock7

NodeBlock7:                                       ; preds = %NodeBlock9
  %Pivot8 = icmp ult i32 %extrel, 7
  br i1 %Pivot8, label %NodeBlock3, label %NodeBlock5

NodeBlock5:                                       ; preds = %NodeBlock7
  %Pivot6 = icmp eq i32 %extrel, 7
  br i1 %Pivot6, label %Case7, label %Case8

NodeBlock3:                                       ; preds = %NodeBlock7
  %Pivot4.not = icmp eq i32 %extrel, 6
  br i1 %Pivot4.not, label %Case6, label %Case5

NodeBlock1:                                       ; preds = %NodeBlock9
  %Pivot2 = icmp slt i32 %extrel, 3
  br i1 %Pivot2, label %LeafBlock, label %NodeBlock

NodeBlock:                                        ; preds = %NodeBlock1
  %Pivot = icmp eq i32 %extrel, 3
  br i1 %Pivot, label %Case3, label %Case4

LeafBlock:                                        ; preds = %NodeBlock1
  %SwitchLeaf = icmp eq i32 %extrel, 1
  br i1 %SwitchLeaf, label %Case2, label %LeafBlock.Default_crit_edge

LeafBlock.Default_crit_edge:                      ; preds = %LeafBlock
  br label %Default

Case1:                                            ; preds = %LeafBlock107
  %indCase1 = add i32 %phi1, 2
  br label %Backedge

Case2:                                            ; preds = %LeafBlock
  %indCase2 = add i32 %phi1, 3
  br label %Backedge

Case3:                                            ; preds = %NodeBlock
  %indCase3 = add i32 %phi1, 4
  br label %Backedge

Case4:                                            ; preds = %NodeBlock
  %indCase4 = add i32 %phi1, 5
  br label %Backedge

Case5:                                            ; preds = %NodeBlock3
  %indCase5 = add i32 %phi1, 6
  br label %Backedge

Case6:                                            ; preds = %NodeBlock3
  %indCase6 = add i32 %phi1, 7
  br label %Backedge

Case7:                                            ; preds = %NodeBlock5
  %indCase7 = add i32 %phi1, 8
  br label %Backedge

Case8:                                            ; preds = %NodeBlock5
  %indCase8 = add i32 %phi1, 9
  br label %Backedge

Case9:                                            ; preds = %NodeBlock11
  %indCase9 = add i32 %phi1, 10
  br label %Backedge

Case10:                                           ; preds = %NodeBlock11
  %indCase10 = add i32 %phi1, 11
  br label %Backedge

Case11:                                           ; preds = %NodeBlock13
  %indCase11 = add i32 %phi1, 12
  br label %Backedge

Case12:                                           ; preds = %NodeBlock13
  %indCase12 = add i32 %phi1, 13
  br label %Backedge

Case13:                                           ; preds = %NodeBlock17
  %indCase13 = add i32 %phi1, 14
  br label %Backedge

Case14:                                           ; preds = %NodeBlock17
  %indCase14 = add i32 %phi1, 15
  br label %Backedge

Case15:                                           ; preds = %NodeBlock19
  %indCase15 = add i32 %phi1, 16
  br label %Backedge

Case16:                                           ; preds = %NodeBlock19
  %indCase16 = add i32 %phi1, 17
  br label %Backedge

Case17:                                           ; preds = %NodeBlock29
  %indCase17 = add i32 %phi1, 18
  br label %Backedge

Case18:                                           ; preds = %NodeBlock27
  %indCase18 = add i32 %phi1, 19
  br label %Backedge

Case19:                                           ; preds = %NodeBlock27
  %indCase19 = add i32 %phi1, 20
  br label %Backedge

Case20:                                           ; preds = %NodeBlock31
  %indCase20 = add i32 %phi1, 21
  br label %Backedge

Case21:                                           ; preds = %NodeBlock31
  %indCase21 = add i32 %phi1, 22
  br label %Backedge

Case22:                                           ; preds = %NodeBlock33
  %indCase22 = add i32 %phi1, 23
  br label %Backedge

Case23:                                           ; preds = %NodeBlock33
  %indCase23 = add i32 %phi1, 24
  br label %Backedge

Case24:                                           ; preds = %NodeBlock39
  %indCase24 = add i32 %phi1, 25
  br label %Backedge

Case25:                                           ; preds = %NodeBlock39
  %indCase25 = add i32 %phi1, 26
  br label %Backedge

Case26:                                           ; preds = %NodeBlock41
  %indCase26 = add i32 %phi1, 27
  br label %Backedge

Case27:                                           ; preds = %NodeBlock41
  %indCase27 = add i32 %phi1, 28
  br label %Backedge

Case28:                                           ; preds = %NodeBlock45
  %indCase28 = add i32 %phi1, 29
  br label %Backedge

Case29:                                           ; preds = %NodeBlock45
  %indCase29 = add i32 %phi1, 30
  br label %Backedge

Case30:                                           ; preds = %NodeBlock47
  %indCase30 = add i32 %phi1, 31
  br label %Backedge

Case31:                                           ; preds = %NodeBlock47
  %indCase31 = add i32 %phi1, 32
  br label %Backedge

Case32:                                           ; preds = %NodeBlock59
  %indCase32 = add i32 %phi1, 33
  br label %Backedge

Case33:                                           ; preds = %NodeBlock57
  %indCase33 = add i32 %phi1, 34
  br label %Backedge

Case34:                                           ; preds = %NodeBlock57
  %indCase34 = add i32 %phi1, 35
  br label %Backedge

Case35:                                           ; preds = %NodeBlock61
  %indCase35 = add i32 %phi1, 36
  br label %Backedge

Case36:                                           ; preds = %NodeBlock61
  %indCase36 = add i32 %phi1, 37
  br label %Backedge

Case37:                                           ; preds = %NodeBlock63
  %indCase37 = add i32 %phi1, 38
  br label %Backedge

Case38:                                           ; preds = %NodeBlock63
  %indCase38 = add i32 %phi1, 39
  br label %Backedge

Case39:                                           ; preds = %NodeBlock69
  %indCase39 = add i32 %phi1, 40
  br label %Backedge

Case40:                                           ; preds = %NodeBlock69
  %indCase40 = add i32 %phi1, 41
  br label %Backedge

Case41:                                           ; preds = %NodeBlock71
  %indCase41 = add i32 %phi1, 42
  br label %Backedge

Case42:                                           ; preds = %NodeBlock71
  %indCase42 = add i32 %phi1, 43
  br label %Backedge

Case43:                                           ; preds = %NodeBlock75
  %indCase43 = add i32 %phi1, 44
  br label %Backedge

Case44:                                           ; preds = %NodeBlock75
  %indCase44 = add i32 %phi1, 45
  br label %Backedge

Case45:                                           ; preds = %NodeBlock77
  %indCase45 = add i32 %phi1, 46
  br label %Backedge

Case46:                                           ; preds = %NodeBlock77
  %indCase46 = add i32 %phi1, 47
  br label %Backedge

Case47:                                           ; preds = %NodeBlock87
  %indCase47 = add i32 %phi1, 48
  br label %Backedge

Case48:                                           ; preds = %NodeBlock85
  %indCase48 = add i32 %phi1, 49
  br label %Backedge

Case49:                                           ; preds = %NodeBlock85
  %indCase49 = add i32 %phi1, 50
  br label %Backedge

Case50:                                           ; preds = %NodeBlock89
  %indCase50 = add i32 %phi1, 51
  br label %Backedge

Case51:                                           ; preds = %NodeBlock89
  %indCase51 = add i32 %phi1, 52
  br label %Backedge

Case52:                                           ; preds = %NodeBlock91
  %indCase52 = add i32 %phi1, 53
  br label %Backedge

Case53:                                           ; preds = %NodeBlock91
  %indCase53 = add i32 %phi1, 54
  br label %Backedge

Case54:                                           ; preds = %NodeBlock97
  %indCase54 = add i32 %phi1, 55
  br label %Backedge

Case55:                                           ; preds = %NodeBlock97
  %indCase55 = add i32 %phi1, 56
  br label %Backedge

Case56:                                           ; preds = %NodeBlock99
  %indCase56 = add i32 %phi1, 57
  br label %Backedge

Case57:                                           ; preds = %NodeBlock99
  %indCase57 = add i32 %phi1, 58
  br label %Backedge

Case58:                                           ; preds = %NodeBlock103
  %indCase58 = add i32 %phi1, 59
  br label %Backedge

Case59:                                           ; preds = %NodeBlock103
  %indCase59 = add i32 %phi1, 60
  br label %Backedge

Case60:                                           ; preds = %LeafBlock105
  %indCase60 = add i32 %phi1, 61
  br label %Backedge

Backedge:                                         ; preds = %Case60, %Case59, %Case58, %Case57, %Case56, %Case55, %Case54, %Case53, %Case52, %Case51, %Case50, %Case49, %Case48, %Case47, %Case46, %Case45, %Case44, %Case43, %Case42, %Case41, %Case40, %Case39, %Case38, %Case37, %Case36, %Case35, %Case34, %Case33, %Case32, %Case31, %Case30, %Case29, %Case28, %Case27, %Case26, %Case25, %Case24, %Case23, %Case22, %Case21, %Case20, %Case19, %Case18, %Case17, %Case16, %Case15, %Case14, %Case13, %Case12, %Case11, %Case10, %Case9, %Case8, %Case7, %Case6, %Case5, %Case4, %Case3, %Case2, %Case1
  %phi2 = phi i32 [ %indCase1, %Case1 ], [ %indCase2, %Case2 ], [ %indCase3, %Case3 ], [ %indCase4, %Case4 ], [ %indCase5, %Case5 ], [ %indCase6, %Case6 ], [ %indCase7, %Case7 ], [ %indCase8, %Case8 ], [ %indCase9, %Case9 ], [ %indCase10, %Case10 ], [ %indCase11, %Case11 ], [ %indCase12, %Case12 ], [ %indCase13, %Case13 ], [ %indCase14, %Case14 ], [ %indCase15, %Case15 ], [ %indCase16, %Case16 ], [ %indCase17, %Case17 ], [ %indCase18, %Case18 ], [ %indCase19, %Case19 ], [ %indCase20, %Case20 ], [ %indCase21, %Case21 ], [ %indCase22, %Case22 ], [ %indCase23, %Case23 ], [ %indCase24, %Case24 ], [ %indCase25, %Case25 ], [ %indCase26, %Case26 ], [ %indCase27, %Case27 ], [ %indCase28, %Case28 ], [ %indCase29, %Case29 ], [ %indCase30, %Case30 ], [ %indCase31, %Case31 ], [ %indCase32, %Case32 ], [ %indCase33, %Case33 ], [ %indCase34, %Case34 ], [ %indCase35, %Case35 ], [ %indCase36, %Case36 ], [ %indCase37, %Case37 ], [ %indCase38, %Case38 ], [ %indCase39, %Case39 ], [ %indCase40, %Case40 ], [ %indCase41, %Case41 ], [ %indCase42, %Case42 ], [ %indCase43, %Case43 ], [ %indCase44, %Case44 ], [ %indCase45, %Case45 ], [ %indCase46, %Case46 ], [ %indCase47, %Case47 ], [ %indCase48, %Case48 ], [ %indCase49, %Case49 ], [ %indCase50, %Case50 ], [ %indCase51, %Case51 ], [ %indCase52, %Case52 ], [ %indCase53, %Case53 ], [ %indCase54, %Case54 ], [ %indCase55, %Case55 ], [ %indCase56, %Case56 ], [ %indCase57, %Case57 ], [ %indCase58, %Case58 ], [ %indCase59, %Case59 ], [ %indCase60, %Case60 ]
  br label %BB1

Default:                                          ; preds = %LeafBlock.Default_crit_edge, %LeafBlock105.Default_crit_edge, %LeafBlock107.Default_crit_edge
  ret void
}

define spir_kernel void @kernel_interpreter_v2(i32 addrspace(4)* align 4 %addr1, i32 %op1, <8 x i32> %r0, <8 x i32> %payloadHeader) #0 {
Entry:
  %0 = addrspacecast i32 addrspace(4)* %addr1 to i32 addrspace(1)*
  %load1 = load i32, i32 addrspace(1)* %0, align 4
  %ind1 = add nsw i32 %load1, %op1
  br label %BB1

BB1:                                              ; preds = %Backedge, %Entry
  %phi1 = phi i32 [ %ind1, %Entry ], [ %phi2, %Backedge ]
  %1 = ashr i32 %phi1, 31
  %2 = call i32 addrspace(4)* @llvm.genx.GenISA.pair.to.ptr.p4i32(i32 %phi1, i32 %1)
  %3 = addrspacecast i32 addrspace(4)* %2 to i32 addrspace(1)*
  %load2 = load i32, i32 addrspace(1)* %3, align 4
  %Pivot120 = icmp slt i32 %load2, 32
  br i1 %Pivot120, label %NodeBlock55, label %NodeBlock117

NodeBlock117:                                     ; preds = %BB1
  %Pivot118 = icmp ult i32 %load2, 47
  br i1 %Pivot118, label %NodeBlock83, label %NodeBlock115

NodeBlock115:                                     ; preds = %NodeBlock117
  %Pivot116 = icmp ult i32 %load2, 54
  br i1 %Pivot116, label %NodeBlock95, label %NodeBlock113

NodeBlock113:                                     ; preds = %NodeBlock115
  %Pivot114 = icmp ult i32 %load2, 58
  br i1 %Pivot114, label %NodeBlock101, label %NodeBlock111

NodeBlock111:                                     ; preds = %NodeBlock113
  %Pivot112 = icmp ult i32 %load2, 60
  br i1 %Pivot112, label %NodeBlock103, label %NodeBlock109

NodeBlock109:                                     ; preds = %NodeBlock111
  %Pivot110 = icmp ult i32 %load2, 98
  br i1 %Pivot110, label %LeafBlock105, label %LeafBlock107

LeafBlock107:                                     ; preds = %NodeBlock109
  %SwitchLeaf108 = icmp eq i32 %load2, 98
  br i1 %SwitchLeaf108, label %Case1, label %LeafBlock107.Default_crit_edge

LeafBlock107.Default_crit_edge:                   ; preds = %LeafBlock107
  br label %Default

LeafBlock105:                                     ; preds = %NodeBlock109
  %SwitchLeaf106 = icmp eq i32 %load2, 60
  br i1 %SwitchLeaf106, label %Case60, label %LeafBlock105.Default_crit_edge

LeafBlock105.Default_crit_edge:                   ; preds = %LeafBlock105
  br label %Default

NodeBlock103:                                     ; preds = %NodeBlock111
  %Pivot104.not = icmp eq i32 %load2, 59
  br i1 %Pivot104.not, label %Case59, label %Case58

NodeBlock101:                                     ; preds = %NodeBlock113
  %Pivot102 = icmp ult i32 %load2, 56
  br i1 %Pivot102, label %NodeBlock97, label %NodeBlock99

NodeBlock99:                                      ; preds = %NodeBlock101
  %Pivot100 = icmp eq i32 %load2, 56
  br i1 %Pivot100, label %Case56, label %Case57

NodeBlock97:                                      ; preds = %NodeBlock101
  %Pivot98.not = icmp eq i32 %load2, 55
  br i1 %Pivot98.not, label %Case55, label %Case54

NodeBlock95:                                      ; preds = %NodeBlock115
  %Pivot96 = icmp ult i32 %load2, 50
  br i1 %Pivot96, label %NodeBlock87, label %NodeBlock93

NodeBlock93:                                      ; preds = %NodeBlock95
  %Pivot94 = icmp ult i32 %load2, 52
  br i1 %Pivot94, label %NodeBlock89, label %NodeBlock91

NodeBlock91:                                      ; preds = %NodeBlock93
  %Pivot92 = icmp eq i32 %load2, 52
  br i1 %Pivot92, label %Case52, label %Case53

NodeBlock89:                                      ; preds = %NodeBlock93
  %Pivot90.not = icmp eq i32 %load2, 51
  br i1 %Pivot90.not, label %Case51, label %Case50

NodeBlock87:                                      ; preds = %NodeBlock95
  %Pivot88 = icmp ult i32 %load2, 48
  br i1 %Pivot88, label %Case47, label %NodeBlock85

NodeBlock85:                                      ; preds = %NodeBlock87
  %Pivot86 = icmp eq i32 %load2, 48
  br i1 %Pivot86, label %Case48, label %Case49

NodeBlock83:                                      ; preds = %NodeBlock117
  %Pivot84 = icmp ult i32 %load2, 39
  br i1 %Pivot84, label %NodeBlock67, label %NodeBlock81

NodeBlock81:                                      ; preds = %NodeBlock83
  %Pivot82 = icmp ult i32 %load2, 43
  br i1 %Pivot82, label %NodeBlock73, label %NodeBlock79

NodeBlock79:                                      ; preds = %NodeBlock81
  %Pivot80 = icmp ult i32 %load2, 45
  br i1 %Pivot80, label %NodeBlock75, label %NodeBlock77

NodeBlock77:                                      ; preds = %NodeBlock79
  %Pivot78 = icmp eq i32 %load2, 45
  br i1 %Pivot78, label %Case45, label %Case46

NodeBlock75:                                      ; preds = %NodeBlock79
  %Pivot76.not = icmp eq i32 %load2, 44
  br i1 %Pivot76.not, label %Case44, label %Case43

NodeBlock73:                                      ; preds = %NodeBlock81
  %Pivot74 = icmp ult i32 %load2, 41
  br i1 %Pivot74, label %NodeBlock69, label %NodeBlock71

NodeBlock71:                                      ; preds = %NodeBlock73
  %Pivot72 = icmp eq i32 %load2, 41
  br i1 %Pivot72, label %Case41, label %Case42

NodeBlock69:                                      ; preds = %NodeBlock73
  %Pivot70.not = icmp eq i32 %load2, 40
  br i1 %Pivot70.not, label %Case40, label %Case39

NodeBlock67:                                      ; preds = %NodeBlock83
  %Pivot68 = icmp ult i32 %load2, 35
  br i1 %Pivot68, label %NodeBlock59, label %NodeBlock65

NodeBlock65:                                      ; preds = %NodeBlock67
  %Pivot66 = icmp ult i32 %load2, 37
  br i1 %Pivot66, label %NodeBlock61, label %NodeBlock63

NodeBlock63:                                      ; preds = %NodeBlock65
  %Pivot64 = icmp eq i32 %load2, 37
  br i1 %Pivot64, label %Case37, label %Case38

NodeBlock61:                                      ; preds = %NodeBlock65
  %Pivot62.not = icmp eq i32 %load2, 36
  br i1 %Pivot62.not, label %Case36, label %Case35

NodeBlock59:                                      ; preds = %NodeBlock67
  %Pivot60 = icmp ult i32 %load2, 33
  br i1 %Pivot60, label %Case32, label %NodeBlock57

NodeBlock57:                                      ; preds = %NodeBlock59
  %Pivot58 = icmp eq i32 %load2, 33
  br i1 %Pivot58, label %Case33, label %Case34

NodeBlock55:                                      ; preds = %BB1
  %Pivot56 = icmp slt i32 %load2, 17
  br i1 %Pivot56, label %NodeBlock25, label %NodeBlock53

NodeBlock53:                                      ; preds = %NodeBlock55
  %Pivot54 = icmp ult i32 %load2, 24
  br i1 %Pivot54, label %NodeBlock37, label %NodeBlock51

NodeBlock51:                                      ; preds = %NodeBlock53
  %Pivot52 = icmp ult i32 %load2, 28
  br i1 %Pivot52, label %NodeBlock43, label %NodeBlock49

NodeBlock49:                                      ; preds = %NodeBlock51
  %Pivot50 = icmp ult i32 %load2, 30
  br i1 %Pivot50, label %NodeBlock45, label %NodeBlock47

NodeBlock47:                                      ; preds = %NodeBlock49
  %Pivot48 = icmp eq i32 %load2, 30
  br i1 %Pivot48, label %Case30, label %Case31

NodeBlock45:                                      ; preds = %NodeBlock49
  %Pivot46.not = icmp eq i32 %load2, 29
  br i1 %Pivot46.not, label %Case29, label %Case28

NodeBlock43:                                      ; preds = %NodeBlock51
  %Pivot44 = icmp ult i32 %load2, 26
  br i1 %Pivot44, label %NodeBlock39, label %NodeBlock41

NodeBlock41:                                      ; preds = %NodeBlock43
  %Pivot42 = icmp eq i32 %load2, 26
  br i1 %Pivot42, label %Case26, label %Case27

NodeBlock39:                                      ; preds = %NodeBlock43
  %Pivot40.not = icmp eq i32 %load2, 25
  br i1 %Pivot40.not, label %Case25, label %Case24

NodeBlock37:                                      ; preds = %NodeBlock53
  %Pivot38 = icmp ult i32 %load2, 20
  br i1 %Pivot38, label %NodeBlock29, label %NodeBlock35

NodeBlock35:                                      ; preds = %NodeBlock37
  %Pivot36 = icmp ult i32 %load2, 22
  br i1 %Pivot36, label %NodeBlock31, label %NodeBlock33

NodeBlock33:                                      ; preds = %NodeBlock35
  %Pivot34 = icmp eq i32 %load2, 22
  br i1 %Pivot34, label %Case22, label %Case23

NodeBlock31:                                      ; preds = %NodeBlock35
  %Pivot32.not = icmp eq i32 %load2, 21
  br i1 %Pivot32.not, label %Case21, label %Case20

NodeBlock29:                                      ; preds = %NodeBlock37
  %Pivot30 = icmp ult i32 %load2, 18
  br i1 %Pivot30, label %Case17, label %NodeBlock27

NodeBlock27:                                      ; preds = %NodeBlock29
  %Pivot28 = icmp eq i32 %load2, 18
  br i1 %Pivot28, label %Case18, label %Case19

NodeBlock25:                                      ; preds = %NodeBlock55
  %Pivot26 = icmp slt i32 %load2, 9
  br i1 %Pivot26, label %NodeBlock9, label %NodeBlock23

NodeBlock23:                                      ; preds = %NodeBlock25
  %Pivot24 = icmp ult i32 %load2, 13
  br i1 %Pivot24, label %NodeBlock15, label %NodeBlock21

NodeBlock21:                                      ; preds = %NodeBlock23
  %Pivot22 = icmp ult i32 %load2, 15
  br i1 %Pivot22, label %NodeBlock17, label %NodeBlock19

NodeBlock19:                                      ; preds = %NodeBlock21
  %Pivot20 = icmp eq i32 %load2, 15
  br i1 %Pivot20, label %Case15, label %Case16

NodeBlock17:                                      ; preds = %NodeBlock21
  %Pivot18.not = icmp eq i32 %load2, 14
  br i1 %Pivot18.not, label %Case14, label %Case13

NodeBlock15:                                      ; preds = %NodeBlock23
  %Pivot16 = icmp ult i32 %load2, 11
  br i1 %Pivot16, label %NodeBlock11, label %NodeBlock13

NodeBlock13:                                      ; preds = %NodeBlock15
  %Pivot14 = icmp eq i32 %load2, 11
  br i1 %Pivot14, label %Case11, label %Case12

NodeBlock11:                                      ; preds = %NodeBlock15
  %Pivot12.not = icmp eq i32 %load2, 10
  br i1 %Pivot12.not, label %Case10, label %Case9

NodeBlock9:                                       ; preds = %NodeBlock25
  %Pivot10 = icmp slt i32 %load2, 5
  br i1 %Pivot10, label %NodeBlock1, label %NodeBlock7

NodeBlock7:                                       ; preds = %NodeBlock9
  %Pivot8 = icmp ult i32 %load2, 7
  br i1 %Pivot8, label %NodeBlock3, label %NodeBlock5

NodeBlock5:                                       ; preds = %NodeBlock7
  %Pivot6 = icmp eq i32 %load2, 7
  br i1 %Pivot6, label %Case7, label %Case8

NodeBlock3:                                       ; preds = %NodeBlock7
  %Pivot4.not = icmp eq i32 %load2, 6
  br i1 %Pivot4.not, label %Case6, label %Case5

NodeBlock1:                                       ; preds = %NodeBlock9
  %Pivot2 = icmp slt i32 %load2, 3
  br i1 %Pivot2, label %LeafBlock, label %NodeBlock

NodeBlock:                                        ; preds = %NodeBlock1
  %Pivot = icmp eq i32 %load2, 3
  br i1 %Pivot, label %Case3, label %Case4

LeafBlock:                                        ; preds = %NodeBlock1
  %SwitchLeaf = icmp eq i32 %load2, 1
  br i1 %SwitchLeaf, label %Case2, label %LeafBlock.Default_crit_edge

LeafBlock.Default_crit_edge:                      ; preds = %LeafBlock
  br label %Default

Case1:                                            ; preds = %LeafBlock107
  %indCase1 = add i32 %phi1, 2
  br label %Backedge

Case2:                                            ; preds = %LeafBlock
  %indCase2 = add i32 %phi1, 3
  br label %Backedge

Case3:                                            ; preds = %NodeBlock
  %indCase3 = add i32 %phi1, 4
  br label %Backedge

Case4:                                            ; preds = %NodeBlock
  %indCase4 = add i32 %phi1, 5
  br label %Backedge

Case5:                                            ; preds = %NodeBlock3
  %indCase5 = add i32 %phi1, 6
  br label %Backedge

Case6:                                            ; preds = %NodeBlock3
  %indCase6 = add i32 %phi1, 7
  br label %Backedge

Case7:                                            ; preds = %NodeBlock5
  %indCase7 = add i32 %phi1, 8
  br label %Backedge

Case8:                                            ; preds = %NodeBlock5
  %indCase8 = add i32 %phi1, 9
  br label %Backedge

Case9:                                            ; preds = %NodeBlock11
  %indCase9 = add i32 %phi1, 10
  br label %Backedge

Case10:                                           ; preds = %NodeBlock11
  %indCase10 = add i32 %phi1, 11
  br label %Backedge

Case11:                                           ; preds = %NodeBlock13
  %indCase11 = add i32 %phi1, 12
  br label %Backedge

Case12:                                           ; preds = %NodeBlock13
  %indCase12 = add i32 %phi1, 13
  br label %Backedge

Case13:                                           ; preds = %NodeBlock17
  %indCase13 = add i32 %phi1, 14
  br label %Backedge

Case14:                                           ; preds = %NodeBlock17
  %indCase14 = add i32 %phi1, 15
  br label %Backedge

Case15:                                           ; preds = %NodeBlock19
  %indCase15 = add i32 %phi1, 16
  br label %Backedge

Case16:                                           ; preds = %NodeBlock19
  %indCase16 = add i32 %phi1, 17
  br label %Backedge

Case17:                                           ; preds = %NodeBlock29
  %indCase17 = add i32 %phi1, 18
  br label %Backedge

Case18:                                           ; preds = %NodeBlock27
  %indCase18 = add i32 %phi1, 19
  br label %Backedge

Case19:                                           ; preds = %NodeBlock27
  %indCase19 = add i32 %phi1, 20
  br label %Backedge

Case20:                                           ; preds = %NodeBlock31
  %indCase20 = add i32 %phi1, 21
  br label %Backedge

Case21:                                           ; preds = %NodeBlock31
  %indCase21 = add i32 %phi1, 22
  br label %Backedge

Case22:                                           ; preds = %NodeBlock33
  %indCase22 = add i32 %phi1, 23
  br label %Backedge

Case23:                                           ; preds = %NodeBlock33
  %indCase23 = add i32 %phi1, 24
  br label %Backedge

Case24:                                           ; preds = %NodeBlock39
  %indCase24 = add i32 %phi1, 25
  br label %Backedge

Case25:                                           ; preds = %NodeBlock39
  %indCase25 = add i32 %phi1, 26
  br label %Backedge

Case26:                                           ; preds = %NodeBlock41
  %indCase26 = add i32 %phi1, 27
  br label %Backedge

Case27:                                           ; preds = %NodeBlock41
  %indCase27 = add i32 %phi1, 28
  br label %Backedge

Case28:                                           ; preds = %NodeBlock45
  %indCase28 = add i32 %phi1, 29
  br label %Backedge

Case29:                                           ; preds = %NodeBlock45
  %indCase29 = add i32 %phi1, 30
  br label %Backedge

Case30:                                           ; preds = %NodeBlock47
  %indCase30 = add i32 %phi1, 31
  br label %Backedge

Case31:                                           ; preds = %NodeBlock47
  %indCase31 = add i32 %phi1, 32
  br label %Backedge

Case32:                                           ; preds = %NodeBlock59
  %indCase32 = add i32 %phi1, 33
  br label %Backedge

Case33:                                           ; preds = %NodeBlock57
  %indCase33 = add i32 %phi1, 34
  br label %Backedge

Case34:                                           ; preds = %NodeBlock57
  %indCase34 = add i32 %phi1, 35
  br label %Backedge

Case35:                                           ; preds = %NodeBlock61
  %indCase35 = add i32 %phi1, 36
  br label %Backedge

Case36:                                           ; preds = %NodeBlock61
  %indCase36 = add i32 %phi1, 37
  br label %Backedge

Case37:                                           ; preds = %NodeBlock63
  %indCase37 = add i32 %phi1, 38
  br label %Backedge

Case38:                                           ; preds = %NodeBlock63
  %indCase38 = add i32 %phi1, 39
  br label %Backedge

Case39:                                           ; preds = %NodeBlock69
  %indCase39 = add i32 %phi1, 40
  br label %Backedge

Case40:                                           ; preds = %NodeBlock69
  %indCase40 = add i32 %phi1, 41
  br label %Backedge

Case41:                                           ; preds = %NodeBlock71
  %indCase41 = add i32 %phi1, 42
  br label %Backedge

Case42:                                           ; preds = %NodeBlock71
  %indCase42 = add i32 %phi1, 43
  br label %Backedge

Case43:                                           ; preds = %NodeBlock75
  %indCase43 = add i32 %phi1, 44
  br label %Backedge

Case44:                                           ; preds = %NodeBlock75
  %indCase44 = add i32 %phi1, 45
  br label %Backedge

Case45:                                           ; preds = %NodeBlock77
  %indCase45 = add i32 %phi1, 46
  br label %Backedge

Case46:                                           ; preds = %NodeBlock77
  %indCase46 = add i32 %phi1, 47
  br label %Backedge

Case47:                                           ; preds = %NodeBlock87
  %indCase47 = add i32 %phi1, 48
  br label %Backedge

Case48:                                           ; preds = %NodeBlock85
  %indCase48 = add i32 %phi1, 49
  br label %Backedge

Case49:                                           ; preds = %NodeBlock85
  %indCase49 = add i32 %phi1, 50
  br label %Backedge

Case50:                                           ; preds = %NodeBlock89
  %indCase50 = add i32 %phi1, 51
  br label %Backedge

Case51:                                           ; preds = %NodeBlock89
  %indCase51 = add i32 %phi1, 52
  br label %Backedge

Case52:                                           ; preds = %NodeBlock91
  %indCase52 = add i32 %phi1, 53
  br label %Backedge

Case53:                                           ; preds = %NodeBlock91
  %indCase53 = add i32 %phi1, 54
  br label %Backedge

Case54:                                           ; preds = %NodeBlock97
  %indCase54 = add i32 %phi1, 55
  br label %Backedge

Case55:                                           ; preds = %NodeBlock97
  %indCase55 = add i32 %phi1, 56
  br label %Backedge

Case56:                                           ; preds = %NodeBlock99
  %indCase56 = add i32 %phi1, 57
  br label %Backedge

Case57:                                           ; preds = %NodeBlock99
  %indCase57 = add i32 %phi1, 58
  br label %Backedge

Case58:                                           ; preds = %NodeBlock103
  %indCase58 = add i32 %phi1, 59
  br label %Backedge

Case59:                                           ; preds = %NodeBlock103
  %indCase59 = add i32 %phi1, 60
  br label %Backedge

Case60:                                           ; preds = %LeafBlock105
  %indCase60 = add i32 %phi1, 61
  br label %Backedge

Backedge:                                         ; preds = %Case60, %Case59, %Case58, %Case57, %Case56, %Case55, %Case54, %Case53, %Case52, %Case51, %Case50, %Case49, %Case48, %Case47, %Case46, %Case45, %Case44, %Case43, %Case42, %Case41, %Case40, %Case39, %Case38, %Case37, %Case36, %Case35, %Case34, %Case33, %Case32, %Case31, %Case30, %Case29, %Case28, %Case27, %Case26, %Case25, %Case24, %Case23, %Case22, %Case21, %Case20, %Case19, %Case18, %Case17, %Case16, %Case15, %Case14, %Case13, %Case12, %Case11, %Case10, %Case9, %Case8, %Case7, %Case6, %Case5, %Case4, %Case3, %Case2, %Case1
  %phi2 = phi i32 [ %indCase1, %Case1 ], [ %indCase2, %Case2 ], [ %indCase3, %Case3 ], [ %indCase4, %Case4 ], [ %indCase5, %Case5 ], [ %indCase6, %Case6 ], [ %indCase7, %Case7 ], [ %indCase8, %Case8 ], [ %indCase9, %Case9 ], [ %indCase10, %Case10 ], [ %indCase11, %Case11 ], [ %indCase12, %Case12 ], [ %indCase13, %Case13 ], [ %indCase14, %Case14 ], [ %indCase15, %Case15 ], [ %indCase16, %Case16 ], [ %indCase17, %Case17 ], [ %indCase18, %Case18 ], [ %indCase19, %Case19 ], [ %indCase20, %Case20 ], [ %indCase21, %Case21 ], [ %indCase22, %Case22 ], [ %indCase23, %Case23 ], [ %indCase24, %Case24 ], [ %indCase25, %Case25 ], [ %indCase26, %Case26 ], [ %indCase27, %Case27 ], [ %indCase28, %Case28 ], [ %indCase29, %Case29 ], [ %indCase30, %Case30 ], [ %indCase31, %Case31 ], [ %indCase32, %Case32 ], [ %indCase33, %Case33 ], [ %indCase34, %Case34 ], [ %indCase35, %Case35 ], [ %indCase36, %Case36 ], [ %indCase37, %Case37 ], [ %indCase38, %Case38 ], [ %indCase39, %Case39 ], [ %indCase40, %Case40 ], [ %indCase41, %Case41 ], [ %indCase42, %Case42 ], [ %indCase43, %Case43 ], [ %indCase44, %Case44 ], [ %indCase45, %Case45 ], [ %indCase46, %Case46 ], [ %indCase47, %Case47 ], [ %indCase48, %Case48 ], [ %indCase49, %Case49 ], [ %indCase50, %Case50 ], [ %indCase51, %Case51 ], [ %indCase52, %Case52 ], [ %indCase53, %Case53 ], [ %indCase54, %Case54 ], [ %indCase55, %Case55 ], [ %indCase56, %Case56 ], [ %indCase57, %Case57 ], [ %indCase58, %Case58 ], [ %indCase59, %Case59 ], [ %indCase60, %Case60 ]
  br label %BB1

Default:                                          ; preds = %LeafBlock.Default_crit_edge, %LeafBlock105.Default_crit_edge, %LeafBlock107.Default_crit_edge
  ret void
}

declare i32 addrspace(4)* @llvm.genx.GenISA.pair.to.ptr.p4i32(i32, i32)
