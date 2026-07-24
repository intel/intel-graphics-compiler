;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2025 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================

; UNSUPPORTED: llvm-17-plus
; Resolution marks the printf string host-only (stringConstants); this checks that
; ProgramScopeConstantAnalysis then places it in the ConstantStrings buffer, not
; in the GPU-resident Constants/Globals buffers.
; RUN: igc_opt --typed-pointers -igc-opencl-printf-resolution -igc-programscope-constant-analysis \
; RUN:   -igc-serialize-metadata -S < %s | FileCheck %s
; ------------------------------------------------
; ProgramScopeConstantAnalysis
; ------------------------------------------------

; The string "abc\0" goes into ConstantStrings (inlineBuffersVec[1]).
; CHECK-DAG: !{!"inlineBuffers", [[CONSTANTS:![0-9]*]], [[CONST_STRINGS:![0-9]*]], [[GLOBALS:![0-9]*]]}
; CHECK-DAG: [[CONST_STRINGS]] = !{!"inlineBuffersVec[1]", [[ALIGN:![0-9]*]], [[CS_SIZE:![0-9]*]], [[CS_BUF:![0-9]*]]}
; CHECK-DAG: [[CS_SIZE]] = !{!"allocSize", i{{32|64}} 4}
; CHECK-DAG: [[CS_BUF]] = !{!"Buffer", [[B0:![0-9]*]], [[B1:![0-9]*]], [[B2:![0-9]*]], [[B3:![0-9]*]]}
; CHECK-DAG: [[B0]] = !{!"BufferVec[0]", i8 97}
; CHECK-DAG: [[B1]] = !{!"BufferVec[1]", i8 98}
; CHECK-DAG: [[B2]] = !{!"BufferVec[2]", i8 99}
; CHECK-DAG: [[B3]] = !{!"BufferVec[3]", i8 0}

; The Constants (vec[0]) and Globals (vec[2]) buffers stay empty.
; CHECK-DAG: [[CONSTANTS]] = !{!"inlineBuffersVec[0]", [[ALIGN]], [[EMPTY_SIZE:![0-9]*]], [[EMPTY_BUF:![0-9]*]]}
; CHECK-DAG: [[EMPTY_SIZE]] = !{!"allocSize", i{{32|64}} 0}
; CHECK-DAG: [[EMPTY_BUF]] = !{!"Buffer"}
; CHECK-DAG: [[GLOBALS]] = !{!"inlineBuffersVec[2]", [[ALIGN]], [[EMPTY_SIZE]], [[EMPTY_BUF]]}

; The string is recorded in the offsets map.
; CHECK-DAG: !{!"inlineProgramScopeOffsetsMap[0]", [4 x i8] addrspace(2)* @.str}

@.str = internal unnamed_addr addrspace(2) constant [4 x i8] c"abc\00", align 1

define spir_kernel void @test() {
  %p = getelementptr inbounds [4 x i8], [4 x i8] addrspace(2)* @.str, i64 0, i64 0
  %c = call spir_func i32 (i8 addrspace(2)*, ...) @printf(i8 addrspace(2)* %p)
  ret void
}

declare i32 @printf(i8 addrspace(2)*, ...)
