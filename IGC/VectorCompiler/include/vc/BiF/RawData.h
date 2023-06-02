/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_BIF_RAW_DATA_H
#define VC_BIF_RAW_DATA_H

#include <llvm/ADT/StringRef.h>

extern unsigned char VCBiFPrintfOCL32RawData[];
extern unsigned int VCBiFPrintfOCL32RawData_size;

extern unsigned char VCBiFPrintfOCL64RawData[];
extern unsigned int VCBiFPrintfOCL64RawData_size;

extern unsigned char VCBiFPrintfZE32RawData[];
extern unsigned int VCBiFPrintfZE32RawData_size;

extern unsigned char VCBiFPrintfZE64RawData[];
extern unsigned int VCBiFPrintfZE64RawData_size;

extern unsigned char VCSPIRVBuiltins64RawData[];
extern unsigned int VCSPIRVBuiltins64RawData_size;

extern llvm::StringRef getVCEmulation64RawDataImpl(llvm::StringRef CPUStr);

#endif
