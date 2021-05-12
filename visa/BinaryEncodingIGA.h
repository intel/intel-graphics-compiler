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

#ifndef _BINARYENCODINGIGA_H_
#define _BINARYENCODINGIGA_H_

#include "G4_IR.hpp"
#include "FlowGraph.h"
#include "iga/IGALibrary/IR/Types.hpp"

#include <string>

namespace vISA {
    // NOTE: IGA internals should be minimally leaked into vISA/G4

    // Encodes a G4_Kernel via the IGA assembler
    struct EncodeResult {
        void *binary;
        size_t binaryLen;
        std::string error; // in case of error
    };

    EncodeResult EncodeKernelIGA(
        vISA::Mem_Manager &m,
        vISA::G4_Kernel& k,
        const std::string &fname);


    iga::SWSB_ENCODE_MODE GetIGASWSBEncodeMode(const IR_Builder& builder);

    bool InstSupportsSaturationIGA(TARGET_PLATFORM p, const G4_INST &i, const IR_Builder& builder);
    bool InstSupportsSrcModifierIGA(TARGET_PLATFORM p, const G4_INST &i, const IR_Builder& builder);



    ///////////////////////////////////////////////////////////////////////////
    // TODO: remove these in step 2
    // const iga::Model *GetModelIGA(TARGET_PLATFORM p);
    //
    // std::pair<const iga::OpSpec*,iga::Subfunction> GetOpInfoIGA(
    //    const G4_INST *inst, iga::Platform p, bool allowUnknownOp);
} // vISA::

#endif //_BINARYENCODINGIGA_H_
