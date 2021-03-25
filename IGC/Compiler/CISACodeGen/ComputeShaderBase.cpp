/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/ScaledNumber.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/ComputeShaderBase.hpp"
#include "Compiler/CISACodeGen/messageEncoding.hpp"
#include "common/allocator.h"
#include "common/secure_mem.h"
#include <iStdLib/utility.h>
#include <algorithm>
#include "Probe/Assertion.h"

using namespace llvm;

namespace IGC
{
    CComputeShaderBase::CComputeShaderBase(llvm::Function* pFunc, CShaderProgram* pProgram)
        : CShader(pFunc, pProgram) {}

    CComputeShaderBase::~CComputeShaderBase() {}

    void CComputeShaderBase::selectWalkOrder(
        bool useLinearWalk,
        uint numberOfTypedAccess,
        uint numberOfUntypedAccess,
        uint threadGroupSize_X,
        uint threadGroupSize_Y,
        uint threadGroupSize_Z)
    {
        const CodeGenContext* pCtx = GetContext();

        if (pCtx->getModuleMetaData()->csInfo.neededThreadIdLayout == ThreadIDLayout::QuadTile)
        {
            m_ThreadIDLayout = ThreadIDLayout::QuadTile;
            return;
        }

        if ((numberOfTypedAccess >= numberOfUntypedAccess) &&
            threadGroupSize_Y % 4 == 0 &&
            !pCtx->getModuleMetaData()->csInfo.disableLocalIdOrderOptimizations &&
            IGC_IS_FLAG_ENABLED(UseTiledCSThreadOrder)) {
            m_ThreadIDLayout = ThreadIDLayout::TileY;
            m_walkOrder = WO_YXZ;
        }

        bool needsLinearWalk =
            pCtx->getModuleMetaData()->csInfo.neededThreadIdLayout == ThreadIDLayout::X;
        if (needsLinearWalk)
        {
            m_ThreadIDLayout = ThreadIDLayout::X;
            m_walkOrder = WO_XYZ;
        }
    }
}
