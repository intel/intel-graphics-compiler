;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; REQUIRES: regkeys
; RUN: igc_opt --opaque-pointers -platformCri -igc-emit-visa -regkey EnableWideMulMad,DumpVISAASMToConsole -disable-output < %s | FileCheck %s

; Check that wide-mul matching does not assert on a ConstantData operand.
; CHECK: mul {{.*}} 0x3:ud
; CHECK: lsc_store.ugm {{.*}} high:d64
define spir_kernel void @test(ptr addrspace(1) %out, i64 %value) {
  %high = call i64 @llvm.genx.GenISA.WideSMulHi(i64 3, i64 %value)
  store i64 %high, ptr addrspace(1) %out
  ret void
}

declare i64 @llvm.genx.GenISA.WideSMulHi(i64, i64)

!igc.functions = !{!0}
!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}

!IGCMetadata = !{!3}
!3 = !{!"ModuleMD", !4}
!4 = !{!"FuncMD", !5, !6}
!5 = !{!"FuncMDMap[0]", ptr @test}
!6 = !{!"FuncMDValue[0]", !7}
!7 = !{!"resAllocMD", !8}
!8 = !{!"argAllocMDList", !9, !10}
!9 = !{!"argAllocMDListVec[0]", !11}
!10 = !{!"argAllocMDListVec[1]", !11}
!11 = !{!"type", i32 0}
