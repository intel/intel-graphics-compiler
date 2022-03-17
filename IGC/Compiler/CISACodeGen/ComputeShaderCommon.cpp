/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/ComputeShaderCommon.hpp"

using namespace llvm;

namespace IGC
{
    CComputeShaderCommon::CComputeShaderCommon(Function* pFunc, CShaderProgram* pProgram)
        : CComputeShaderBase(pFunc, pProgram) {}

    CComputeShaderCommon::~CComputeShaderCommon() {}

    void CComputeShaderCommon::CreateThreadPayloadData(
        void*& pThreadPayload,
        uint& curbeTotalDataLength,
        uint& curbeReadLength,
        ThreadIDLayout layout) const
    {
        typedef uint16_t ThreadPayloadEntry;

        // Find the max thread group dimension
        uint numberOfId = GetNumberOfId();
        uint dimX = numLanes(m_dispatchSize);
        // dimX must align to alignment_X bytes (one GRF)
        uint alignment_X = getGRFSize();
        uint dimX_aligned = iSTD::Align(dimX * sizeof(ThreadPayloadEntry), alignment_X) / sizeof(ThreadPayloadEntry);
        uint dimY = (iSTD::Align(m_threadGroupSize, dimX) / dimX) * numberOfId;
        curbeReadLength = dimX_aligned * numberOfId * sizeof(ThreadPayloadEntry) / alignment_X;

        // m_NOSBufferSize is the additional space for cross-thread constant data (constants set by driver).
        curbeTotalDataLength = iSTD::Align(dimX_aligned * dimY * sizeof(ThreadPayloadEntry) + m_NOSBufferSize, getGRFSize());

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

                if(layout == ThreadIDLayout::TileY)
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
                else if (layout == ThreadIDLayout::X)
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
                else if (layout == ThreadIDLayout::QuadTile)
                {
                    const unsigned int tileSizeX = 2;
                    const unsigned int tileSizeY = 2;
                    ++currThreadX;

                    if (currThreadX % tileSizeX == 0)
                    {
                        ++currThreadY;
                    }

                    if ((currThreadX % tileSizeX == 0) &&
                        (currThreadY % tileSizeY == 0))
                    {
                        currThreadY -= tileSizeY;
                    }
                    else if (currThreadX % tileSizeX == 0)
                    {
                        currThreadX -= tileSizeX;
                    }

                    if (currThreadX >= m_threadGroupSize_X)
                    {
                        currThreadX = 0;
                        currThreadY += tileSizeY;
                    }

                    if (currThreadY >= m_threadGroupSize_Y)
                    {
                        currThreadX = 0;
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
                    IGC_ASSERT_MESSAGE(0, "unhandled layout!");
                }
            }
        }
    }

    CVariable* CComputeShaderCommon::CreateThreadIDinGroup(SGVUsage channelNum)
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

    void CComputeShaderCommon::AllocatePerThreadConstantData(uint32_t &offset)
    {
        // Per-thread constant data.
        if (m_pThread_ID_in_Group_X)
        {
            for (uint i = 0; i < m_pThread_ID_in_Group_X->GetNumberInstance(); i++)
            {
                AllocateInput(m_pThread_ID_in_Group_X, offset, i);
                offset += m_pThread_ID_in_Group_X->GetSize();
                offset = iSTD::Round(offset, 1u << m_pThread_ID_in_Group_X->GetAlign());
            }
        }

        if (m_pThread_ID_in_Group_Y)
        {
            for (uint i = 0; i < m_pThread_ID_in_Group_Y->GetNumberInstance(); i++)
            {
                AllocateInput(m_pThread_ID_in_Group_Y, offset, i);
                offset += m_pThread_ID_in_Group_Y->GetSize();
                offset = iSTD::Round(offset, 1u << m_pThread_ID_in_Group_Y->GetAlign());
            }
        }

        if (m_pThread_ID_in_Group_Z)
        {
            for (uint i = 0; i < m_pThread_ID_in_Group_Z->GetNumberInstance(); i++)
            {
                AllocateInput(m_pThread_ID_in_Group_Z, offset, i);
                offset += m_pThread_ID_in_Group_Z->GetSize();
                offset = iSTD::Round(offset, 1u << m_pThread_ID_in_Group_Z->GetAlign());
            }
        }
    }

    uint CComputeShaderCommon::GetNumberOfId() const
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
