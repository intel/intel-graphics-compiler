/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef GPTIN_DRIVER_COMMON_H
#define GPTIN_DRIVER_COMMON_H

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
/// GTPin <-> Driver interface version

static const uint16_t GTPIN_COMMON_INTERFACE_VERSION = 6;


/*!
 * Common Interface Changelog:
 * 6. Added back context_handle_t for backward compatibility
 * 5. Added driver interface
 * 4. Removed GenerateDriverInterfaceVersion function
 * 3. Added command_buffer_handle_s for OpenCL support
 * 2. Added GTPIN_DRIVER_CALLCONV
 * 1. First introduction of GTPIN_COMMON_INTERFACE_VERSION, addition of debug_data,
 * 0. Not supported
 */


typedef struct resource_handle_s*       resource_handle_t; /// driver's handle to the resource
typedef struct device_handle_s*         device_handle_t;   /// driver's handle to the device
typedef device_handle_t                 context_handle_t;  /// for backward compatibility
typedef struct igc_init_s               igc_init_t;        /// info passed by IGC
typedef struct igc_info_s               igc_info_t;        /// info passed by IGC


typedef struct interface_version_s
{
    uint16_t    specific;
    uint16_t    common;
} interface_version_t;


/**
 * Gen Version
 */
typedef enum
{
    GTPIN_GEN_INVALID,
    GTPIN_GEN_8,
    GTPIN_GEN_9,
    GTPIN_GEN_10,
    GTPIN_GEN_11,
    GTPIN_GEN_12_1,
    GTPIN_XEHP_CORE,
    GTPIN_XE_HPG_CORE,
    GTPIN_XE_HPC_CORE = GTPIN_XE_HPG_CORE,
} GTPIN_GEN_VERSION;


/*!
 * Possible results of GTPin driver interface functions
 */
typedef enum
{
    GTPIN_DI_SUCCESS = 0,                           /// operation is successful
    GTPIN_DI_ERROR_INVALID_ARGUMENT,                /// one of the arguments is invalid (null)
    GTPIN_DI_ERROR_NO_INSTANCE,                     /// no instance of GTPin inside the driver
    GTPIN_DI_ERROR_INSTANCE_ALREADY_CREATED,        /// GTPin was already initialized in the driver
    GTPIN_DI_ERROR_ALLOCATION_FAILED,               /// failed to allocate a buffer
    GTPIN_DI_ERROR_NO_VIRTUAL_ADDRESS_TO_BUFFER,    /// failed to obtain virtual address to the buffer
    GTPIN_DI_FAIL,                                  /// general failure
} GTPIN_DI_STATUS;


/*!
 * Kernel types
 */
typedef enum
{
    GTPIN_KERNEL_TYPE_INVALID,
    GTPIN_KERNEL_TYPE_HS,      /// Hull Shader
    GTPIN_KERNEL_TYPE_DS,      /// Domain Shader
    GTPIN_KERNEL_TYPE_VS,      /// Vertex Shader
    GTPIN_KERNEL_TYPE_PS,      /// Pixel Shader
    GTPIN_KERNEL_TYPE_CS,      /// Compute Shader (GPGPU)
    GTPIN_KERNEL_TYPE_GS       /// Geometry Shader
} GTPIN_KERNEL_TYPE;


/*!
 * SIMD widths
 */
typedef enum
{
    GTPIN_SIMD_INVALID,
    GTPIN_SIMD_4x2  = 4,
    GTPIN_SIMD_8    = 8,
    GTPIN_SIMD_16   = 16,
    GTPIN_SIMD_32   = 32
} GTPIN_SIMD_WIDTH;


/*!
 * GPU interfaces
 */
typedef enum
{
    GFX_OPENCL,
    GFX_CM,
} GFX_TARGET;


/*!
 * Platform information
 */
typedef struct platform_info_s
{
    GTPIN_GEN_VERSION   gen_version;
    uint32_t            device_id;
} platform_info_t;


/*!
 * Driver information
 */
typedef struct driver_info_s
{
    interface_version_t  version;
    GFX_TARGET           gfx_target;
} driver_info_t;


}
#endif /// GPTIN_DRIVER_COMMON_H
