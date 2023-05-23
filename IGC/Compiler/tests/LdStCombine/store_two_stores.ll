;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2017-2023 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================




 ; Given  store i8
 ;        store i8
 ;   combined into
 ;        <2 x i8> store
 ;
 ; Given  store i16
 ;        store i16
 ;   combined into
 ;        insertvalue
 ;        insertvalue
 ;        store i32
 ;
 ; CHECK-LABEL: target datalayout
 ; CHECK: %__StructSOALayout_ = type <{ %__StructAOSLayout_ }>
 ; CHECK: %__StructAOSLayout_ = type <{ i16, i16 }>
 ; CHECK-LABEL: define spir_kernel void @test_two_st
 ; CHECK: store <2 x i8> {{.*}}
 ; CHECK: [[STMP1:%.*]] = insertvalue %__StructSOALayout_ undef, i16 %{{.*}}, 0, 0
 ; CHECK: [[STMP2:%.*]] = insertvalue %__StructSOALayout_ [[STMP1]], i16 %{{.*}}, 0, 1
 ; CHECK: [[STMP3:%.*]] = call i32 @llvm.genx.GenISA.bitcastfromstruct.i32.__StructSOALayout_(%__StructSOALayout_ [[STMP2]])
 ; CHECK: store i32 [[STMP3]]
 ; CHECK: ret void

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-n8:16:32"
target triple = "spir64-unknown-unknown"

%struct.s2xi8_t = type { i8, i8 }
%struct.s2xi16_t = type { i16, i16 }

; Function Attrs: convergent nounwind
define spir_kernel void @test_two_st(i32 addrspace(1)* %d, i16 addrspace(1)* %ss, i8 addrspace(1)* %sc, <8 x i32> %r0, <8 x i32> %payloadHeader, i16 %localIdX, i16 %localIdY, i16 %localIdZ) #0 {
entry:
  %0 = load i8, i8 addrspace(1)* %sc, align 1
  %idxprom = zext i16 %localIdX to i64
  %arrayidx1 = getelementptr inbounds i8, i8 addrspace(1)* %sc, i64 %idxprom
  %1 = load i8, i8 addrspace(1)* %arrayidx1, align 1
  %arrayidx2 = getelementptr inbounds i16, i16 addrspace(1)* %ss, i64 1
  %2 = load i16, i16 addrspace(1)* %arrayidx2, align 2
  %arrayidx4 = getelementptr inbounds i16, i16 addrspace(1)* %ss, i64 %idxprom
  %3 = load i16, i16 addrspace(1)* %arrayidx4, align 2
  %4 = bitcast i32 addrspace(1)* %d to %struct.s2xi8_t addrspace(1)*
  %x.sroa.0.0..sroa_idx = getelementptr inbounds %struct.s2xi8_t, %struct.s2xi8_t addrspace(1)* %4, i64 1, i32 0
  store i8 %0, i8 addrspace(1)* %x.sroa.0.0..sroa_idx, align 2
  %x.sroa.4.0..sroa_idx = getelementptr inbounds %struct.s2xi8_t, %struct.s2xi8_t addrspace(1)* %4, i64 1, i32 1
  store i8 %1, i8 addrspace(1)* %x.sroa.4.0..sroa_idx, align 1
  %arrayidx94 = getelementptr inbounds i32, i32 addrspace(1)* %d, i64 %idxprom
  %arrayidx9 = bitcast i32 addrspace(1)* %arrayidx94 to %struct.s2xi16_t addrspace(1)*
  %y.sroa.0.0..sroa_idx = bitcast i32 addrspace(1)* %arrayidx94 to i16 addrspace(1)*
  store i16 %2, i16 addrspace(1)* %y.sroa.0.0..sroa_idx, align 4
  %y.sroa.4.0..sroa_idx2 = getelementptr inbounds %struct.s2xi16_t, %struct.s2xi16_t addrspace(1)* %arrayidx9, i64 0, i32 1
  store i16 %3, i16 addrspace(1)* %y.sroa.4.0..sroa_idx2, align 2
  ret void
}
