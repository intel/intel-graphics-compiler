/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

__constant int *__SubDeviceID;
__constant int __MaxHWThreadIDPerSubDevice = 1;

int SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(BuiltInSubDeviceIDINTEL, , )(void) {
    if (__SubDeviceID == NULL) {
        return 0;
    }
    return *__SubDeviceID;
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN_NO_OP(GlobalHWThreadIDINTEL, , )(void) {
    int subDeviceId = SPIRV_BUILTIN_NO_OP(BuiltInSubDeviceIDINTEL, , )();
    return __builtin_IB_hw_thread_id() + subDeviceId * __MaxHWThreadIDPerSubDevice;
}
