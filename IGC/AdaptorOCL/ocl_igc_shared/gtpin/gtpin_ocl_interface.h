/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _GPTIN_OCL_INTERFACE_
#define _GPTIN_OCL_INTERFACE_

#include <stdint.h>
#include "gtpin_driver_common_bti.h"

namespace gtpin
{
namespace ocl
{
/*!
* GTPin <-> OpenCL Driver Interface Changelog:
* 2. File reordering
* 1. IGC info added
* 0. Official release of OpenCL support
*/
// GTPin <-> OpenCL driver interface version
static const uint8_t GTPIN_OCL_INTERNAL_INTERFACE_VERSION = 2; /// GTPIN INTERNAL VALUE
static const uint16_t GTPIN_OCL_INTERFACE_VERSION = GENERATE_SPECIFIC_VERSION(GTPIN_OCL_INTERNAL_INTERFACE_VERSION);

/************************************************************************/
/* General Comments:
1.   All memory management is done by the caller.
The callee should not make any assumptions about the availability
of the data out side the scope of the call.
The callee must copy all data that will be used out side
the scope of the call.
*/
/************************************************************************/


/************************************************************************/
/* Events (Driver -> GTPin)                                             */
/* The following functions are implemented by GTPin                     */
/* and called by the driver                                             */
/************************************************************************/

/**
Called up on a creation of a new Device by the application

Params:
(in)  context       - The context created.
(in)  platform_info - information about the platform
(out) igc_init      - IGC's initialization data
*/
typedef void(*OnContextCreateFPTR)(device_handle_t handle, platform_info_t* platform_info, igc_init_t** igc_init);

/**
Called up on a destruction of the device

Params:
(in) context - The context been destroyed.
**/
typedef void(*OnContextDestroyFPTR)(device_handle_t handle);

/**
Called on up the creation of a new shader by the application

Params:
(in) context        -  The handle of the context
(in)  params_in     -  All instrumentation parameters needed for GTPin's instrumentation
(out) params_out    -  GTPin's instrumentation artifacts
**/
typedef void(*OnKernelCreateFPTR)(device_handle_t handle, const instrument_params_in_t* params_in, instrument_params_out_t* params_out);

/**
Called up on applications call to the EnqueueNDRange API.
The driver shall call this function for each shader in the pipeline (PS, VS, ...)
and is submitted into the command buffer

Params:
(in)  cb            -  Driver's internal command buffer handle
(in)  kernel_id     -  Kernel's id as assigned by GTPin in KernelCreate function
(out) entry_offset  -  The entry point in the binary relative to the first instruction
(out) resource      -  GTPin's resource for this kernel
**/
typedef void(*OnKernelSubmitFPTR)(command_buffer_handle_t cb, uint64_t kernel_id, uint32_t* entry_offset, resource_handle_t* resource);

/**
Called up on the creation of driver's internal command buffer

Params:
(in) context -  The handle of the context
(in) cb      -  Driver's internal command buffer handle
**/
typedef void(*OnCommandBufferCreateFPTR)(device_handle_t handle, command_buffer_handle_t cb);

/**
Called up on the submissions for execution driver's internal command buffer

Params:
(in) cb         -  Driver's internal command buffer handle
(out) resource  -  GTPin's resource for polling on to identify command buffer completion
**/
typedef void(*OnCommandBufferCompleteFPTR)(command_buffer_handle_t cb);


/**
Event structure:
Holds the pointers for functions implemented by GTPin.
Those function will be called by the Driver upon a corresponding events.
*/
typedef struct gtpin_events_s
{
    OnContextCreateFPTR         onContextCreate;
    OnContextDestroyFPTR        onContextDestroy;
    OnKernelCreateFPTR          onKernelCreate;
    OnKernelSubmitFPTR          onKernelSubmit;
    OnCommandBufferCreateFPTR   onCommandBufferCreate;
    OnCommandBufferCompleteFPTR onCommandBufferComplete;

} gtpin_events_t;

/**
Initialize GTPin events and driver services structures
This is the entry point into the driver for GTPin and used for the handshake.

This function must be:
1. implemented with the name of GTPin_Init
2. exported and visible outside the driver's image
3. un-mangled with extern "C"

Params:
(in)   gtpin_events     - GTPin's events functions callbacks (if NULL ignored)
(out) driver_services   - Driver's services function callbacks (if NULL ignored)
(out) driver_version    - Driver's version of the interface (if NULL ignored)
*/

//extern "C"
//GTPIN_DI_STATUS GTPin_Init(gtpin::ocl::gtpin_events_t* gtpin_events, driver_services_t* driver_services,
//                           interface_version_t* driver_version);

} // namespace ocl
} // namespace gtpin
#endif
