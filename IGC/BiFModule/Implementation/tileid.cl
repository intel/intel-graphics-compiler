/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

extern int *__SubDeviceID = NULL;
extern int *__MaxHWThreadIDPerSubDevice = NULL;

int SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubDeviceIDINTEL, , )(void) {
    if (__SubDeviceID == NULL) {
        return 0;
    }
    return *__SubDeviceID;
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInMaxHWThreadIDPerSubDeviceINTEL, , )(void) {
    if (__MaxHWThreadIDPerSubDevice == NULL) {
        return 1;
    }
    return *__MaxHWThreadIDPerSubDevice;
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInHWThreadIDINTEL, , )(void) {
    int subDeviceId = SPIRV_BUILTIN_NO_OP(BuiltInSubDeviceIDINTEL, , )();
    int maxHWThreadsPerSubDevice = SPIRV_BUILTIN_NO_OP(BuiltInMaxHWThreadIDPerSubDeviceINTEL, , )();
    return __builtin_IB_hw_thread_id() + subDeviceId * maxHWThreadsPerSubDevice;
}
