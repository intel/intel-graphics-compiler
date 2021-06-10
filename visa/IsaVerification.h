/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <vector>
#include <string>
#include <map>
#include "Common_ISA.h"

//forward declaration
class VISAKernelImpl;

class vISAVerifier
{
    const common_isa_header& isaHeader;
    const print_format_provider_t* header;
    Options* options;

    std::vector<std::string> kerror_list;
    std::vector<std::string> error_list;

    // true -- a label (referred to by its id) is defined in the kernel
    // false -- a label is used in the kernel but not yet defined
    std::map<int, bool> labelDefs;

public:

    vISAVerifier(const common_isa_header& vISAHeader, const print_format_provider_t* kernelHeader, Options* opt) :
    isaHeader(vISAHeader), header(kernelHeader), options(opt) {}

    virtual ~vISAVerifier() = default;

    void run(VISAKernelImpl* kernel);

    bool hasErrors() const { return kerror_list.size() + error_list.size() > 0; }
    size_t getNumErrors() const { return kerror_list.size() + error_list.size(); }

    void writeReport(const char* filename);

 private:

     bool hasFusedEU() const
     {
         bool hasFusedEU = (getGenxPlatform() == GENX_TGLLP || getGenxPlatform() == XE_HP);
         return hasFusedEU;
     }

     void verifyKernelHeader();
     void verifyInstruction(const CISA_INST* inst);

     // checks that can only be done once the whole kernel is processed.
     void finalize();

     void verifyVariableDecl(
         unsigned declID);
     void verifyPredicateDecl(
         unsigned declID);
     void verifyAddressDecl(
         unsigned declID);
     void verifyRegion(
         const CISA_INST* inst,
         unsigned i);
     void verifyRawOperandType(
         const CISA_INST* inst,
         const raw_opnd& opnd,
         bool (*typeFunc)(VISA_Type));
     void verifyRawOperand(
         const CISA_INST* inst, unsigned i);
     void verifyVectorOperand(
         const CISA_INST* inst,
         unsigned i);
     void verifyOperand(
         const CISA_INST* inst,
         unsigned i);
     void verifyInstructionSVM(
         const CISA_INST* inst);
     void verifyInstructionMove(
         const CISA_INST* inst);
     void verifyInstructionSync(
         const CISA_INST* inst);
     void verifyInstructionControlFlow(
         const CISA_INST* inst);
     void verifyInstructionMisc(
         const CISA_INST* inst);
     void verifyInstructionArith(
         const CISA_INST* inst);
     void verifyInstructionLogic(
         const CISA_INST* inst);
     void verifyInstructionCompare(
         const CISA_INST* inst);
     void verifyInstructionAddress(
         const CISA_INST* inst);
     void verifyInstructionSampler(
         const CISA_INST* inst);
     void verifyInstructionSIMDFlow(
         const CISA_INST* inst);
     void verifyInstructionDataport(
         const CISA_INST* inst);
     void verifyKernelAttributes();

     bool checkImmediateIntegerOpnd(
         const vector_opnd& opnd,
         VISA_Type expected_type);

};

