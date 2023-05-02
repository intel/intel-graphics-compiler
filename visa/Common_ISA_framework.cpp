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

#define SIZE_VALUE m_kernel_data_size
#define SIZE_VALUE_INST m_instruction_size

namespace CisaFramework {

int CisaInst::createCisaInstruction(ISA_Opcode opcode, unsigned char exec_size,
                                    unsigned char modifier, PredicateOpnd pred,
                                    VISA_opnd **opnd, int numOpnds,
                                    const VISA_INST_Desc *inst_desc,
                                    vISAVerifier *verifier) {
  uint8_t subOpcode = 0;
  bool hasSubOpcode = false;
  int descOpndCount = inst_desc->opnd_num;

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

CisaBinary::CisaBinary(CISA_IR_Builder *builder)
    : m_mem(4096), m_header_size(0), m_total_size(0),
      m_bytes_written_cisa_buffer(0), m_header_buffer(NULL),
      m_options(builder->getOptions()), parent(builder) {
  memset(&m_header, 0, sizeof(common_isa_header));
}

void CisaBinary::initKernel(int kernelIndex, VISAKernelImpl *kernel) {
  unsigned functionIndex = 0; // separating function and kernel index
  vISA_ASSERT(kernelIndex <
                       (m_upper_bound_kernels + m_upper_bound_functions) &&
                   kernelIndex >= 0,
               "Invalid kernelIndex in CisaBinary initialization.\n");

  int nameLen = (int)std::string_view(kernel->getName()).size();

  if (this->getMajorVersion()) {
    if (kernel->getIsKernel())
      kernelIndex = m_header.num_kernels++;
    else {
      m_header.num_functions++;
      kernel->GetFunctionId(functionIndex);
    }
  }

  if (kernel->getIsKernel()) {
    m_header.kernels[kernelIndex].name_len = (unsigned short)nameLen;
    m_header.kernels[kernelIndex].name = (char *)m_mem.alloc(nameLen + 1);
    m_header.kernels[kernelIndex].name[nameLen] = 0;
    memcpy_s(m_header.kernels[kernelIndex].name,
             m_header.kernels[kernelIndex].name_len, kernel->getName(),
             m_header.kernels[kernelIndex].name_len);

    m_header.kernels[kernelIndex].offset =
        0; // will be set later during finalize
    m_header.kernels[kernelIndex].size = kernel->getCisaBinarySize();
    m_header.kernels[kernelIndex].cisa_binary_buffer =
        kernel->getCisaBinaryBuffer(); // buffer containing entire kernel
    m_header.kernels[kernelIndex].input_offset =
        kernel->getInputOffset(); // for now relative to the beginning of the
                                  // kernel object

    // Workaround for patching in FE. This way space for data structures is
    // allocated
    unsigned int numGenBinariesWillBePatched =
        kernel->getOptions()->getuInt32Option(vISA_NumGenBinariesWillBePatched);
    m_header.kernels[kernelIndex].num_gen_binaries =
        static_cast<unsigned char>(numGenBinariesWillBePatched);
    m_header.kernels[kernelIndex].gen_binaries = (gen_binary_info *)m_mem.alloc(
        sizeof(gen_binary_info) *
        (int)m_header.kernels[kernelIndex].num_gen_binaries);

    for (int i = 0; i < m_header.kernels[kernelIndex].num_gen_binaries; i++) {
      m_header.kernels[kernelIndex].gen_binaries[i].binary_offset = 0;
      m_header.kernels[kernelIndex].gen_binaries[i].binary_offset = 0;
      m_header.kernels[kernelIndex].gen_binaries[i].platform = 0;
    }
  }

  if (!kernel->getIsKernel()) {
    m_header.functions[functionIndex].linkage = 0; // deprecated and MBZ
    m_header.functions[functionIndex].name_len = (unsigned short)nameLen;
    m_header.functions[functionIndex].name = (char *)m_mem.alloc(nameLen + 1);
    memcpy_s(m_header.functions[functionIndex].name,
             m_header.functions[functionIndex].name_len, kernel->getName(),
             m_header.functions[functionIndex].name_len);
    m_header.functions[functionIndex].name[nameLen] = 0;
    m_header.functions[functionIndex].offset =
        0; // will be set later during finalize
    m_header.functions[functionIndex].size = kernel->getCisaBinarySize();
    m_header.functions[functionIndex].cisa_binary_buffer =
        kernel->getCisaBinaryBuffer(); // buffer containing entire kernel
  }
}

unsigned long CisaBinary::writeInToCisaHeaderBuffer(const void *value,
                                                    int size) {
  vISA_ASSERT(m_bytes_written_cisa_buffer + size <= m_header_size,
               "Size of CISA instructions header buffer is exceeded.");

  memcpy_s(&m_header_buffer[m_bytes_written_cisa_buffer], size, value, size);
  m_bytes_written_cisa_buffer += size;

  return m_bytes_written_cisa_buffer;
}

/*
    calculates total size of the header and sets all the kernel offsets from the
   beginning of the file
*/
int CisaBinary::finalizeCisaBinary() {
  m_bytes_written_cisa_buffer = 0;

  m_total_size = m_header_size = m_header.getSizeInBinary();

  m_header_buffer = (char *)this->m_mem.alloc(m_header_size);
  memset(m_header_buffer, 0, m_header_size);

  writeInToCisaHeaderBuffer(&m_header.magic_number,
                            sizeof(m_header.magic_number));

  // we must use the latest vISA version since the internal data structures
  // always use it
  uint8_t major = COMMON_ISA_MAJOR_VER;
  uint8_t minor = COMMON_ISA_MINOR_VER;
  writeInToCisaHeaderBuffer(&major, sizeof(uint8_t));
  writeInToCisaHeaderBuffer(&minor, sizeof(uint8_t));

  writeInToCisaHeaderBuffer(&m_header.num_kernels,
                            sizeof(m_header.num_kernels));

  for (int i = 0; i < m_header.num_kernels; i++) {
    writeInToCisaHeaderBuffer(&m_header.kernels[i].name_len,
                              sizeof(m_header.kernels[i].name_len));
    writeInToCisaHeaderBuffer(m_header.kernels[i].name,
                              m_header.kernels[i].name_len);

    // setting offset to the compiled cisa binary kernel
    // to correct offset in final cisa binary
    m_header.kernels[i].offset = m_total_size;
    // for patching later if genx binary is generated
    this->m_kernelOffsetLocationsArray[i] = m_bytes_written_cisa_buffer;
    writeInToCisaHeaderBuffer(&m_header.kernels[i].offset,
                              sizeof(m_header.kernels[i].offset));

    writeInToCisaHeaderBuffer(&m_header.kernels[i].size,
                              sizeof(m_header.kernels[i].size));

    // this was originally set to relative offset from the
    // compiled cisa binary kernel
    // changing it to absoute offset from the beginning of the
    // cisa binary
    m_header.kernels[i].input_offset += m_total_size;
    this->m_kernelInputOffsetLocationsArray[i] = m_bytes_written_cisa_buffer;
    writeInToCisaHeaderBuffer(&m_header.kernels[i].input_offset,
                              sizeof(m_header.kernels[i].input_offset));

    // setting offset to the compiled genx binary kernel
    // to correct offset in final cisa binary
    m_total_size += m_header.kernels[i].size;
    m_total_size += m_header.kernels[i].binary_size;

    vISA_ASSERT(m_header.kernels[i].variable_reloc_symtab.num_syms == 0,
           "variable relocation not supported");
    writeInToCisaHeaderBuffer(
        &m_header.kernels[i].variable_reloc_symtab.num_syms,
        sizeof(m_header.kernels[i].variable_reloc_symtab.num_syms));

    vISA_ASSERT(m_header.kernels[i].function_reloc_symtab.num_syms == 0,
           "function relocation not supported");
    writeInToCisaHeaderBuffer(
        &m_header.kernels[i].function_reloc_symtab.num_syms,
        sizeof(m_header.kernels[i].function_reloc_symtab.num_syms));

    // for now gen binaries
    this->m_krenelBinaryInfoLocationsArray[i] = m_bytes_written_cisa_buffer;
    writeInToCisaHeaderBuffer(&m_header.kernels[i].num_gen_binaries,
                              sizeof(m_header.kernels[i].num_gen_binaries));
    for (int j = 0; j < m_header.kernels[i].num_gen_binaries; j++) {
      m_header.kernels[i].gen_binaries[j].binary_offset = m_total_size;
      m_header.kernels[i].gen_binaries[j].platform = 0;
      m_header.kernels[i].gen_binaries[j].binary_size = 0;
      writeInToCisaHeaderBuffer(
          &m_header.kernels[i].gen_binaries[j].platform,
          sizeof(m_header.kernels[i].gen_binaries[j].platform));
      writeInToCisaHeaderBuffer(
          &m_header.kernels[i].gen_binaries[j].binary_offset,
          sizeof(m_header.kernels[i].gen_binaries[j].binary_offset));
      writeInToCisaHeaderBuffer(
          &m_header.kernels[i].gen_binaries[j].binary_size,
          sizeof(m_header.kernels[i].gen_binaries[j].binary_size));

      m_total_size += m_header.kernels[i].gen_binaries[j].binary_size;
    }
  }

  // file-scope variables are no longer supported
  uint16_t numFileScopeVariables = 0;
  writeInToCisaHeaderBuffer(&numFileScopeVariables,
                            sizeof(numFileScopeVariables));

  writeInToCisaHeaderBuffer(&m_header.num_functions,
                            sizeof(m_header.num_functions));

  for (int i = 0; i < m_header.num_functions; i++) {
    writeInToCisaHeaderBuffer(&m_header.functions[i].linkage,
                              sizeof(m_header.functions[i].linkage));
    writeInToCisaHeaderBuffer(&m_header.functions[i].name_len,
                              sizeof(m_header.functions[i].name_len));
    writeInToCisaHeaderBuffer(m_header.functions[i].name,
                              m_header.functions[i].name_len);

    // setting offset to the compiled cisa binary kernel
    // to correct offset in final cisa binary
    m_header.functions[i].offset = m_total_size;
    this->m_functionOffsetLocationsArray[i] = this->m_bytes_written_cisa_buffer;
    writeInToCisaHeaderBuffer(&m_header.functions[i].offset,
                              sizeof(m_header.functions[i].offset));

    writeInToCisaHeaderBuffer(&m_header.functions[i].size,
                              sizeof(m_header.functions[i].size));

    vISA_ASSERT(m_header.functions[i].variable_reloc_symtab.num_syms == 0,
           "variable relocation not supported");
    writeInToCisaHeaderBuffer(
        &m_header.functions[i].variable_reloc_symtab.num_syms,
        sizeof(m_header.functions[i].variable_reloc_symtab.num_syms));

    vISA_ASSERT(m_header.functions[i].function_reloc_symtab.num_syms == 0,
           "function relocation not supported");
    writeInToCisaHeaderBuffer(
        &m_header.functions[i].function_reloc_symtab.num_syms,
        sizeof(m_header.functions[i].function_reloc_symtab.num_syms));

    m_total_size += m_header.functions[i].size;
  }

  return VISA_SUCCESS;
}

int CisaBinary::dumpToStream(std::ostream *os) {
  os->write(this->m_header_buffer, this->m_header_size);

  for (int i = 0; i < m_header.num_kernels; i++) {
    os->write(m_header.kernels[i].cisa_binary_buffer, m_header.kernels[i].size);
    os->write(m_header.kernels[i].genx_binary_buffer,
              m_header.kernels[i].binary_size);
  }

  for (int i = 0; i < m_header.num_functions; i++) {
    os->write(m_header.functions[i].cisa_binary_buffer,
              m_header.functions[i].size);
  }
  return VISA_SUCCESS;
}

int CisaBinary::dumpToFile(std::string binFileName) {
  if (binFileName == "") {
    binFileName = "temp.isa";
  }
  std::ofstream os(binFileName.c_str(), std::ios::binary | std::ios::out);
  if (!os) {
    std::cerr << binFileName << ": unable to open output file\n";
    return VISA_FAILURE;
  }
  int result = dumpToStream(&os);
  os.close();
  return result;
}

void CisaBinary::patchKernel(int index, unsigned int genxBufferSize,
                             void *buffer, int platform) {
  int copySize = 0;
  m_header.kernels[index].offset += genxBinariesSize;
  copySize = sizeof(m_header.kernels[index].offset);
  memcpy_s(&m_header_buffer[this->m_kernelOffsetLocationsArray[index]],
           copySize, &m_header.kernels[index].offset, copySize);

  m_header.kernels[index].input_offset += genxBinariesSize;
  copySize = sizeof(m_header.kernels[index].input_offset);
  memcpy_s(&m_header_buffer[this->m_kernelInputOffsetLocationsArray[index]],
           copySize, &m_header.kernels[index].input_offset, copySize);

  int offsetGenBinary = this->m_krenelBinaryInfoLocationsArray[index];
  m_header.kernels[index].num_gen_binaries = 1;
  copySize = sizeof(m_header.kernels[index].num_gen_binaries);
  memcpy_s(&m_header_buffer[offsetGenBinary], copySize,
           &m_header.kernels[index].num_gen_binaries, copySize);
  offsetGenBinary += sizeof(m_header.kernels[index].num_gen_binaries);

  m_header.kernels[index].gen_binaries[0].platform =
      static_cast<unsigned char>(platform);
  copySize = sizeof(m_header.kernels[index].gen_binaries[0].platform);
  memcpy_s(&m_header_buffer[offsetGenBinary], copySize,
           &m_header.kernels[index].gen_binaries[0].platform, copySize);
  offsetGenBinary += sizeof(m_header.kernels[index].gen_binaries[0].platform);

  m_header.kernels[index].gen_binaries[0].binary_offset += genxBinariesSize;
  copySize = sizeof(m_header.kernels[index].gen_binaries[0].binary_offset);
  memcpy_s(&m_header_buffer[offsetGenBinary], copySize,
           &m_header.kernels[index].gen_binaries[0].binary_offset, copySize);
  offsetGenBinary +=
      sizeof(m_header.kernels[index].gen_binaries[0].binary_offset);

  m_header.kernels[index].gen_binaries[0].binary_size = genxBufferSize;
  copySize = sizeof(m_header.kernels[index].gen_binaries[0].binary_size);
  memcpy_s(&m_header_buffer[offsetGenBinary], copySize,
           &m_header.kernels[index].gen_binaries[0].binary_size, copySize);
  offsetGenBinary +=
      sizeof(m_header.kernels[index].gen_binaries[0].binary_size);

  m_header.kernels[index].genx_binary_buffer = (char *)buffer;
  m_header.kernels[index].binary_size = genxBufferSize;
  this->genxBinariesSize += genxBufferSize;
}

void CisaBinary::patchFunctionWithGenBinary(int index,
                                            unsigned int genxBufferSize,
                                            char *buffer) {
  m_header.functions[index].offset += genxBinariesSize;
  size_t copySize = sizeof(m_header.functions[index].offset);
  memcpy_s(&m_header_buffer[this->m_functionOffsetLocationsArray[index]],
           copySize, &m_header.functions[index].offset, copySize);

  m_header.functions[index].genx_binary_buffer = buffer;

  this->genxBinariesSize += genxBufferSize;
}

void CisaBinary::patchFunction(int index, unsigned genxBufferSize) {
  m_header.functions[index].offset += genxBinariesSize;
  size_t copySize = sizeof(m_header.functions[index].offset);
  memcpy_s(&m_header_buffer[this->m_functionOffsetLocationsArray[index]],
           copySize, &m_header.functions[index].offset, copySize);

  this->genxBinariesSize += genxBufferSize;
}

bool allowDump(const Options &options, const std::string &fileName) {
  const char *regex = options.getOptionCstr(vISA_ShaderDumpFilter);
  if (!regex || *regex == '\0')
    return true;

  std::regex fileRegex(regex);

  return std::regex_search(fileName, fileRegex);
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
    sstr << printInstruction(m_header, &header, inst, kernel->getOptions())
         << "\n";
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
  const bool genIsaasm = m_options.getOption(vISA_GenerateISAASM);
  const bool genCombinedIsaasm =
      m_options.getOption(vISA_GenerateCombinedISAASM);
  if (!genIsaasm && !genCombinedIsaasm)
    return VISA_SUCCESS;

  const bool isaasmToConsole = m_options.getOption(vISA_ISAASMToConsole);
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

    // Do we still want to support regex for file dump filters when writing
    // a combined isaasm file?
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
        vISA_ASSERT(!out.fail(), "Failed to write CISA ASM to file");
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
      vISA_ASSERT(!out.fail(), "Failed to write combined CISA ASM to file");
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
