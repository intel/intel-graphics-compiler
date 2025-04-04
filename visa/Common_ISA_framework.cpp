/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Common_ISA_framework.h"
#include "Common_ISA.h"
#include "Common_ISA_util.h"
#include "JitterDataStruct.h"
#include "VISAKernel.h"
#include "visa_igc_common_header.h"
#include <fstream>
#include <regex>
#include <sstream>

#include "IsaDisassembly.h"
#include "IsaVerification.h"

namespace CisaFramework {

int CisaInst::createCisaInstruction(ISA_Opcode opcode, unsigned char exec_size,
                                    unsigned char modifier, PredicateOpnd pred,
                                    VISA_opnd **opnd, int numOpnds,
                                    const VISA_INST_Desc *inst_desc,
                                    vISAVerifier *verifier) {
  uint8_t subOpcode = 0;
  bool hasSubOpcode = false;
  int descOpndCount = inst_desc->opnd_num;
  vASSERT(descOpndCount <= MAX_OPNDS_PER_INST);

  for (int i = 0; i < descOpndCount; i++) {
    if (inst_desc->opnd_desc[i].opnd_type == OPND_SUBOPCODE) {
      descOpndCount +=
          inst_desc->getSubInstDesc((uint8_t)opnd[i]->_opnd.other_opnd)
              .opnd_num;
      hasSubOpcode = true;
      subOpcode = static_cast<uint8_t>(opnd[0]->_opnd.other_opnd);
      break;
    }
  }

  // TODO: move the FIXME below above this if statment and
  //      int implicitOperands = 0;
  //      for (...) // parent op descriptors
  //          if (type == EXEC_SIZE || type == PRED)
  //             implicitOperands++;
  //      for (...) // subop op descriptors
  //          if (type == EXEC_SIZE || type == PRED)
  //              implicitOperands++;
  // TODO: rename numOpnds to explicitOpnds to be clear
  // TODO: rename descOpndCount to descOpndsToEncodeCount
  //
  // compare (descOpndCount != explicitOpnds + implicitOperands)
  //    mismatch assert
  if (opcode != ISA_FCALL) {
    // != doens't work here because predication and exec size are treated
    // differently; it might be descOpndCount == numOpnds (+ HAS_EXEC) (+
    // HAS_PRED) if (descOpndCount != numOpnds)
    if (descOpndCount < numOpnds) {
      std::string msg = "Number of operands mismatch between CISA instruction "
                        "description and value passed in.";
      std::cerr << msg << ": " << descOpndCount << " " << numOpnds << "\n";
      vISA_ASSERT(false, msg);
    }
  }

  m_inst_desc = inst_desc;
  m_cisa_instruction.opnd_num = numOpnds;
  m_cisa_instruction.opcode = opcode;
  m_cisa_instruction.execsize = exec_size;
  m_cisa_instruction.modifier = modifier;
  m_cisa_instruction.pred = pred;
  m_cisa_instruction.opnd_array =
      (VISA_opnd **)m_mem.alloc(sizeof(VISA_opnd *) * numOpnds);
  std::copy_n(opnd, numOpnds, m_cisa_instruction.opnd_array);
  m_cisa_instruction.isa_type = inst_desc->type;

  // FIXME: this is a mess and needs to be cleaned up
  //   I think this comment refers to merging the subopcode and opcode case
  //   that is we're hunting for EXEC_SIZE and PRED operands in both the
  //   suboperand and parent
  for (int i = 0; i < descOpndCount; i++) {
    if (inst_desc->opnd_desc[i].opnd_type == OPND_EXECSIZE ||
        inst_desc->opnd_desc[i].opnd_type == OPND_PRED) {
      m_size += static_cast<short>(
          Get_VISA_Type_Size((VISA_Type)inst_desc->opnd_desc[i].data_type));
    }
  }
  if (hasSubOpcode) {
    int numOpnd = inst_desc->getSubInstDesc(subOpcode).opnd_num;
    for (int i = 0; i < numOpnd; i++) {
      OpndDesc desc = inst_desc->getSubInstDesc(subOpcode).opnd_desc[i];
      if (desc.opnd_type == OPND_EXECSIZE || desc.opnd_type == OPND_PRED) {
        m_size +=
            static_cast<short>(Get_VISA_Type_Size((VISA_Type)desc.data_type));
      }
    }
  }

  for (int i = 0; i < numOpnds; i++) {
    if (opnd[i] == NULL) {
      vASSERT(false);
      std::cerr << "ONE OF THE OPERANDS IS NULL!\n";
      return VISA_FAILURE;
    }
    m_size += opnd[i]->size;
  }

#if defined(_DEBUG) || defined(_INTERNAL)
  if (verifier) {
    int status = verifier->verifyInstruction(&m_cisa_instruction);
    if (status == VISA_FAILURE) {
      std::cerr << verifier->getLastErrorFound().value_or("");
      vASSERT(false);
      return VISA_FAILURE;
    }
  }
#endif

  return VISA_SUCCESS;
}

bool allowDump(const Options &options, const std::string &fullPath) {
  const char *regex = options.getOptionCstr(vISA_ShaderDumpRegexFilter);
  if (!regex || *regex == '\0')
    return true;

  std::regex fileRegex(regex);

  const auto fileNameIdx = fullPath.find_last_of("/\\");
  if (std::string::npos != fileNameIdx)
  {
      const auto fileNamePos = 1 + fileNameIdx;
      const auto fileName = fullPath.substr(fileNamePos, fullPath.size() - fileNamePos);

      return std::regex_search(fileName, fileRegex);
  }
  else
  {
      // Do NOT dump if there's no prefix path.
      return false;
  }
}

} // namespace CisaFramework

// repKernel is usually the same as kernel except for when doing Pixel Shader
// code patching, in which case the kernel is the payload prologue while the
// repKernel has the actual shader body.
std::string CISA_IR_Builder::isaDump(const VISAKernelImpl *kernel,
                                     const VISAKernelImpl *repKernel,
                                     bool printVersion) const {
  std::stringstream sstr;
  VISAKernel_format_provider header(repKernel);

  sstr << header.printKernelHeader(printVersion);

  auto inst_iter = kernel->getInstructionListBegin();
  auto inst_iter_end = kernel->getInstructionListEnd();
  for (; inst_iter != inst_iter_end; inst_iter++) {
    CisaFramework::CisaInst *cisa_inst = *inst_iter;
    const CISA_INST *inst = cisa_inst->getCISAInst();
    // FIXME: This seems very suspicious that inst is coming from kernel but
    // repKernel (which is used to look up variable name) is potentially a
    // different kernel. Have we verified that .visaasm dump works correctly for
    // PS code patching?
    sstr << header.printInstruction(inst, kernel->getOptions()) << "\n";
  }

#ifdef DLL_MODE
  // Print the options used to compile this vISA object to assist debugging.
  if (kernel->getCISABuilder()->getBuilderOption() == VISA_BUILDER_BOTH) {
    sstr
        << "\n//Platform: "
        << kernel->getCISABuilder()->getPlatformInfo()->getGenxPlatformString();
    sstr << "\n//Build option: \"" << kernel->getOptions()->getFullArgString()
         << "\"";
  }
#endif

  return sstr.str();
}

int CISA_IR_Builder::isaDump(const char *combinedIsaasmName) const {
#ifdef IS_RELEASE_DLL
  return VISA_SUCCESS;
#else
  const bool isaasmToConsole = m_options.getOption(vISA_ISAASMToConsole);
  const bool genIsaasm = m_options.getOption(vISA_GenerateISAASM);
  const bool allowIsaasmDump = combinedIsaasmName && combinedIsaasmName[0] != '\0' &&
      CisaFramework::allowDump(m_options, combinedIsaasmName);
  const bool genCombinedIsaasm =
    m_options.getOption(vISA_GenerateCombinedISAASM) &&
    (isaasmToConsole || allowIsaasmDump);
  if (!genIsaasm && !genCombinedIsaasm)
    return VISA_SUCCESS;

  std::stringstream ss;

  VISAKernelImpl *mainKernel = *kernel_begin();
  for (auto kTempIt = kernel_begin(); kTempIt != kernel_end(); ++kTempIt) {
    std::stringstream asmName;
    VISAKernelImpl *kTemp = *kTempIt;
    if (kTemp->getIsKernel()) {
      mainKernel = kTemp;
      asmName << kTemp->getOutputAsmPath();
    } else if (kTemp->getIsFunction()) {
      // function 0 has kernel_f0.visaasm
      unsigned funcId = 0;
      kTemp->GetFunctionId(funcId);
      if (mainKernel) {
        asmName << mainKernel->getOutputAsmPath();
      } else {
        // No mainKernel, use the function name instead
        asmName << kTemp->getName();
      }
      asmName << "_f";
      asmName << funcId;
    } else {
      vASSERT(kTemp->getIsPayload());
      asmName << mainKernel->getOutputAsmPath();
      asmName << "_payload";
    }
    asmName << ".visaasm";
    std::string asmFileName = sanitizePathString(asmName.str());

    if (!CisaFramework::allowDump(m_options, asmFileName))
      continue;

    VISAKernelImpl *repKernel = kTemp->getIsPayload() ? mainKernel : kTemp;
    if (genCombinedIsaasm) {
      // only print version when emitting the first kernel/function in the
      // output.
      ss << isaDump(kTemp, repKernel, !ss.rdbuf()->in_avail());
      vISA_ASSERT(!ss.fail(), "Failed to write combined CISA ASM to file");
    }
    if (genIsaasm) {
      if (isaasmToConsole) {
        std::cout << isaDump(kTemp, repKernel);
      } else {
        std::ofstream out(asmFileName);
        out << isaDump(kTemp, repKernel);
      }
    }
  }

  if (genCombinedIsaasm) {
    if (isaasmToConsole) {
      std::cout << ss.rdbuf();
    } else {
      vASSERT(combinedIsaasmName);
      std::ofstream out(combinedIsaasmName);
      out << ss.rdbuf();
    }
  }
  // Return early exit if emitting isaasm to console.
  return isaasmToConsole ? VISA_EARLY_EXIT : VISA_SUCCESS;
#endif // IS_RELEASE_DLL
}

// It's unfortunate we have to define vISAVerifier functions here due to the
// #include mess..
void vISAVerifier::run(VISAKernelImpl *kernel) {
  irBuilder = kernel->getIRBuilder();
  verifyKernelHeader();
  for (auto iter = kernel->getInstructionListBegin(),
            iterEnd = kernel->getInstructionListEnd();
       iter != iterEnd; ++iter) {
    auto inst = (*iter)->getCISAInst();
    verifyInstruction(inst);
  }
  finalize();
}
