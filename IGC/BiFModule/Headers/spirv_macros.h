/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __SPIRV_MACROS_H__
#define __SPIRV_MACROS_H__

#if !defined(__USE_KHRONOS_SPIRV_TRANSLATOR__)
#define SPIRV_OVERLOADABLE
#define SPIRV_BUILTIN(opcode, old_mangling, new_mangling) \
    __builtin_spirv_Op##opcode##old_mangling
#define SPIRV_BUILTIN_NO_OP(opcode, old_mangling, new_mangling) \
    __builtin_spirv_##opcode##old_mangling
#define SPIRV_OCL_BUILTIN(func, old_mangling, new_mangling) \
    __builtin_spirv_OpenCL_##func##old_mangling
#else
#define SPIRV_OVERLOADABLE __attribute__((overloadable))
#define SPIRV_BUILTIN(opcode, old_mangling, new_mangling) \
    __spirv_##opcode##new_mangling
#define SPIRV_BUILTIN_NO_OP(opcode, old_mangling, new_mangling) \
    __spirv_##opcode##new_mangling
#define SPIRV_OCL_BUILTIN(func, old_mangling, new_mangling) \
    __spirv_ocl_##func##new_mangling
#endif

#endif // __SPIRV_MACROS_H__