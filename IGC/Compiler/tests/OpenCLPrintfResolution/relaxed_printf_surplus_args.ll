;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2026 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
; RUN: igc_opt --opaque-pointers -igc-opencl-printf-resolution -S < %s | FileCheck %s

; The runtime consumes as many arguments as the format has conversions and
; parses what follows as the next record. An inline format's conversions are
; counted at run time, so each argument is reserved and written only if the
; format consumes it.

define spir_kernel void @test(ptr addrspace(1) %fmt, i32 %a, <2 x i32> %b, <8 x i32> %r0, <8 x i32> %payloadHeader, ptr addrspace(2) %constBase, ptr %privateBase, ptr addrspace(1) %printfBuffer) #0 {
  %call = call spir_func i32 (ptr addrspace(1), ...) @printf(ptr addrspace(1) %fmt, i32 %a, <2 x i32> %b) #0
  ret void
}

; CHECK-LABEL: @test(
; CHECK:       strlen.loop:
; CHECK:         [[COUNT:%fmt_conv_count]] = phi i32 [ 0, %{{.*}} ], [ [[COUNT_NEXT:%fmt_conv_count_next]], %strlen.loop ]
; CHECK:         [[OPEN:%fmt_after_open]] = phi i1 [ false, %{{.*}} ], [ [[OPEN_NEXT:%.*]], %strlen.loop ]
; CHECK:         [[CH:%.*]] = load i8
; CHECK:         [[IS_PCT:%.*]] = icmp eq i8 [[CH]], 37
; CHECK:         [[D0:%.*]] = select i1 [[OPEN]], i32 -1, i32 1
; CHECK:         [[D1:%.*]] = select i1 [[IS_PCT]], i32 [[D0]], i32 0
; CHECK:         [[COUNT_NEXT]] = add i32 [[COUNT]], [[D1]]
; CHECK:         [[NOT_OPEN:%.*]] = xor i1 [[OPEN]], true
; CHECK:         [[OPEN_NEXT]] = and i1 [[IS_PCT]], [[NOT_OPEN]]
; CHECK:       strlen.done:
; Fixed part: 8-byte marker, i32 record (8) and <2 x i32> record (16).
; CHECK:         [[SIZE0:%.*]] = add i32 32, %fmt_aligned_len
; CHECK:         [[SKIP_A:%.*]] = icmp uge i32 0, [[COUNT_NEXT]]
; CHECK:         [[SKIP_A_SIZE:%.*]] = select i1 [[SKIP_A]], i32 8, i32 0
; CHECK:         [[SIZE1:%.*]] = sub i32 [[SIZE0]], [[SKIP_A_SIZE]]
; CHECK:         [[SKIP_B:%.*]] = icmp uge i32 1, [[COUNT_NEXT]]
; CHECK:         [[SKIP_B_SIZE:%.*]] = select i1 [[SKIP_B]], i32 16, i32 0
; CHECK:         [[SIZE2:%.*]] = sub i32 [[SIZE1]], [[SKIP_B_SIZE]]
; The runtime record size has no upper bound, so it is compared with the room
; left instead of adding it to the offset.
; CHECK:       reserve.loop:
; CHECK:         [[OFFSET:%.*]] = phi i32
; CHECK:         [[ROOM:%.*]] = sub i32 4194304, [[OFFSET]]
; CHECK:         icmp ule i32 [[SIZE2]], [[ROOM]]
; CHECK:         add i32 [[OFFSET]], [[SIZE2]]
; CHECK:         call i32 @__builtin_IB_atomic_cmpxchg_global_i32(
; CHECK:       write_offset_true:
; CHECK:         call void @llvm.memcpy.p1.p1.i32
; CHECK:         [[USE_A:%.*]] = icmp ult i32 0, [[COUNT_NEXT]]
; CHECK:         br i1 [[USE_A]], label %[[WRITE_A:.*]], label %write_args_done
; CHECK:       [[WRITE_A]]:
; CHECK:         store i32 %a
; CHECK:         [[USE_B:%.*]] = icmp ult i32 1, [[COUNT_NEXT]]
; CHECK:         br i1 [[USE_B]], label %[[WRITE_B:.*]], label %write_args_done
; CHECK:       [[WRITE_B]]:
; CHECK:         store <2 x i32> %b
; CHECK:         br label %write_args_done
; CHECK:       write_args_done:
; CHECK-NEXT:    br label %bblockJoin

declare spir_func i32 @printf(ptr addrspace(1), ...) #0

attributes #0 = { nounwind }

!igc.functions = !{!0}
!IGCMetadata = !{!9}

!0 = !{ptr @test, !1}
!1 = !{!2}
!2 = !{!"function_type", i32 0}
!9 = !{!"ModuleMD", !10}
!10 = !{!"FuncMD", !11, !12}
!11 = distinct !{!"FuncMDMap[0]", ptr @test}
!12 = !{!"FuncMDValue[0]", !13, !14, !15, !26}
!13 = !{!"localOffsets"}
!14 = !{!"funcArgs"}
!15 = !{!"functionType", !"KernelFunction"}
!16 = !{!"argId", i32 0}
!17 = !{!"implicitArgInfoListVec[0]", !16}
!18 = !{!"argId", i32 1}
!19 = !{!"implicitArgInfoListVec[1]", !18}
!20 = !{!"argId", i32 11}
!21 = !{!"implicitArgInfoListVec[2]", !20}
!22 = !{!"argId", i32 13}
!23 = !{!"implicitArgInfoListVec[3]", !22}
!24 = !{!"argId", i32 14}
!25 = !{!"implicitArgInfoListVec[4]", !24}
!26 = !{!"implicitArgInfoList", !17, !19, !21, !23, !25}
