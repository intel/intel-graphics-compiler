/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <stdint.h>

namespace iOpenCL
{
//
// The layout of the (IGC) program debug data is as follows:
//
/*
    --------------------------------------------------------------------------
   |   SProgramDebugDataHeaderIGC:                                            |
    --------------------------------------------------------------------------
   |    Program Kernel Data Table:                                            |
   |    (All kernels have debug data entries in here. If kernel has no debug  |
   |     info the debug info size will be zero.)                              |
   |     --->   (IGC) Program Kernel Data 1                                   |
   |     --->   ...                                                           |
   |     --->   (IGC) Program Kernel Data n                                   |
    --------------------------------------------------------------------------
*/

//
// The layout of the (IGC) kernel data is as follows:
//
/*
    --------------------------------------------------------------------------
   |   (IGC) Program Kernel Data:                                             |
    --------------------------------------------------------------------------
   |   SKernelDebugDataHeaderIGC:                                             |
   |    (All kernels have debug data entries in here. If kernel has no debug  |
   |     info the debug info size will be zero.)                              |
    --------------------------------------------------------------------------
   |   Kernel name:                                                           |
   |    (NULL terminated string aligned on sizeof(DWORD).)                    |
    --------------------------------------------------------------------------
   |   VISA debug info:                                                       |
    --------------------------------------------------------------------------
   |   GenISA debug info:                                                     |
    --------------------------------------------------------------------------
*/

/*****************************************************************************\
STRUCT: SProgramDebugDataHeaderIGC
\*****************************************************************************/
typedef struct _SProgramDebugDataHeaderIGC
{
    uint32_t         Magic;
    uint32_t         Version;
    uint32_t         Size;
    uint32_t         Device;
    uint32_t         SteppingId;
    uint32_t         GPUPointerSizeInBytes;
    uint32_t         NumberOfKernels;
} SProgramDebugDataHeaderIGC;


/*****************************************************************************\
STRUCT: SKernelDebugDataHeaderIGC
\*****************************************************************************/
typedef struct _SKernelDebugDataHeaderIGC
{
    uint32_t         KernelNameSize;
    uint32_t         SizeVisaDbgInBytes;
    uint32_t         SizeGenIsaDbgInBytes;
} SKernelDebugDataHeaderIGC;


//
// The layout of the program debug data is as follows:
//
/*
    --------------------------------------------------------------------------
   |   SProgramDebugDataHeader:                                               |
    --------------------------------------------------------------------------
   |   Program String Table:                                                  |
   |   (This is a sequence of null-terminated strings. The first set of       |
   |    strings correspond to the directory table entries appearing in order. |
   |    The second set of strings correspond to the directory table entries   |
   |    appearing in order. The third set of strings correspond to the kernel |
   |    names appearing in order. (Note not all kernels may have associated   |
   |    debug data. The debug data reader must use the KernelIndex field in   |
   |    SKernelDebugDataHeader to check if debug data exist for a kernel that |
   |    is present in kernel binary data).                                    |
    --------------------------------------------------------------------------
   |    Program Directory Table:                                              |
   |     --->    SProgramDebugDataDirTableHeader                              |
   |             (Its string entries appear in order in the string table.)    |
    --------------------------------------------------------------------------
   |    Program File Table:                                                   |
   |     --->    SProgramDebugDataFileTableHeader                             |
   |             (Its string entries appear in order in the string table.)    |
   |     --->    SProgramDebugDataFileTableEntry 1                            |
   |     --->    ...                                                          |
   |     --->    SProgramDebugDataFileTableEntry n                            |
    --------------------------------------------------------------------------
   |    Program Kernel Data Table:                                            |
   |    (Only kernels that have debug data have entries in here. The          |
   |     KernelIndex field is used to specfy the kernel whose debug data      |
   |     appears here.)                                                       |
   |     --->   Program Kernel Data 1                                         |
   |     --->   ...                                                           |
   |     --->   Program Kernel Data n                                         |
    --------------------------------------------------------------------------
*/

//
// The layout of the kernel data is as follows:
//
/*
    --------------------------------------------------------------------------
   |   Program Kernel Data:                                                   |
    --------------------------------------------------------------------------
   |   SKernelDebugDataHeader:                                                |
   |    (Only kernels that have debug data have entries in here. The          |
   |     KernelIndex field is used to specfy the kernel whose debug data      |
   |     appears here.)                                                       |
    --------------------------------------------------------------------------
   |   SKernelDebugDataLineTableHeader:                                       |
    --------------------------------------------------------------------------
   |   SKernelDebugDataLineTableEntry 1:                                      |
    --------------------------------------------------------------------------
   |   ...                                                                    |
    --------------------------------------------------------------------------
   |   SKernelDebugDataLineTableEntry n:                                      |
    --------------------------------------------------------------------------
*/

/*****************************************************************************\
STRUCT: SProgramDebugDataHeader
\*****************************************************************************/
typedef struct _SProgramDebugDataHeader
{
    uint32_t         Magic;
    uint32_t         Version;
    uint32_t         Size;
    uint32_t         StringTableSize;
    uint32_t         DirTableSize;
    uint32_t         FileTableSize;
    uint32_t         Device;
    uint32_t         SteppingId;
    uint32_t         GPUPointerSizeInBytes;
    uint32_t         NumberOfKernels;
    uint32_t         NumberOfKernelsWithDebugData;
} SProgramDebugDataHeader;

/*****************************************************************************\
STRUCT: SProgramProgramDebugDataDirTableHeader
\*****************************************************************************/
typedef struct _SProgramDebugDataDirTableHeader
{
    uint32_t         NumberOfDirs;
} SProgramDebugDataDirTableHeader;

/*****************************************************************************\
STRUCT: SProgramProgramDebugDataFileTableHeader
\*****************************************************************************/
typedef struct _SProgramDebugDataFileTableHeader
{
    uint32_t         NumberOfFiles;
} SProgramDebugDataFileTableHeader;

/*****************************************************************************\
STRUCT: SProgramDebugDataFileTableEntry
\*****************************************************************************/
typedef struct _SProgramDebugDataFileTableEntry
{
    uint32_t         DirIndex;
} SProgramDebugDataFileTableEntry;

/*****************************************************************************\
STRUCT: SKernelDebugDataHeader
\*****************************************************************************/
typedef struct _SKernelDebugDataHeader
{
    uint32_t         KernelIndex;
    uint32_t         CodeOffset;
    uint32_t         Size;
} SKernelDebugDataHeader;

/*****************************************************************************\
STRUCT: SKernelDebugDataLineTableHeader
\*****************************************************************************/
typedef struct _SKernelDebugDataLineTableHeader
{
    uint32_t         NumberOfEntries;
} SKernelDebugDataLineTableHeader;

/*****************************************************************************\
STRUCT: SKernelDebugDataLineTableEntry
\*****************************************************************************/
typedef struct _SKernelDebugDataLineTableEntry
{
    uint32_t         Offset;
    uint32_t         LineColumnNumber;
    uint32_t         FileIndex;
} SKernelDebugDataLineTableEntry;

}; // iOpenCL
