/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#ifndef _GPTIN_DRIVER_COMMON_
#define _GPTIN_DRIVER_COMMON_

#include <stdint.h>

#ifdef _WIN32
#define GTPIN_DRIVER_CALLCONV __fastcall
#else
#define GTPIN_DRIVER_CALLCONV
#endif

/************************************************************************/
/* Data Types                                                           */
/************************************************************************/
namespace gtpin
{

// GTPin <-> Driver interface version

static const uint16_t GTPIN_COMMON_INTERFACE_VERSION = 3;

/*
 * Common Interface Changelog:
 * 3. Added command_buffer_handle_s for OpenCL support
 * 2. Added GTPIN_DRIVER_CALLCONV
 * 1. First introduction of GTPIN_COMMON_INTERFACE_VERSION, addition of debug_data,
 * 0. Not supported
 */

typedef struct resource_handle_s*       resource_handle_t;          // driver's handle to the resource
typedef struct context_handle_s*        context_handle_t;           // driver's handle to the context/device
typedef struct command_buffer_handle_s* command_buffer_handle_t;    // driver's handle to the command buffer
typedef struct igc_init_s               igc_init_t;                 // info passed by IGC
typedef struct igc_info_s               igc_info_t;                 // info passed by IGC

typedef struct interface_version_s
{
    uint16_t    specific;
    uint16_t    common;
} interface_version_t;

/**
Gen Version
*/
typedef enum
{
    GTPIN_GEN_INVALID,
    GTPIN_GEN_8,
    GTPIN_GEN_9,
    GTPIN_GEN_10,
    GTPIN_GEN_11
} GTPIN_GEN_VERSION;

typedef enum
{
    GTPIN_DI_SUCCESS = 0,
    GTPIN_DI_ERROR_INVALID_ARGUMENT,
    GTPIN_DI_ERROR_NO_INSTANCE,                     // no instance of GTPin inside the driver
    GTPIN_DI_ERROR_INSTANCE_ALREADY_CREATED,        // GTPin was already initialized in the driver
    GTPIN_DI_ERROR_ALLOCATION_FAILED,               // failed to allocate a buffer
    GTPIN_DI_ERROR_NO_VIRTUAL_ADDRESS_TO_BUFFER,    // failed to obtain virtual address to the buffer
} GTPIN_DI_STATUS;

typedef enum
{
    GTPIN_KERNEL_TYPE_INVALID,
    GTPIN_KERNEL_TYPE_HS,       // Hull Shader
    GTPIN_KERNEL_TYPE_DS,       // Domain Shader
    GTPIN_KERNEL_TYPE_VS,       // Vertex Shader
    GTPIN_KERNEL_TYPE_PS,       // Pixel Shader
    GTPIN_KERNEL_TYPE_CS,       // Compute Shader (GPGPU)
    GTPIN_KERNEL_TYPE_GS        // Geometry Shader
} GTPIN_KERNEL_TYPE;

typedef enum
{
    GTPIN_SIMD_INVALID,
    GTPIN_SIMD_4x2  = 4,
    GTPIN_SIMD_8    = 8,
    GTPIN_SIMD_16   = 16,
    GTPIN_SIMD_32   = 32
} GTPIN_SIMD_WIDTH;


/**
Resource addressing mode
*/
typedef enum
{
    GTPIN_BUFFER_BINDFULL,     // using binding table index
    GTPIN_BUFFER_BINDLESS,     // using an offset to the surface table in a register
    GTPIN_BUFFER_STATELESS,    // using an address to the resource in a register
} GTPIN_BUFFER_TYPE;

/**
Platform info structure
*/
typedef struct platform_info_s
{
    GTPIN_GEN_VERSION   gen_version;
    uint32_t            device_id;

} platform_info_t;

/**
The offest of the register in the GRF.
e.g. r2.5(dw) will be represented as 2*256 + 5*4
*/
typedef struct reg_desc_s
{
    uint32_t    reg_offset;     // the location of the register in the GRF (in bytes)
    uint32_t    size;     // size of the register in bytes
} reg_desc_t;

/**
Buffer's descriptor - could be:
1. BTI - binding table index (in bindfull buffer addressing)
2. register (in bindless / stateless buffer addressing)
*/
typedef union buffer_desc_s
{
    uint32_t        BTI;
    reg_desc_t      reg_desc;
} buffer_desc_t;


/**
kernel instrumentation parameters structure:
*/
typedef struct instrument_params_s
{
    GTPIN_KERNEL_TYPE   kernel_type;
    GTPIN_SIMD_WIDTH    simd;
    const uint8_t*      orig_kernel_binary;     // the original kernel binary
    uint32_t            orig_kernel_size;       // size of the kernel binary in bytes

    GTPIN_BUFFER_TYPE   buffer_type;
    buffer_desc_t       buffer_desc;
    uint64_t            igc_hash_id;

    char*               kernel_name;            // the kernel name
    const igc_info_t*   igc_info;               // information form IGC

    // START Exists only from COMMON_SUPPORTED_FEATURE_SOURCELINE_MAPPING
    const void*         debug_data;             // debug data including the elf file
    uint32_t            debug_data_size;        // size of the elf file in bytes
    // End Exists only from COMMON_SUPPORTED_FEATURE_SOURCELINE_MAPPING
} instrument_params_in_t;


/**
kernel instrumented data structure:
*/
typedef struct instrument_params_out_s
{
    uint8_t*            inst_kernel_binary;     // the instrumented binary
    uint32_t            inst_kernel_size;       // size in bytes on the instrumented binary
    uint64_t            kernel_id;              // GTPin's associated kernel id
} instrument_params_out_t;


/**
Allocate a buffer(resource) for GTPin

Params:
(in)  context   - The handle to the context
(in)  size      - Size of the buffer to allocate
(out) resource  - The handle to the created resource
*/
typedef GTPIN_DI_STATUS(GTPIN_DRIVER_CALLCONV *BufferAllocateFPTR)(context_handle_t context, uint32_t size, resource_handle_t* resource);

/**
Deallocate GTPin's buffer

Params:
(in)  context   - The handle to the context
(in) resource   - The handle to the resource
*/
typedef GTPIN_DI_STATUS(GTPIN_DRIVER_CALLCONV *BufferDeallocateFPTR)(context_handle_t context, resource_handle_t resource);

/**
Map GTPin's buffer to obtain the virtual address
Params:

(in)  context   - The handle to the context
(in)  resource  - The handle to the resource
(out) address   - The virtual address of the resource

*/
typedef GTPIN_DI_STATUS(GTPIN_DRIVER_CALLCONV *BufferMapFPTR)(context_handle_t context, resource_handle_t resource, uint8_t** address);

/**
UnMap GTPin's allocated buffer

Params:
(in)  context   - The handle to the context
(in)  resource  - The handle to the resource

*/
typedef GTPIN_DI_STATUS(GTPIN_DRIVER_CALLCONV *BufferUnMapFPTR)(context_handle_t context, resource_handle_t resource);


/************************************************************************/
/* Services (GTPin -> Driver)                                           */
/* The following functions are implemented by the driver                */
/* and called by GTPin                                                  */
/************************************************************************/

typedef struct driver_services_s
{
    BufferAllocateFPTR       bufferAllocate;     // request the Driver to allocate a buffer
    BufferDeallocateFPTR     bufferDeallocate;   // request the Driver to de-allocate a buffer
    BufferMapFPTR            bufferMap;          // request the Driver to map a buffer
    BufferUnMapFPTR          bufferUnMap;        // request the Driver to unmap a buffer

} driver_services_t;

inline uint32_t GenerateDriverInterfaceVersion(uint16_t common_version, uint16_t specific_version)
{
    return (common_version << 16) | specific_version;
}

}
#endif
