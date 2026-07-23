;=========================== begin_copyright_notice ============================
;
; Copyright (C) 2022 Intel Corporation
;
; SPDX-License-Identifier: MIT
;
;============================ end_copyright_notice =============================
;
; UNSUPPORTED: llvm-17-plus
; RUN: igc_opt --typed-pointers -igc-programscope-constant-analysis -igc-serialize-metadata -S < %s | FileCheck %s
; ------------------------------------------------
; ProgramScopeConstantAnalysis
; ------------------------------------------------

; Test checks that md is updated with buffer info

@a = internal addrspace(2) constant [2 x i32] [i32 0, i32 1], align 4
@d = internal addrspace(1) global i32 addrspace(2)* getelementptr inbounds ([2 x i32], [2 x i32] addrspace(2)* @a, i32 0, i32 0), align 8
@c = internal addrspace(1) global i32 0, align 4

; CHECK: @[[LLVM_USED:[a-zA-Z0-9_$"\\.-]+]] = appending global [3 x i8*] [i8* addrspacecast (i8 addrspace(2)* bitcast ([2 x i32] addrspace(2)* @a to i8 addrspace(2)*) to i8*), i8* addrspacecast (i8 addrspace(1)* bitcast (i32 addrspace(2)* addrspace(1)* @d to i8 addrspace(1)*) to i8*), i8* addrspacecast (i8 addrspace(1)* bitcast (i32 addrspace(1)* @c to i8 addrspace(1)*) to i8*)], section "llvm.metadata"

define spir_kernel void @test_program(i32 addrspace(1)* %dst) {
entry:
  %0 = load i32 addrspace(2)*, i32 addrspace(2)* addrspace(1)* @d, align 8
  %1 = load i32, i32 addrspace(2)* %0, align 4
  store i32 %1, i32 addrspace(1)* @c, align 4
  ret void
}


!igc.functions = !{!3}
!IGCMetadata = !{!0}

!0 = !{!"ModuleMD", !1}
!1 = !{!"stringConstants", !2}
!2 = !{!"stringConstantsSet[0]", i32 addrspace(1)* @c}
!3 = !{void (i32 addrspace(1)*)* @test_program, !4}
!4 = !{!4}
!5 = !{!"function_type", i32 0}

; CHECK-DAG: !{!"inlineBuffers", [[INLINE_BUFFERS_CONSTANTS:![0-9]*]], [[INLINE_BUFFERS_CONSTANT_STRINGS:![0-9]*]], [[INLINE_BUFFERS_GLOBALS:![0-9]*]]}
; CHECK-DAG: [[INLINE_BUFFERS_CONSTANTS]] = !{!"inlineBuffersVec[0]", [[ALIGNMENT:![0-9]*]], [[VEC0_ALLOCSIZE:![0-9]*]], [[VEC0_BUFFER:![0-9]*]]}
; CHECK-DAG: [[ALIGNMENT]] = !{!"alignment", i32 0}
; CHECK-DAG: [[VEC0_ALLOCSIZE]] = !{!"allocSize", i{{32|64}} 8}
; CHECK-DAG: [[VEC0_BUFFER]] = !{!"Buffer", [[VEC0_BUFFER0:![0-9]*]], [[VEC0_BUFFER1:![0-9]*]], [[VEC0_BUFFER2:![0-9]*]], [[VEC0_BUFFER3:![0-9]*]], [[VEC0_BUFFER4:![0-9]*]], [[VEC0_BUFFER5:![0-9]*]], [[VEC0_BUFFER6:![0-9]*]], [[VEC0_BUFFER7:![0-9]*]]}
; CHECK-DAG: [[VEC0_BUFFER0]] = !{!"BufferVec[0]", i8 0}
; CHECK-DAG: [[VEC0_BUFFER1]] = !{!"BufferVec[1]", i8 0}
; CHECK-DAG: [[VEC0_BUFFER2]] = !{!"BufferVec[2]", i8 0}
; CHECK-DAG: [[VEC0_BUFFER3]] = !{!"BufferVec[3]", i8 0}
; CHECK-DAG: [[VEC0_BUFFER4]] = !{!"BufferVec[4]", i8 1}
; CHECK-DAG: [[VEC0_BUFFER5]] = !{!"BufferVec[5]", i8 0}
; CHECK-DAG: [[VEC0_BUFFER6]] = !{!"BufferVec[6]", i8 0}
; CHECK-DAG: [[VEC0_BUFFER7]] = !{!"BufferVec[7]", i8 0}
; CHECK-DAG: [[INLINE_BUFFERS_CONSTANT_STRINGS]] = !{!"inlineBuffersVec[1]", [[ALIGNMENT]], [[VEC1_ALLOCSIZE:![0-9]*]], [[VEC1_BUFFER:![0-9]*]]}
; CHECK-DAG: [[VEC1_ALLOCSIZE]] = !{!"allocSize", i{{32|64}} 0}
; CHECK-DAG: [[VEC1_BUFFER]] = !{!"Buffer"}
; CHECK-DAG: [[INLINE_BUFFERS_GLOBALS]] = !{!"inlineBuffersVec[2]", [[ALIGNMENT]], [[GVEC0_ALLOCSIZE:![0-9]*]], [[GVEC0_BUFFER:![0-9]*]]}
; CHECK-DAG: [[GVEC0_ALLOCSIZE]] = !{!"allocSize", i{{32|64}} 12}
; CHECK-DAG: [[GVEC0_BUFFER]] = !{!"Buffer", [[VEC0_BUFFER0]], [[VEC0_BUFFER1]], [[VEC0_BUFFER2]], [[VEC0_BUFFER3]], [[GVEC0_BUFFER4:![0-9]*]], [[VEC0_BUFFER5]], [[VEC0_BUFFER6]], [[VEC0_BUFFER7]]}
; CHECK-DAG: [[GVEC0_BUFFER4]] = !{!"BufferVec[4]", i8 0}
; CHECK-DAG: !{!"GlobalBufferAddressRelocInfo", [[GLOBAL_BUFFER_ADDRESS_RELOC_INFO_VEC0:![0-9]*]]}
; CHECK-DAG: [[GLOBAL_BUFFER_ADDRESS_RELOC_INFO_VEC0]] = !{!"GlobalBufferAddressRelocInfoVec[0]", [[BUFFER_OFFSET:![0-9]*]], [[POINTER_SIZE:![0-9]*]], [[SYMBOL:![0-9]*]]}
; CHECK-DAG: [[BUFFER_OFFSET]] = !{!"BufferOffset", i32 0}
; CHECK-DAG: [[POINTER_SIZE]] = !{!"PointerSize", i32 8}
; CHECK-DAG: [[SYMBOL]] = !{!"Symbol", !"a"}
; CHECK-DAG: !{!"inlineProgramScopeOffsets", [[INLINE_PROGRAM_SCOPE_OFFSETS_MAP0:![0-9]*]], [[INLINE_PROGRAM_SCOPE_OFFSETS_VALUE0:![0-9]*]], [[INLINE_PROGRAM_SCOPE_OFFSETS_MAP1:![0-9]*]], [[INLINE_PROGRAM_SCOPE_OFFSETS_VALUE1:![0-9]*]], [[INLINE_PROGRAM_SCOPE_OFFSETS_MAP2:![0-9]*]], [[INLINE_PROGRAM_SCOPE_OFFSETS_VALUE2:![0-9]*]]}
; CHECK-DAG: [[INLINE_PROGRAM_SCOPE_OFFSETS_MAP0]] = !{!"inlineProgramScopeOffsetsMap[0]", [2 x i32] addrspace(2)* @a}
; CHECK-DAG: [[INLINE_PROGRAM_SCOPE_OFFSETS_VALUE0]] = !{!"inlineProgramScopeOffsetsValue[0]", i64 0}
; CHECK-DAG: [[INLINE_PROGRAM_SCOPE_OFFSETS_MAP1]] = !{!"inlineProgramScopeOffsetsMap[1]", i32 addrspace(2)* addrspace(1)* @d}
; CHECK-DAG: [[INLINE_PROGRAM_SCOPE_OFFSETS_VALUE1]] = !{!"inlineProgramScopeOffsetsValue[1]", i64 0}
; CHECK-DAG: [[INLINE_PROGRAM_SCOPE_OFFSETS_MAP2]] = !{!"inlineProgramScopeOffsetsMap[2]", i32 addrspace(1)* @c}
; CHECK-DAG: [[INLINE_PROGRAM_SCOPE_OFFSETS_VALUE2]] = !{!"inlineProgramScopeOffsetsValue[2]", i64 8}
