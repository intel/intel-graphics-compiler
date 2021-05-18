/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef LIB_GENXCODEGEN_TARGETINFO_GENXTARGETINFO_H
#define LIB_GENXCODEGEN_TARGETINFO_GENXTARGETINFO_H

namespace llvm {

class Target;

Target &getTheGenXTarget32();
Target &getTheGenXTarget64();

}

#endif
