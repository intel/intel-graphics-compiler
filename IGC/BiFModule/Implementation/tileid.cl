/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

extern int __SubDeviceID;

int SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubDeviceIDINTEL, , )(void) {

    // When __SubDeviceID is declared as an extern int,
    // it is lowered to LLVM-IR like:
    // @__SubDeviceID = external addrspace(1) global i32, align 4
    // This global address is being then patched by the runtime
    // and can be set to null when implicit scaling is disabled.
    // One may wonder why __SubDeviceID is not declared as an extern int*
    // In this case this would end up as a pointer to pointer in LLVM-IR.
    // It would generate two loads and wouldn't be consistent with runtime behavior.
    volatile int* p = &__SubDeviceID;
    if (p == NULL) {
        return 0;
    }
    return *p;
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(GlobalHWThreadIDINTEL, , )(void) {
    int subDeviceId = SPIRV_BUILTIN_NO_OP(BuiltInSubDeviceIDINTEL, , )();
    return __builtin_IB_hw_thread_id() + subDeviceId * BIF_FLAG_CTRL_GET(MaxHWThreadIDPerSubDevice);
}
