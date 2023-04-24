;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXGASDynamicResolution -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s

define spir_kernel void @kernel(i32 addrspace(1)* %global_buffer, i32 addrspace(3)* %local_buffer) #0 {
entry:
  br label %bb1
bb1:
  br label %bb2
bb2:
  %v = load i32, i32 addrspace(3)* %local_buffer, align 4
  %c = icmp ne i32 %v, 0
  br i1 %c, label %gen.to.as3, label %gen.to.as1
gen.to.as3:
  %generic_ptr_3 = addrspacecast i32 addrspace(3)* %local_buffer to i32 addrspace(4)*
  br label %body
gen.to.as1:
  %generic_ptr_1 = addrspacecast i32 addrspace(1)* %global_buffer to i32 addrspace(4)*
  br label %body
body:
  %generic_ptr = phi i32 addrspace(4)* [ %generic_ptr_1, %gen.to.as1 ], [ %generic_ptr_3, %gen.to.as3 ]
  ; CHECK: %generic_ptr = phi i32 addrspace(4)* [ %generic_ptr_1, %gen.to.as1 ], [ %generic_ptr_3, %gen.to.as3 ]

  %ld = load i32, i32 addrspace(4)* %generic_ptr, align 4
  ; CHECK: %[[LD_CAST:.*]] = ptrtoint i32 addrspace(4)* %generic_ptr to i64
  ; CHECK: %[[LD_CAST_V:.*]] = bitcast i64  %[[LD_CAST:.*]] to <2 x i32>
  ; CHECK: %[[LD_TAG:.*]] = extractelement <2 x i32> %[[LD_CAST_V:.*]], i64 1
  ; CHECK: %isLocalTag = icmp eq i32 %[[LD_TAG:.*]], 1073741824
  ; CHECK: br i1 %isLocalTag, label %LocalBlock, label %GlobalBlock

  ; CHECK: LocalBlock:
  ; CHECK:   %[[LOCAL_PTR:.*]] = addrspacecast i32 addrspace(4)* %generic_ptr to i32 addrspace(3)*
  ; CHECK:   %localLoad = load i32, i32 addrspace(3)* %[[LOCAL_PTR:.*]], align 4

  ; CHECK: GlobalBlock:
  ; CHECK:   %[[GLOBAL_PTR:.*]] = addrspacecast i32 addrspace(4)* %generic_ptr to i32 addrspace(1)*
  ; CHECK:   %globalLoad = load i32, i32 addrspace(1)* %[[LOCAL_PTR:.*]], align 4

  ; CHECK: %ld = phi i32 [ %localLoad, %LocalBlock ], [ %globalLoad, %GlobalBlock ]

  br label %exit
exit:
  store i32 %ld, i32 addrspace(1)* %global_buffer, align 4
  ; CHECK: store i32 %ld, i32 addrspace(1)* %global_buffer, align 4
  ret void
}

attributes #0 = { noinline nounwind "CMGenxMain" }
