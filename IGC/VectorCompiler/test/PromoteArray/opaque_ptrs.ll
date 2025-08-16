;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; RUN: %opt_opaque_ptrs %use_old_pass_manager% -GenXPromoteArray -march=genx64 -mcpu=XeLP -S < %s | FileCheck %s --check-prefixes=CHECK

define dllexport spir_kernel void @f_f(ptr addrspace(1) %out) {
; CHECK: [[ALLOCA:%.*]] = alloca <4 x i32>
  %alloca = alloca [4 x i32], align 64
; CHECK-NEXT: [[LOAD0:%.*]] = load <4 x i32>, ptr [[ALLOCA]]
; CHECK-NEXT: [[INS0:%.*]] = insertelement <4 x i32> [[LOAD0]], i32 0, i32 0
; CHECK-NEXT: [[INS1:%.*]] = insertelement <4 x i32> [[INS0]], i32 1, i32 1
; CHECK-NEXT: store <4 x i32> [[INS1]], ptr [[ALLOCA]]
  store <2 x i32> <i32 0, i32 1>, ptr %alloca
; CHECK-NEXT: [[LOAD1:%.*]] = load <4 x i32>, ptr [[ALLOCA]]
; CHECK-NEXT: [[INS2:%.*]] = insertelement <4 x i32> [[LOAD1]], i32 2, i32 2
; CHECK-NEXT: [[INS3:%.*]] = insertelement <4 x i32> [[INS2]], i32 3, i32 3
; CHECK-NEXT: store <4 x i32> [[INS3]], ptr [[ALLOCA]]
  %gep1 = getelementptr i8, ptr %alloca, i64 8
  store <2 x i32> <i32 2, i32 3>, ptr %gep1
; CHECK-NEXT: [[LOAD2:%.*]] = load <4 x i32>, ptr [[ALLOCA]]
; CHECK-NEXT: [[BC:%.*]] = bitcast <4 x i32> [[LOAD2]] to <16 x i8>
; CHECK-NEXT: [[EX0:%.*]] = extractelement <16 x i8> [[BC]], i32 0
; CHECK-NEXT: [[INS4:%.*]] = insertelement <12 x i8> undef, i8 [[EX0]], i32 0
; CHECK-NEXT: [[EX1:%.*]] = extractelement <16 x i8> [[BC]], i32 1
; CHECK-NEXT: [[INS5:%.*]] = insertelement <12 x i8> [[INS4]], i8 [[EX1]], i32 1
; CHECK-NEXT: [[EX2:%.*]] = extractelement <16 x i8> [[BC]], i32 2
; CHECK-NEXT: [[INS6:%.*]] = insertelement <12 x i8> [[INS5]], i8 [[EX2]], i32 2
; CHECK-NEXT: [[EX3:%.*]] = extractelement <16 x i8> [[BC]], i32 3
; CHECK-NEXT: [[INS7:%.*]] = insertelement <12 x i8> [[INS6]], i8 [[EX3]], i32 3
; CHECK-NEXT: [[EX4:%.*]] = extractelement <16 x i8> [[BC]], i32 4
; CHECK-NEXT: [[INS8:%.*]] = insertelement <12 x i8> [[INS7]], i8 [[EX4]], i32 4
; CHECK-NEXT: [[EX5:%.*]] = extractelement <16 x i8> [[BC]], i32 5
; CHECK-NEXT: [[INS9:%.*]] = insertelement <12 x i8> [[INS8]], i8 [[EX5]], i32 5
; CHECK-NEXT: [[EX6:%.*]] = extractelement <16 x i8> [[BC]], i32 6
; CHECK-NEXT: [[INS10:%.*]] = insertelement <12 x i8> [[INS9]], i8 [[EX6]], i32 6
; CHECK-NEXT: [[EX7:%.*]] = extractelement <16 x i8> [[BC]], i32 7
; CHECK-NEXT: [[INS11:%.*]] = insertelement <12 x i8> [[INS10]], i8 [[EX7]], i32 7
; CHECK-NEXT: [[EX8:%.*]] = extractelement <16 x i8> [[BC]], i32 8
; CHECK-NEXT: [[INS12:%.*]] = insertelement <12 x i8> [[INS11]], i8 [[EX8]], i32 8
; CHECK-NEXT: [[EX9:%.*]] = extractelement <16 x i8> [[BC]], i32 9
; CHECK-NEXT: [[INS13:%.*]] = insertelement <12 x i8> [[INS12]], i8 [[EX9]], i32 9
; CHECK-NEXT: [[EX10:%.*]] = extractelement <16 x i8> [[BC]], i32 10
; CHECK-NEXT: [[INS14:%.*]] = insertelement <12 x i8> [[INS13]], i8 [[EX10]], i32 10
; CHECK-NEXT: [[EX11:%.*]] = extractelement <16 x i8> [[BC]], i32 11
; CHECK-NEXT: [[INS15:%.*]] = insertelement <12 x i8> [[INS14]], i8 [[EX11]], i32 11
  %gep2 = getelementptr i8, ptr %alloca, i64 4
  %load1 = load <12 x i8>, ptr %alloca
; CHECK-NEXT: [[LOAD3:%.*]] = load <4 x i32>, ptr [[ALLOCA]]
; CHECK-NEXT: [[BC1:%.*]] = bitcast <4 x i32> [[LOAD3]] to <16 x i8>
; CHECK-NEXT: [[EX12:%.*]] = extractelement <12 x i8> [[INS15]], i32 0
; CHECK-NEXT: [[INS16:%.*]] = insertelement <16 x i8> [[BC1]], i8 [[EX12]], i32 4
; CHECK-NEXT: [[EX13:%.*]] = extractelement <12 x i8> [[INS15]], i32 1
; CHECK-NEXT: [[INS17:%.*]] = insertelement <16 x i8> [[INS16]], i8 [[EX13]], i32 5
; CHECK-NEXT: [[EX14:%.*]] = extractelement <12 x i8> [[INS15]], i32 2
; CHECK-NEXT: [[INS18:%.*]] = insertelement <16 x i8> [[INS17]], i8 [[EX14]], i32 6
; CHECK-NEXT: [[EX15:%.*]] = extractelement <12 x i8> [[INS15]], i32 3
; CHECK-NEXT: [[INS19:%.*]] = insertelement <16 x i8> [[INS18]], i8 [[EX15]], i32 7
; CHECK-NEXT: [[EX16:%.*]] = extractelement <12 x i8> [[INS15]], i32 4
; CHECK-NEXT: [[INS20:%.*]] = insertelement <16 x i8> [[INS19]], i8 [[EX16]], i32 8
; CHECK-NEXT: [[EX17:%.*]] = extractelement <12 x i8> [[INS15]], i32 5
; CHECK-NEXT: [[INS21:%.*]] = insertelement <16 x i8> [[INS20]], i8 [[EX17]], i32 9
; CHECK-NEXT: [[EX18:%.*]] = extractelement <12 x i8> [[INS15]], i32 6
; CHECK-NEXT: [[INS22:%.*]] = insertelement <16 x i8> [[INS21]], i8 [[EX18]], i32 10
; CHECK-NEXT: [[EX19:%.*]] = extractelement <12 x i8> [[INS15]], i32 7
; CHECK-NEXT: [[INS23:%.*]] = insertelement <16 x i8> [[INS22]], i8 [[EX19]], i32 11
; CHECK-NEXT: [[EX20:%.*]] = extractelement <12 x i8> [[INS15]], i32 8
; CHECK-NEXT: [[INS24:%.*]] = insertelement <16 x i8> [[INS23]], i8 [[EX20]], i32 12
; CHECK-NEXT: [[EX21:%.*]] = extractelement <12 x i8> [[INS15]], i32 9
; CHECK-NEXT: [[INS25:%.*]] = insertelement <16 x i8> [[INS24]], i8 [[EX21]], i32 13
; CHECK-NEXT: [[EX22:%.*]] = extractelement <12 x i8> [[INS15]], i32 10
; CHECK-NEXT: [[INS26:%.*]] = insertelement <16 x i8> [[INS25]], i8 [[EX22]], i32 14
; CHECK-NEXT: [[EX23:%.*]] = extractelement <12 x i8> [[INS15]], i32 11
; CHECK-NEXT: [[INS27:%.*]] = insertelement <16 x i8> [[INS26]], i8 [[EX23]], i32 15
; CHECK-NEXT: [[BC2:%.*]] = bitcast <16 x i8> [[INS27]] to <4 x i32>
; CHECK-NEXT: store <4 x i32> [[BC2]], ptr [[ALLOCA]]
  store <12 x i8> %load1, ptr %gep2
; CHECK-NEXT: [[LOAD4:%.*]] = load <4 x i32>, ptr [[ALLOCA]]
; CHECK-NEXT: [[EX24:%.*]] = extractelement <4 x i32> [[LOAD4]], i32 0
; CHECK-NEXT: [[INS28:%.*]] = insertelement <2 x i32> undef, i32 [[EX24]], i32 0
; CHECK-NEXT: [[EX25:%.*]] = extractelement <4 x i32> [[LOAD4]], i32 1
; CHECK-NEXT: [[INS29:%.*]] = insertelement <2 x i32> [[INS28]], i32 [[EX25]], i32 1
  %load2 = load <2 x i32>, ptr %alloca
; CHECK-NEXT: store <2 x i32> [[INS29]], ptr addrspace(1) %out
  store <2 x i32> %load2, ptr addrspace(1) %out
; CHECK-NEXT: ret void
  ret void
}
