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

    void CComputeShaderBase::CreateThreadPayloadData(
        void*& pThreadPayload,
        uint& curbeTotalDataLength,
        uint& curbeReadLength,
        bool tileY)
    {
        typedef uint16_t ThreadPayloadEntry;

        // Find the max thread group dimension
        const OctEltUnit SIZE_OF_DQWORD = OctEltUnit(2);
        const OctEltUnit SIZE_OF_OWORD = OctEltUnit(1);
        uint numberOfId = GetNumberOfId();
        uint dimX = numLanes(m_dispatchSize);
        // dimX must align to alignment_X bytes (one GRF)
        uint alignment_X = EltUnit(SIZE_OF_OWORD).Count() * sizeof(DWORD);
        uint dimX_aligned = iSTD::Align(dimX * sizeof(ThreadPayloadEntry), alignment_X) / sizeof(ThreadPayloadEntry);
        uint dimY = (iSTD::Align(m_threadGroupSize, dimX) / dimX) * numberOfId;
        curbeReadLength = dimX_aligned * numberOfId * sizeof(ThreadPayloadEntry) / alignment_X;

        uint alignedVal = EltUnit(SIZE_OF_DQWORD).Count() * sizeof(ThreadPayloadEntry); // Oct Element is 8 Entries
        // m_NOSBufferSize is the additional space for cross-thread constant data (constants set by driver).
        curbeTotalDataLength = iSTD::Align(dimX_aligned * dimY * sizeof(ThreadPayloadEntry) + m_NOSBufferSize, alignedVal);

        IGC_ASSERT_MESSAGE((pThreadPayload == nullptr), "Thread payload should be a null variable");

        unsigned threadPayloadEntries = curbeTotalDataLength / sizeof(ThreadPayloadEntry);

        ThreadPayloadEntry* pThreadPayloadMem = (ThreadPayloadEntry*)IGC::aligned_malloc(threadPayloadEntries * sizeof(ThreadPayloadEntry), 16);
        IGC_ASSERT(nullptr != pThreadPayloadMem);
        std::fill(pThreadPayloadMem, pThreadPayloadMem + threadPayloadEntries, 0);

        pThreadPayload = pThreadPayloadMem;

        // Increase the pointer to per-thread constant data by the number of allocated
        // cross-thread constants.
        pThreadPayloadMem += (m_NOSBufferSize / sizeof(ThreadPayloadEntry));

        uint currThreadX = 0;
        uint currThreadY = 0;
        uint currThreadZ = 0;

        // Current heuristic is trivial, if there are more typed access than untyped access we walk in tile
        // otherwise we walk linearly

        for (uint y = 0; y < dimY; y += numberOfId)
        {
            for (uint x = 0; x < dimX; ++x)
            {
                uint lane = 0;
                if (m_pThread_ID_in_Group_X)
                {
                    pThreadPayloadMem[(y + lane) * dimX_aligned + x] = currThreadX;
                    lane++;
                }
                if (m_pThread_ID_in_Group_Y)
                {
                    pThreadPayloadMem[(y + lane) * dimX_aligned + x] = currThreadY;
                    lane++;
                }
                if (m_pThread_ID_in_Group_Z)
                {
                    pThreadPayloadMem[(y + lane) * dimX_aligned + x] = currThreadZ;
                    lane++;
                }

                if(tileY)
                {
                    const unsigned int tileSizeY = 4;
                    ++currThreadY;

                    if (currThreadY % tileSizeY == 0)
                    {
                        currThreadY -= tileSizeY;
                        ++currThreadX;
                    }

                    if (currThreadX >= m_threadGroupSize_X)
                    {
                        currThreadX = 0;
                        currThreadY += tileSizeY;
                    }

                    if (currThreadY >= m_threadGroupSize_Y)
                    {
                        currThreadY = 0;
                        ++currThreadZ;
                    }

                    if (currThreadZ >= m_threadGroupSize_Z)
                    {
                        currThreadZ = 0;
                    }
                }
                else
                {
                    ++currThreadX;

                    if (currThreadX >= m_threadGroupSize_X)
                    {
                        currThreadX = 0;
                        ++currThreadY;
                    }

                    if (currThreadY >= m_threadGroupSize_Y)
                    {
                        currThreadY = 0;
                        ++currThreadZ;
                    }

                    if (currThreadZ >= m_threadGroupSize_Z)
                    {
                        currThreadZ = 0;
                    }
                }
            }
        }
    }

    CVariable* CComputeShaderBase::CreateThreadIDinGroup(SGVUsage channelNum)
    {
        IGC_ASSERT_MESSAGE((channelNum <= THREAD_ID_IN_GROUP_Z), "Thread id's are in 3 dimensions only");
        IGC_ASSERT_MESSAGE((channelNum >= THREAD_ID_IN_GROUP_X), "Thread id's are in 3 dimensions only");

        switch(channelNum)
        {
        case THREAD_ID_IN_GROUP_X:
            if(m_pThread_ID_in_Group_X == nullptr)
            {
                m_pThread_ID_in_Group_X = GetNewVariable(
                    numLanes(m_SIMDSize), ISA_TYPE_W, getGRFAlignment(), false, m_numberInstance, "threadIdInGroupX");
            }
            return m_pThread_ID_in_Group_X;
        case THREAD_ID_IN_GROUP_Y:
            if(m_pThread_ID_in_Group_Y == nullptr)
            {
                m_pThread_ID_in_Group_Y = GetNewVariable(
                    numLanes(m_SIMDSize), ISA_TYPE_W, getGRFAlignment(), false, m_numberInstance, "threadIdInGroupY");
            }
            return m_pThread_ID_in_Group_Y;
        case THREAD_ID_IN_GROUP_Z:
            if(m_pThread_ID_in_Group_Z == nullptr)
            {
                m_pThread_ID_in_Group_Z = GetNewVariable(
                    numLanes(m_SIMDSize), ISA_TYPE_W, getGRFAlignment(), false, m_numberInstance, "threadIdInGroupZ");
            }
            return m_pThread_ID_in_Group_Z;
        default:
            IGC_ASSERT_MESSAGE(0, "Invalid channel number");
            break;
        }

        return nullptr;
    }

    void CComputeShaderBase::AllocatePerThreadConstantData(uint32_t &offset)
    {
        // Per-thread constant data.
        if (m_pThread_ID_in_Group_X)
        {
            for (uint i = 0; i < m_pThread_ID_in_Group_X->GetNumberInstance(); i++)
            {
                AllocateInput(m_pThread_ID_in_Group_X, offset, i);
                offset += m_pThread_ID_in_Group_X->GetSize();
                offset = iSTD::Round(offset, alignmentSize[m_pThread_ID_in_Group_X->GetAlign()]);
            }
        }

        if (m_pThread_ID_in_Group_Y)
        {
            for (uint i = 0; i < m_pThread_ID_in_Group_Y->GetNumberInstance(); i++)
            {
                AllocateInput(m_pThread_ID_in_Group_Y, offset, i);
                offset += m_pThread_ID_in_Group_Y->GetSize();
                offset = iSTD::Round(offset, alignmentSize[m_pThread_ID_in_Group_Y->GetAlign()]);
            }
        }

        if (m_pThread_ID_in_Group_Z)
        {
            for (uint i = 0; i < m_pThread_ID_in_Group_Z->GetNumberInstance(); i++)
            {
                AllocateInput(m_pThread_ID_in_Group_Z, offset, i);
                offset += m_pThread_ID_in_Group_Z->GetSize();
                offset = iSTD::Round(offset, alignmentSize[m_pThread_ID_in_Group_Z->GetAlign()]);
            }
        }
    }

    uint CComputeShaderBase::GetNumberOfId()
    {
        uint numberIdPushed = 0;

        if (m_pThread_ID_in_Group_X)
        {
            ++numberIdPushed;
        }

        if (m_pThread_ID_in_Group_Y)
        {
            ++numberIdPushed;
        }

        if (m_pThread_ID_in_Group_Z)
        {
            ++numberIdPushed;
        }

        return numberIdPushed;
    }
}
