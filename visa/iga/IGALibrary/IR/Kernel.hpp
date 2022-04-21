/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_KERNEL_HPP
#define _IGA_KERNEL_HPP
// WARNING: the IR is subject to change without any notice.  External tools
// should use the official interfaces in the external API.  Those interfaces
// are tested between releases and maintained even with changes to the IR.

#include "../ErrorHandler.hpp"
#include "../MemManager/MemManager.hpp"
#include "../MemManager/StdArenaAllocator.hpp"
#include "../Models/Models.hpp"
#include "Block.hpp"
#include "Instruction.hpp"

#include <list>

namespace iga {
    typedef std::list<
        iga::Block*, std_arena_based_allocator<iga::Block*>> BlockList;

    class Kernel
    {
    public:
        Kernel(const Model &model);
        ~Kernel();
        // disabling copy constructor to prevent problems with
        // shallow copy and mem manager
        Kernel(const Kernel &) = delete;
        Kernel& operator=(const Kernel&) = delete;

        MemManager&       getMemManager() { return m_mem; }
        const Model&      getModel() const { return m_model; }
        const BlockList&  getBlockList() const { return m_blocks; }
        BlockList&        getBlockList() { return m_blocks; }
        size_t            getInstructionCount() const;

        ///////////////////////////////////////////////////////////////////////
        // Kernel construction API's
        ///////////////////////////////////////////////////////////////////////

        // Creates a block at a given PC.  The block must be appended to
        // the kernel.
        Block *createBlock();
        void appendBlock(Block *blk);

        // Instruction constructors, the instruction returned must be appended
        // to a block or some other storage
        Instruction *createBasicInstruction(
            const OpSpec& op,
            const Predication& pred,
            const RegRef& freg,
            ExecSize execSize,
            ChannelOffset choff,
            MaskCtrl mc,
            FlagModifier fm,
            Subfunction sf); // InvalidFC::INVALID;

        Instruction *createBranchInstruction(
            const OpSpec& op,
            const Predication& predOpnd,
            const RegRef& flagReg,
            ExecSize execSize,
            ChannelOffset choff,
            MaskCtrl ectr,
            Subfunction sf);

        Instruction *createSendInstruction(
            const OpSpec& op,
            SFID sfid,
            const Predication& predOpnd,
            const RegRef& flagReg,
            ExecSize execSize,
            ChannelOffset choff,
            MaskCtrl ectr,
            const SendDesc &extDesc,
            const SendDesc &msgDesc);


        Instruction *createNopInstruction();
        Instruction *createIllegalInstruction();
        Instruction *createSyncNopInstruction(SWSB sw);
        Instruction *createSyncAllRdInstruction(SWSB sw);
        Instruction *createSyncAllWrInstruction(SWSB sw);
    private:
        const Model&                      m_model;
        MemManager                        m_mem;

        BlockList                         m_blocks;
    };
} // namespace

#endif
