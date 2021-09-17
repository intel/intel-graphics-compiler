/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _G4_VERIFIER_H_
#define _G4_VERIFIER_H_

#include "FlowGraph.h"
#include "G4_IR.hpp"
#include "Optimizer.h"

#include <ostream>
#include <atomic>

namespace vISA
{
class G4Verifier {
public:
    /// Control the verification behavior, dump or assert.
    enum VerifyControl {
        VC_ASSERT,     // Assert on the first violation.
        VC_WARN,       // Warn for each violation.
        VC_NoDump,     // No dump.
        VC_NewDump,    // Enable dump and truncate the dump file if exists.
        VC_AppendDump, // Enable dump and append to the dump file if exists.
    };

private:
    /// Kernel to be verified.
    const G4_Kernel &kernel;

    /// Store all immediate dump data.
    std::ofstream dumpText;

    /// Control the verification behavior.
    VerifyControl verifyCtrl;

    /// Invalid instruction index, useful for setting a break point
    /// based on the hit count.
    static std::atomic<int> index;

    /// pass for which the verifier is called after
    const Optimizer::PassIndex passIndex;

public:
    G4Verifier(G4_Kernel &k, VerifyControl ctrl, Optimizer::PassIndex index);

    ~G4Verifier()
    {
        if (dumpText.is_open())
            dumpText.close();
    }

    /// Verification dispatcher.
    void verify();

    /// Verify a single instruction.
    bool verifyInst(G4_INST *inst);

private:
    /// Check whether def-use/use-def chains are valid.
    bool verifyDefUseChain(G4_INST *inst);

    void printDefUse(G4_INST *def, G4_INST *use, Gen4_Operand_Number pos);
    void printDefUseImpl(std::ostream &os, G4_INST *def, G4_INST *use,
                         Gen4_Operand_Number pos);

    /// Assert if this is a violation and verification control is VC_ASSERT.
    void assertIfEnable() const;

    bool dataHazardCheck(G4_Operand* opnd1, G4_Operand* opnd2);

    void verifyOpcode(G4_INST* inst);

    void verifyOpnd(G4_Operand* opnd, G4_INST* inst);

    void verifySend(G4_INST* inst);

    void verifyDstSrcOverlap(G4_INST* inst);

    void verifyDpas(G4_INST* inst);
    void verifyAccMov(G4_INST* inst);
    void verifyBFMixedMode(G4_INST* inst);

};
}
/// Interface.
void verifyG4Kernel(vISA::G4_Kernel &kernel, vISA::Optimizer::PassIndex index, bool alwaysOn,
                vISA::G4Verifier::VerifyControl ctrl = vISA::G4Verifier::VC_ASSERT);


void verifyG4Inst(vISA::G4_Kernel &kernel, vISA::G4_INST *inst, vISA::Optimizer::PassIndex index);


#endif
