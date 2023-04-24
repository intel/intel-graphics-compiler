;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; RUN: opt %use_old_pass_manager% -GenXGASDynamicResolution -march=genx64 -mcpu=Gen9 -S < %s | FileCheck %s
;
target datalayout = "e-p:64:64-i64:64-n8:16:32:64"

define spir_func void @foo(i8* %.p0, i8 addrspace(3)* %.p3, i8 addrspace(1)* %.p1) {
  ret void
}

; CHECK-LABEL: @kernelA
define spir_kernel void @kernelA(i8 addrspace(1)* %global_buffer) #1 {
  %.p1top4 = addrspacecast i8 addrspace(1)* %global_buffer to i8 addrspace(4)*

  ; COM: Since there are no private->generic casts to.private.explicit will return null.
  %call1 = tail call i8* @llvm.vc.internal.cast.to.ptr.explicit.p0i8(i8 addrspace(4)* %.p1top4)

  ; COM: Since there are no local->generic casts to.local.explicit will return null.
  %call2 = tail call i8 addrspace(3)* @llvm.vc.internal.cast.to.ptr.explicit.p3i8(i8 addrspace(4)* %.p1top4)

  %call3 = tail call i8 addrspace(1)* @llvm.vc.internal.cast.to.ptr.explicit.p1i8(i8 addrspace(4)* %.p1top4)

  call spir_func void @foo(i8* %call1, i8 addrspace(3)* %call2, i8 addrspace(1)* %call3)
  ; CHECK-DAG: call spir_func void @foo(i8* null, i8 addrspace(3)* null
  ret void
}

; CHECK-LABEL: @kernelB
define spir_kernel void @kernelB(i8* %private_buffer, i8 addrspace(3)* %local_buffer) #1 {
  %.p3top4 = addrspacecast i8 addrspace(3)* %local_buffer to i8 addrspace(4)*
  %.p0top4 = addrspacecast i8* %private_buffer to i8 addrspace(4)*

  %call1 = tail call i8* @llvm.vc.internal.cast.to.ptr.explicit.p0i8(i8 addrspace(4)* %.p0top4)
  ; CHECK-DAG: %[[P2I_0:[^ ]+]] = ptrtoint i8 addrspace(4)* %.p0top4 to i64
  ; CHECK-DAG: %[[BCAST_0:[^ ]+]] = bitcast i64 %[[P2I_0:[^ ]+]] to <2 x i32>
  ; CHECK-DAG: %[[EXTRACT_0:[^ ]+]] = extractelement <2 x i32> %[[BCAST_0:[^ ]+]], i64 1
  ; CHECK-DAG: %[[AND:[^ ]+]] = and i32 %[[EXTRACT_0:[^ ]+]], -536870912
  ; CHECK-DAG: %isPrivateTag = icmp eq i32 %[[AND:[^ ]+]], 536870912
  ; CHECK-DAG: %[[P4_TO_P0:[^ ]+]] = addrspacecast i8 addrspace(4)* %.p0top4 to i8*
  ; CHECK-DAG: %[[CALL1:[^ ]+]] = select i1 %isPrivateTag, i8* %[[P4_TO_P0:[^ ]+]], i8* null

  %call2 = tail call i8 addrspace(3)* @llvm.vc.internal.cast.to.ptr.explicit.p3i8(i8 addrspace(4)* %.p3top4)
  ; CHECK-DAG: %[[P2I_1:[^ ]+]] = ptrtoint i8 addrspace(4)* %.p3top4 to i64
  ; CHECK-DAG: %[[BCAST_1:[^ ]+]] = bitcast i64 %[[P2I_1:[^ ]+]] to <2 x i32>
  ; CHECK-DAG: %[[EXTRACT_1:[^ ]+]] = extractelement <2 x i32> %[[BCAST_1:[^ ]+]], i64 1
  ; CHECK-DAG: %isLocalTag = icmp eq i32 %[[EXTRACT_1:[^ ]+]], 1073741824
  ; CHECK-DAG: %[[P4_TO_P3:[^ ]+]] = addrspacecast i8 addrspace(4)* %.p3top4 to i8 addrspace(3)*
  ; CHECK-DAG: %[[CALL2:[^ ]+]] = select i1 %isLocalTag, i8 addrspace(3)* %[[P4_TO_P3:[^ ]+]], i8 addrspace(3)* null

  ; COM: Since there are no global->generic casts to.global.explicit will return null.
  %call3 = tail call i8 addrspace(1)* @llvm.vc.internal.cast.to.ptr.explicit.p1i8(i8 addrspace(4)* %.p0top4)

  call spir_func void @foo(i8* %call1, i8 addrspace(3)* %call2, i8 addrspace(1)* %call3)
  ; CHECK-DAG: call spir_func void @foo(i8* %[[CALL1:[^ ]+]], i8 addrspace(3)* %[[CALL2:[^ ]+]], i8 addrspace(1)* null)

  ret void
}

declare !internal_intrinsic_id !3 i8* @llvm.vc.internal.cast.to.ptr.explicit.p0i8(i8 addrspace(4)*) #0
declare !internal_intrinsic_id !3 i8 addrspace(3)* @llvm.vc.internal.cast.to.ptr.explicit.p3i8(i8 addrspace(4)*) #0
declare !internal_intrinsic_id !3 i8 addrspace(1)* @llvm.vc.internal.cast.to.ptr.explicit.p1i8(i8 addrspace(4)*) #0

attributes #0 = { nounwind readnone }
attributes #1 = { noinline nounwind "CMGenxMain" }

!3 = !{i32 7965}