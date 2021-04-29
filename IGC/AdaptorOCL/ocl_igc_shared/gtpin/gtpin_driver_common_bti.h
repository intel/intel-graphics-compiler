/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef GPTIN_DRIVER_COMMON_BTI_H
#define GPTIN_DRIVER_COMMON_BTI_H

#include <stdint.h>

#include "gtpin_driver_common.h"


/************************************************************************/
/* Data Types                                                           */
/************************************************************************/
namespace gtpin
{
/*!
* GTPin <-> Driver Interface Changelog:
* 1. File reordering
*/
static const uint8_t GTPIN_COMMON_BTI_INTERFACE_VERSION = 1;

#define GENERATE_SPECIFIC_VERSION(PLATFORM_INTERFACE_VERSION) ((GTPIN_COMMON_BTI_INTERFACE_VERSION << 8) | PLATFORM_INTERFACE_VERSION)

typedef struct command_buffer_handle_s* command_buffer_handle_t; /// driver's handle to the command buffer


/*!
* Resource addressing mode
*/
typedef enum
{
    GTPIN_BUFFER_BINDFULL,  /// using binding table index
    GTPIN_BUFFER_BINDLESS,  /// using an offset to the surface table in a register
    GTPIN_BUFFER_STATELESS, /// using an address to the resource in a register
} GTPIN_BUFFER_TYPE;


/*!
* The offest of the register in the GRF.
* e.g. r2.5(dw) will be represented as 2*256 + 5*4
*/
typedef struct reg_desc_s
{
    uint32_t    reg_offset; /// the location of the register in the GRF (in bytes)
    uint32_t    size;       /// size of the register in bytes
} reg_desc_t;


/*!
* Buffer's descriptor - could be:
* 1. BTI - binding table index (in bindfull buffer addressing)
* 2. register (in bindless / stateless buffer addressing)
*/
typedef union buffer_desc_s
{
    uint32_t        BTI;      /// Binding table index
    reg_desc_t      reg_desc;
} buffer_desc_t;


/*!
* kernel instrumentation parameters structure:
*/
typedef struct instrument_params_s
{
    GTPIN_KERNEL_TYPE   kernel_type;
    GTPIN_SIMD_WIDTH    simd;
    const uint8_t*      orig_kernel_binary; /// the original kernel binary
    uint32_t            orig_kernel_size;   /// size of the kernel binary in bytes

    GTPIN_BUFFER_TYPE   buffer_type;
    buffer_desc_t       buffer_desc;
    uint64_t            igc_hash_id;

    char*               kernel_name;        /// the kernel name
    const igc_info_t*   igc_info;           /// information form IGC

    /// START Exists only from COMMON_SUPPORTED_FEATURE_SOURCELINE_MAPPING
    const void*         debug_data;         /// debug data including the elf file
    uint32_t            debug_data_size;    /// size of the elf file in bytes
    /// End Exists only from COMMON_SUPPORTED_FEATURE_SOURCELINE_MAPPING
} instrument_params_in_t;


/*!
* kernel instrumented data structure:
*/
typedef struct instrument_params_out_s
{
    uint8_t*            inst_kernel_binary; /// the instrumented binary
    uint32_t            inst_kernel_size;   /// size in bytes on the instrumented binary
    uint64_t            kernel_id;          /// GTPin's associated kernel id
} instrument_params_out_t;


/*!
* Allocate a buffer(resource) for GTPin
*
* @param[in]  deviceHandle  The handle to the device
* @param[in]  size          Size of the buffer to allocate
* @param[out] resource      The handle to the created resource
*/
typedef GTPIN_DI_STATUS(GTPIN_DRIVER_CALLCONV *BufferAllocateFPTR)(device_handle_t deviceHandle, uint32_t size, resource_handle_t* resource);


/*!
* Deallocate GTPin's buffer
*
* @param[in] deviceHandle   The handle to the device
* @param[in] resource       The handle to the resource
*/
typedef GTPIN_DI_STATUS(GTPIN_DRIVER_CALLCONV *BufferDeallocateFPTR)(device_handle_t deviceHandle, resource_handle_t resource);

/*!
* Map GTPin's buffer to obtain the virtual address
*
* @param[in]  deviceHandle  The handle to the device
* @param[in]  resource      The handle to the resource
* @param[out] address       The virtual address of the resource
*/
typedef GTPIN_DI_STATUS(GTPIN_DRIVER_CALLCONV *BufferMapFPTR)(device_handle_t deviceHandle, resource_handle_t resource, uint8_t** address);

/*!
* UnMap GTPin's allocated buffer
*
* @param[in] deviceHandle   The handle to the device
* @param[in] resource       The handle to the resource
*/
typedef GTPIN_DI_STATUS(GTPIN_DRIVER_CALLCONV *BufferUnMapFPTR)(device_handle_t deviceHandle, resource_handle_t resource);


/************************************************************************/
/* Services (GTPin -> Driver)                                           */
/* The following functions are implemented by the driver                */
/* and called by GTPin                                                  */
/************************************************************************/

typedef struct driver_services_s
{
    BufferAllocateFPTR       bufferAllocate;   /// request the Driver to allocate a buffer
    BufferDeallocateFPTR     bufferDeallocate; /// request the Driver to de-allocate a buffer
    BufferMapFPTR            bufferMap;        /// request the Driver to map a buffer
    BufferUnMapFPTR          bufferUnMap;      /// request the Driver to unmap a buffer
} driver_services_t;

} /// namespace gtpin

#endif /// GPTIN_DRIVER_COMMON_BTI_H
