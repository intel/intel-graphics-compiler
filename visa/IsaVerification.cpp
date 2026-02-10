/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IS_RELEASE_DLL

#include "Assertions.h"
#include "Common_ISA.h"
#include "Common_ISA_util.h"

#include "IsaVerification.h"

#include "Attributes.hpp"
#include "IsaDisassembly.h"
#include "PreDefinedVars.h"
#include "VISAKernel.h"

#include "G4_IR.hpp"
#include "G4_Kernel.hpp"
/* stdio.h portability code start */
#ifdef _WIN32
#else
#define _scprintf(...) snprintf(NULL, 0, __VA_ARGS__)
#endif

#include <iostream>
#include <limits>
#include <sstream>
#include <string>

using namespace vISA;

#define REPORT_HEADER(opt, cond, ...)                                          \
  do                                                                           \
    if (!(cond)) {                                                             \
      int sz = _scprintf(__VA_ARGS__) + 1;                                     \
      char *buf = (char *)malloc(sz);                                          \
      vASSERT(buf);                                                            \
      memset(buf, 0, sz);                                                      \
      sprintf_s(buf, sz, __VA_ARGS__);                                         \
      error_list.push_back(                                                    \
          createIsaError(header, std::string(buf), opt));                      \
      free(buf);                                                               \
    }                                                                          \
  while (0)

#define REPORT_INSTRUCTION(opt, cond, ...)                                     \
  do                                                                           \
    if (!(cond)) {                                                             \
      int sz = _scprintf(__VA_ARGS__) + 1;                                     \
      char *buf = (char *)malloc(sz);                                          \
      vASSERT(buf);                                                            \
      memset(buf, 0, sz);                                                      \
      sprintf_s(buf, sz, __VA_ARGS__);                                         \
      error_list.push_back(                                                    \
          createIsaError(header, std::string(buf), opt, inst));                \
      free(buf);                                                               \
    }                                                                          \
  while (0)

#define COMMON_ISA_MAX_SURFACE_SIZE 128
#define COMMON_ISA_MAX_SAMPLER_SIZE 128
#define COMMON_ISA_MAX_MEDIA_BLOCK_WIDTH_BDW_PLUS 64
#define COMMON_ISA_MAX_MEDIA_BLOCK_HEIGHT 64

std::string raw_opnd::toString() const {
  std::stringstream sstr;
  sstr << "V" << index;
  if (offset != 0) {
    sstr << "." << offset;
  }
  return sstr.str();
}

void vISAVerifier::writeReport(const char *filename) {
  if (kerror_list.size() > 0 || error_list.size() > 0) {
    std::ofstream report;
    report.open(filename);

    if (kerror_list.size() > 0) {
      report << "Kernel Header / Declare Errors:\n";
      for (auto I = kerror_list.begin(), E = kerror_list.end(); I != E; I++)
        report << (*I) << "\n";
      report << "\n\n\n";
    }

    report << "Instruction / Operand / Region Errors:\n";

    for (auto I = error_list.begin(), E = error_list.end(); I != E; I++)
      report << (*I) << "\n";

    report << "\n\n\n";
    report.close();
  }
}

static int getDstIndex(const CISA_INST *inst) {
  ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
  int dstIndex = -1;
  if (inst->opnd_num > 0) {
    switch (ISA_Inst_Table[opcode].type) {
    case ISA_Inst_Mov:
    case ISA_Inst_Arith:
    case ISA_Inst_Logic:
    case ISA_Inst_Address:
      dstIndex = 0;
      break;
    case ISA_Inst_Compare:
      dstIndex = 1;
      break;
    default:
      break; // Prevent gcc warning
    }
  }
  return dstIndex;
}

// diagDumpInstructionOperandDecls() is used to generate the error report
static std::string
diagDumpInstructionOperandDecls(const print_format_provider_t *header,
                                const CISA_INST *inst, Options *options) {
  std::stringstream sstr;

  unsigned numPreDefinedVars = Get_CISA_PreDefined_Var_Count();

  for (unsigned i = 0; i < inst->opnd_num; i++) {
    if (inst->opnd_array[i]->opnd_type == CISA_OPND_VECTOR) {
      const vector_opnd &opnd = getVectorOperand(inst, i);
      uint32_t index = opnd.getOperandIndex();

      if (numPreDefinedVars <= index && index < header->getVarCount())
        switch (opnd.getOperandClass()) {
        case OPERAND_STATE:
        case OPERAND_GENERAL:
          sstr << printVariableDecl(header, index - numPreDefinedVars, options);
          break;
        case OPERAND_ADDRESS:
        case OPERAND_INDIRECT:
          sstr << printAddressDecl(header, index);
          break;
        case OPERAND_PREDICATE:
          sstr << printPredicateDecl(header, index);
          break;
        case OPERAND_ADDRESSOF:
          sstr << "ADDRESSOF Operand decl... are those even allowed>\n";
          break;
        case OPERAND_IMMEDIATE:
          sstr << "Immediate operand: "
               << getPrimitiveOperand<unsigned>(inst, i) << "\n";
          break;
        default:
          sstr << "Operand type: " << opnd.getOperandClass()
               << " unable to print."
               << "\n";
          break;
        }
    } else if (inst->opnd_array[i]->opnd_type == CISA_OPND_RAW) {
      uint32_t index = getRawOperand(inst, i).index;
      if (numPreDefinedVars <= index &&
          index < header->getVarCount() + numPreDefinedVars)
        sstr << printVariableDecl(header, index - numPreDefinedVars, options);
    } else if (inst->opnd_array[i]->opnd_type ==
               CISA_OPND_OTHER) // new loader only
    {
    } else {
      sstr << "unknown operand?";
    }

    sstr << "\n";
    if (i != inst->opnd_num - 1)
      sstr << std::setw(33) << "                               ";
  }

  return sstr.str();
}

static std::string createIsaError(const VISAKernel_format_provider *header,
                                  std::string msg, Options *opt,
                                  const CISA_INST *inst = nullptr) {
  std::stringstream sstr;
  if (!inst)
    sstr << "\n/-------------------------------------------!!!KERNEL HEADER "
            "ERRORS FOUND!!!-------------------------------------------\\\n";
  else
    sstr << "\n/--------------------------------------------!!!INSTRUCTION "
            "ERROR FOUND!!!---------------------------------------------\\\n";
  sstr << std::setw(33) << "Error in CISA routine with name: "
       << (char *)header->getString(header->getNameIndex()) << "\n";
  sstr << std::setw(33) << "Error Message: " << msg << "\n";

  if (inst) {
    sstr << std::setw(33) << "Diagnostics:\n";
    sstr << std::setw(33) << " Instruction variables' decls: ";
    sstr << diagDumpInstructionOperandDecls(header, inst, opt) << "\n";
    sstr << std::setw(33)
         << " Violating Instruction: " << header->printInstruction(inst, opt)
         << "\n";
  }

  sstr << "\\------------------------------------------------------------------"
          "----------------------------------------------------/\n";
  return sstr.str();
}

// If region/offset is available, return true; otherwise, return false.
bool vISAVerifier::getRegion(const vector_opnd &VecOpnd, uint16_t &row_offset,
                             uint16_t &col_offset, uint16_t &v_stride,
                             uint16_t &width, uint16_t &h_stride) const {
  Common_ISA_Operand_Class operand_class = VecOpnd.getOperandClass();

  uint16_t region = 0;
  row_offset = 0;
  col_offset = 0;

  switch (operand_class) {
  case OPERAND_GENERAL:
    region = VecOpnd.opnd_val.gen_opnd.region;
    row_offset = VecOpnd.opnd_val.gen_opnd.row_offset;
    col_offset = VecOpnd.opnd_val.gen_opnd.col_offset;
    break;
  case OPERAND_INDIRECT:
    region = VecOpnd.opnd_val.indirect_opnd.region;
    break;
  default:
    return false;
  }

  Common_ISA_Region_Val w = (Common_ISA_Region_Val)((region >> 4) & 0xF);
  Common_ISA_Region_Val h = (Common_ISA_Region_Val)((region >> 8) & 0xF);
  Common_ISA_Region_Val v = (Common_ISA_Region_Val)((region)&0xF);

  v_stride = Common_ISA_Get_Region_Value(v);
  h_stride = Common_ISA_Get_Region_Value(h);
  width = Common_ISA_Get_Region_Value(w);
  return true;
}

VISA_Type vISAVerifier::getOperandVISAType(const CISA_INST *I,
                                           unsigned Ix) const {
  switch (getOperandType(I, Ix)) {
  case CISA_OPND_VECTOR: {
    const vector_opnd &vec_opnd = getVectorOperand(I, Ix);
    return getVectorOperandType(header, vec_opnd);
  }
  case CISA_OPND_RAW: {
    const raw_opnd &raw_opnd = getRawOperand(I, Ix);
    return getRawOperandType(header, raw_opnd);
  }
  default:
    break;
  }
  return ISA_TYPE_NUM;
}

// If any of I's operands uses the GivenType, return true; otherwise, return
// false.
bool vISAVerifier::useGivenVISAType(const CISA_INST *I,
                                    VISA_Type GivenType) const {
  ISA_Opcode opc = (ISA_Opcode)I->opcode;
  int ndst = (int)ISA_Inst_Table[opc].n_dsts;
  if (opc == ISA_CMP) {
    // cmp's 1st opnd is cmp relop, which returns ISA_TYPE_NUM as its type.
    // Thus, it is ignored as expected.
    ++ndst;
  }
  for (int j = 0; j < ndst; j++) {
    VISA_Type dstType = getOperandVISAType(I, j);
    if (dstType == GivenType)
      return true;
  }
  for (int j = 0; j < ISA_Inst_Table[opc].n_srcs; j++) {
    VISA_Type srcType = getOperandVISAType(I, j + ndst);
    if (srcType == GivenType)
      return true;
  }
  return false;
}

void vISAVerifier::verifyPredicateDecl(unsigned declID) {
  std::string declError = std::string(" Error in predicate variable decl: ") +
                          printPredicateDecl(header, declID);

  REPORT_HEADER(options,
                header->getPred(declID)->name_index < header->getStringCount(),
                "P%d's name index(%d) is not valid: %s", declID,
                header->getPred(declID)->name_index, declError.c_str());

  switch (header->getPred(declID)->num_elements) {
  case 1:
  case 2:
  case 4:
  case 8:
  case 16:
  case 32:
    break;
  default:
    REPORT_HEADER(options, false,
                  "P%d's number of elements(%d) is not valid: %s", declID,
                  header->getPred(declID)->num_elements, declError.c_str());
  }
}

void vISAVerifier::verifyAddressDecl(unsigned declID) {
  std::string declError = std::string(" Error in address variable decl: ") +
                          printAddressDecl(header, declID);

  REPORT_HEADER(options,
                header->getAddr(declID)->name_index < header->getStringCount(),
                "A%d's name index(%d) is not valid: %s", declID,
                header->getAddr(declID)->name_index, declError.c_str());
  REPORT_HEADER(options,
                header->getAddr(declID)->num_elements <=
                    irBuilder->getNumAddrRegisters(),
                "Max possible address registers are %d: %s",
                irBuilder->getNumAddrRegisters(),
                declError.c_str());

/// TODO: Fix/Reenable this verification check.
#if 0
    switch (header->getAddr(declID)->num_elements)
    {
        case 1:
        case 2:
        case 4:
        case 8:
        case 16:
            break;
        default:
            REPORT_HEADER(options,false, "A%d's number of elements(%d) is not valid.", declID, header->getAddr(declID)->num_elements);
    }
#endif
}

void vISAVerifier::verifyVariableDecl(unsigned declID) {
  std::string declError = std::string(" Error in CISA variable decl: ") +
                          printVariableDecl(header, declID, options);

  const var_info_t *var = header->getVar(declID);
  VISA_Align align = var->getAlignment();

  unsigned numPreDefinedVars = Get_CISA_PreDefined_Var_Count();

  REPORT_HEADER(options, var->name_index < header->getStringCount(),
                "V%d's name index(%d) is not valid: %s",
                declID + numPreDefinedVars, var->name_index, declError.c_str());

  switch (var->getType()) {
  case ISA_TYPE_V:
  case ISA_TYPE_VF:
  case ISA_TYPE_BOOL:
  case ISA_TYPE_UV:
    REPORT_HEADER(options, false, "V%d's type(%s) is not legal: %s", declID,
                  CISATypeTable[var->getType()].typeName, declError.c_str());
    return;
  default:
    break; // Prevent gcc warning
  }

  REPORT_HEADER(options,
                !var->dcl->getAddressed() ||
                    var->getSize() < irBuilder->kernel.getNumRegTotal() *
                                         irBuilder->kernel.getGRFSize(),
                "V%d (size = %d bytes) is indirectly addressed and spans "
                "complete GRF file of %d GRFs. This cannot be allocated by "
                "RA.",
                declID + numPreDefinedVars, var->getSize(),
                irBuilder->kernel.getNumRegTotal() *
                    irBuilder->kernel.getGRFSize());
  REPORT_HEADER(options,
                var->num_elements != 0 &&
                    var->num_elements <= irBuilder->getMaxVariableSize(),
                "V%d's number of elements(%d) is out of range: %s",
                declID + numPreDefinedVars, var->num_elements,
                declError.c_str());
  REPORT_HEADER(options, !(var->alias_index == 0 && var->alias_offset != 0),
                "V%d's alias offset must be zero when it is not aliased: %s",
                declID + numPreDefinedVars, declError.c_str());

  if (var->alias_index >= numPreDefinedVars) {
    const var_info_t *currAliasVar =
        header->getVar(var->alias_index - numPreDefinedVars);
    unsigned totalOffset = var->alias_offset;

    std::set<uint32_t> visitedAliasIndices;

    while (currAliasVar->alias_index >= numPreDefinedVars) {
      if (visitedAliasIndices.find(currAliasVar->alias_index) !=
          visitedAliasIndices.end()) {
        REPORT_HEADER(options, false,
                      "Circular alias detected, alias index: %d",
                      currAliasVar->alias_index);
        break;
      }

      visitedAliasIndices.insert(currAliasVar->alias_index);

      REPORT_HEADER(options,
                    currAliasVar->alias_index <
                        header->getVarCount() + numPreDefinedVars,
                    "Aliased variable aliases to an invalid alias index. "
                    "Variable count: %d. invalid index: %d. %s",
                    header->getVarCount() + numPreDefinedVars,
                    currAliasVar->alias_index, declError.c_str());

      totalOffset += currAliasVar->alias_offset;
      currAliasVar =
          header->getVar(currAliasVar->alias_index - numPreDefinedVars);
    }

    if (currAliasVar->alias_index < header->getVarCount() + numPreDefinedVars) {
      REPORT_HEADER(
          options,
          totalOffset <
              (currAliasVar->num_elements *
               (unsigned)CISATypeTable[currAliasVar->getType()].typeSize),
          "Variable decl's alias offset exceeds the bounds of the aliased "
          "variable decl allocation size: %s",
          declError.c_str());
      VISA_Align baseAlign =
          std::max(currAliasVar->getAlignment(),
                   currAliasVar->getTypeAlignment(irBuilder->getGRFSize()));
      REPORT_HEADER(
          options, baseAlign >= var->getTypeAlignment(irBuilder->getGRFSize()),
          "base variable must be at least type-aligned to this variable: %s",
          declError.c_str());
    }
  }

  switch (align) {
  case ALIGN_BYTE:
    break;
  case ALIGN_WORD:
    break;
  case ALIGN_DWORD:
    break;
  case ALIGN_QWORD:
    break;
  case ALIGN_OWORD:
    break;
  case ALIGN_GRF:
    break;
  case ALIGN_2_GRF:
    break;
  case ALIGN_HWORD:
    break;
  case ALIGN_32WORD:
    break;
  case ALIGN_64WORD:
    break;
  default:
    REPORT_HEADER(options, false,
                  "V%d's variable alignment is not a valid alignment value: %s",
                  declID, declError.c_str());
  }

  REPORT_HEADER(
      options,
      !(var->alias_index != 0 &&
        var->alias_index >= header->getVarCount() + numPreDefinedVars),
      "V%d's alias index must point to a valid CISA variable index "
      "between 0 and %d. Currently points to invalid index V%d: %s",
      declID + numPreDefinedVars, header->getVarCount() + numPreDefinedVars - 1,
      var->alias_index, declError.c_str());

/// INFO: Got rid of this verification check a long time ago.
///       Checking for unused variables is not that important
///       and it made it a lot simpler for refactoring to just
///       get rid of code that did the analysis required for it.
#if 0
    REPORT_HEADER(options,m_used, "V%d is an unused variable in the CISA bytecode. Find out why it is declared.", declID);
#endif
}

// get the start byte offset from the top level declare
static unsigned int getStartByteOffset(const print_format_provider_t *header,
                                       const var_info_t *var,
                                       unsigned int numPredefinedVars) {
  unsigned int offset = 0;
  while (var->alias_index != 0) {
    offset += var->alias_offset;
    if (var->alias_index <= numPredefinedVars) {
      // predefined variables don't have aliases, so we can stop
      break;
    } else {
      var = header->getVar(var->alias_index - numPredefinedVars);
    }
  }
  return offset;
}

void vISAVerifier::verifyRegion(const CISA_INST *inst, unsigned i) {
  ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
  /// Dataport instructions must be verified separately
  if (ISA_Inst_Data_Port == ISA_Inst_Table[opcode].type)
    return;

  // Region parameters for plane are ignored
  if (opcode == ISA_PLANE)
    return;

  vISA_ASSERT(inst->opnd_array[i]->opnd_type == CISA_OPND_VECTOR,
              "Should only be verifying region on a vector operand");
  const vector_opnd &vect = getVectorOperand(inst, i);

  uint32_t operand_index = vect.getOperandIndex();
  Common_ISA_Operand_Class operand_class = vect.getOperandClass();

  unsigned dstIndex = getDstIndex(inst);
  bool isDst = (i == dstIndex);

  unsigned numPreDefinedVars = Get_CISA_PreDefined_Var_Count();

  uint16_t region = 0;
  uint8_t row_offset = 0;
  uint8_t col_offset = 0;

  switch (operand_class) {
  case OPERAND_GENERAL:
    region = vect.opnd_val.gen_opnd.region;
    row_offset = vect.opnd_val.gen_opnd.row_offset;
    col_offset = vect.opnd_val.gen_opnd.col_offset;
    break;
  case OPERAND_INDIRECT:
    region = vect.opnd_val.indirect_opnd.region;
    break;
  default:
    return;
  }

  Common_ISA_Region_Val width = (Common_ISA_Region_Val)((region >> 4) & 0xF);
  Common_ISA_Region_Val h_stride = (Common_ISA_Region_Val)((region >> 8) & 0xF);
  Common_ISA_Region_Val v_stride = (Common_ISA_Region_Val)((region)&0xF);

  short v_stride_val = Common_ISA_Get_Region_Value(v_stride);
  short h_stride_val = Common_ISA_Get_Region_Value(h_stride);
  short width_val = Common_ISA_Get_Region_Value(width);

  /// INFO: Catch zero width, because we'll sigsegv otherwise.
  REPORT_INSTRUCTION(options, width_val, "CISA region has width of 0");

  uint8_t exec_sz = 0;
  switch (inst->getExecSize()) {
  case EXEC_SIZE_1:
    exec_sz = 1;
    break;
  case EXEC_SIZE_2:
    exec_sz = 2;
    break;
  case EXEC_SIZE_4:
    exec_sz = 4;
    break;
  case EXEC_SIZE_8:
    exec_sz = 8;
    break;
  case EXEC_SIZE_16:
    exec_sz = 16;
    break;
  case EXEC_SIZE_32:
    exec_sz = 32;
    break;
  default:
    REPORT_INSTRUCTION(options, false, "Invalid execution size");
  }

  if (isDst) {
    REPORT_INSTRUCTION(
        options, 0 != h_stride_val,
        "Horizontal Stride should not be 0 for a destination operand.");

    // for dst set width = exec size and vstride = width * hstride, so their
    // bound can be verified
    width_val = exec_sz;
    v_stride_val = width_val * h_stride_val;
  }

  if (width_val == 0) {
    return;
  }

  REPORT_INSTRUCTION(options, h_stride != REGION_NULL,
                     "Horizontal Stride should not be REGION_NULL");

  switch (h_stride_val) {
  case 0:
  case 1:
  case 2:
  case 4:
    break;
  default:
    REPORT_INSTRUCTION(
        options, false,
        "Legal CISA region horizontal stride parameter values: {0, 1, 2, 4}.");
  }

  if (width != REGION_NULL) {
    switch (width_val) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
      break;
    default:
      REPORT_INSTRUCTION(
          options, false,
          "Legal CISA region width parameter values: {1, 2, 4, 8, 16}.");
    }

    REPORT_INSTRUCTION(options, exec_sz >= width_val,
                       "Execution size must be greater than width.");
    if (exec_sz < width_val) {
      // no point in continuing verifiication
      return;
    }
  }

  if (v_stride != REGION_NULL) {
    switch (v_stride_val) {
    case 0:
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32:
      break;
    default:
      REPORT_INSTRUCTION(options, false,
                         "Legal CISA region vertical stride parameter values: "
                         "{0, 1, 2, 4, 8, 16, 32}.");
    }
  } else if (!isDst) {
    // check for out-of-bound addresses for VxH operand
    int numAddr = exec_sz / width_val;
    REPORT_INSTRUCTION(options, numAddr <= (int)irBuilder->getNumAddrRegisters(),
                       "Number of Address for indirect operand exceeds 16");
  }

  if (operand_index >= numPreDefinedVars)
    if (operand_class == OPERAND_GENERAL) {
      const var_info_t *var =
          header->getVar(vect.getOperandIndex() - numPreDefinedVars);
      VISA_Type isa_type = getVectorOperandType(header, vect);
      unsigned VN_size = CISATypeTable[isa_type].typeSize;

      uint16_t num_elements = var->num_elements;
      unsigned var_size = VN_size * num_elements;

      unsigned last_region_elt_byte;
      if (width_val != -1) {
        unsigned last_region_element =
            (((exec_sz / width_val) - 1) * v_stride_val) +
            ((width_val - 1) * h_stride_val);
        last_region_elt_byte = (last_region_element * VN_size) + VN_size - 1;
      } else {
        // DstRegRegion with width 0 (width_val=-1)
        last_region_elt_byte =
            (exec_sz - 1) * h_stride_val * VN_size + VN_size - 1;
      }

      unsigned grfSize = irBuilder->getGRFSize();

      // Check if the operand may touch more than 2 GRFs due to bad alignment
      // So far vISA is able to handle the splitting of:
      // moves, logic, cmp and arithmetic instructions
      if (ISA_Inst_Table[opcode].type != ISA_Inst_Mov &&
          ISA_Inst_Table[opcode].type != ISA_Inst_Logic &&
          ISA_Inst_Table[opcode].type != ISA_Inst_Compare &&
          ISA_Inst_Table[opcode].type != ISA_Inst_Arith) {
        REPORT_INSTRUCTION(
            options, (grfSize * 2u) > last_region_elt_byte,
            "CISA operand region access out of 2 GRF boundary (within %d "
            "bytes): %d",
            (grfSize * 2), last_region_elt_byte);

        // check if the operand may touch more than 2 GRFs due to bad alignment
        unsigned startByte =
            getStartByteOffset(header, var, numPreDefinedVars) +
            row_offset * grfSize +
            col_offset * CISATypeTable[var->getType()].typeSize;
        unsigned endByte = startByte + last_region_elt_byte;
        unsigned startGRF = startByte / grfSize;
        unsigned endGRF = endByte / grfSize;
        REPORT_INSTRUCTION(
            options, endGRF == startGRF || endGRF == (startGRF + 1),
            "CISA operand accesses more than 2 GRF due to mis-alignment: start "
            "byte offset = %d, end byte offset = %d",
            startByte, endByte);
      }

      unsigned firstElementIndex = row_offset * grfSize + col_offset * VN_size;

      for (int i = 0; i < exec_sz / width_val; i++) {
        for (int j = 0; j < width_val; j++) {
          unsigned region_offset =
              firstElementIndex +
              (((i * v_stride_val) + (j * h_stride_val)) * VN_size);

          // Madw instruction has both low and high results in dst. So, need
          // to check the offset of high result.
          unsigned dstHiOffset = 0;
          if (inst->opcode == ISA_MADW && isDst) {
            dstHiOffset =
                (region_offset - firstElementIndex + 1 + grfSize - 1) &
                       (~(grfSize - 1));  // GRF-aligned
            region_offset += dstHiOffset;
          }

          if (region_offset >= var_size) {
#ifndef DLL_MODE
            std::cout << "WARNING: CISA region and offset cause an out of "
                         "bounds byte access: "
                      << region_offset << "\n";
            std::cout
                << "An access should not exceed the declared allocation size: "
                << var_size << "\n";
            std::cout << "  The access fails the following check to determine "
                         "correct bounds (see CISA manual section 5.1 "
                         "Region-based Addressing):\n";
            if (inst->opcode == ISA_MADW && isDst) {
              std::cout
                  << "(row_offset * GRF_SIZE + col_offset * type_size) + "
                     "(((i * v_stride) + (j * h_stride)) * type_size) + "
                     "dstHiOffset < type_size * num_elements:\n";
              std::cout << "(" << (int)row_offset << " * " << grfSize << " + "
                        << (int)col_offset << " * " << VN_size << ") + (((" << i
                        << " * " << v_stride_val << ") + (" << j << " * "
                        << h_stride_val << ")) * " << VN_size << ") + "
                        << dstHiOffset << " < " << VN_size << " * "
                        << num_elements << "\n";
            } else {
              std::cout
                  << "  (row_offset * GRF_SIZE + col_offset * type_size) + "
                     "(((i * v_stride) + (j * h_stride)) * type_size) < "
                     "type_size * num_elements:\n";
              std::cout << "(" << (int)row_offset << " * " << grfSize << " + "
                        << (int)col_offset << " * " << VN_size << ") + (((" << i
                        << " * " << v_stride_val << ") + (" << j << " * "
                        << h_stride_val << ")) * " << VN_size << ") < "
                        << VN_size << " * " << num_elements << "\n";
            }
            std::cout << "Violating Instruction: "
                      << header->printInstruction(inst, options)
                      << "\n";
#endif // DLL_MODE
          }
        }
      }
    }
}

static bool isDWordType(VISA_Type type) {
  return CISATypeTable[type].typeSize == 4;
}

// verify if this raw operand has the correct type as determined by typeFunc
// (false means illegal type) Many vISA messages require the raw operand to have
// certain types
void vISAVerifier::verifyRawOperandType(const CISA_INST *inst,
                                        const raw_opnd &opnd,
                                        bool (*typeFunc)(VISA_Type)) {
  unsigned numPreDefinedVars = Get_CISA_PreDefined_Var_Count();
  uint32_t variable_count = header->getVarCount();

  uint32_t opnd_index = opnd.index;

  if (opnd_index < variable_count + numPreDefinedVars &&
      numPreDefinedVars <= opnd_index) {
    const var_info_t *currVar = header->getVar(opnd_index - numPreDefinedVars);
    REPORT_INSTRUCTION(options, typeFunc(currVar->getType()),
                       "Raw Operand %s has incorrect type %s",
                       opnd.toString().c_str(),
                       CISATypeTable[currVar->getType()].typeName);
  }
}

void vISAVerifier::verifyRawOperand(const CISA_INST *inst, unsigned i) {
  unsigned numPreDefinedVars = Get_CISA_PreDefined_Var_Count();
  uint32_t variable_count = header->getVarCount();

  const raw_opnd &opnd = getRawOperand(inst, i);

  if (numPreDefinedVars > opnd.index) {
    return; // allow raw operands to be predefined variables
  }

  uint32_t opnd_index = opnd.index - numPreDefinedVars;
  uint16_t opnd_offset = opnd.offset;

  if (opnd_index < variable_count) {
    const var_info_t *currVar = header->getVar(opnd_index);
    unsigned totalOffset = opnd_offset;

    std::set<uint32_t> visitedAliasIndices;

    while (numPreDefinedVars <= currVar->alias_index) {
      if (visitedAliasIndices.find(currVar->alias_index) !=
          visitedAliasIndices.end()) {
        REPORT_INSTRUCTION(options, false,
                           "Circular alias detected, alias index: %d",
                           currVar->alias_index);
        break;
      }

      visitedAliasIndices.insert(currVar->alias_index);

      REPORT_INSTRUCTION(
          options, currVar->alias_index < variable_count + numPreDefinedVars,
          "Aliased variable aliases to an invalid alias index. "
          "Variable count: %d. invalid index: %d",
          variable_count + numPreDefinedVars, currVar->alias_index);

      totalOffset += currVar->alias_offset;
      if (numPreDefinedVars > currVar->alias_index) {
        break; // // allow alias index to be predefined variables
      }
      currVar = header->getVar(currVar->alias_index - numPreDefinedVars);
    }

    if (currVar->getSize() >= irBuilder->numEltPerGRF<Type_UB>() &&
        totalOffset % irBuilder->numEltPerGRF<Type_UB>() != 0) {
      // raw operand must be GRF-aligned if it's >= 1GRF
      REPORT_INSTRUCTION(options, false,
                         "Raw operand should be GRF-aligned: Raw offset is %d",
                         totalOffset);
    }

    if (numPreDefinedVars <= currVar->alias_index) {
      REPORT_INSTRUCTION(
          options,
          totalOffset <
              currVar->num_elements *
                  (unsigned)CISATypeTable[currVar->getType()].typeSize,
          "A CISA raw operand's offset field must be within the allocated "
          "operand size. "
          "Raw offset is %d, allocated number of elements is %d",
          (int)opnd_offset, (int)currVar->num_elements);
    }
  } else {
    /// TODO: Verify predefined vars.
  }
}

static bool isReadWritePreDefinedVar(uint32_t index, uint32_t byteOffset) {
  PreDefinedVarsInternal internalIndex = mapExternalToInternalPreDefVar(index);
  if (internalIndex == PreDefinedVarsInternal::ARG ||
      internalIndex == PreDefinedVarsInternal::RET ||
      internalIndex == PreDefinedVarsInternal::FE_SP ||
      internalIndex == PreDefinedVarsInternal::FE_FP ||
      internalIndex == PreDefinedVarsInternal::CR0 ||
      internalIndex == PreDefinedVarsInternal::DBG ||
      internalIndex == PreDefinedVarsInternal::VAR_NULL ||
      internalIndex == PreDefinedVarsInternal::IMPL_ARG_BUF_PTR ||
      internalIndex == PreDefinedVarsInternal::LOCAL_ID_BUF_PTR) {
    return true;
  } else if (internalIndex == PreDefinedVarsInternal::TSC && byteOffset == 16) {
    // tm0.4
    return true;
  } else if (internalIndex == PreDefinedVarsInternal::SR0 &&
             (byteOffset == 4 || byteOffset == 8)) {
    // sr0.1, sr0.2
    return true;
  } else {
    return false;
  }
}

void vISAVerifier::verifyVectorOperand(const CISA_INST *inst, unsigned i) {
  unsigned numPreDefinedVars = Get_CISA_PreDefined_Var_Count();

  ISA_Opcode opcode = (ISA_Opcode)inst->opcode;

  const vector_opnd &opnd = getVectorOperand(inst, i);

  uint32_t operand_index = opnd.getOperandIndex();
  Common_ISA_Operand_Class operand_class = opnd.getOperandClass();
  VISA_Modifier operand_modifier = opnd.getOperandModifier();

  unsigned dstIndex = getDstIndex(inst);

  if (inst->opnd_num <= 0) {
    REPORT_INSTRUCTION(options, false, "Incorrect number of operands loaded.");
    return;
  }

  if (operand_class != OPERAND_GENERAL && operand_class != OPERAND_INDIRECT) {
    REPORT_INSTRUCTION(options, operand_modifier == MODIFIER_NONE,
                       "Operand modifier for non-general and "
                       "non-indirect operands must be MODIFIER_NONE.");
  }

  switch (operand_modifier) {
  case MODIFIER_ABS:
  case MODIFIER_NEG:
  case MODIFIER_SAT:
  case MODIFIER_NEG_ABS:
    if (!isShiftOp(opcode)) {
      REPORT_INSTRUCTION(
          options, ISA_Inst_Table[opcode].type != ISA_Inst_Logic,
          "Only arithmetic modifiers should be used with "
          "arithmetic instruction general or indirect operands.");
    }
    break;
  case MODIFIER_NOT:
    REPORT_INSTRUCTION(options, ISA_Inst_Table[opcode].type == ISA_Inst_Logic,
                       "Only logical modifiers should be used with "
                       "logical instruction general or indirect operands.");
    break;
  default:
    break; // Prevent gcc warning
  }

  if (operand_class == OPERAND_IMMEDIATE) {
    /// Do checks for immediate operands here
    REPORT_INSTRUCTION(options,
                       getVectorOperandType(header, opnd) != ISA_TYPE_BOOL,
                       "Boolean types for immediate (constant literals) "
                       "operands are disallowed.");
  }

  if (operand_class == OPERAND_GENERAL) {
    REPORT_INSTRUCTION(
        options, operand_index < header->getVarCount() + numPreDefinedVars,
        "Variable V%d is not declaired in CISA symtab.", operand_index);

    if (operand_index < header->getVarCount() + numPreDefinedVars &&
        numPreDefinedVars <= operand_index) {
      // TODO: add variable verification.
    }
  }

  verifyRegion(inst, i);

  if (dstIndex == i) {
    REPORT_INSTRUCTION(options, operand_class != OPERAND_IMMEDIATE,
                       "Constant Immediate operands are not allowed to be used "
                       "as destination operands.");

    if (operand_class == OPERAND_GENERAL) {
      if (operand_index < numPreDefinedVars) {
        uint32_t byteOffset =
            opnd.opnd_val.gen_opnd.row_offset * irBuilder->getGRFSize() +
            opnd.opnd_val.gen_opnd.col_offset *
                Get_VISA_Type_Size(getPredefinedVarType(
                    mapExternalToInternalPreDefVar(operand_index)));
        REPORT_INSTRUCTION(options,
                           isReadWritePreDefinedVar(operand_index, byteOffset),
                           "Not allowed to write to a read only variable");
      }
    }
  }

  // ToDo: add bounds check for pre-defined variables
}

void vISAVerifier::verifyOperand(const CISA_INST *inst, unsigned i) {
  // skip verifying bdpas like other dpas variant as it is verified in
  // verifyInstructionMisc().
  if (inst->opcode == ISA_BDPAS)
    return;
  if (inst->opcode == ISA_DPAS || inst->opcode == ISA_DPASW) {
    // skip, as dpas is verified in verifyInstructionMisc().
    return;
  }
  vISA_ASSERT(header, "Argument Exception: argument header is NULL.");
  vISA_ASSERT(inst, "Argument Exception: argument inst   is NULL.");
  vISA_ASSERT(inst->opnd_num > i,
               "No such operand, i, for instruction inst.");
  switch (getOperandType(inst, i)) {
  case CISA_OPND_OTHER: /* unable to verify some random primitive operand. */
    break;
  case CISA_OPND_VECTOR:
    verifyVectorOperand(inst, i);
    break;
  case CISA_OPND_RAW:
    verifyRawOperand(inst, i);
    break;
  default:
    vISA_ASSERT_UNREACHABLE("Invalid operand type.");
  }
}

void vISAVerifier::verifyInstructionSVM(const CISA_INST *inst) {
  if (hasExecSize((ISA_Opcode)inst->opcode)) {
    REPORT_INSTRUCTION(options, EXEC_SIZE_32 != inst->getExecSize(),
                       "Execution size should not be SIMD32 for SVM messages.");
  }
}

void vISAVerifier::verifyInstructionMove(const CISA_INST *inst) {
  ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
  if (ISA_SHFL_IDX4 == opcode) {
    verifyInstructionShflIdx4(inst);
    return;
  }
  if (opcode == ISA_DNSCL) {
    verifyInstructionDnscl(inst);
    return;
  }

  unsigned i = 0;

  uint8_t opext =
      opcode == ISA_FMINMAX ? getPrimitiveOperand<uint8_t>(inst, i++) : 0;

  const vector_opnd &dst = getVectorOperand(inst, i++);
  const vector_opnd &src0 = getVectorOperand(inst, i++);

  Common_ISA_Operand_Class operand_class_dst = dst.getOperandClass();
  Common_ISA_Operand_Class operand_class_src0 = src0.getOperandClass();

  switch (opcode) {
  case ISA_MOV: {
    VISA_Type dstType = getVectorOperandType(header, dst);
    VISA_Type src0Type = getVectorOperandType(header, src0);
    VISA_Modifier dstModifier = dst.getOperandModifier();

    if (OPERAND_PREDICATE == operand_class_src0) {
      REPORT_INSTRUCTION(options, EXEC_SIZE_1 == inst->getExecSize(),
                         "Execution size for a flag copy mov instruction "
                         "should be 1, as it is a scalar copy.");
      REPORT_INSTRUCTION(options,
                         IsIntType(dstType) && dstType != ISA_TYPE_Q &&
                             dstType != ISA_TYPE_UQ,
                         "dst operand type for a flag copy mov instruction "
                         "should be non-64-bit integer.");
      REPORT_INSTRUCTION(
          options,
          CISATypeTable[dstType].typeSize >= CISATypeTable[src0Type].typeSize,
          "dst operand type for a flag copy mov instruction should be "
          "greater than or equal to the size of the src0 operand's type size.");
      REPORT_INSTRUCTION(options, dstModifier != MODIFIER_SAT,
                         "saturation is not allowed for dst operands of a flag "
                         "copy mov instruction");
      REPORT_INSTRUCTION(options, inst->pred.isNullPred(),
                         "predication is not allowed for dst operands of a "
                         "flag copy mov instruction");
    }

    /* Commented out because we are too lame to follow our own specification. */
    /*
    REPORT_INSTRUCTION(options,operand_class_dst == OPERAND_GENERAL  ||
                      operand_class_dst == OPERAND_INDIRECT,
                      "Destination operand of CISA MOV instruction only "
                      "supports general and indirect operands.");

    REPORT_INSTRUCTION(options,operand_class_src0 == OPERAND_GENERAL  ||
                      operand_class_src0 == OPERAND_INDIRECT ||
                      operand_class_src0 == OPERAND_IMMEDIATE,
                      "Source0 operand of CISA MOV instruction only "
                      "supports general, indirect, and immediate operands.");
                      */
    break;
  }

  case ISA_MOVS: {
    REPORT_INSTRUCTION(options,
                       operand_class_dst == OPERAND_GENERAL ||
                           operand_class_dst == OPERAND_STATE,
                       "Destination operand of CISA MOVS instruction only "
                       "supports general and indirect operands.");

    REPORT_INSTRUCTION(options,
                       operand_class_src0 == OPERAND_GENERAL ||
                           operand_class_src0 == OPERAND_STATE ||
                           operand_class_src0 == OPERAND_INDIRECT ||
                           operand_class_src0 == OPERAND_IMMEDIATE,
                       "Source0 operand of CISA MOVS instruction only "
                       "supports general, indirect, and immediate operands.");
    break;
  }
  case ISA_FCVT: {
    REPORT_INSTRUCTION(options, irBuilder->getPlatform() >= Xe_PVC,
                       "fcvt is not supported on the selected platform");

    REPORT_INSTRUCTION(options, operand_class_dst == OPERAND_GENERAL,
                       "Destination operand of fcvt instruction only "
                       "supports general and operands.");

    REPORT_INSTRUCTION(options, operand_class_src0 == OPERAND_GENERAL,
                       "Source0 operand of fcvt instruction only "
                       "supports general,  operands.");

    VISA_Type dstType = getVectorOperandType(header, dst);
    VISA_Type src0Type = getVectorOperandType(header, src0);

    if (dstType == ISA_TYPE_B || src0Type == ISA_TYPE_B) {
      REPORT_INSTRUCTION(options, irBuilder->getPlatform() >= Xe3,
                         "HF8 fcvt is not supported on the selected platform");
    }

    if (dstType == ISA_TYPE_UB) {
      REPORT_INSTRUCTION(options, src0Type == ISA_TYPE_HF,
                         "FCVT with UB(actually BF8) dst must have HF src");
    } else if (src0Type == ISA_TYPE_UB) {
      REPORT_INSTRUCTION(options, dstType == ISA_TYPE_HF,
                         "FCVT with UB(actually BF8) src must have HF dst");
    } else if (dstType == ISA_TYPE_UD) {
      REPORT_INSTRUCTION(options, src0Type == ISA_TYPE_F,
                         "FCVT with UD(actually TF32) dst must have F src");
    } else if (src0Type == ISA_TYPE_UD) {
      REPORT_INSTRUCTION(options, false,
                         "FCVT with UD(actually TF32) src0 is not supported");
    } else if (dstType == ISA_TYPE_B) {
      REPORT_INSTRUCTION(options, src0Type == ISA_TYPE_HF,
                         "FCVT with B(actually HF8) dst must have HF src");
    } else if (src0Type == ISA_TYPE_B) {
      REPORT_INSTRUCTION(options, dstType == ISA_TYPE_HF,
                         "FCVT with B(actually HF8) src must have HF dst");
    } else {
      REPORT_INSTRUCTION(options, false,
                         "FCVT must have either UB(actually BF8) dst or src");
    }

    // Check if NoMask is required
    switch (dstType) {
    case ISA_TYPE_UB:
    case ISA_TYPE_B: {
      REPORT_INSTRUCTION(options, isNoMask(inst->getExecMask()),
                         "FCVT must use noMask for HF to FP8 conversion");
    }
    default:
      break;
    }

    bool supportSat = false;
    if (irBuilder->getPlatform() >= Xe3)
        supportSat = true;
    VISA_Modifier dstModifier = dst.getOperandModifier();
    VISA_Modifier srcModifier = src0.getOperandModifier();
    REPORT_INSTRUCTION(
        options, (supportSat || dstModifier == MODIFIER_NONE),
        "destination modifier not supported for FP8/TF32 conversion");
    REPORT_INSTRUCTION(options, srcModifier == MODIFIER_NONE,
                       "source modifier not supported for BF8/TF32 conversion");
    break;
  }
  case ISA_SETP: {
    REPORT_INSTRUCTION(options, operand_class_dst == OPERAND_PREDICATE,
                       "Destination operand of CISA SETP instruction only "
                       "supports predicate operands.");

    REPORT_INSTRUCTION(options,
                       operand_class_src0 == OPERAND_GENERAL ||
                           operand_class_src0 == OPERAND_INDIRECT ||
                           operand_class_src0 == OPERAND_IMMEDIATE,
                       "Source0 operand of CISA SETP instruction only "
                       "supports general, indirect, and immediate operands.");
    break;
  }
  case ISA_SEL:
  case ISA_FMINMAX: {
    REPORT_INSTRUCTION(options, opext <= 1,
                       "FMINMAX opext must be either 0x0 or 0x1 (min or max).");

    const vector_opnd &src1 = getVectorOperand(inst, i++);
    Common_ISA_Operand_Class operand_class_src1 = src1.getOperandClass();

    REPORT_INSTRUCTION(options,
                       operand_class_dst == OPERAND_GENERAL ||
                           operand_class_dst == OPERAND_INDIRECT,
                       "Destination operand of CISA SEL instruction only "
                       "supports general and indirect operands.");

    REPORT_INSTRUCTION(options,
                       operand_class_src0 == OPERAND_GENERAL ||
                           operand_class_src0 == OPERAND_INDIRECT ||
                           operand_class_src0 == OPERAND_IMMEDIATE,
                       "Source0 operand of CISA SEL instruction only "
                       "supports general, indirect, and immediate operands.");

    REPORT_INSTRUCTION(options,
                       operand_class_src1 == OPERAND_GENERAL ||
                           operand_class_src1 == OPERAND_INDIRECT ||
                           operand_class_src1 == OPERAND_IMMEDIATE,
                       "Source1 operand of CISA SEL instruction only "
                       "supports general, indirect, and immediate operands.");
    break;
  }
  default:
    REPORT_INSTRUCTION(options, false,
                       "Illegal Move Instruction Opcode: %d, %s.", opcode,
                       ISA_Inst_Table[opcode].str);
  }
}

void vISAVerifier::verifyInstructionSync(const CISA_INST *inst) {
  ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
  switch (opcode) {
  case ISA_BARRIER:
    return;
  case ISA_SAMPLR_CACHE_FLUSH:
    return;
  case ISA_WAIT:
    return;
  case ISA_FENCE:
    return;
  case ISA_YIELD:
    return;
  case ISA_SBARRIER:
    return;
  case ISA_NBARRIER:
    return;
  default:
    REPORT_INSTRUCTION(options, false,
                       "Illegal Synchronization Instruction Opcode: %d, %s.",
                       opcode, ISA_Inst_Table[opcode].str);
  }
}

void vISAVerifier::verifyInstructionControlFlow(const CISA_INST *inst) {
  ISA_Opcode opcode = (ISA_Opcode)inst->opcode;

  switch (opcode) {
  case ISA_JMP:
  case ISA_CALL: {
    bool isFC =
        static_cast<G4_Label *>(inst->opnd_array[0]->g4opnd)->isFCLabel();
    auto labelId = getPrimitiveOperand<uint16_t>(inst, 0);
    if (labelId >= header->getLabelCount()) {
      REPORT_INSTRUCTION(options, false, "bad label id %d", labelId);
    } else {
      auto iter = labelDefs.find(labelId);
      if (iter == labelDefs.end() && !isFC) {
        labelDefs[labelId] = false;
      }
      // nothing needs to be done if the label is already in the map
    }
    break;
  }
  case ISA_RET:
  case ISA_FRET:
  case ISA_IFCALL:
  case ISA_FADDR: // no checks for now
    break;
  case ISA_SUBROUTINE:
  case ISA_LABEL: {
    auto labelId = getPrimitiveOperand<uint16_t>(inst, 0);
    if (labelId >= header->getLabelCount()) {
      REPORT_INSTRUCTION(options, false, "bad label id %d", labelId);
    } else {
      auto iter = labelDefs.find(labelId);
      if (iter != labelDefs.end() && iter->second) {
        REPORT_INSTRUCTION(options, false, "label is redefined");
      }
      labelDefs[labelId] = true;
    }
    break;
  }
  case ISA_FCALL: {
    break;
  }
  case ISA_SWITCHJMP: {
    REPORT_INSTRUCTION(options, !irBuilder->hasFusedEU(),
                       "switchjmp is not supported on this platform");
    break;
  }
  default:
    REPORT_INSTRUCTION(
        options, false,
        "Illegal Scalar Control Flow Instruction Opcode: %d, %s.", opcode,
        ISA_Inst_Table[opcode].str);
  }
}

void vISAVerifier::verifyInstructionMisc(const CISA_INST *inst) {
  unsigned i = 0;
  unsigned numPreDefinedSurfs = Get_CISA_PreDefined_Surf_Count();

  ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
  switch (opcode) {
  case ISA_VME_IME: {
    uint8_t stream_mode = getPrimitiveOperand<uint8_t>(inst, i++);
    REPORT_INSTRUCTION(
        options,
        stream_mode == VME_STREAM_DISABLE || stream_mode == VME_STREAM_OUT ||
            stream_mode == VME_STREAM_IN || stream_mode == VME_STREAM_IN_OUT,
        "CISA ISA_VME_IME instruction uses illegal stream mode.");

    uint8_t search_ctrl = getPrimitiveOperand<uint8_t>(inst, i++);
    REPORT_INSTRUCTION(
        options,
        search_ctrl == VME_SEARCH_SINGLE_REF_SINGLE_REC_SINGLE_START ||
            search_ctrl == VME_SEARCH_SINGLE_REF_SINGLE_REC_DUAL_START ||
            search_ctrl == VME_SEARCH_SINGLE_REF_DUAL_REC ||
            search_ctrl == VME_SEARCH_DUAL_REF_DUAL_REC,
        "CISA ISA_VME_IME instruction uses illegal search ctrl.");

    i++; /// uni input
    i++; /// ime input

    uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
    REPORT_INSTRUCTION(options,
                       (unsigned)surface <
                           numPreDefinedSurfs + header->getSurfaceCount(),
                       "CISA ISA_VME_IME uses undeclared surface.");

    break;
  }
  case ISA_VME_SIC: {
    i++; /// uni input
    i++; /// sic input

    uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
    REPORT_INSTRUCTION(options,
                       (unsigned)surface <
                           numPreDefinedSurfs + header->getSurfaceCount(),
                       "CISA ISA_VME_SIC uses undeclared surface.");

    break;
  }
  case ISA_VME_FBR: {
    getRawOperand(inst, i++); // const raw_opnd& UNIInput
    getRawOperand(inst, i++); // const raw_opnd& FBRInput

    uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
    REPORT_INSTRUCTION(options,
                       (unsigned)surface <
                           numPreDefinedSurfs + header->getSurfaceCount(),
                       "CISA ISA_VME_FBR uses undeclared surface.");

    Common_ISA_Operand_Class operand_class_FBRMbMode =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(options,
                       operand_class_FBRMbMode == OPERAND_GENERAL ||
                           operand_class_FBRMbMode == OPERAND_INDIRECT ||
                           operand_class_FBRMbMode == OPERAND_IMMEDIATE,
                       "FBRMbMode operand of CISA VME_FBR instrution should "
                       "be either a general, indirect, or immediate operand.");

    Common_ISA_Operand_Class operand_class_FBRSubMbShape =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(
        options,
        operand_class_FBRSubMbShape == OPERAND_GENERAL ||
            operand_class_FBRSubMbShape == OPERAND_INDIRECT ||
            operand_class_FBRSubMbShape == OPERAND_IMMEDIATE,
        "FBRSubMbShape operand of CISA VME_FBR instrution should "
        "be either a general, indirect, or immediate operand.");

    Common_ISA_Operand_Class operand_class_FBRSubPredMode =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(
        options,
        operand_class_FBRSubPredMode == OPERAND_GENERAL ||
            operand_class_FBRSubPredMode == OPERAND_INDIRECT ||
            operand_class_FBRSubPredMode == OPERAND_IMMEDIATE,
        "FBRSubPredMode operand of CISA VME_FBR instrution should "
        "be either a general, indirect, or immediate operand.");

    getRawOperand(inst, i++); // const raw_opnd& output

    break;
  }
  case ISA_VME_IDM: {
    break;
  }
  case ISA_LOC: {
    break;
  }
  case ISA_FILE: {
    break;
  }
  case ISA_RAW_SEND: {
    getPrimitiveOperand<uint8_t>(inst, i++); // uint8_t modifier

    getPrimitiveOperand<uint8_t>(inst, i++); // unit8_t exMsgDesc

    uint8_t numSrc = getPrimitiveOperand<uint8_t>(inst, i++);
    REPORT_INSTRUCTION(
        options, numSrc >= 1 && numSrc <= 15,
        "Number of message source GRFs must be between 1 and 15");

    uint8_t numDst = getPrimitiveOperand<uint8_t>(inst, i++);
    REPORT_INSTRUCTION(
        options, numDst <= 16,
        "Number of message destination GRFs must be between 0 and 16");

    const vector_opnd &desc = getVectorOperand(inst, i++);
    Common_ISA_Operand_Class operand_class_desc = desc.getOperandClass();
    REPORT_INSTRUCTION(options,
                       operand_class_desc == OPERAND_GENERAL ||
                           operand_class_desc == OPERAND_INDIRECT ||
                           operand_class_desc == OPERAND_IMMEDIATE,
                       "desc operand of CISA RAW_SEND instrution should "
                       "be either a general, indirect, or immediate operand.");

    if (operand_class_desc == OPERAND_IMMEDIATE) {
      /// Structure describes a send message descriptor. Only expose
      /// several data fields; others are unnamed.
      struct MsgDescLayout {
        uint32_t funcCtrl : 19;     // Function control (bit 0:18)
        uint32_t headerPresent : 1; // Header present (bit 19)
        uint32_t rspLength : 5;     // Response length (bit 20:24)
        uint32_t msgLength : 4;     // Message length (bit 25:28)
        uint32_t simdMode2 : 1;     // 16-bit input (bit 29)
        uint32_t returnFormat : 1;  // 16-bit return (bit 30)
        uint32_t EOT : 1;           // EOT
      };

      /// View a message descriptor in two different ways:
      /// - as a 32-bit unsigned integer
      /// - as a structure
      /// This simplifies the implementation of extracting subfields.
      union DescData {
        uint32_t value;
        MsgDescLayout layout;
      } udesc;

      udesc.value = desc.opnd_val.const_opnd._val.ival;

      REPORT_INSTRUCTION(options, numSrc >= udesc.layout.msgLength,
                         "message length mismatch for raw send: msgLength (%d) "
                         "must be not greater than numSrc (%d)",
                         udesc.layout.msgLength, numSrc);
      REPORT_INSTRUCTION(options, numDst >= udesc.layout.rspLength,
                         "response length mismatch for raw send: rspLength "
                         "(%d) must be not greater than numDst (%d)",
                         udesc.layout.rspLength, numDst);
    }

    /// src: todo
    /// dst: todo

    break;
  }
  case ISA_RAW_SENDS: {
    getPrimitiveOperand<uint8_t>(inst, i++); // uint8_t modifier

    uint8_t numSrc0 = getPrimitiveOperand<uint8_t>(inst, i++);
    REPORT_INSTRUCTION(options, numSrc0 >= 1 && numSrc0 <= 32,
                       "Number of source0 GRFs must be between 1 and 32");

    uint8_t numSrc1 = getPrimitiveOperand<uint8_t>(inst, i++);
    REPORT_INSTRUCTION(options, numSrc1 <= 32,
                       "Number of source1 GRFs must be between 1 and 32");

    uint8_t numDst = getPrimitiveOperand<uint8_t>(inst, i++);
    REPORT_INSTRUCTION(
        options, numDst <= 32,
        "Number of message destination GRFs must be between 0 and 32");

    getPrimitiveOperand<uint8_t>(inst, i++); // uint8_t exMsgDesc

    Common_ISA_Operand_Class operand_class_desc =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(options,
                       operand_class_desc == OPERAND_GENERAL ||
                           operand_class_desc == OPERAND_INDIRECT ||
                           operand_class_desc == OPERAND_IMMEDIATE,
                       "desc operand of CISA RAW_SEND instrution should "
                       "be either a general, indirect, or immediate operand.");

    /// src0: todo
    /// src1: todo
    /// dst: todo

    break;
  }
  case ISA_RAW_SENDG: {
    (void)getPrimitiveOperand<uint8_t>(inst, i++); // modifier
    (void)getPrimitiveOperand<uint32_t>(inst, i++); // sfid
    //
    auto checkPayload = [&](
      const char *which, const raw_opnd &op, uint32_t lenB, uint32_t maxGRFs)
    {
      REPORT_INSTRUCTION(
          options, lenB <= maxGRFs * irBuilder->getGRFSize(),
          "%s: raw send payload length too large", which);
      (void)op; // TODO: check other stuff (e.g. null or GRF only?)
    };
    // There is no encoding for destination length in sendg*.
    // However, block2d allows 32 as a special case.
    const uint32_t MAX_DST_GRFS = 32;
    // For sources, we allow up to the max number of GRFs we can legally
    // encode, which is 31 (5-bits).  Different shared functions will
    // definitely have further restrictions, but the raw_sendg user is on their
    // own in figuring this out.
    const uint32_t MAX_SRC_GRFS = 31;
    const raw_opnd &dst = getRawOperand(inst, i++);
    uint32_t dstLenB = getPrimitiveOperand<uint32_t>(inst, i++);
    checkPayload("dst", dst, dstLenB, MAX_DST_GRFS);
    const raw_opnd &src0 = getRawOperand(inst, i++);
    uint32_t src0LenB = getPrimitiveOperand<uint32_t>(inst, i++);
    checkPayload("src0", src0, src0LenB, MAX_SRC_GRFS);
    const raw_opnd &src1 = getRawOperand(inst, i++);
    uint32_t src1LenB = getPrimitiveOperand<uint32_t>(inst, i++);
    checkPayload("src1", src1, src1LenB, MAX_SRC_GRFS);
    //
    const vector_opnd &ind0 = getVectorOperand(inst, i++);
    const vector_opnd &ind1 = getVectorOperand(inst, i++);
    (void)ind0; (void)ind1; // TODO: should be <0;1,0> and :q
    //
    uint32_t descL = getPrimitiveOperand<uint32_t>(inst, i++);
    uint32_t descH = getPrimitiveOperand<uint32_t>(inst, i++);
    uint64_t desc = (uint64_t)descL | ((uint64_t)descH << 32);
    // Desc[46:0] or Desc[41:0]
    const int numDescBits = 47; // TODO: only 42 if ind1 is set (not %null)
    bool descOob = desc & ~((1ull << numDescBits) - 1);
    REPORT_INSTRUCTION(
        options, !descOob, "descriptor sets invalid bits (too high)");
    break;
  }
  case ISA_3D_URB_WRITE: {
    uint8_t numOut = getPrimitiveOperand<uint8_t>(inst, i++);
    getPrimitiveOperand<uint16_t>(inst, i++); // uint16_t channelMask
    uint16_t globalOff = getPrimitiveOperand<uint16_t>(inst, i++);

    REPORT_INSTRUCTION(
        options, !(numOut < 1 || numOut > 8),
        "Valid range for num_out parameter of URB write is [1,8]");
    REPORT_INSTRUCTION(
        options, globalOff <= 2047,
        "Valid range for global_offset parameter of URB write is [0,2047]");
    REPORT_INSTRUCTION(
        options,
        inst->getExecSize() == EXEC_SIZE_1 ||
            inst->getExecSize() == EXEC_SIZE_8,
        "Only execution size of 1 or 8 is supported for URB write");

    break;
  }
  case ISA_DPAS:
  case ISA_DPASW:
  {
    auto FNIsInt = [](VISA_Type Ty) -> bool {
      return (Ty == ISA_TYPE_UD || Ty == ISA_TYPE_D);
    };
    auto FNIsIntOrFloat = [](VISA_Type Ty) -> bool {
      return (Ty == ISA_TYPE_UD || Ty == ISA_TYPE_D || Ty == ISA_TYPE_F);
    };
    auto FNIsIntOrFloatXePVC = [](VISA_Type Ty) -> bool {
      // Previously, W or UW means BF. keep it until visa's inputs no longer use
      // W/UW to mean BF!
      bool isHFOrBF = (Ty == ISA_TYPE_HF || Ty == ISA_TYPE_BF ||
                       Ty == ISA_TYPE_UW || Ty == ISA_TYPE_W);
      return (Ty == ISA_TYPE_UD || Ty == ISA_TYPE_D || Ty == ISA_TYPE_F ||
              isHFOrBF);
    };
       // No predicate
    REPORT_INSTRUCTION(options, inst->pred.isNullPred(),
                       "%s inst does not support predicate",
                       ISA_Inst_Table[opcode].str);

    // dst
    verifyRawOperand(inst, i);
    const raw_opnd &dst = getRawOperand(inst, i);
    if (irBuilder->getPlatform() < Xe_PVC) {
      verifyRawOperandType(inst, dst, FNIsIntOrFloat);
    }
    else {
      verifyRawOperandType(inst, dst, FNIsIntOrFloatXePVC);
    }

    // src0
    verifyRawOperand(inst, ++i);
    const raw_opnd &src0 = getRawOperand(inst, i);
    if (irBuilder->getPlatform() < Xe_PVC) {
      verifyRawOperandType(inst, src0, FNIsIntOrFloat);
    }
    else {
      verifyRawOperandType(inst, src0, FNIsIntOrFloatXePVC);
    }

    // src1
    verifyRawOperand(inst, ++i);
    const raw_opnd &src1 = getRawOperand(inst, i);
    {
      verifyRawOperandType(inst, src1, FNIsInt);
    }

    // src2
    const vector_opnd &src2 = getVectorOperand(inst, ++i);
    VISA_Type src2Ty = getVectorOperandType(header, src2);
    VISA_Modifier src2Mod = src2.getOperandModifier();
    REPORT_INSTRUCTION(options, src2Mod == MODIFIER_NONE,
                       "Modifier not allowed for %s",
                       ISA_Inst_Table[opcode].str);
    {
      REPORT_INSTRUCTION(options, FNIsInt(src2Ty), "Only U/UD allowed for %s",
                         ISA_Inst_Table[opcode].str);
    }


    if (irBuilder->getPlatform() >= Xe_PVC) {
      REPORT_INSTRUCTION(options, opcode != ISA_DPASW,
                         "%s instuction is not supported on selected platform",
                         ISA_Inst_Table[opcode].str);

      {
        // execsize must be simd16 for PVC
        REPORT_INSTRUCTION(
            options, inst->getExecSize() == EXEC_SIZE_16,
            "Only execution size of 16 is supported for %s on platform %s",
            ISA_Inst_Table[opcode].str, irBuilder->getGenxPlatformString());
      }
    } else {
      // execsize must be simd8 for XeHP_SDV
      REPORT_INSTRUCTION(
          options, inst->getExecSize() == EXEC_SIZE_8,
          "Only execution size of 8 is supported for %s on platform %s",
          ISA_Inst_Table[opcode].str, irBuilder->getGenxPlatformString());
    }

    break;
  }
  case ISA_BDPAS: {
    auto IsLegalDstOrSrc0Ty = [](VISA_Type Ty) -> bool {
      return Ty == ISA_TYPE_F || Ty == ISA_TYPE_BF || Ty == ISA_TYPE_HF;
    };
    auto IsLegalSrc1OrSrc2Ty = [](VISA_Type Ty) -> bool {
      return Ty == ISA_TYPE_BF || Ty == ISA_TYPE_HF || Ty == ISA_TYPE_UD;
    };
    // No predicate
    REPORT_INSTRUCTION(options, inst->pred.isNullPred(),
                       "%s inst does not support predicate",
                       ISA_Inst_Table[opcode].str);
    // execsize must be simd16
    REPORT_INSTRUCTION(
        options, inst->getExecSize() == EXEC_SIZE_16,
        "Only execution size of 16 is supported for %s",
        ISA_Inst_Table[opcode].str);
    // dst
    verifyRawOperand(inst, i);
    const raw_opnd &dst = getRawOperand(inst, i);
    verifyRawOperandType(inst, dst, IsLegalDstOrSrc0Ty);
    ++i;
    // src0
    verifyRawOperand(inst, i);
    const raw_opnd &src0 = getRawOperand(inst, i);
    verifyRawOperandType(inst, src0, IsLegalDstOrSrc0Ty);
    ++i;
    // src1
    verifyRawOperand(inst, i);
    const raw_opnd &src1 = getRawOperand(inst, i);
    verifyRawOperandType(inst, src1, IsLegalSrc1OrSrc2Ty);
    ++i;
    // src2
    verifyRawOperand(inst, i);
    const raw_opnd &src2 = getRawOperand(inst, i);
    verifyRawOperandType(inst, src2, IsLegalSrc1OrSrc2Ty);
    ++i;
    // TODO: Check src3/src4 subreg offset.
    // src3
    const vector_opnd &src3 = getVectorOperand(inst, i);
    if (src3.opnd_val.gen_opnd.index != 0) {
      VISA_Type src3Ty = getVectorOperandType(header, src3);
      REPORT_INSTRUCTION(
          options,
          src3Ty == ISA_TYPE_UB,
          "Only UB src3 allowed for %s",
          ISA_Inst_Table[opcode].str);
    }
    ++i;
    // src4
    const vector_opnd &src4 = getVectorOperand(inst, i);
    if (src4.opnd_val.gen_opnd.index != 0) {
      VISA_Type src4Ty = getVectorOperandType(header, src4);
      REPORT_INSTRUCTION(
          options,
          src4Ty == ISA_TYPE_UB,
          "Only UB src4 allowed for %s",
          ISA_Inst_Table[opcode].str);
    }
    break;
  }
  case ISA_LIFETIME: {
    uint8_t properties = getPrimitiveOperand<uint8_t>(inst, i++);
    getPrimitiveOperand<uint32_t>(inst, i++); // uint32_t varId

    unsigned char type = (properties >> 4) & 0x3;

    if (type != OPERAND_GENERAL && type != OPERAND_ADDRESS &&
        type != OPERAND_PREDICATE) {
      REPORT_INSTRUCTION(options, false, "Invalid encoding for register file");
    }

    break;
  }
  case ISA_BREAKPOINT:
    break;
  default:
    REPORT_INSTRUCTION(options, false,
                       "Illegal Miscellaneous Flow Instruction Opcode: %d, %s.",
                       opcode, ISA_Inst_Table[opcode].str);
  }
}

/// Returns true if this vector operand is an integer immediate constant and
/// its value fits into an expected type. Returns false, otherwise.
///
bool vISAVerifier::checkImmediateIntegerOpnd(const vector_opnd &opnd,
                                             VISA_Type expected_type) {
  vISA_ASSERT(IsIntType(expected_type), "integer type expected");

  // Not an immediate.
  if (!opnd.isImmediate())
    return false;

  VISA_Type opnd_type = opnd.getImmediateType();
  if (!IsIntOrIntVecType(opnd_type)) {
    return false;
  }

  bool is_vector = (opnd_type == ISA_TYPE_V || opnd_type == ISA_TYPE_UV);
  if (is_vector) {
    // if the operand type is unsigned, then always fits;
    // if the expected type is signed, then always fits;
    // otherwise, to be determined.
    if ((opnd_type == ISA_TYPE_UV) || IsSingedIntType(expected_type))
      return true;
  }

  // Retrieve the immediate integer value.
  int64_t val = 0;
  if (opnd_type == ISA_TYPE_Q || opnd_type == ISA_TYPE_UQ)
    val = typecastVals(&opnd.opnd_val.const_opnd._val.lval, opnd_type);
  else
    val = typecastVals(&opnd.opnd_val.const_opnd._val.ival, opnd_type);

  // The operand type is signed, and the expected type is unsigned.
  // All elements in this packed integer vector should be non-negative,
  // otherwise it does not fit.
  if (is_vector) {
    vISA_ASSERT(IsUnsignedIntType(expected_type), "unexpected signed type");

    // Truncate to 32 bit, each 4 bits consist of a vector component.
    // Sign bits are 3, 7, 11, 15, 19, 23, 27 and 31.
    return ((int32_t)val & 0x88888888) == 0;
  }

  switch (expected_type) {
  case ISA_TYPE_B:
    return (int8_t)val == val;
  case ISA_TYPE_UB:
    return (val >= 0) && ((uint8_t)val == val);
  case ISA_TYPE_W:
    return (int16_t)val == val;
  case ISA_TYPE_UW:
    return (val >= 0) && ((uint16_t)val == val);
  case ISA_TYPE_D:
    return (int32_t)val == val;
  case ISA_TYPE_UD:
    return (val >= 0) && ((uint32_t)val == val);
  case ISA_TYPE_Q: {
    // It does not fit into Q only if operand has type UQ and its value
    // exceeds the limit.
    if (opnd_type == ISA_TYPE_UQ) {
      // The actual value is to reprensent as an unsigned integer.
      uint64_t u_val = static_cast<uint64_t>(val);
      return u_val <= (uint64_t)std::numeric_limits<int64_t>::max();
    }

    // Fit for all other cases.
    return true;
  }
  case ISA_TYPE_UQ: {
    // UQ values fit.
    if (opnd_type == ISA_TYPE_UQ)
      return true;

    // Not UQ, but Q or smaller types.
    return val >= 0;
  }
  default:
    break;
  }
  return false;
}

void vISAVerifier::verifyInstructionArith(const CISA_INST *inst) {
  ISA_Opcode opcode = (ISA_Opcode)inst->opcode;

  if (opcode == ISA_SRND) {
    verifyInstructionSrnd(inst);
    return;
  }
  if (useGivenVISAType(inst, ISA_TYPE_BF)) {
    // Let bfmode verifier to verify it.
    return;
  }

  unsigned i = 0;
  const vector_opnd &dst = getVectorOperand(inst, i);
  VISA_Type dstType = getVectorOperandType(header, dst);
  VISA_Modifier dstModifier = dst.getOperandModifier();
  auto platform = irBuilder->getPlatform();

  if (opcode == ISA_PLANE)
    REPORT_INSTRUCTION(options, platform <= Xe3,
                       "plane instruction is not supported on this platform");
  REPORT_INSTRUCTION(options,
                     dst.getOperandClass() == OPERAND_GENERAL ||
                         dst.getOperandClass() == OPERAND_INDIRECT,
                     "Destination of CISA arithmetic instruction should be "
                     "general or indirect operand.");

  REPORT_INSTRUCTION(
      options, dstModifier == MODIFIER_NONE || dstModifier == MODIFIER_SAT,
      "Illegal destination modifier for CISA arithmetic instruction.");

  // check if sat is allowed for this instruction
  if (dstModifier == MODIFIER_SAT) {
    switch (opcode) {
    case ISA_FRC:
    case ISA_LZD:
    case ISA_MOD:
    case ISA_MULH:
    case ISA_MADW:
    case ISA_INVM:
    case ISA_RSQTM:
      REPORT_INSTRUCTION(options, false, "%s does not support saturation",
                         ISA_Inst_Table[opcode].str);
      break;
    case ISA_DIV:
      REPORT_INSTRUCTION(options,
                         dstType == ISA_TYPE_F || dstType == ISA_TYPE_HF ||
                             dstType == ISA_TYPE_DF,
                         "%s does not support saturation on integer types.",
                         ISA_Inst_Table[opcode].str);
    default:
      break;
    }
  }

  if (dstType == ISA_TYPE_DF) {
    if (opcode != ISA_MUL && opcode != ISA_ADD && opcode != ISA_MAD &&
        opcode != ISA_DIV && opcode != ISA_INV && opcode != ISA_SQRTM &&
        opcode != ISA_SQRT && opcode != ISA_DIVM &&
        opcode != ISA_INVM && opcode != ISA_RSQTM) {
      REPORT_INSTRUCTION(options, false,
                         "Only mul/add/mad/div/inv/sqrtm/sqrt/divm/invm/rsqtm "
                         "are allowed to use double precision floating point "
                         "operands.");
    }
  }

  if ((opcode == ISA_DIV && IsIntType(dstType)) || opcode == ISA_MOD) {
    REPORT_INSTRUCTION(
        options, platform < Xe_XeHPSDV,
        "int divide/remainder is not supported for this platform");
  }

  /// check dst type is supported by the instruction
  switch (opcode) {
  case ISA_COS:
  case ISA_EXP:
  case ISA_LOG:
  case ISA_POW:
  case ISA_SIN:
  case ISA_TANH:
  case ISA_SIGM:
    /// float and half float
    REPORT_INSTRUCTION(options, dstType == ISA_TYPE_F || dstType == ISA_TYPE_HF,
                       "%s only supports single and half float type",
                       ISA_Inst_Table[opcode].str);
    break;
  case ISA_RNDD:
  case ISA_RNDU:
  case ISA_RNDE:
  case ISA_RNDZ:
  case ISA_PLANE:
  case ISA_DP2:
  case ISA_DP3:
  case ISA_DP4:
  case ISA_DPH:
  case ISA_FRC:
  case ISA_LRP:
    /// float only
    REPORT_INSTRUCTION(options, dstType == ISA_TYPE_F,
                       "%s only supports single float type",
                       ISA_Inst_Table[opcode].str);
    break;
  case ISA_LINE:
    /// float or int only
    REPORT_INSTRUCTION(
        options, dstType == ISA_TYPE_F || IsIntType(dstType),
        "%s only supports integer and single precision float types",
        ISA_Inst_Table[opcode].str);
    break;
  case ISA_AVG:
  case ISA_MOD:
    /// int only
    REPORT_INSTRUCTION(options,
                       dstType == ISA_TYPE_UD || dstType == ISA_TYPE_D ||
                           dstType == ISA_TYPE_UW || dstType == ISA_TYPE_W ||
                           dstType == ISA_TYPE_UB || dstType == ISA_TYPE_B,
                       "%s only supports integer type",
                       ISA_Inst_Table[opcode].str);
    break;
  case ISA_LZD:
    /// UD only
    REPORT_INSTRUCTION(options, dstType == ISA_TYPE_UD,
                       "lzd only supports UD type");
    break;
  case ISA_MULH:
  case ISA_DP4A:
  case ISA_MADW:
    /// U or UD only
    REPORT_INSTRUCTION(options, dstType == ISA_TYPE_D || dstType == ISA_TYPE_UD,
                       "%s only support D/UD dst type",
                       ISA_Inst_Table[opcode].str);
    break;
  case ISA_SAD2:
  case ISA_SAD2ADD:
    /// dst must be w or uw
    REPORT_INSTRUCTION(options, dstType == ISA_TYPE_W || dstType == ISA_TYPE_UW,
                       "sad2/sad2add only supports W/UW dst type.");
    REPORT_INSTRUCTION(options,
                       irBuilder->getPlatformGeneration() < PlatformGen::XE,
                       "sad2/sad2add is not supported on Xe.");
    break;
  case ISA_ADDC:
  case ISA_SUBB:
    REPORT_INSTRUCTION(options, dstType == ISA_TYPE_UD,
                       "%s only supports single UD type",
                       ISA_Inst_Table[opcode].str);
    break;
  case ISA_ADD3:
  case ISA_ADD3O:
    REPORT_INSTRUCTION(options,
                       dstType == ISA_TYPE_UD || dstType == ISA_TYPE_D ||
                           dstType == ISA_TYPE_UW || dstType == ISA_TYPE_W,
                       "%s only supports integer D/W type",
                       ISA_Inst_Table[opcode].str);
    break;
  default:
    REPORT_INSTRUCTION(options,
                       dstType == ISA_TYPE_F || dstType == ISA_TYPE_DF ||
                           dstType == ISA_TYPE_HF || IsIntType(dstType),
                       "%s has illegal dst type", ISA_Inst_Table[opcode].str);
  }

  // verify each source operand
  for (unsigned i = 0; i < ISA_Inst_Table[opcode].n_srcs; i++) {
    const vector_opnd &src = getVectorOperand(
        inst,
        i + ISA_Inst_Table[opcode].n_dsts); /// dst is at index 0, addc/subbb
                                            /// have two destinations
    VISA_Type srcType = getVectorOperandType(header, src);

    if (!irBuilder->hasPackedRestrictedFloatVector())
      REPORT_INSTRUCTION(options, srcType != ISA_TYPE_VF,
                         ":vf datatype is not supported on this platform");

    VISA_Modifier srcModifier = src.getOperandModifier();

    REPORT_INSTRUCTION(
        options, opcode != ISA_LZD || srcModifier == MODIFIER_NONE,
        "lzd does not support source modifier");

    REPORT_INSTRUCTION(
        options, srcModifier != MODIFIER_SAT && srcModifier != MODIFIER_NOT,
        "unsupported source modifier for arithmetic instruction");

    REPORT_INSTRUCTION(options,
                       src.getOperandClass() == OPERAND_GENERAL ||
                           src.getOperandClass() == OPERAND_INDIRECT ||
                           src.getOperandClass() == OPERAND_IMMEDIATE,
                       "source in arithmetic instruction must be general, "
                       "indirect, or immediate");

    if (srcType == ISA_TYPE_DF) {
      if (opcode != ISA_MUL && opcode != ISA_ADD && opcode != ISA_MAD &&
          opcode != ISA_DIV && opcode != ISA_INV && opcode != ISA_SQRTM &&
          opcode != ISA_SQRT && opcode != ISA_DIVM &&
          opcode != ISA_INVM && opcode != ISA_RSQTM) {
        REPORT_INSTRUCTION(
            options, false,
            "Only mul/add/mad/div/inv/sqrtm/sqrt/divm/invm/rsqtm are allowed "
            "to use double precision floating point operands.");
      }
    }

    if (dstType == ISA_TYPE_F || dstType == ISA_TYPE_DF ||
        dstType == ISA_TYPE_HF || srcType == ISA_TYPE_DF ||
        srcType == ISA_TYPE_F || srcType == ISA_TYPE_HF) {
      REPORT_INSTRUCTION(
          options,
          dstType == srcType ||
              (dstType == ISA_TYPE_F && srcType == ISA_TYPE_VF) ||
              (dstType == ISA_TYPE_F && srcType == ISA_TYPE_HF) ||
              (dstType == ISA_TYPE_HF && srcType == ISA_TYPE_F),
          "Arithmetic instructions that use single or double precision or half "
          "float types "
          "must use the same type for all of their operannds: dst(%s) and "
          "src%d(%s).",
          CISATypeTable[dstType].typeName, i, CISATypeTable[srcType].typeName);
    } else {
      /// source must have integer type
      REPORT_INSTRUCTION(
          options,
          IsIntType(srcType) ||
              (src.getOperandClass() == OPERAND_IMMEDIATE &&
               (srcType == ISA_TYPE_V || srcType == ISA_TYPE_UV)),
          "immediate src%d has %d type, and it must have integer type", i,
          srcType);
    }

    switch (opcode) {
    case ISA_SAD2:
    case ISA_SAD2ADD: {
      bool is_valid_imm = false;
      if (i == 2) {
        is_valid_imm = checkImmediateIntegerOpnd(src, ISA_TYPE_W) ||
                       checkImmediateIntegerOpnd(src, ISA_TYPE_UW);
      } else {
        is_valid_imm = checkImmediateIntegerOpnd(src, ISA_TYPE_B) ||
                       checkImmediateIntegerOpnd(src, ISA_TYPE_UB);
      }
      REPORT_INSTRUCTION(
          options,
          is_valid_imm ||
              (i == 2 && (srcType == ISA_TYPE_W || srcType == ISA_TYPE_UW)) ||
              (i <= 1 && (srcType == ISA_TYPE_B || srcType == ISA_TYPE_UB)),
          "sad2/sad2add only supports B/UB types for src0 and src1; W/UW for "
          "src2 (sad2add). src%d has invalid type.",
          i);
      break;
    }
    case ISA_MUL:
    case ISA_DIV:
      REPORT_INSTRUCTION(options,
                         srcType != ISA_TYPE_Q && srcType != ISA_TYPE_UQ,
                         "mul does not support Q/UQ types for src%d", i);
      break;
    case ISA_DIVM:
      REPORT_INSTRUCTION(
          options,
          srcType == ISA_TYPE_F || srcType == ISA_TYPE_DF ||
              srcType == ISA_TYPE_VF,
          "ieee div does not support types for src%d, other than F/DF/VF", i);
      break;
    case ISA_SQRTM:
      REPORT_INSTRUCTION(
          options,
          srcType == ISA_TYPE_F || srcType == ISA_TYPE_DF ||
              srcType == ISA_TYPE_VF,
          "ieee sqrt does not support types for src%d, other than F/DF/VF", i);
      break;
    case ISA_INVM:
    case ISA_RSQTM:
      REPORT_INSTRUCTION(
          options,
          srcType == ISA_TYPE_F || srcType == ISA_TYPE_DF,
          "invm/rsqtm do not support types for src%d, other than F/DF", i);
      break;
    case ISA_ADDC:
    case ISA_SUBB: {
      REPORT_INSTRUCTION(options,
                         srcType == ISA_TYPE_UD || srcType == ISA_TYPE_UV,
                         "%s src0 and src1 only supports single UD type",
                         ISA_Inst_Table[opcode].str);
      break;
    }
    case ISA_DP4A:
      REPORT_INSTRUCTION(options,
                         srcType == ISA_TYPE_D || srcType == ISA_TYPE_UD,
                         "%s src0 and src1 only supports single UD type",
                         ISA_Inst_Table[opcode].str);
      break;
    case ISA_ADD3:
    case ISA_ADD3O:
      REPORT_INSTRUCTION(options,
                         srcType == ISA_TYPE_D || srcType == ISA_TYPE_UD ||
                             srcType == ISA_TYPE_W || srcType == ISA_TYPE_UW,
                         "%s src operand only supports integer D/W type",
                         ISA_Inst_Table[opcode].str);
      break;
    default:
      break; // Prevent gcc warning
    }
  }

  // check for IEEE macros support
  // !hasMadm() check
  bool noMadm = platform == GENX_ICLLP || platform == GENX_TGLLP ||
                platform == Xe_DG2 || platform == Xe_MTL || platform == Xe_ARL;
  if (noMadm) {
    bool fOpcodeIEEE = (opcode == ISA_DIVM) || (opcode == ISA_SQRTM);
    bool dfOpcodeIEEE = fOpcodeIEEE || (opcode == ISA_INV) ||
                        (opcode == ISA_DIV) || (opcode == ISA_SQRT);
    REPORT_INSTRUCTION(options,
                       !(dstType == ISA_TYPE_DF && dfOpcodeIEEE) &&
                           !(dstType == ISA_TYPE_F && fOpcodeIEEE),
                       "IEEE instruction %s is not supported on %s platform",
                       ISA_Inst_Table[opcode].str,
                       irBuilder->getGenxPlatformString());

    REPORT_INSTRUCTION(options,
                       (opcode != ISA_INVM && opcode != ISA_RSQTM),
                       "Instruction %s is not supported on %s platform",
                       ISA_Inst_Table[opcode].str,
                       irBuilder->getGenxPlatformString());
  }
}

void vISAVerifier::verifyInstructionLogic(const CISA_INST *inst) {
  ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
  if (opcode == ISA_LFSR) {
    verifyInstructionLfsr(inst);
    return;
  }

  bool pred_logic = false;
  unsigned opend_count = inst->opnd_num;
  if (opcode == ISA_BFN) {
    // check opnd type of the last opend, which is BooleanFuncCtrl
    REPORT_INSTRUCTION(options,
                       getOperandType(inst, opend_count - 1) == CISA_OPND_OTHER,
                       "last opnd should be CISA_OPND_OTHER type");
    // skip below type check for booleanFuncCtrl
    --opend_count;
  }
  for (unsigned i = 0; i < opend_count; i++) {
    const vector_opnd &opnd = getVectorOperand(inst, i);
    VISA_Type opnd_type = getVectorOperandType(header, opnd);

    REPORT_INSTRUCTION(options, opnd.getOperandClass() != OPERAND_ADDRESS,
                       "Common ISA Logic instrutions are not allowed to have "
                       "address operands.");

    REPORT_INSTRUCTION(options, !pred_logic || opnd_type == ISA_TYPE_BOOL,
                       "Operand type of logic operantion for predicate "
                       "operands should all be BOOL "
                       "(ie if one operand is BOOL they all have to be BOOL).");

    if (i == 0 && ((opcode == ISA_SHL &&
                    (opnd_type == ISA_TYPE_Q || opnd_type == ISA_TYPE_UQ)) ||
                   (opcode == ISA_SHR && opnd_type == ISA_TYPE_UQ))) {
      REPORT_INSTRUCTION(
          options,
          irBuilder->getPlatform() <= Xe_XeHPSDV ||
              irBuilder->getPlatform() >= Xe_PVC,
          "*Q destination data type is not supported for this platform");
    }

    if (opcode == ISA_ROR || opcode == ISA_ROL) {
      switch (opnd_type) {
      case ISA_TYPE_B:
      case ISA_TYPE_UB:
        REPORT_INSTRUCTION(options, false, "ror/rol does not support i8 types");
        break;
      case ISA_TYPE_UQ:
      case ISA_TYPE_Q:
        // This string was changed from "ror/rol only support i64 types on PVC+"
        // due to IP leak concers.
        REPORT_INSTRUCTION(
            options, irBuilder->getPlatform() >= Xe_PVC,
            "ror/rol does not support i64 types on the selected platform");
        break;
      default:
        break;
      }
    }

    switch (opnd_type) {
    case ISA_TYPE_B:
    case ISA_TYPE_UB:
    case ISA_TYPE_W:
    case ISA_TYPE_D:
    case ISA_TYPE_UW:
    case ISA_TYPE_UD:
    case ISA_TYPE_V:
    case ISA_TYPE_UV:
      break;
    case ISA_TYPE_UQ:
    case ISA_TYPE_Q:
      REPORT_INSTRUCTION(
          options, opcode != ISA_FBL && opcode != ISA_FBH && opcode != ISA_CBIT,
          "fbl/fbh/cbit does not support Q/UQ type.");
      break;
    case ISA_TYPE_BOOL: {
      pred_logic = true;
      REPORT_INSTRUCTION(options, inst->pred.isNullPred(),
                         "Predicate can not be used in logic operantion for "
                         "predicate operands.");
      break;
    }
    case ISA_TYPE_DF:
    case ISA_TYPE_F:
    case ISA_TYPE_HF: {
      REPORT_INSTRUCTION(options, false,
                         "All operands of logic instructions must be of "
                         "integral type! opnd %d has float type %d",
                         i, (int)(opnd_type));
      break;
    }
    default: {
      REPORT_INSTRUCTION(options, false,
                         "All operands of logic instructions must be of "
                         "integral type! opnd %d has unknown type %d",
                         i, (int)(opnd_type));
    }
    }
  }
}

void vISAVerifier::verifyInstructionCompare(const CISA_INST *inst) {
  ///     opnd0              opnd1  opnd2 opnd3
  /// cmp.rel_op (exec_size) dst    src1  src2

  [[maybe_unused]] ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
  vISA_ASSERT(ISA_CMP == opcode, "illegal opcode for compare instruction");

  for (unsigned i = 0; i < inst->opnd_num; i++) {
    if (i > 0) {
      Common_ISA_Operand_Class operand_class =
          getVectorOperand(inst, i).getOperandClass();
      switch (i) {
      case 1:
        REPORT_INSTRUCTION(options,
                           operand_class == OPERAND_PREDICATE ||
                               operand_class == OPERAND_GENERAL,
                           "CISA compare instruction destination only supports "
                           "a predicate operand.");
        break;
      default:
        REPORT_INSTRUCTION(options,
                           operand_class != OPERAND_ADDRESS &&
                               operand_class != OPERAND_PREDICATE,
                           "CISA compare instruction sources do not support "
                           "address or predicate operands.");
        break;
      }
    } else {
      /// TODO: Verify rel_op
    }
  }
}

void vISAVerifier::verifyInstructionAddress(const CISA_INST *inst) {
  [[maybe_unused]] ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
  vISA_ASSERT(ISA_ADDR_ADD == opcode,
              "Illegal opcode for address instruction.");

  for (unsigned i = 0; i < inst->opnd_num; i++) {
    Common_ISA_Operand_Class operand_class =
        getVectorOperand(inst, i).getOperandClass();

    if (0 == i) {
      REPORT_INSTRUCTION(options, operand_class == OPERAND_ADDRESS,
                         "CISA address instruction destination only supports "
                         "an address operand.");
      continue;
    }

    REPORT_INSTRUCTION(
        options, operand_class != OPERAND_PREDICATE,
        "CISA ADDR_ADD instruction sources do not support predicate operands.");
    if (1 == i) {
      if (operand_class == OPERAND_GENERAL) {
        uint32_t numPredefinedVars = Get_CISA_PreDefined_Var_Count();
        uint32_t varIndex = getVectorOperand(inst, i).opnd_val.gen_opnd.index;
        REPORT_INSTRUCTION(
            options, varIndex >= numPredefinedVars,
            "Can not take the address of a pre-defined variable");
      } else if (operand_class == OPERAND_STATE) {
        if (getVectorOperand(inst, i).getStateOpClass() == STATE_OPND_SURFACE) {
          uint32_t numPredefinedSurfs = Get_CISA_PreDefined_Surf_Count();
          uint32_t surfIndex =
              getVectorOperand(inst, i).opnd_val.state_opnd.index;
          REPORT_INSTRUCTION(
              options, surfIndex >= numPredefinedSurfs,
              "Can not take the address of a pre-defined surface");
        }
      }
    }

    if (2 == i) {
      VISA_Type opnd_type =
          getVectorOperandType(header, getVectorOperand(inst, i));
      REPORT_INSTRUCTION(
          options,
          opnd_type == ISA_TYPE_B || opnd_type == ISA_TYPE_UB ||
              opnd_type == ISA_TYPE_W || opnd_type == ISA_TYPE_UW,
          "Data type of the second source of ADDR_ADD should be WORD or BYTE.");
    }
  }
}

void vISAVerifier::verifyInstructionSampler(const CISA_INST *inst) {
  ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
  unsigned numPreDefinedSurfs = Get_CISA_PreDefined_Surf_Count();
  unsigned i = 0;

  switch (opcode) {
  case ISA_SAMPLE_UNORM: {
    uint8_t channel = getPrimitiveOperand<uint8_t>(inst, i++);
    REPORT_INSTRUCTION(options, channel,
                       "CISA SAMPLER ISA_SAMPLE_UNORM instruction only accepts "
                       "non-zero channel masks.");

    uint8_t sampler = getPrimitiveOperand<uint8_t>(inst, i++);
    REPORT_INSTRUCTION(
        options, sampler < header->getSamplerCount(),
        "CISA SAMPLER ISA_SAMPLE_UNORM instruction uses undeclared sampler.");

    uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
    REPORT_INSTRUCTION(options, 0 != surface,
                       "Surface T0 (the SLM surface) is not allowed for the "
                       "SAMPLER instruction ISA_SAMPLE_UNORM.");
    REPORT_INSTRUCTION(
        options, surface - numPreDefinedSurfs < header->getSurfaceCount(),
        "CISA SAMPLER instruction ISA_SAMPLE_UNORM uses undefined surface.");

    Common_ISA_Operand_Class operand_class_uoff =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(
        options,
        operand_class_uoff == OPERAND_GENERAL ||
            operand_class_uoff == OPERAND_INDIRECT ||
            operand_class_uoff == OPERAND_IMMEDIATE,
        "u_offset operand of CISA SAMPLER ISA_SAMPLE_UNORM instrution should "
        "be either a general, indirect, or immediate operand.");

    Common_ISA_Operand_Class operand_class_voff =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(
        options,
        operand_class_voff == OPERAND_GENERAL ||
            operand_class_voff == OPERAND_INDIRECT ||
            operand_class_voff == OPERAND_IMMEDIATE,
        "v_offset operand of CISA SAMPLER ISA_SAMPLE_UNORM instrution should "
        "be either a general, indirect, or immediate operand.");

    Common_ISA_Operand_Class operand_class_udelta =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(
        options,
        operand_class_udelta == OPERAND_GENERAL ||
            operand_class_udelta == OPERAND_INDIRECT ||
            operand_class_udelta == OPERAND_IMMEDIATE,
        "u_delta operand of CISA SAMPLER ISA_SAMPLE_UNORM instrution should "
        "be either a general, indirect, or immediate operand.");

    Common_ISA_Operand_Class operand_class_vdelta =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(
        options,
        operand_class_vdelta == OPERAND_GENERAL ||
            operand_class_vdelta == OPERAND_INDIRECT ||
            operand_class_vdelta == OPERAND_IMMEDIATE,
        "v_delta operand of CISA SAMPLER ISA_SAMPLE_UNORM instrution should "
        "be either a general, indirect, or immediate operand.");

    /// dst: TODO

    break;
  }
  case ISA_LOAD:
  case ISA_SAMPLE: {
    uint8_t mod = getPrimitiveOperand<uint8_t>(inst, i++);

    if (opcode == ISA_SAMPLE) {
      uint8_t sampler = getPrimitiveOperand<uint8_t>(inst, i++);
      REPORT_INSTRUCTION(
          options, sampler < header->getSamplerCount(),
          "CISA SAMPLER SAMPLE/LOAD instruction uses undeclared sampler.");
    }

    uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
    REPORT_INSTRUCTION(options, 0 != surface,
                       "Surface T0 (the SLM surface) is not allowed for the "
                       "SAMPLER instruction ISA_SAMPLE/ISA_LOAD.");
    REPORT_INSTRUCTION(
        options, surface - numPreDefinedSurfs < header->getSurfaceCount(),
        "CISA SAMPLER instruction ISA_SAMPLE/ISA_LOAD uses undefined surface.");

    uint8_t channel = mod & 0xF;
    REPORT_INSTRUCTION(options, channel,
                       "CISA SAMPLER ISA_SAMPLE/ISA_LOAD instruction only "
                       "accepts non-zero channel masks.");

    uint8_t SIMD_mode = (mod >> 4) & 0x3;
    if ((unsigned)SIMD_mode == 0) {
      SIMD_mode = 8;
    } else if ((unsigned)SIMD_mode == 1) {
      SIMD_mode = 16;
    } else {
      REPORT_INSTRUCTION(options, false,
                         "Illegal SIMD mode used in ISA_SAMPLE/ISA_LOAD inst.");
    }

    /// u/r/v/dst:TODO

    break;
  }
  case ISA_3D_SAMPLE: {
    uint16_t value = getPrimitiveOperand<uint16_t>(inst, i++);
    bool pixelNullMask = (value & (1 << 8)) != 0;
    bool cpsEnable = (value & (1 << 9)) != 0;
    VISASampler3DSubOpCode subOp = VISASampler3DSubOpCode(value & 0xFF);

    if (pixelNullMask) {
      REPORT_INSTRUCTION(options, irBuilder->getPlatform() >= GENX_SKL,
                         "Pixel Null Mask Enable only valid for SKL+");
    }

    if (cpsEnable) {
      auto execSize = inst->getExecSize();

      REPORT_INSTRUCTION(options,
                         execSize == EXEC_SIZE_8 || execSize == EXEC_SIZE_16,
                         "CPS LOD Compensation Enable must be disabled unless"
                         " SIMD mode is simd8* or simd16*");

      bool isSupportedOp = subOp == VISA_3D_SAMPLE ||
                           subOp == VISA_3D_SAMPLE_B ||
                           subOp == VISA_3D_SAMPLE_C ||
                           subOp == VISA_3D_SAMPLE_B_C ||
                           subOp == VISA_3D_LOD ||
                           subOp == VISA_3D_SAMPLE_PO ||
                           subOp == VISA_3D_SAMPLE_PO_B ||
                           subOp == VISA_3D_SAMPLE_PO_C;

      REPORT_INSTRUCTION(options, isSupportedOp,
                         "CPS LOD Compensation Enable is only supported for"
                         " sample, sample_b, sample_b_c, sample_c and LOD");
    }
    bool isPairedSurface = isNotNullRawOperand(inst, 8);
    REPORT_INSTRUCTION(
        options,
        (isPairedSurface && irBuilder->getPlatform() >= Xe2) ||
            !isPairedSurface,
        "Paired surface can not be defined on this platform");

    break;
  }
  case ISA_3D_LOAD:
  case ISA_3D_GATHER4: {
    uint16_t value = getPrimitiveOperand<uint16_t>(inst, i++);
    bool pixelNullMask = (value & (1 << 8)) != 0;

    if (pixelNullMask) {
      REPORT_INSTRUCTION(options, irBuilder->getPlatform() >= GENX_SKL,
                         "Pixel Null Mask Enable only valid for SKL+");
    }

    bool isPairedSurface =
        isNotNullRawOperand(inst, opcode == ISA_3D_LOAD ? 6 : 8);

    REPORT_INSTRUCTION(
        options,
        (isPairedSurface && irBuilder->getPlatform() >= Xe2) ||
            !isPairedSurface,
        "Paired surface can not be defined on this platform");
    break;
  }
  case ISA_3D_INFO:
  case ISA_3D_RT_WRITE:
  case ISA_3D_URB_WRITE: {
    // TODO: Add verification code for 3d specific opcodes
    break;
  }
  case ISA_AVS: {
    uint8_t channel = getPrimitiveOperand<uint8_t>(inst, i++);
    REPORT_INSTRUCTION(
        options, channel,
        "CISA SAMPLER AVS instruction only accepts non-zero channel masks.");

    uint8_t sampler = getPrimitiveOperand<uint8_t>(inst, i++);
    REPORT_INSTRUCTION(
        options, sampler < header->getSamplerCount(),
        "CISA VA MINMAXFILTER instruction uses undeclared sampler.");

    uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
    REPORT_INSTRUCTION(options, 0 != surface,
                       "Surface T0 (the SLM surface) is not allowed for the "
                       "SAMPLER AVS instruction.");
    REPORT_INSTRUCTION(options,
                       surface < numPreDefinedSurfs + header->getSurfaceCount(),
                       "CISA VA instruction MINMAX uses undefined surface.");

    Common_ISA_Operand_Class operand_class_uoff =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(options,
                       operand_class_uoff == OPERAND_GENERAL ||
                           operand_class_uoff == OPERAND_INDIRECT ||
                           operand_class_uoff == OPERAND_IMMEDIATE,
                       "u_offset operand of CISA SAMPLER AVS instrution should "
                       "be either a general, indirect, or immediate operand.");

    Common_ISA_Operand_Class operand_class_voff =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(options,
                       operand_class_voff == OPERAND_GENERAL ||
                           operand_class_voff == OPERAND_INDIRECT ||
                           operand_class_voff == OPERAND_IMMEDIATE,
                       "v_offset operand of CISA SAMPLER AVS instrution should "
                       "be either a general, indirect, or immediate operand.");

    Common_ISA_Operand_Class operand_class_udelta =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(options,
                       operand_class_udelta == OPERAND_GENERAL ||
                           operand_class_udelta == OPERAND_INDIRECT ||
                           operand_class_udelta == OPERAND_IMMEDIATE,
                       "u_delta operand of CISA SAMPLER AVS instrution should "
                       "be either a general, indirect, or immediate operand.");

    Common_ISA_Operand_Class operand_class_vdelta =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(options,
                       operand_class_vdelta == OPERAND_GENERAL ||
                           operand_class_vdelta == OPERAND_INDIRECT ||
                           operand_class_vdelta == OPERAND_IMMEDIATE,
                       "v_delta operand of CISA SAMPLER AVS instrution should "
                       "be either a general, indirect, or immediate operand.");

    Common_ISA_Operand_Class operand_class_u2d =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(options,
                       operand_class_u2d == OPERAND_GENERAL ||
                           operand_class_u2d == OPERAND_INDIRECT ||
                           operand_class_u2d == OPERAND_IMMEDIATE,
                       "u2d operand of CISA SAMPLER AVS instrution should "
                       "be either a general, indirect, or immediate operand.");

    Common_ISA_Operand_Class operand_class_groupid =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(options,
                       operand_class_groupid == OPERAND_GENERAL ||
                           operand_class_groupid == OPERAND_INDIRECT ||
                           operand_class_groupid == OPERAND_IMMEDIATE,
                       "groupid operand of CISA SAMPLER AVS instrution should "
                       "be either a general, indirect, or immediate operand.");

    Common_ISA_Operand_Class operand_class_verticalBlockNumber =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(
        options,
        operand_class_verticalBlockNumber == OPERAND_GENERAL ||
            operand_class_verticalBlockNumber == OPERAND_INDIRECT ||
            operand_class_verticalBlockNumber == OPERAND_IMMEDIATE,
        "verticalBlockNumber operand of CISA SAMPLER AVS instrution should "
        "be either a general, indirect, or immediate operand.");

    uint8_t cntrl = ((getPrimitiveOperand<uint8_t>(inst, i++)) & 0xF);

    switch (cntrl) {
    case 0:
    case 1:
    case 2:
    case 3:
      break;
    default:
      REPORT_INSTRUCTION(options, false,
                         "cntrl for CISA SAMPLER AVS instruction should be a "
                         "value 0-3 (8/16bit full/chrominance down sample).");
    }

    Common_ISA_Operand_Class operand_class_v2d =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(options,
                       operand_class_v2d == OPERAND_GENERAL ||
                           operand_class_v2d == OPERAND_INDIRECT ||
                           operand_class_v2d == OPERAND_IMMEDIATE,
                       "v2d operand of CISA SAMPLER AVS instrution should "
                       "be either a general, indirect, or immediate operand.");

    uint8_t execMode = ((getPrimitiveOperand<uint8_t>(inst, i++)) & 0xF);

    switch (execMode) {
    case 0:
    case 1:
    case 2:
    case 3:
      break;
    default:
      REPORT_INSTRUCTION(options, false,
                         "execMode for CISA SAMPLER AVS instruction should "
                         "be a value 0-3 (16x4, 8x4, 16x8, or 4x4).");
    }

    Common_ISA_Operand_Class operand_class_iefbypass =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(
        options,
        operand_class_iefbypass == OPERAND_GENERAL ||
            operand_class_iefbypass == OPERAND_INDIRECT ||
            operand_class_iefbypass == OPERAND_IMMEDIATE,
        "iefbypass operand of CISA SAMPLER AVS instruction should "
        "be either a general, indirect, or immediate operand.");

    /// dst: TODO

    break;
  }
  case ISA_VA: {
    ISA_VA_Sub_Opcode subOpcode =
        (ISA_VA_Sub_Opcode)getPrimitiveOperand<uint8_t>(inst, i++);
    switch (subOpcode) {
    case MINMAX_FOPCODE: {
      uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
      REPORT_INSTRUCTION(
          options, 0 != surface,
          "Surface T0 (the SLM surface) is not allowed for VA instructions.");
      REPORT_INSTRUCTION(
          options, surface - numPreDefinedSurfs < header->getSurfaceCount(),
          "CISA VA instruction MINMAX uses undefined surface.");

      Common_ISA_Operand_Class operand_class_uoff =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_uoff == OPERAND_GENERAL ||
              operand_class_uoff == OPERAND_INDIRECT ||
              operand_class_uoff == OPERAND_IMMEDIATE,
          "u_offset operand of CISA MINMAX instrution should "
          "be either a general, indirect, or immediate operand.");

      Common_ISA_Operand_Class operand_class_voff =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_voff == OPERAND_GENERAL ||
              operand_class_voff == OPERAND_INDIRECT ||
              operand_class_voff == OPERAND_IMMEDIATE,
          "v_offset operand of CISA MINMAX instrution should "
          "be either a general, indirect, or immediate operand.");

      /// mmf mode
      const vector_opnd &mmf = getVectorOperand(inst, i++);
      if (mmf.getOperandClass() == OPERAND_IMMEDIATE) {
        [[maybe_unused]] unsigned val = mmf.opnd_val.const_opnd._val.ival;
        vISA_ASSERT(val <= VA_MIN_ENABLE,
                    "MINMAX MMF Mode operand out of range.");
      }

      /// dst: TODO

      break;
    }
    case MINMAXFILTER_FOPCODE: {
      uint8_t sampler = getPrimitiveOperand<uint8_t>(inst, i++);
      REPORT_INSTRUCTION(
          options, sampler < header->getSamplerCount(),
          "CISA VA MINMAXFILTER instruction uses undeclared sampler.");

      uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
      REPORT_INSTRUCTION(
          options, 0 != surface,
          "Surface T0 (the SLM surface) is not allowed for VA instructions.");
      REPORT_INSTRUCTION(
          options, surface - numPreDefinedSurfs < header->getSurfaceCount(),
          "CISA VA instruction MINMAXFILTER uses undefined surface.");

      Common_ISA_Operand_Class operand_class_uoff =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_uoff == OPERAND_GENERAL ||
              operand_class_uoff == OPERAND_INDIRECT ||
              operand_class_uoff == OPERAND_IMMEDIATE,
          "u_offset operand of CISA MINMAXFILTER instrution should "
          "be either a general, indirect, or immediate operand.");

      Common_ISA_Operand_Class operand_class_voff =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_voff == OPERAND_GENERAL ||
              operand_class_voff == OPERAND_INDIRECT ||
              operand_class_voff == OPERAND_IMMEDIATE,
          "v_offset operand of CISA MINMAXFILTER instrution should "
          "be either a general, indirect, or immediate operand.");

      getPrimitiveOperand<uint8_t>(inst, i++); // uint8_t cntrl, &0xf
      getPrimitiveOperand<uint8_t>(inst, i++); // uint8_t execMode, &0xf

      /// mmf mode
      const vector_opnd &mmf = getVectorOperand(inst, i++);
      if (mmf.getOperandClass() == OPERAND_IMMEDIATE) {
        [[maybe_unused]] unsigned val = mmf.opnd_val.const_opnd._val.ival;
        vISA_ASSERT(val <= VA_MIN_ENABLE,
                    "MINMAXFILTER MMF Mode operand out of range.");
      }

      /// dst

      break;
    }
    case BoolCentroid_FOPCODE:
    case Centroid_FOPCODE: {
      uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
      REPORT_INSTRUCTION(
          options, 0 != surface,
          "Surface T0 (the SLM surface) is not allowed for VA instructions.");
      REPORT_INSTRUCTION(
          options, surface - numPreDefinedSurfs < header->getSurfaceCount(),
          "CISA VA VA CENTROID/BOOLCENTROID instruction MINMAX uses undefined "
          "surface.");

      Common_ISA_Operand_Class operand_class_uoff =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_uoff == OPERAND_GENERAL ||
              operand_class_uoff == OPERAND_INDIRECT ||
              operand_class_uoff == OPERAND_IMMEDIATE,
          "u_offset operand of CISA VA CENTROID/BOOLCENTROID instrution should "
          "be either a general, indirect, or immediate operand.");

      Common_ISA_Operand_Class operand_class_voff =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_voff == OPERAND_GENERAL ||
              operand_class_voff == OPERAND_INDIRECT ||
              operand_class_voff == OPERAND_IMMEDIATE,
          "v_offset operand of CISA VA CENTROID/BOOLCENTROID instrution should "
          "be either a general, indirect, or immediate operand.");

      Common_ISA_Operand_Class operand_class_vsize =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_vsize == OPERAND_GENERAL ||
              operand_class_vsize == OPERAND_INDIRECT ||
              operand_class_vsize == OPERAND_IMMEDIATE,
          "v_size operand of CISA VA CENTROID/BOOLCENTROID instrution should "
          "be either a general, indirect, or immediate operand.");

      /// h size
      if (subOpcode == BoolCentroid_FOPCODE) {
        Common_ISA_Operand_Class operand_class_hsize =
            getVectorOperand(inst, i++).getOperandClass();
        REPORT_INSTRUCTION(
            options,
            operand_class_hsize == OPERAND_GENERAL ||
                operand_class_hsize == OPERAND_INDIRECT ||
                operand_class_hsize == OPERAND_IMMEDIATE,
            "h_size operand of CISA VA CENTROID/BOOLCENTROID instrution should "
            "be either a general, indirect, or immediate operand.");
      }

      /// dst: TODO

      break;
    }
    case Convolve_FOPCODE:
    case Dilate_FOPCODE:
    case ERODE_FOPCODE: {
      uint8_t sampler = getPrimitiveOperand<uint8_t>(inst, i++);
      REPORT_INSTRUCTION(
          options, sampler < header->getSamplerCount(),
          "CISA VA CONVOLVE/ERODE/DILATE instruction uses undeclared sampler.");

      uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
      REPORT_INSTRUCTION(
          options, 0 != surface,
          "Surface T0 (the SLM surface) is not allowed for VA instructions.");
      REPORT_INSTRUCTION(
          options, surface - numPreDefinedSurfs < header->getSurfaceCount(),
          "CISA VA CONVOLVE/ERODE/DILATE instruction MINMAX uses undefined "
          "surface.");

      Common_ISA_Operand_Class operand_class_uoff =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_uoff == OPERAND_GENERAL ||
              operand_class_uoff == OPERAND_INDIRECT ||
              operand_class_uoff == OPERAND_IMMEDIATE,
          "u_offset operand of CISA CONVOLVE/ERODE/DILATE instrution should "
          "be either a general, indirect, or immediate operand.");

      Common_ISA_Operand_Class operand_class_voff =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_voff == OPERAND_GENERAL ||
              operand_class_voff == OPERAND_INDIRECT ||
              operand_class_voff == OPERAND_IMMEDIATE,
          "v_offset operand of CISA CONVOLVE/ERODE/DILATE instrution should "
          "be either a general, indirect, or immediate operand.");

      /// uint8_t execMode   = ((getPrimitiveOperand<uint8_t>(inst, i)) & 0x3);
      /// uint8_t regionSize = ((getPrimitiveOperand<uint8_t>(inst, i)) & 0xC)
      /// >> 0x2; dst: TODO

      break;
    }
    default:
      REPORT_INSTRUCTION(options, false, "Invalid VA sub-opcode: %d.",
                         subOpcode);
    }

    break;
  }
  case ISA_VA_SKL_PLUS: {
    ISA_VA_Sub_Opcode subOpcode =
        (ISA_VA_Sub_Opcode)getPrimitiveOperand<uint8_t>(inst, i++);
    switch (subOpcode) {
    case VA_OP_CODE_LBP_CORRELATION: {
      uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
      REPORT_INSTRUCTION(
          options, 0 != surface,
          "Surface T0 (the SLM surface) is not allowed for VA++ instructions.");
      REPORT_INSTRUCTION(
          options, surface - numPreDefinedSurfs < header->getSurfaceCount(),
          "CISA VA++ instruction LBP Correlation uses undefined surface.");

      Common_ISA_Operand_Class operand_class_uoff =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_uoff == OPERAND_GENERAL ||
              operand_class_uoff == OPERAND_INDIRECT ||
              operand_class_uoff == OPERAND_IMMEDIATE,
          "u_offset operand of CISA LBP Correlation instrution "
          "should be either a general, indirect, or immediate operand.");

      Common_ISA_Operand_Class operand_class_voff =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_voff == OPERAND_GENERAL ||
              operand_class_voff == OPERAND_INDIRECT ||
              operand_class_voff == OPERAND_IMMEDIATE,
          "v_offset operand of CISA LBP Correlation instrution "
          "should be either a general, indirect, or immediate operand.");

      Common_ISA_Operand_Class operand_class_disparity =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_disparity == OPERAND_GENERAL ||
              operand_class_disparity == OPERAND_INDIRECT ||
              operand_class_disparity == OPERAND_IMMEDIATE,
          "Disparity operand of CISA LBP Correlation instrution "
          "should be either a general, indirect, or immediate operand.");

      /// dst
      /// TODO: Check dst for type W for for GRF alignment

      break;
    }
    case VA_OP_CODE_1PIXEL_CONVOLVE:
    case VA_OP_CODE_1D_CONVOLVE_VERTICAL:
    case VA_OP_CODE_1D_CONVOLVE_HORIZONTAL: {
      uint8_t sampler = getPrimitiveOperand<uint8_t>(inst, i++);
      REPORT_INSTRUCTION(options, sampler < header->getSamplerCount(),
                         "CISA VA++ instruction uses undeclared sampler.");

      uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
      REPORT_INSTRUCTION(
          options, 0 != surface,
          "Surface T0 (the SLM surface) is not allowed for VA++ instructions.");
      REPORT_INSTRUCTION(
          options, surface - numPreDefinedSurfs < header->getSurfaceCount(),
          "CISA VA++ instruction uses undefined surface.");

      Common_ISA_Operand_Class operand_class_uoff =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_uoff == OPERAND_GENERAL ||
              operand_class_uoff == OPERAND_INDIRECT ||
              operand_class_uoff == OPERAND_IMMEDIATE,
          "u_offset operand of CISA VA++ instrution "
          "should be either a general, indirect, or immediate operand.");

      Common_ISA_Operand_Class operand_class_voff =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_voff == OPERAND_GENERAL ||
              operand_class_voff == OPERAND_INDIRECT ||
              operand_class_voff == OPERAND_IMMEDIATE,
          "v_offset operand of CISA VA++ instrution "
          "should be either a general, indirect, or immediate operand.");

      uint8_t mode = ((getPrimitiveOperand<uint8_t>(inst, i++)) & 0xF);

      switch (mode & 0x3) {
      case 0:
        break;
      case 2:
        break;
      case 3:
        break;
      default:
        REPORT_INSTRUCTION(options, false,
                           "Invalid mode field for CISA VA++ instruction. "
                           "Only 4x16, 1x16, and 1x1 (in the case of "
                           "1 pixel convolve) are supported.");
      }

      break;
    }
    case VA_OP_CODE_LBP_CREATION: {
      uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
      REPORT_INSTRUCTION(
          options, 0 != surface,
          "Surface T0 (the SLM surface) is not allowed for VA++ instructions.");
      REPORT_INSTRUCTION(
          options, surface - numPreDefinedSurfs < header->getSurfaceCount(),
          "CISA LBP Creation VA++ instruction uses undefined surface.");

      Common_ISA_Operand_Class operand_class_uoff =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_uoff == OPERAND_GENERAL ||
              operand_class_uoff == OPERAND_INDIRECT ||
              operand_class_uoff == OPERAND_IMMEDIATE,
          "u_offset operand of CISA LBP Creation VA++ instrution "
          "should be either a general, indirect, or immediate operand.");

      Common_ISA_Operand_Class operand_class_voff =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_voff == OPERAND_GENERAL ||
              operand_class_voff == OPERAND_INDIRECT ||
              operand_class_voff == OPERAND_IMMEDIATE,
          "v_offset operand of CISA LBP Creation VA++ instrution "
          "should be either a general, indirect, or immediate operand.");

      uint8_t mode = ((getPrimitiveOperand<uint8_t>(inst, i++)) & 0xF);

      switch (mode & 0x3) {
      case 0:
        break;
      case 1:
        break;
      case 2:
        break;
      default:
        REPORT_INSTRUCTION(
            options, false,
            "Invalid mode field for CISA LBP Creation VA++ instruction. "
            "Only 5x5, 3x3, or both modes are supported.");
      }

      /// dst
      /// TODO: Checks for modes/types/decls

      break;
    }
    case VA_OP_CODE_FLOOD_FILL: {
      getPrimitiveOperand<uint8_t>(inst, i++); // uint8_t is8Connect

      i++; /// h-mask

      Common_ISA_Operand_Class operand_class_vmask_left =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_vmask_left == OPERAND_GENERAL ||
              operand_class_vmask_left == OPERAND_INDIRECT ||
              operand_class_vmask_left == OPERAND_IMMEDIATE,
          "Pixel direction v-mask left of CISA VA++ FloodFill instruction only "
          "supports general, indirect, and immediate operands.");

      Common_ISA_Operand_Class operand_class_vmask_right =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(options,
                         operand_class_vmask_right == OPERAND_GENERAL ||
                             operand_class_vmask_right == OPERAND_INDIRECT ||
                             operand_class_vmask_right == OPERAND_IMMEDIATE,
                         "Pixel direction v-mask right of CISA VA++ FloodFill "
                         "instruction only "
                         "supports general, indirect, and immediate operands.");

      Common_ISA_Operand_Class operand_class_loopcount =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_loopcount == OPERAND_GENERAL ||
              operand_class_loopcount == OPERAND_INDIRECT ||
              operand_class_loopcount == OPERAND_IMMEDIATE,
          "loop_count of Common ISA sample instrution is invalid type.");

      break;
    }
    case VA_OP_CODE_CORRELATION_SEARCH: {
      uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
      REPORT_INSTRUCTION(
          options, 0 != surface,
          "Surface T0 (the SLM surface) is not allowed for VA++ instructions.");
      REPORT_INSTRUCTION(
          options, surface < numPreDefinedSurfs + header->getSurfaceCount(),
          "CISA VA++ instruction uses undefined surface.");

      Common_ISA_Operand_Class operand_class_uoff =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(options,
                         operand_class_uoff == OPERAND_GENERAL ||
                             operand_class_uoff == OPERAND_INDIRECT ||
                             operand_class_uoff == OPERAND_IMMEDIATE,
                         "u_offset of Common ISA Meida LD/ST instrution "
                         "should not be address or predicate operand.");

      Common_ISA_Operand_Class operand_class_voff =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(options,
                         operand_class_voff == OPERAND_GENERAL ||
                             operand_class_voff == OPERAND_INDIRECT ||
                             operand_class_voff == OPERAND_IMMEDIATE,
                         "v_offset of Common ISA Meida LD/ST instrution "
                         "should not be address or predicate operand.");

      Common_ISA_Operand_Class operand_class_verticalOrigin =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_verticalOrigin == OPERAND_GENERAL ||
              operand_class_verticalOrigin == OPERAND_INDIRECT ||
              operand_class_verticalOrigin == OPERAND_IMMEDIATE,
          "CISA VA++ instruction Correlation search operand verticalOrigin "
          "can only be of operand class general, indirect, or immediate.");

      Common_ISA_Operand_Class operand_class_horizontalOrigin =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_horizontalOrigin == OPERAND_GENERAL ||
              operand_class_horizontalOrigin == OPERAND_INDIRECT ||
              operand_class_horizontalOrigin == OPERAND_IMMEDIATE,
          "CISA VA++ instruction Correlation search operand horizontalOrigin "
          "can only be of operand class general, indirect, or immediate.");

      Common_ISA_Operand_Class operand_class_xDirectionSize =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_xDirectionSize == OPERAND_GENERAL ||
              operand_class_xDirectionSize == OPERAND_INDIRECT ||
              operand_class_xDirectionSize == OPERAND_IMMEDIATE,
          "CISA VA++ instruction Correlation search operand xDirectionSize "
          "can only be of operand class general, indirect, or immediate.");

      Common_ISA_Operand_Class operand_class_yDirectionSize =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_yDirectionSize == OPERAND_GENERAL ||
              operand_class_yDirectionSize == OPERAND_INDIRECT ||
              operand_class_yDirectionSize == OPERAND_IMMEDIATE,
          "CISA VA++ instruction Correlation search operand yDirectionSize "
          "can only be of operand class general, indirect, or immediate.");

      Common_ISA_Operand_Class operand_class_xDirectionSearchSize =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_xDirectionSearchSize == OPERAND_GENERAL ||
              operand_class_xDirectionSearchSize == OPERAND_INDIRECT ||
              operand_class_xDirectionSearchSize == OPERAND_IMMEDIATE,
          "CISA VA++ instruction Correlation search operand "
          "xDirectionSearchSize "
          "can only be of operand class general, indirect, or immediate.");

      Common_ISA_Operand_Class operand_class_yDirectionSearchSize =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(
          options,
          operand_class_yDirectionSearchSize == OPERAND_GENERAL ||
              operand_class_yDirectionSearchSize == OPERAND_INDIRECT ||
              operand_class_yDirectionSearchSize == OPERAND_IMMEDIATE,
          "CISA VA++ instruction Correlation search operand "
          "yDirectionSearchSize "
          "can only be of operand class general, indirect, or immediate.");

      /// dst
      /// TODO: Check dst type that it is only Type_D

      break;
    }
    case ISA_HDC_CONV:
    case ISA_HDC_ERODE:
    case ISA_HDC_DILATE:
    case ISA_HDC_LBPCORRELATION:
    case ISA_HDC_LBPCREATION:
    case ISA_HDC_MMF:
    case ISA_HDC_1PIXELCONV:
    case ISA_HDC_1DCONV_H:
    case ISA_HDC_1DCONV_V: {
      if (subOpcode != ISA_HDC_LBPCORRELATION &&
          subOpcode != ISA_HDC_LBPCREATION) {
        /// sampler
        uint8_t sampler = getPrimitiveOperand<uint8_t>(inst, i++);
        REPORT_INSTRUCTION(options, sampler < header->getSamplerCount(),
                           "CISA VA++ instruction uses undeclared sampler.");
      }

      // input surface
      uint8_t surface = getPrimitiveOperand<uint8_t>(inst, i++);
      REPORT_INSTRUCTION(
          options, 0 != surface,
          "Surface T0 (the SLM surface) is not allowed for VA++ instructions.");
      REPORT_INSTRUCTION(
          options, surface < numPreDefinedSurfs + header->getSurfaceCount(),
          "CISA VA++ instruction uses undefined surface.");

      // u_offset
      Common_ISA_Operand_Class operand_class_uoff =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(options,
                         operand_class_uoff == OPERAND_GENERAL ||
                             operand_class_uoff == OPERAND_INDIRECT ||
                             operand_class_uoff == OPERAND_IMMEDIATE,
                         "u_offset of Common ISA HDC INSTRUCTION "
                         "should not be address or predicate operand.");

      // v_offset
      Common_ISA_Operand_Class operand_class_voff =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(options,
                         operand_class_voff == OPERAND_GENERAL ||
                             operand_class_voff == OPERAND_INDIRECT ||
                             operand_class_voff == OPERAND_IMMEDIATE,
                         "v_offset of Common ISA HDC instruction "
                         "should not be address or predicate operand.");

      if (subOpcode == ISA_HDC_CONV || subOpcode == ISA_HDC_MMF ||
          subOpcode == ISA_HDC_1PIXELCONV || subOpcode == ISA_HDC_1DCONV_H ||
          subOpcode == ISA_HDC_1DCONV_V) {
        // pixel size
        // FOr 2D Convovle bit 4 is for indicating SKL or BDW mode
        uint8_t pixel_size = getPrimitiveOperand<uint8_t>(inst, i++) & 0XF;
        REPORT_INSTRUCTION(
            options, pixel_size < 2,
            "CISA VA++ instruction uses invalid output pixel size.");
      }

      if (subOpcode == ISA_HDC_MMF || subOpcode == ISA_HDC_LBPCREATION) {
        // mode
        uint8_t mode = getPrimitiveOperand<uint8_t>(inst, i++);
        REPORT_INSTRUCTION(options, mode <= 2,
                           "CISA VA++ instruction uses invalid mode.");
      }

      if (subOpcode == ISA_HDC_LBPCORRELATION) {
        /// disparity
        Common_ISA_Operand_Class operand_class_disparity =
            getVectorOperand(inst, i++).getOperandClass();
        REPORT_INSTRUCTION(options,
                           operand_class_disparity == OPERAND_GENERAL ||
                               operand_class_disparity == OPERAND_INDIRECT ||
                               operand_class_disparity == OPERAND_IMMEDIATE,
                           "disparity of Common ISA HDC LBPCORRELATION "
                           "should not be address or predicate operand.");
      }

      if (subOpcode == ISA_HDC_1PIXELCONV) {
        /// offsets raw operand
        i++;
      }

      /// dst surface
      uint8_t dstsurface = getPrimitiveOperand<uint8_t>(inst, i++);
      REPORT_INSTRUCTION(
          options, 0 != dstsurface,
          "Surface T0 (the SLM surface) is not allowed for VA++ instructions.");
      REPORT_INSTRUCTION(
          options, dstsurface < numPreDefinedSurfs + header->getSurfaceCount(),
          "CISA VA++ instruction uses undefined destination surface.");

      /// x offset
      Common_ISA_Operand_Class operand_class_xoff =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(options,
                         operand_class_xoff == OPERAND_GENERAL ||
                             operand_class_xoff == OPERAND_INDIRECT ||
                             operand_class_xoff == OPERAND_IMMEDIATE,
                         "x_offset of Common ISA HDC instruction "
                         "should not be address or predicate operand.");

      /// y offset
      Common_ISA_Operand_Class operand_class_yoff =
          getVectorOperand(inst, i++).getOperandClass();
      REPORT_INSTRUCTION(options,
                         operand_class_yoff == OPERAND_GENERAL ||
                             operand_class_yoff == OPERAND_INDIRECT ||
                             operand_class_yoff == OPERAND_IMMEDIATE,
                         "y_offset of Common ISA HDC instruction "
                         "should not be address or predicate operand.");
      break;
    }
    default:
      REPORT_INSTRUCTION(options, false, "Invalid VA++ sub-opcode: %d.",
                         subOpcode);
    }

    break;
  }
  default:
    REPORT_INSTRUCTION(options, false,
                       "Illegal Sampler Instruction Opcode: %d, %s.", opcode,
                       ISA_Inst_Table[opcode].str);
  }
}

void vISAVerifier::verifyInstructionSIMDFlow(const CISA_INST *inst) {
  ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
  switch (opcode) {
  case ISA_GOTO: {
    auto labelId = getPrimitiveOperand<uint16_t>(inst, 0);
    if (labelId >= header->getLabelCount()) {
      REPORT_INSTRUCTION(options, false, "bad label id %d", labelId);
    } else {
      auto iter = labelDefs.find(labelId);
      if (iter == labelDefs.end()) {
        labelDefs[labelId] = false;
      }
      // nothing needs to be done if the label is already in the map
    }
    break;
  }
  default:
    REPORT_INSTRUCTION(options, false,
                       "Illegal SIMD CF Instruction Opcode: %d, %s.", opcode,
                       ISA_Inst_Table[opcode].str);
  }
}

static bool isUDType(VISA_Type T) { return ISA_TYPE_UD == T; }

static bool isDType(VISA_Type T) { return ISA_TYPE_D == T; }

static bool isDorUDTYpe(VISA_Type T) { return isUDType(T) || isDType(T); }

static bool isFType(VISA_Type T) { return ISA_TYPE_F == T; }

void vISAVerifier::verifyInstructionDataport(const CISA_INST *inst) {
  ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
  unsigned numPreDefinedSurfs = Get_CISA_PreDefined_Surf_Count();

  unsigned i = 0;

  uint8_t surface = 0;
  uint8_t modifier = 0;

  switch (opcode) {
  case ISA_MEDIA_ST:
  case ISA_MEDIA_LD: {
    uint8_t plane = 0;
    uint8_t block_width = 0;
    uint8_t block_height = 0;

    if (ISA_MEDIA_LD == opcode || ISA_MEDIA_ST == opcode) {
      modifier =
          getPrimitiveOperand<uint8_t>(inst, i++); // for skipping the modifier

      if (ISA_MEDIA_LD == opcode)
        REPORT_INSTRUCTION(options, modifier < MEDIA_LD_Mod_NUM,
                           "MEDIA_LD modifier must be 0-5 not %d", modifier);

      if (ISA_MEDIA_ST == opcode)
        REPORT_INSTRUCTION(options, modifier < MEDIA_ST_Mod_NUM,
                           "MEDIA_ST modifier must be 0-3 not %d", modifier);
    }

    surface = getPrimitiveOperand<uint8_t>(inst, i++);
    REPORT_INSTRUCTION(
        options, 0 != surface,
        "Surface T0 (the SLM surface) is not allowed for MEDIA_LD/MEDIA_ST");
    REPORT_INSTRUCTION(options,
                       surface < numPreDefinedSurfs + header->getSurfaceCount(),
                       "CISA dataport instruction uses an undeclared surface.");

    if (ISA_MEDIA_LD == opcode || ISA_MEDIA_ST == opcode) {
      plane = getPrimitiveOperand<uint8_t>(inst, i++);
      REPORT_INSTRUCTION(
          options, plane <= 3,
          "MEDIA_LD/MEDIA_ST plane must be in the range [0, 3]: %d", plane);
    }

    block_width = getPrimitiveOperand<uint8_t>(inst, i++);
    block_height = getPrimitiveOperand<uint8_t>(inst, i++);

    if (ISA_MEDIA_LD == opcode || ISA_MEDIA_ST == opcode) {
      REPORT_INSTRUCTION(
          options,
          1 <= block_width &&
              block_width <= COMMON_ISA_MAX_MEDIA_BLOCK_WIDTH_BDW_PLUS,
          "MEDIA_LD/MEDIA_ST block width must be in the range [1, 32]: %d",
          block_width);

      REPORT_INSTRUCTION(
          options,
          1 <= block_height &&
              block_height <= COMMON_ISA_MAX_MEDIA_BLOCK_HEIGHT,
          "MEDIA_LD/MEDIA_ST block height must be in the range [1, 64]: %d",
          block_height);
    }

    if (ISA_MEDIA_LD == opcode) {
      REPORT_INSTRUCTION(
          options,
          ((1 <= block_width && block_width <= 4 && block_height <= 64) ||
           (5 <= block_width && block_width <= 8 && block_height <= 32) ||
           (9 <= block_width && block_width <= 16 && block_height <= 16) ||
           (17 <= block_width && block_width <= 32 && block_height <= 8) ||
           (33 <= block_width && block_width <= 64 && block_height <= 4)) &&
              (block_width * block_height <= 256),
          "MEDIA_LD only supports objects that fit into a single dataport "
          "transaction where block width <= 64 bytes and size <= 256 bytes. "
          "Block width: %d. Block height: %d",
          block_width, block_height);
    } else if (ISA_MEDIA_ST == opcode) {
      REPORT_INSTRUCTION(
          options,
          ((1 <= block_width && block_width <= 4 && block_height <= 64) ||
           (5 <= block_width && block_width <= 8 && block_height <= 32) ||
           (9 <= block_width && block_width <= 16 && block_height <= 16) ||
           (17 <= block_width && block_width <= 32 && block_height <= 8) ||
           (33 <= block_width && block_width <= 64 && block_height <= 4)) &&
              (block_width * block_height <= 256),
          "MEDIA_ST only supports objects that fit into a single dataport "
          "transaction where block width <= 64 bytes and size <= 256 bytes. "
          "Block width: %d. Block height: %d",
          block_width, block_height);
    }

    Common_ISA_Operand_Class operand_class_xoff =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(options,
                       operand_class_xoff != OPERAND_ADDRESS &&
                           operand_class_xoff != OPERAND_PREDICATE,
                       "x_offset of Common ISA Meida LD/ST instrution should "
                       "not be address or predicate operand.");

    Common_ISA_Operand_Class operand_class_yoff =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(options,
                       operand_class_yoff != OPERAND_ADDRESS &&
                           operand_class_yoff != OPERAND_PREDICATE,
                       "y_offset of Common ISA Meida LD/ST instrution should "
                       "not be address or predicate operand.");

    break;
  }
  case ISA_OWORD_ST:
  case ISA_OWORD_LD:
  case ISA_OWORD_LD_UNALIGNED: {
    uint8_t size = getPrimitiveOperand<uint8_t>(inst, i++);
    size = size & 0x7;

    REPORT_INSTRUCTION(
        options, size < OWORD_NUM_ILLEGAL,
        "OWORD_LD*/OWORD_ST size must be in the range [0, 3] "
        "(ie, OWord block size must be 1/2/4/8. OWord block size: %d",
        size);

    if (ISA_OWORD_ST != opcode) {
      modifier = getPrimitiveOperand<uint8_t>(inst, i++);
    }

    surface = getPrimitiveOperand<uint8_t>(inst, i++);
    if (irBuilder->getPlatform() < GENX_ICLLP) {
      REPORT_INSTRUCTION(
          options, 0 != surface,
          "Surface T0 (the SLM surface) is not allowed for OWORD_LD*/OWORD_ST");
    }
    REPORT_INSTRUCTION(options,
                       surface < numPreDefinedSurfs + header->getSurfaceCount(),
                       "CISA dataport instruction uses an undeclared surface.");

    Common_ISA_Operand_Class operand_class =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(options,
                       operand_class != OPERAND_ADDRESS &&
                           operand_class != OPERAND_PREDICATE,
                       "Offset of Common ISA OWORD LD/ST instrutions should "
                       "not be address or predicate operands.");

    break;
  }
  case ISA_GATHER:
  case ISA_SCATTER: {
    uint8_t elt_size = 0;
    uint8_t num_elts = 0;
    if (ISA_SCATTER == opcode || ISA_GATHER == opcode) {
      elt_size = getPrimitiveOperand<uint8_t>(inst, i++);
      elt_size = elt_size & 0x3;
      switch ((GATHER_SCATTER_ELEMENT_SIZE)elt_size) {
      case GATHER_SCATTER_BYTE:
        elt_size = 1;
        break;
      case GATHER_SCATTER_WORD:
        elt_size = 2;
        break;
      case GATHER_SCATTER_DWORD:
        elt_size = 4;
        break;
      default:
        REPORT_INSTRUCTION(
            options, false,
            "Incorrect element size for Gather/Scatter CISA inst.");
        break;
      }
    }

    if (ISA_GATHER == opcode)
      getPrimitiveOperand<uint8_t>(inst, i++);

    num_elts = getPrimitiveOperand<uint8_t>(inst, i++);
    num_elts = num_elts & 0x3;
    if ((unsigned)num_elts == 0)
      num_elts = 8;
    else if ((unsigned)num_elts == 1)
      num_elts = 16;
    else if ((unsigned)num_elts == 2)
      num_elts = 1;

    surface = getPrimitiveOperand<uint8_t>(inst, i++);
    // SLM uses surface0 as seen in nbody_SLM: gather
    REPORT_INSTRUCTION(options,
                       surface < numPreDefinedSurfs + header->getSurfaceCount(),
                       "CISA dataport instruction uses an undeclared surface.");

    Common_ISA_Operand_Class operand_class_goff =
        getVectorOperand(inst, i++).getOperandClass();
    REPORT_INSTRUCTION(options,
                       operand_class_goff != OPERAND_ADDRESS &&
                           operand_class_goff != OPERAND_PREDICATE,
                       "global_offset of Common ISA gather/scatter instrution "
                       "should not be address or predicate operand.");

    // check that dst/src have dword type
    getRawOperand(inst, i++); // const raw_opnd& elementOffset
    const raw_opnd &srcDst = getRawOperand(inst, i++);
    verifyRawOperandType(inst, srcDst, isDWordType);
    break;
  }
  case ISA_GATHER4_TYPED:
  case ISA_SCATTER4_TYPED: {
    if (getVersionAsInt(header->getMajorVersion(), header->getMinorVersion()) >=
        getVersionAsInt(3, 2)) {
      uint8_t ch_mask = 0;
      ch_mask = getPrimitiveOperand<uint8_t>(inst, i++);
      ch_mask = ch_mask & 0xF;
      REPORT_INSTRUCTION(
          options, ch_mask != 0x0,
          "At least one channel must be enabled for TYPED GATEHR4/SCATTER4");

      surface = getPrimitiveOperand<uint8_t>(inst, i++);
      REPORT_INSTRUCTION(options, (0 != surface && 5 != surface),
                         "Surface T0/T5 (the SLM surface) is not allowed for "
                         "TYPED SCATTTER4/GATHER4");
      REPORT_INSTRUCTION(
          options, surface < numPreDefinedSurfs + header->getSurfaceCount(),
          "CISA dataport TYPED SCATTTER4/GATHER4 instruction uses an "
          "undeclared surface.");
    }
    break;
  }
  case ISA_GATHER4_SCALED:
  case ISA_SCATTER4_SCALED: {
    break;
  }
  case ISA_GATHER_SCALED:
  case ISA_SCATTER_SCALED: {
    break;
  }
  case ISA_DWORD_ATOMIC: {
    unsigned subOpc = getPrimitiveOperand<uint8_t>(inst, i++);
    auto subOp = static_cast<VISAAtomicOps>(subOpc & 0x1F);
    switch (subOp) {
    default:
      REPORT_INSTRUCTION(options, false, "Invalid DWORD ATOMIC sub op.");
    case ATOMIC_ADD:
    case ATOMIC_SUB:
    case ATOMIC_INC:
    case ATOMIC_DEC:
    case ATOMIC_MIN:
    case ATOMIC_MAX:
    case ATOMIC_XCHG:
    case ATOMIC_CMPXCHG:
    case ATOMIC_AND:
    case ATOMIC_OR:
    case ATOMIC_XOR:
    case ATOMIC_IMIN:
    case ATOMIC_IMAX:
    case ATOMIC_PREDEC:
    case ATOMIC_FMAX:
    case ATOMIC_FMIN:
    case ATOMIC_FCMPWR:
    case ATOMIC_FADD:
    case ATOMIC_FSUB:
      break;
    }
    surface = getPrimitiveOperand<uint8_t>(inst, i++);
    REPORT_INSTRUCTION(options,
                       surface < numPreDefinedSurfs + header->getSurfaceCount(),
                       "CISA dataport instruction uses an undeclared surface.");

    const raw_opnd &offsets = getRawOperand(inst, i++);
    verifyRawOperandType(inst, offsets, isUDType);

    // Check remaining raw operands.
    VISAAtomicOps subOpKind = static_cast<VISAAtomicOps>(subOpc);
    bool (*typeFn)(VISA_Type) = 0;
    switch (subOpKind) {
    default:
      typeFn = isUDType;
      break;
    case ATOMIC_IMAX:
    case ATOMIC_IMIN:
      typeFn = isDType;
      break;
    case ATOMIC_ADD:
    case ATOMIC_SUB:
      typeFn = isDorUDTYpe;
      break;
    case ATOMIC_FMAX:
    case ATOMIC_FMIN:
    case ATOMIC_FCMPWR:
    case ATOMIC_FADD:
    case ATOMIC_FSUB:
      typeFn = isFType;
      break;
    }

    // Check src0:
    //
    // - for INC and DEC operations, src0 must be V0 (the null variable);
    // - for IMIN and IMAX it must have type D;
    // - for all other operations, it must have type UD.
    const raw_opnd &src0 = getRawOperand(inst, i++);
    if (subOpKind == ATOMIC_INC || subOpKind == ATOMIC_DEC ||
        subOpKind == ATOMIC_PREDEC
    ) {
      REPORT_INSTRUCTION(options, src0.index == 0,
                         "src0 in ISA_DWORD_ATOMIC inst must be "
                         "V0 for INC/DEC/PREDEC.");
    } else {
      verifyRawOperandType(inst, src0, typeFn);
    }
    // Check src1:
    //
    // - for CMPXCHG operation, it must have type UD;
    // - for all other operations, it must be V0 (the null variable).
    //
    const raw_opnd &src1 = getRawOperand(inst, i++);
    if (subOpKind == ATOMIC_CMPXCHG || subOpKind == ATOMIC_FCMPWR) {
      verifyRawOperandType(inst, src1, typeFn);
    } else {
      REPORT_INSTRUCTION(options, src1.index == 0,
                         "src1 in ISA_DWORD_ATOMIC inst must be "
                         "V0 for non CMPXCHG operations.");
    }
    // Check dst:
    //
    // - for IMIN and IMAX, it must have type D;
    // - for all other operations, it must have type UD.
    //
    const raw_opnd &dst = getRawOperand(inst, i++);
    verifyRawOperandType(inst, dst, typeFn);
    break;
  }
  case ISA_3D_TYPED_ATOMIC:
  case ISA_3D_RT_WRITE:
  case ISA_QW_GATHER:
  case ISA_QW_SCATTER: {
    // no verification for now
    break;
  }
  default:
    REPORT_INSTRUCTION(options, false,
                       "Illegal dataport Instruction Opcode: %d, %s.", opcode,
                       ISA_Inst_Table[opcode].str);
  }
}

void vISAVerifier::verifyBFMixedMode(const CISA_INST *inst) {
  // Only verify inst type: Mov/Compare/Arith for BF mixed mode
  ISA_Opcode opcode = (ISA_Opcode)inst->opcode;
  switch (ISA_Inst_Table[opcode].type) {
  case ISA_Inst_Mov:
  case ISA_Inst_Arith:
  case ISA_Inst_Compare:
    break;
  default:
    return;
  }

  if (!useGivenVISAType(inst, ISA_TYPE_BF)) {
    // Skip if inst does not use BF
    return;
  }

  // Skip srnd
  if (opcode == ISA_SRND)
    return;

  if (!irBuilder->hasBFMixMode()) {
    REPORT_INSTRUCTION(options, false,
                       "BF type is not allowed on this platform");
    return;
  }

  switch (opcode) {
  case ISA_MUL:
  case ISA_MAD:
  case ISA_MOV:
  case ISA_ADD:
  case ISA_SEL:
  case ISA_CMP:
    break;
  case ISA_COS:
  case ISA_DIV:
  case ISA_EXP:
  case ISA_INV:
  case ISA_LOG:
  case ISA_POW:
  case ISA_RSQRT:
  case ISA_SIN:
  case ISA_SQRT:
  case ISA_FMINMAX:
  case ISA_TANH:
  case ISA_SIGM:
    if (irBuilder->supportPureBF())
      break;
  default:
    REPORT_INSTRUCTION(options, false,
                       "BF opnd is not allowed on this instruction");
    return;
  }

  // Only F or BF are allowed
  // opernads: 0 : srcStart-1        --> dst
  //           srcStart : n_srcs-1   --> src
  int srcStart = (int)ISA_Inst_Table[opcode].n_dsts;
  // fminmax's 1st opnd is minmax op
  // cmp's 1st opnd is cmp relop
  if (opcode == ISA_FMINMAX || opcode == ISA_CMP)
    srcStart++;
  // CMP's target must be bool, skip checking cmp's dst
  if (opcode != ISA_CMP) {
    int dstStart = opcode == ISA_FMINMAX ? 1 : 0;
    for (int j = dstStart; j < srcStart; j++) {
      VISA_Type dstType = getOperandVISAType(inst, j);
      REPORT_INSTRUCTION(options,
                         (dstType == ISA_TYPE_F || dstType == ISA_TYPE_BF),
                         "Dst opnd in BF mixed mode should be either BF or F");
      if (irBuilder->supportPureBF() && inst->isMath())
        REPORT_INSTRUCTION(
            options, (dstType == ISA_TYPE_BF),
            "Math instructions must be pure BF mode");
    }
  }
  for (int j = 0; j < ISA_Inst_Table[opcode].n_srcs; j++) {
    VISA_Type srcType = getOperandVISAType(inst, j + srcStart);
    REPORT_INSTRUCTION(options,
                       (srcType == ISA_TYPE_F || srcType == ISA_TYPE_BF),
                       "Src opnd in BF mixed mode should be either BF or F");
    if (irBuilder->supportPureBF() && inst->isMath())
      REPORT_INSTRUCTION(options, (srcType == ISA_TYPE_BF),
                         "Math instructions must be pure BF mode");
  }
  return;
}

struct LscInstVerifier {
  const print_format_provider_t *header;
  const CISA_INST *inst;
  const IR_Builder &builder;
  std::stringstream errorStream;
  Options *options;
  int execSize;

  LscOpInfo opInfo;
  LSC_OP subOp;
  LSC_SFID sfid;

  int currOpIx = 0;

  LscInstVerifier(const print_format_provider_t *_header,
                  const CISA_INST *_inst, const IR_Builder &_builder,
                  Options *_options)
      : header(_header), inst(_inst), builder(_builder),
        options(_options) {
    if (_inst->opcode == ISA_LSC_FENCE) {
      subOp = LSC_FENCE;
      sfid = getNextEnumU8<LSC_SFID>();
    } else if (_inst->opcode == ISA_LSC_TYPED) {
      subOp = getNextEnumU8<LSC_OP>();
      sfid = LSC_TGM;
    } else {
      subOp = getNextEnumU8<LSC_OP>();
      sfid = getNextEnumU8<LSC_SFID>();
    }
    (void)LscOpInfoFind(subOp, opInfo);
    execSize = 1;
    switch (inst->execsize & 0xF) {
    case EXEC_SIZE_1:
      execSize = 1;
      break;
    case EXEC_SIZE_2:
      execSize = 2;
      break;
    case EXEC_SIZE_4:
      execSize = 4;
      break;
    case EXEC_SIZE_8:
      execSize = 8;
      break;
    case EXEC_SIZE_16:
      execSize = 16;
      break;
    case EXEC_SIZE_32:
      execSize = 32;
      break;
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  template <typename T> T getPrimitive(int absOpIx) {
    return getPrimitiveOperand<T>(inst, absOpIx);
  }

  template <typename T> T getNextEnumU8() {
    return (T)getPrimitive<uint8_t>(currOpIx++);
  }
  template <typename T> T getNext() { return getPrimitive<T>(currOpIx++); }

  // LSC_TYPED and non-block2d LSC_UNTYPED
  // "next" because it advances the operand pointer
  LSC_DATA_SHAPE getNextDataShape() {
    // chmask only valid on LSC_LOAD_QUAD/LSC_STORE_QUAD
    // but retained in the binary format
    LSC_DATA_SHAPE dataShape{};
    dataShape.size = getNextEnumU8<LSC_DATA_SIZE>();
    dataShape.order = getNextEnumU8<LSC_DATA_ORDER>();
    // untyped and typed have both data elems and chmask
    // (even though the IR only uses one)
    auto dataElems = getNextEnumU8<LSC_DATA_ELEMS>();
    int chMask = (int)getNextEnumU8<int>();
    if (opInfo.hasChMask()) {
      dataShape.chmask = chMask;
    } else {
      (void)chMask; // not used
      dataShape.elems = dataElems;
    }
    return dataShape;
  }
  LSC_DATA_SHAPE_BLOCK2D getNextDataShape2D() {
    LSC_DATA_SHAPE_BLOCK2D dataShape2D{};
    dataShape2D.size = getNextEnumU8<LSC_DATA_SIZE>();
    dataShape2D.order = getNextEnumU8<LSC_DATA_ORDER>();
    dataShape2D.blocks = (int)getNext<uint8_t>();
    dataShape2D.width = (int)getNext<uint16_t>();
    dataShape2D.height = (int)getNext<uint16_t>();
    dataShape2D.vnni = getNext<uint8_t>() != 0;
    return dataShape2D;
  }

  ///////////////////////////////////////////////////////////////////////////

  template <typename T1, typename T2 = const char *, typename T3 = const char *>
  bool verify(bool z, T1 t1, T2 t2 = "", T3 t3 = "") {
    if (!z)
      error(t1, t2, t3);
    return z;
  }
  template <typename T1, typename T2 = const char *, typename T3 = const char *>
  void error(T1 t1, T2 t2 = "", T3 t3 = "") {
    std::stringstream ss;
    ss << " * ";
    ss << t1;
    ss << t2;
    ss << t3;
    ss << "\n";
    errorStream << ss.str();
  }
  void badEnum(const char *which, int value) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::uppercase << value << ": ";
    ss << which;
    error(ss.str());
  }

  int verifyDataSize(LSC_DATA_SIZE dataSize) {
    int dataSizeBytes = 0;
    switch (dataSize) {
    case LSC_DATA_SIZE_8b:
      dataSizeBytes = 1;
      break;
    case LSC_DATA_SIZE_16b:
      dataSizeBytes = 2;
      break;
    case LSC_DATA_SIZE_32b:
      dataSizeBytes = 4;
      break;
    case LSC_DATA_SIZE_64b:
      dataSizeBytes = 8;
      break;
    case LSC_DATA_SIZE_8c32b:
      dataSizeBytes = 4;
      break;
    case LSC_DATA_SIZE_16c32b:
      dataSizeBytes = 4;
      break;
    case LSC_DATA_SIZE_16c32bH:
      dataSizeBytes = 4;
      break;
    default:
      badEnum("invalid LSC_DATA_SIZE", dataSize);
    }
    return dataSizeBytes;
  }

  bool verifyDataOrder(LSC_DATA_ORDER dataOrder) {
    bool transposed = false;
    switch (dataOrder) {
    case LSC_DATA_ORDER_NONTRANSPOSE:
      transposed = false;
      break;
    case LSC_DATA_ORDER_TRANSPOSE:
      transposed = true;
      break;
    default:
      badEnum("invalid LSC_DATA_ORDER", dataOrder);
    }
    return transposed;
  }

  int verifyDataElems(LSC_DATA_ELEMS dataElems) {
    int vecSize = 0;
    switch (dataElems) {
    case LSC_DATA_ELEMS_1:
      vecSize = 1;
      break;
    case LSC_DATA_ELEMS_2:
      vecSize = 2;
      break;
    case LSC_DATA_ELEMS_3:
      vecSize = 3;
      break;
    case LSC_DATA_ELEMS_4:
      vecSize = 4;
      break;
    case LSC_DATA_ELEMS_8:
      vecSize = 8;
      break;
    case LSC_DATA_ELEMS_16:
      vecSize = 16;
      break;
    case LSC_DATA_ELEMS_32:
      vecSize = 32;
      break;
    case LSC_DATA_ELEMS_64:
      vecSize = 64;
      break;
      break;
    default:
      badEnum("LSC_DATA_ELEMS", dataElems);
    }
    return vecSize;
  }

  void verifyDataShape(LSC_DATA_SHAPE dataShape) {
    int dataSizeBytes = verifyDataSize(dataShape.size);
    if (dataSizeBytes == 0)
      return; // verifyDataSize reported error


    bool transposed = verifyDataOrder(dataShape.order);
    if (transposed) {
      verify(!opInfo.hasChMask(), "LSC_DATA_ORDER_TRANSPOSE on ",
             opInfo.mnemonic);
      verify(execSize == 1, "LSC_DATA_ORDER_TRANSPOSE requires ExecSize of 1");
      verify(!opInfo.isAtomic(), "LSC atomics do not support transpose mode");
    } else if (opInfo.hasChMask()) {
      // quad store (store_cmask) can only support X, XY, XYZ, XYZW
      if (opInfo.op == LSC_STORE_QUAD) {
        // for tgm, stores have to target continuous vector elements
        if (builder.isEfficient64bEnabled()) {
          if (sfid == LSC_TGM) {
            verify(
                dataShape.chmask == LSC_DATA_CHMASK_X ||
                    dataShape.chmask ==
                        (LSC_DATA_CHMASK_X | LSC_DATA_CHMASK_Y) ||
                    dataShape.chmask == (LSC_DATA_CHMASK_X | LSC_DATA_CHMASK_Y |
                                         LSC_DATA_CHMASK_Z) ||
                    dataShape.chmask == (LSC_DATA_CHMASK_X | LSC_DATA_CHMASK_Y |
                                         LSC_DATA_CHMASK_Z | LSC_DATA_CHMASK_W),
                "lsc_store_quad channel mask must be contiguous for TGM"
                "(.x, .xy, .xyz, or .xyzw)");
          }
        } else
          verify(
              dataShape.chmask == LSC_DATA_CHMASK_X ||
                  dataShape.chmask == (LSC_DATA_CHMASK_X | LSC_DATA_CHMASK_Y) ||
                  dataShape.chmask == (LSC_DATA_CHMASK_X | LSC_DATA_CHMASK_Y |
                                       LSC_DATA_CHMASK_Z) ||
                  dataShape.chmask == (LSC_DATA_CHMASK_X | LSC_DATA_CHMASK_Y |
                                       LSC_DATA_CHMASK_Z | LSC_DATA_CHMASK_W),
              "lsc_store_quad channel mask must be contiguous "
              "(.x, .xy, .xyz, or .xyzw)");
      }
    } else if (!opInfo.isBlock2D()) {
      // SIMT access not block2d
      verify((dataShape.elems == LSC_DATA_ELEMS_1) ||
                 (dataShape.size == LSC_DATA_SIZE_32b ||
                  dataShape.size == LSC_DATA_SIZE_64b),
             "only D32 and D64 support vector load");
      if (sfid == LSC_UGML) {
        verify(dataShape.elems == LSC_DATA_ELEMS_1 ||
                   dataShape.elems == LSC_DATA_ELEMS_2 ||
                   dataShape.elems == LSC_DATA_ELEMS_4,
               "UGML only supports vector sizes 1, 2, and 4");
      } else if (sfid == LSC_SLM || sfid == LSC_UGM) {
        verify(dataShape.elems == LSC_DATA_ELEMS_1 ||
                   dataShape.elems == LSC_DATA_ELEMS_2 ||
                   dataShape.elems == LSC_DATA_ELEMS_3 ||
                   dataShape.elems == LSC_DATA_ELEMS_4 ||
                   dataShape.elems == LSC_DATA_ELEMS_8,
               "SLM/UGM only supports vector sizes 1, 2, 3, 4, and 8");
      }
      if (opInfo.isAtomic()) {
        verify(dataShape.size == LSC_DATA_SIZE_16c32b ||
                   dataShape.size == LSC_DATA_SIZE_32b ||
                   dataShape.size == LSC_DATA_SIZE_64b,
               "LSC atomics only support D16U32, D32, or D64");
        // TODO: opInfo.op == LSC_ATOMIC_ICAS only in B0+ (none in A0)
        verify(sfid != LSC_SLM || dataShape.size != LSC_DATA_SIZE_64b ||
                   opInfo.op == LSC_ATOMIC_ICAS,
               "LSC SLM D64 atomics only support icas");
        if (builder.getPlatform() >= Xe2) {
          if (builder.getPlatform() > Xe3)
            verify((opInfo.op != LSC_ATOMIC_FADD &&
                    opInfo.op != LSC_ATOMIC_FSUB) ||
                       (sfid == LSC_UGM || sfid == LSC_TGM || sfid == LSC_SLM),
                   "LSC atomic fadd/fsub only support UGM, TGM, SLM");
          else
            verify((opInfo.op != LSC_ATOMIC_FADD &&
                    opInfo.op != LSC_ATOMIC_FSUB) ||
                       (sfid == LSC_UGM || sfid == LSC_UGML || sfid == LSC_TGM),
                   "LSC atomic fadd/fsub only support UGM, UGML and TGM");


          verify((opInfo.op != LSC_APNDCTR_ATOMIC_ADD &&
                  opInfo.op != LSC_APNDCTR_ATOMIC_SUB) ||
                     sfid == LSC_UGM,
                 "LSC append counter atomic add/sub only support UGM");

          verify((opInfo.op != LSC_APNDCTR_ATOMIC_ADD &&
                  opInfo.op != LSC_APNDCTR_ATOMIC_SUB) ||
                     dataShape.size == LSC_DATA_SIZE_32b,
                 "LSC append counter atomic add/sub only support D32");
        } else
        {
          verify(
              (opInfo.op != LSC_ATOMIC_FADD && opInfo.op != LSC_ATOMIC_FSUB) ||
                  (sfid == LSC_UGM || sfid == LSC_UGML),
              "LSC atomic fadd/fsub only support UGM and UGML");
        }
      }
    }

    int vecSize = 0;
    if (!opInfo.hasChMask()) {
      vecSize = verifyDataElems(dataShape.elems);
      if (vecSize != 1) {
        if (opInfo.isAtomic()) {
          error("LSC_DATA_ELEMS must be 1 for atomic operations");
        }
        switch (dataShape.size) {
        case LSC_DATA_SIZE_8c32b:
        case LSC_DATA_SIZE_16c32b:
        case LSC_DATA_SIZE_16c32bH:
          error("LSC_DATA_SIZE: conversion types may not use vector");
          break;
        default:
          break;
        }
      }
    } else if (dataShape.chmask == 0) {
      error("LSC_DATA_SHAPE::chmask: must not be 0");
    } else if (dataShape.chmask & ~0xF) {
      error("LSC_DATA_SHAPE::chmask: has high bits set");
    } else {
      vecSize = 0;
      for (int i = 0; i < 4; i++)
        if (dataShape.chmask & (1 << i))
          vecSize++;
    }
    if (vecSize == 0)
      return;

    // data payloads cannot be more than 8 registers
    const uint32_t BYTES_PER_REG = builder.getGRFSize();
    int dataRegs = 0;
    if (transposed) {
      // must be SIMD1 (whether they had that right or not, it will be)
      // (and we've already complained about it if not)
      dataRegs = std::max<int>(1, vecSize * dataSizeBytes / BYTES_PER_REG);
    } else {
      //
      // int execSize = BYTES_PER_REG/2;
      // apparently :d32x8 and :d64x4 are permissible if SIMD16, so
      // we'll pull the real ExecSize
      //
      // TODO: we need to recheck with HW that they indeed put this
      // optimization in
      int regPerVecElem =
          std::max<int>(1, dataSizeBytes * execSize / BYTES_PER_REG);
      dataRegs = vecSize * regPerVecElem;
    }
    if (opInfo.op != LSC_LOAD_BLOCK2D)
      verify(dataRegs <= 8, "this message accesses more than 8 registers");
  }

  void verifyCachingOptsL1L2L3() {
    auto l1 = getNextEnumU8<LSC_CACHE_OPT>();
    auto l2 = getNextEnumU8<LSC_CACHE_OPT>();
    auto l3 = getNextEnumU8<LSC_CACHE_OPT>();

    uint32_t enc = 0;
    LSC_CACHE_OPTS cacheOpts{l1, l2, l3};
    // set isBits17_19 to false to check for all cases
    if (!LscTryEncodeCacheOptsL1L2L3(opInfo, cacheOpts, enc)) {
      if (opInfo.isLoad()) {
        error("invalid cache-control options for load (#53560)");
      } else if (opInfo.isAtomic()) {
        if (cacheOpts.l1 != LSC_CACHE_OPT::LSC_CACHING_UNCACHED) {
          error("atomics must use options with uncached L1"
                " (#53561 & #53542 [19:17])");
        } else {
          error("invalid cache-control options for atomic (#53561)");
        }
      } else {
        error("invalid cache-control options for store (#53561)");
      }
    }
  }
  void verifyCachingOpts() {
    if (builder.isEfficient64bEnabled())
      return verifyCachingOptsL1L2L3();
    auto l1 = getNextEnumU8<LSC_CACHE_OPT>();
    auto l3 = getNextEnumU8<LSC_CACHE_OPT>();
    if (builder.getPlatform() < Xe2)
    {
      if (sfid == LSC_TGM || sfid == LSC_UGML) {
        verify(l1 == LSC_CACHING_DEFAULT && l3 == LSC_CACHING_DEFAULT,
               "Messages to UGML and TGM require default cache settings"
               " (#53561)");
        return;
      }
    }
    uint32_t enc = 0;
    LSC_CACHE_OPTS cacheOpts{l1, l3};
    // set isBits17_19 to false to check for all cases
    if (!LscTryEncodeCacheOpts(opInfo, cacheOpts, enc, false)) {
      if (opInfo.isLoad()) {
        error("invalid cache-control options for load (#53560)");
      } else if (opInfo.isAtomic()) {
        if (cacheOpts.l1 != LSC_CACHE_OPT::LSC_CACHING_UNCACHED) {
          error("atomics must use options with uncached L1"
                " (#53561 & #53542 [19:17])");
        } else {
          error("invalid cache-control options for atomic (#53561)");
        }
      } else {
        error("invalid cache-control options for store (#53561)");
      }
    }
    // TODO: we should proscribe default/default if prefetch
  }

  void verifyAddrSize(LSC_ADDR_SIZE addrSize) {
    // ADDR payloads limited to 4 registers
    //
    // SIMD32 64b will fit in PVC (as will SIMD16 64b on DG2)
    //  ==> nothing to check
    switch (addrSize) {
    case LSC_ADDR_SIZE_16b:
      verify(sfid == LSC_SLM,
             ":a16 (LSC_ADDR_SIZE_16b) only allowed on .slm SFID");
      break;
    case LSC_ADDR_SIZE_32b:
    case LSC_ADDR_SIZE_32bU:
    case LSC_ADDR_SIZE_32bS:
      break;
    case LSC_ADDR_SIZE_64b:
      if (sfid == LSC_TGM || sfid == LSC_SLM)
        error(".tgm not allowed with :a64 (LSC_ADDR_SIZE_64b)");
      if (subOp == LSC_LOAD_BLOCK2D || subOp == LSC_STORE_BLOCK2D)
        error("block2d does not allow 64b address types");
      break;
    default:
      break;
    }
  }

  void verifyAddressType(LSC_ADDR_TYPE addrType, int surfIxAbs) {
    if (!verifyVectorOperand("Surface", surfIxAbs)) {
      return;
    }
    const auto &vo = getVectorOperand(inst, surfIxAbs);

    switch (addrType) {
    case LSC_ADDR_TYPE_FLAT:
    case LSC_ADDR_TYPE_ARG:
      verify(sfid != LSC_TGM, ".tgm may not use flat address model");
      switch (vo.tag & 0x7) {
      case OPERAND_IMMEDIATE:
        if (!builder.isEfficient64bEnabled()) {
          verify(vo.opnd_val.const_opnd._val.ival == 0,
                 "Surface: for LSC_ADDR_TYPE_FLAT only imm 0 or %null "
                 "allowed on this platform");
        }
        break;
      case OPERAND_GENERAL:
        // efficient64b lifts the %null restriction
        // (%null or 0x0 means no base offset)
        verify(vo.opnd_val.gen_opnd.index == 0 ||
                  builder.isEfficient64bEnabled(),
               "Surface: must be %null reg for LSC_ADDR_TYPE_FLAT");
        break;
      default:
        error("Surface: invalid operand type");
      }
      break;
    case LSC_ADDR_TYPE_BSS:
    case LSC_ADDR_TYPE_SS:
    case LSC_ADDR_TYPE_BTI:
    case LSC_ADDR_TYPE_SURF:
      verify(sfid != LSC_SLM, "slm must use flat address model");
      switch (vo.tag & 0x7) {
      case OPERAND_IMMEDIATE:
        break;
      case OPERAND_GENERAL:
        verify(vo.opnd_val.gen_opnd.index != 0,
               "Surface: must not be null (V0) reg for non-FLAT AddrType");
        break;
      default:
        error("Surface: invalid operand type");
      }
      break;
    default:
      badEnum("LSC_ADDR_TYPE is invalid", addrType);
      break;
    }
  }

  bool verifyRawOperand(const char *which, int absIx) {
    if (getOperandType(inst, absIx) != CISA_OPND_RAW) {
      error(which, ": expected vISA RawOperand");
      return false;
    }
    return true;
  }

  bool verifyRawOperandNull(const char *which, int absIx) {
    if (!verifyRawOperand(which, absIx)) {
      return false;
    } else {
      const raw_opnd &ro = getRawOperand(inst, absIx);
      if (ro.index != 0) {
        error(which, "; operand must be null");
        return false;
      }
      return true;
    }
  }

  bool verifyRawOperandNonNull(const char *which, int absIx) {
    if (!verifyRawOperand(which, absIx)) {
      return false;
    } else {
      const raw_opnd &ro = getRawOperand(inst, absIx);
      if (ro.index == 0) {
        error(which, ": operand must not be null");
        return false;
      }
      return true;
    }
  }

  bool verifyVectorOperand(const char *which, int absIx) {
    if (getOperandType(inst, absIx) != CISA_OPND_VECTOR) {
      error(which, ": expected vISA vector operand");
      return false;
    }
    return true;
  }
  bool verifyVectorOperandNotNull(const char *which, int absIx) {
    if (!verifyVectorOperand(which, absIx)) {
      return false;
    }
    const vector_opnd &vo = getVectorOperand(inst, absIx);
    if (!vo.isImmediate() && vo.getOperandIndex() == 0) {
      error(which, ": vector operand must not be null");
      return false;
    }
    return true;
  }

  // check dst and src1 and src2
  void verifyDataOperands(int dstOpIx, int src1DataIx) {
    if (opInfo.isStore()) {
      verifyRawOperandNull("DstData", dstOpIx);
    } else {
      // atomics and loads can have either null or non-null
      verifyRawOperand("DstData", dstOpIx); // DstData
    }

    if (opInfo.isLoad()) {
      verifyRawOperandNull("Src1Data", src1DataIx);
      if (!opInfo.isStrided() && !opInfo.isBlock2D())
        verifyRawOperandNull("Src2Data", src1DataIx + 1);
    } else if (opInfo.isStore()) {
      verifyRawOperandNonNull("Src1Data", src1DataIx);
      if (!opInfo.isStrided() && !opInfo.isBlock2D())
        verifyRawOperandNull("Src2Data", src1DataIx + 1);
    } else if (opInfo.isAtomic()) {
      if (opInfo.extraOperands > 0) {
        verifyRawOperandNonNull("Src1Data (in binary atomic)", src1DataIx);
      } else {
        verifyRawOperandNull("Src1Data (in binary atomic)", src1DataIx);
      }
      if (opInfo.extraOperands == 2) {
        verifyRawOperandNonNull("Src2Data (in ternary atomic)", src1DataIx + 1);
      } else {
        verifyRawOperandNull("Src2Data (in ternary atomic)", src1DataIx + 1);
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  void verifyFence() {
    // SLM only gets .group and .none
    // c.f. ASpec 5.4.1.12.9 "Fence Message Handling" (pg 62)
    auto fenceOp = getNextEnumU8<LSC_FENCE_OP>();
    auto scope = getNextEnumU8<LSC_SCOPE>();
    switch (fenceOp) {
    case LSC_FENCE_OP_NONE:
      break;
    case LSC_FENCE_OP_EVICT:
    case LSC_FENCE_OP_INVALIDATE:
    case LSC_FENCE_OP_DISCARD:
    case LSC_FENCE_OP_CLEAN:
    case LSC_FENCE_OP_FLUSHL3:
    case LSC_FENCE_OP_TYPE6:
      verify(sfid != LSC_SLM, "lsc_fence.slm fence op must be .none");
      break;
    default:
      badEnum("invalid LSC_FENCE", fenceOp);
      break;
    }
    switch (scope) {
    case LSC_SCOPE_GROUP:
      break;
    case LSC_SCOPE_LOCAL:
    case LSC_SCOPE_TILE:
    case LSC_SCOPE_GPU:
    case LSC_SCOPE_GPUS:
    case LSC_SCOPE_SYSREL:
    case LSC_SCOPE_SYSACQ:
      verify(sfid != LSC_SLM, "lsc_fence.slm must use .group scope");
      break;
    default:
      badEnum("invalid LSC_SCOPE", scope);
      break;
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  void verifyUntypedBlock2D() {
    bool valid = true;

    verifyCachingOpts();
    //
    const auto dataShape2D = getNextDataShape2D();
    int dataSizeBytes = verifyDataSize(dataShape2D.size);
    verifyDataOrder(dataShape2D.order);
    valid &=
        verify(dataShape2D.blocks > 0, "block2d blocks (vec_len) must be >= 1");
    valid &= verify(dataShape2D.height > 0, "blocks2d height must be > 0");
    valid &= verify(dataShape2D.width > 0, "blocks2d width must be > 0");
    valid &= verify(opInfo.op != LSC_STORE_BLOCK2D || !dataShape2D.vnni,
                    "block2d stores may not use VNNI transform");
    valid &= verify(opInfo.op != LSC_STORE_BLOCK2D || dataShape2D.blocks == 1,
                    "block2d stores must have block count==1");

    if (!valid)
      return;

    bool transpose = dataShape2D.order == LSC_DATA_ORDER_TRANSPOSE;
    bool transform = dataShape2D.vnni;
    unsigned totalBytes = dataShape2D.width * dataShape2D.blocks * dataSizeBytes;
    if (subOp == LSC_LOAD_BLOCK2D) {
      // for block2d prefetches, width * blocks * data size <= 64 or == 256B
      // block2d prefetches have null destination
      if (builder.getPlatform() > Xe3 && isNullRawOperand(inst, 10)) {
        valid &= verify((totalBytes == 256 || totalBytes <= 64),
          totalBytes, ": block2d width * data size must be == 256Bytes or <= 64B for 2D loads prefetches");
      }
      else {
        valid &= verify(totalBytes <= 64,
          totalBytes, ": block2d width * data size must be <= 64Bytes for 2D loads");
      }
    } else {
      // subOp is LSC_STORE_BLOCK2D
      valid &= verify(totalBytes <= 512,
          totalBytes, ": block2d width * data size must be <= 512Bytes for 2D stores");
    }

    int rows = !transpose ? dataShape2D.height : dataShape2D.width;
    if (builder.getPlatform() > Xe3) {
      valid &= verify(rows <= 64, rows,
                      ": block2d row count must be less than <= 64");
    } else {
      valid &= verify(rows <= 32, rows,
                      ": block2d row count must be less than <= 32");
    }
    //
    // HAS constraints on valid block dimensions
    //   c.f. HAS 5.1.2.1 Allowed Sizes (pg 6)  - NN
    //   c.f. HAS 5.1.3.1 Allowed Sizes (pg 9)  - TN
    //   c.f. HAS 5.1.1.1 Allowed Sizes (pg 12) - TN
    //   c.f. HAS 5.2.1.1 Allowed Sizes (pg 15) - TT
    //
    // in general all but (TN) can use block counts as follows
    //    data size    block count (vec_len)
    //    u8/u16:      1, 2, and 4
    //    u32:         1 and 2
    //    u64:         1
    //
    // TN must use a block size of vec len of 1
    if (!transpose || transform) {
      switch (dataShape2D.size) {
      case LSC_DATA_SIZE_8b:
        valid &= verify(dataShape2D.blocks == 1 || dataShape2D.blocks == 2 ||
                            dataShape2D.blocks == 4,
                        dataShape2D.blocks,
                        ": block2d count (vec_len) must be 1, 2, or 4 for u8");
        break;
      case LSC_DATA_SIZE_16b:
        valid &= verify(dataShape2D.blocks == 1 || dataShape2D.blocks == 2 ||
                            dataShape2D.blocks == 4,
                        dataShape2D.blocks,
                        ": block2d count (vec_len) must be 1, 2, or 4 for u16");
        break;
      case LSC_DATA_SIZE_32b:
        if (builder.getPlatform() > Xe3 && isNullRawOperand(inst, 10)) {
          valid &= verify(
              dataShape2D.blocks == 1 || dataShape2D.blocks == 2 ||
                  dataShape2D.blocks == 4,
              dataShape2D.blocks,
              ": block2d count (vec_len) must be 1, 2, or 4 for u32 prefetch");
        } else {
          valid &= verify(dataShape2D.blocks == 1 || dataShape2D.blocks == 2,
                          dataShape2D.blocks,
                          ": block2d count (vec_len) must be 1 or 2 for u32");
        }
        break;
      case LSC_DATA_SIZE_64b:
        if (builder.getPlatform() > Xe3 && isNullRawOperand(inst, 10)) {
          valid &= verify(
              dataShape2D.blocks == 1 || dataShape2D.blocks == 2 ||
                  dataShape2D.blocks == 4,
              dataShape2D.blocks,
              ": block2d count (vec_len) must be 1, 2, or 4 for u64 prefetch");
        } else {
          valid &= verify(dataShape2D.blocks == 1, dataShape2D.blocks,
                          ": block2d count (vec_len) must be 1 for u64");
        }
        break;
      default:
        error("invalid data type size for block2d load");
        return;
      }
    } else {
      valid &=
          verify(dataShape2D.blocks == 1,
                 "block2d count (vec_len) must be 1 for block2d transposed");
    }
    if (!valid)
      return;

    //
    if (dataShape2D.blocks > 16) {
      // this constrains us to the encoding space; specific hardware
      // may have additional constraints (e.g. only 1, 2, and 4)
      error("block2d data shape block count (array/vec len) > 16");
    }
    if (inst->opnd_num - currOpIx < LSC_BLOCK2D_ADDR_PARAMS) {
      // TODO: change this when we add grammar for this flavor of block2D operation
      verifyVectorOperandNotNull("addrPayload", currOpIx + 1);
      // imm x and y offsets are currOpIx+2 and currOpIx+3
      verifyDataOperands(currOpIx, currOpIx + 4);
    } else {
      //
      /////////////////////////////////////////
      // now we're at the register operands
      //
      //
      verifyVectorOperandNotNull("SurfaceBase", currOpIx + 1);
      verifyVectorOperandNotNull("SurfaceWidth", currOpIx + 2);
      verifyVectorOperandNotNull("SurfaceHeight", currOpIx + 3);
      verifyVectorOperandNotNull("SurfacePitch", currOpIx + 4);
      verifyVectorOperandNotNull("OffsetX", currOpIx + 5);
      verifyVectorOperandNotNull("OffsetY", currOpIx + 7);
      verifyDataOperands(currOpIx, currOpIx + 9);
    }
  }

  void verifyExtendedCacheCtrlInst() {
    if (sfid != LSC_UGM) {
      error("Extended cache control instruction can only be used on UGM");
    }

    verifyCachingOpts();

    // skip the cache control operation and cache control size verification
    // since there are no constraints on their configuration at the moment
    currOpIx += 2;

    auto addrType = getNextEnumU8<LSC_ADDR_TYPE>();
    if (addrType != LSC_ADDR_TYPE_FLAT)
      error("Extended cache control instructions must have flat address type");
    auto addrSize = getNextEnumU8<LSC_ADDR_SIZE>();
    if (addrSize != LSC_ADDR_SIZE_64b) {
      error("Extended cache control instructions must have a64 address type");
    }

    verifyRawOperand("Src0Addr", currOpIx); // Src0Addr
  }
  ///////////////////////////////////////////////////////////////////////////
  void verifyUntypedBasic() {
    verifyCachingOpts();

    // skip ov
    if (hasOV(sfid, opInfo.op))
      getNext<unsigned>();

       //
    auto addrType = getNextEnumU8<LSC_ADDR_TYPE>();
    uint16_t immediateScale = getNext<uint16_t>();
    if ((immediateScale & (immediateScale - 1)) != 0 &&
        !builder.isEfficient64bEnabled()) {
      error("immediate scale must be power of two "
            "(someone could enable this though)");
    }
    int32_t immediateOffset = getNext<int32_t>();

    auto addrSize = getNextEnumU8<LSC_ADDR_SIZE>();
    verifyAddrSize(addrSize);
    if (addrSize == LSC_ADDR_SIZE_16b) {
      if (immediateOffset > 0x7FFF || immediateOffset < -0x7FFF) {
        error("immediate offset overflows A16 address arithmetic");
      }
    }

    auto dataShape = getNextDataShape();
    verifyDataShape(dataShape);

    // now we are at the registers
    verifyAddressType(addrType, currOpIx); // Surface
    int dstIx = currOpIx + 2;
    int src1DataIx = currOpIx + 4;
    int src0Ix = currOpIx + 3;
    if (opInfo.isStrided())
      src1DataIx = currOpIx + 5;
    const char *src0Name = opInfo.isStrided() ? "Src0AddrBase" : "Src0Addr";
    verifyRawOperand(src0Name, src0Ix); // Src0Addr
    if (opInfo.isStrided()) {
      if (verifyVectorOperand("Src0AddrStride", currOpIx + 3)) {
        const auto &vo = getVectorOperand(inst, currOpIx + 3);
        switch (vo.tag & 0x7) {
        case OPERAND_IMMEDIATE:
          if (vo.opnd_val.const_opnd._val.ival > 0xFFFF) {
            error("Src0AddrStride: pitch exceeds 16 bits");
          }
          break; // okay
        case OPERAND_GENERAL:
          if (vo.opnd_val.gen_opnd.index == 0) {
            error("Src0AddrStride",
                  "VectorOperand must be immediate or non-null");
          }
          break;
        default:
          error("Src0AddrStride", "invalid operand type");
        }
      }
    }
    verifyDataOperands(dstIx, src1DataIx);
  }

  ///////////////////////////////////////////////////////////////////////////
  void verifyTyped() {
    verifyCachingOpts();

    auto addrType = getNextEnumU8<LSC_ADDR_TYPE>();
    auto addrSize = getNextEnumU8<LSC_ADDR_SIZE>();
    verifyAddrSize(addrSize);

    auto dataShape = getNextDataShape();
    verifyDataShape(dataShape);

    verifyAddressType(addrType, currOpIx);
    const unsigned uIx = currOpIx + 3;
    const unsigned vIx = currOpIx + 5;
    const unsigned rIx = currOpIx + 7;
    const unsigned lodIx = currOpIx + 9;
    const unsigned dstOpIx = currOpIx + 2;
    const unsigned src1OpIx = currOpIx + 10;

    // check all the Src0Addr fields (U, V, R, LOD)
    if (opInfo.op == LSC_READ_STATE_INFO) {
      verifyRawOperandNonNull("Src0Addr_UVRL", uIx);  // SIMD1 (U, V, R, LOD)
      verifyRawOperandNull("Src0Addr_Vs", vIx);     // V's
      verifyRawOperandNull("Src0Addr_Rs", rIx);     // R's
      verifyRawOperandNull("Src0Addr_LODs", lodIx); // LOD's
    } else {
      verifyRawOperandNonNull("Src0Addr_Us", uIx); // U's
      verifyRawOperand("Src0Addr_Vs", vIx);        // V's
      verifyRawOperand("Src0Addr_Rs", rIx);        // R's
      verifyRawOperand("Src0Addr_LODs", lodIx);    // LOD's
    }

    // check Dst, Src1, and Src2
    verifyDataOperands(dstOpIx, src1OpIx);
  }

  LSC_DATA_SHAPE_TYPED_BLOCK2D getNextDataShapeTyped2D() {
    LSC_DATA_SHAPE_TYPED_BLOCK2D dataShape2D{};
    dataShape2D.width = (int)getNext<uint16_t>();
    dataShape2D.height = (int)getNext<uint16_t>();
    return dataShape2D;
  }

  void verifyTypedBlock2D() {
    verifyCachingOpts();

    auto addrType = getNextEnumU8<LSC_ADDR_TYPE>();
    const auto dataShape2D = getNextDataShapeTyped2D();
    verify(dataShape2D.height > 0 && dataShape2D.width <= 64,
           "blocks2d height must (0, 64]");
    verify(dataShape2D.width > 0 && dataShape2D.width <= 64,
           "blocks2d width must be (0, 64]");
    verify(dataShape2D.width * dataShape2D.height <= 256,
           "blocks2d size can not exceed 256");

    verifyAddressType(addrType, currOpIx);
    //
    /////////////////////////////////////////
    // now we're at the register operands
    //   0 - Surface Base
    //   1 - Surface Index
    //   2 - Dst
    //   3 - BlockStartOffsetX
    //   4 - ImmOffX
    //   5 - BlockStartOffsetY
    //   6 - ImmOffY
    //   7 - Src1(data sent)
    //
    verifyVectorOperandNotNull("OffsetX", currOpIx + 3);
    verifyVectorOperandNotNull("OffsetY", currOpIx + 5);
    //
    verifyDataOperands(currOpIx + 2, currOpIx + 7);
  }

  void verifyUntypedAppendCounterAtomic() {
    verifyCachingOpts();

    auto addrType = getNextEnumU8<LSC_ADDR_TYPE>();

    auto dataShape = getNextDataShape();
    verifyDataShape(dataShape);

    // now we are at the registers
    verifyAddressType(addrType, currOpIx);         // surface
    verifyRawOperandNull("Src0Addr", currOpIx + 3); // src0 address is null
    verifyRawOperandNonNull("Src1Data", currOpIx + 4); // src1 data
  }
  void verify() {
    if (builder.isEfficient64bEnabled()) {
      verifyLSCEfficient64b();
    } else {
      verifyLSC();
    }
  }
  void verifyLSCEfficient64b() {
    if (opInfo.mnemonic == nullptr || opInfo.op == LSC_INVALID) {
      error("invalid LSC subop");
      return;
    }
    if (inst->opcode == ISA_LSC_FENCE) {
      verifyFence();
    } else if (inst->opcode == ISA_LSC_UNTYPED) {
      if (subOp == LSC_LOAD_BLOCK2D || subOp == LSC_STORE_BLOCK2D) {
        verifyUntypedBlock2D();
      } else {
        if (subOp == LSC_EXTENDED_CACHE_CTRL)
          verifyExtendedCacheCtrlInst();
        else
          verifyUntypedBasic();
      }
    } else if (inst->opcode == ISA_LSC_TYPED) {
      if (subOp == LSC_LOAD_BLOCK2D || subOp == LSC_STORE_BLOCK2D) {
        return verifyTypedBlock2D();
      }
      verifyTyped();
    } else {
      badEnum("invalid LSC op code", inst->opcode);
    }
  }
  void verifyLSC() {
    if (opInfo.mnemonic == nullptr || opInfo.op == LSC_INVALID) {
      error("invalid LSC subop");
      return;
    }
    if (inst->opcode == ISA_LSC_FENCE) {
      verifyFence();
    } else if (inst->opcode == ISA_LSC_UNTYPED) {
      if (subOp == LSC_LOAD_BLOCK2D || subOp == LSC_STORE_BLOCK2D) {
        verifyUntypedBlock2D();
      } else if (subOp == LSC_APNDCTR_ATOMIC_ADD ||
                 subOp == LSC_APNDCTR_ATOMIC_SUB ||
                 subOp == LSC_APNDCTR_ATOMIC_STORE) {
        verifyUntypedAppendCounterAtomic();
      } else {
        verifyUntypedBasic();
      }
    } else if (inst->opcode == ISA_LSC_TYPED) {
      if (subOp == LSC_LOAD_BLOCK2D || subOp == LSC_STORE_BLOCK2D) {
        return verifyTypedBlock2D();
      }
      verifyTyped();
    } else {
      badEnum("invalid LSC op code", inst->opcode);
    }
  }
}; // LscInstVerifier

void vISAVerifier::verifyInstructionLsc(const CISA_INST *inst) {
  LscInstVerifier verifier(header, inst, *irBuilder, options);
  verifier.verify();
  if (verifier.errorStream.tellp() > 0) {
    std::stringstream ss;
    ss << "in instruction " << header->printInstruction(inst, options) << "\n";
    ss << verifier.errorStream.str();
    error_list.push_back(ss.str());
  }
}
void vISAVerifier::verifyInstructionSrnd(const CISA_INST *inst) {
  const vector_opnd &dst = getVectorOperand(inst, 0);
  VISA_Type dstType = getVectorOperandType(header, dst);

  REPORT_INSTRUCTION(options, isNoMask(inst->getExecMask()),
                     "srnd must use noMask");

  // dst
  REPORT_INSTRUCTION(
      options, dst.getOperandClass() == OPERAND_GENERAL,
      "Destination of this CISA instruction should be general operand.");

  // src
  const vector_opnd &src0 = getVectorOperand(inst, 1);
  VISA_Type src0Type = getVectorOperandType(header, src0);
  VISA_Modifier src0Modifier = src0.getOperandModifier();
  const vector_opnd &src1 = getVectorOperand(inst, 2);
  VISA_Type src1Type = getVectorOperandType(header, src1);
  VISA_Modifier src1Modifier = src1.getOperandModifier();

  REPORT_INSTRUCTION(
      options, src0Modifier == MODIFIER_NONE && src1Modifier == MODIFIER_NONE,
      "Source modifiers for this instruction are not allowed");
  REPORT_INSTRUCTION(
      options,
      (src0.getOperandClass() == OPERAND_GENERAL ||
       src0.getOperandClass() == OPERAND_IMMEDIATE) &&
          (src1.getOperandClass() == OPERAND_GENERAL ||
           src1.getOperandClass() == OPERAND_IMMEDIATE),
      "Sources in this instruction must be general or immediate");

  // FIXME, ISA_TYPE_F | ISA_TYPE_HF of src1 is to be WA the LIT test of VC
  REPORT_INSTRUCTION(options,
                     src1Type == ISA_TYPE_UW || src1Type == ISA_TYPE_UB ||
                         src1Type == ISA_TYPE_F || src1Type == ISA_TYPE_HF,
                     "src1 use UW/UB type");
  if (irBuilder->getPlatform() > Xe3) {
    REPORT_INSTRUCTION(
        options,
        ((dstType == ISA_TYPE_B || dstType == ISA_TYPE_UB) &&
         (src0Type == ISA_TYPE_HF || src0Type == ISA_TYPE_BF)) ||
            (dstType == ISA_TYPE_HF && src0Type == ISA_TYPE_F),
        "Src and Dst types mismatch. Only (dst=ub(bf8)/b(hf8), src=hf/bf) or "
        "(dst=hf, src=f) supported.");
  } else {
    REPORT_INSTRUCTION(
        options,
        (dstType == ISA_TYPE_UB && src0Type == ISA_TYPE_HF) ||
            (dstType == ISA_TYPE_HF && src0Type == ISA_TYPE_F),
        "Src and Dst types mismatch. Only (dst=ub(bf8), src=hf) or "
        "(dst=hf, src=f) supported.");
  }
}

void vISAVerifier::verifyKernelAttributes() {
  /// Verify SLMSize, if present, shows up only once
  unsigned int numSLMSize = 0;
  unsigned int numNamedBarrierCnt = 0;
  for (unsigned int i = 0; i < header->getAttrCount(); i++) {
    auto attr = header->getAttr(i);
    const char *attrName = header->getString(attr->nameIndex);
    Attributes::ID aID = Attributes::getAttributeID(attrName);
    if (aID == Attributes::ATTR_SLMSize) {
      numSLMSize++;
    } else if (aID == Attributes::ATTR_NBarrierCnt) {
      numNamedBarrierCnt++;
    }
  }

  REPORT_HEADER(options, numSLMSize <= 1,
                "More than 1 kernel attribute defined %s",
                Attributes::getAttributeName(Attributes::ATTR_SLMSize));

  REPORT_HEADER(options, numNamedBarrierCnt <= 1,
                "More than 1 kernel attribute defined %s",
                Attributes::getAttributeName(Attributes::ATTR_NBarrierCnt));
}

void vISAVerifier::verifyKernelHeader() {
  /// Verify variable decls.
  unsigned numPreDefinedVars = Get_CISA_PreDefined_Var_Count();
  unsigned numPreDefinedSurfs = Get_CISA_PreDefined_Surf_Count();
  for (unsigned i = 0; i < header->getVarCount(); i++) {
    verifyVariableDecl(i);
  }

  /// Verify address decls.
  for (unsigned i = 0; i < header->getAddrCount(); i++) {
    verifyAddressDecl(i);
  }

  /// Verify predicate decls.
  for (unsigned i = 0; i < header->getPredCount(); i++) {
    verifyPredicateDecl(i);
  }

  /// Verify labels.
  for (unsigned i = 0; i < header->getLabelCount(); i++) {
    REPORT_HEADER(options,
                  header->getLabel(i)->name_index < header->getStringCount(),
                  "label%d's name index(%d) is not valid", i,
                  header->getLabel(i)->name_index);
  }

  /// Verify sampler.
  for (unsigned i = 0; i < header->getSamplerCount(); i++) {
    REPORT_HEADER(options,
                  header->getSampler(i)->name_index < header->getStringCount(),
                  "S%d's name index(%d) is not valid", i,
                  header->getSampler(i)->name_index);
    REPORT_HEADER(options,
                  header->getSampler(i)->num_elements <=
                      COMMON_ISA_MAX_SAMPLER_SIZE,
                  "S%d's number of elements(%d) is not valid", i,
                  header->getSampler(i)->num_elements);
  }

  /// Verify surface.
  unsigned int numPredSurf = Get_CISA_PreDefined_Surf_Count();
  for (unsigned i = numPredSurf; i < header->getSurfaceCount(); i++) {
    REPORT_HEADER(options,
                  header->getSurface(i)->name_index < header->getStringCount(),
                  "T%d's name index(%d) is not valid", i,
                  header->getSurface(i)->name_index);
    REPORT_HEADER(options,
                  header->getSurface(i)->num_elements <=
                      COMMON_ISA_MAX_SURFACE_SIZE,
                  "T%d's number of elements(%d) is not valid", i,
                  header->getSurface(i)->num_elements);
  }

  // Verify inputs.
  // v3.3+, kernel may have explicit arguments followed by implicit ones.
  // This information is only used by the CM runtime, not by Finalizer.
  uint32_t versionNum =
      getVersionAsInt(header->getMajorVersion(), header->getMinorVersion());
  if (versionNum >= getVersionAsInt(3, 3)) {
    for (unsigned i = 0; i < header->getInputCount(); i++) {
      if (header->getInput(i)->getImplicitKind() != 0) {
        break;
      }
    }
  }

  GRFMode GRFInfo(irBuilder->getPlatform(), irBuilder->getGRFSize(), options);
  unsigned GRFNumber = GRFInfo.getMaxGRF();

  // [Begin, end) is an interval for each input. We check two things
  // - no overlap
  // - do not overflow i.e. end < 32 * (256 - 1)
  for (unsigned i = 0; i < header->getInputCount(); i++) {
    {
      auto pi = header->getInput(i);
      // Pseudo inputs are user defined, so no error checking is
      // done by compiler.
      if (pi->isPseudoInput())
        continue;
      unsigned Begin = pi->offset;
      unsigned End = Begin + pi->size;
      if (End >= (uint32_t)(irBuilder->getGRFSize() * (GRFNumber - 1))) {
        REPORT_HEADER(options, false, "Input V%d is out of bound [%d, %d)",
                      pi->index, Begin, End);
      }
      for (unsigned j = 0; j < i; ++j) {
        auto pj = header->getInput(j);
        if (pj->isPseudoInput())
          continue;
        unsigned Begin1 = pj->offset, End1 = pj->offset + pj->size;
        if ((Begin >= Begin1 && Begin < End1) ||
            (Begin1 >= Begin && Begin1 < End)) {
          REPORT_HEADER(options, false,
                        "Input V%d = [%d, %d) intersects with V%d = [%d, %d)",
                        pi->index, Begin, End, pj->index, Begin1, End1);
        }
      }
    }

    switch (header->getInput(i)->getInputClass()) {
    case INPUT_GENERAL:
      if (header->getInput(i)->index < numPreDefinedVars) {
        REPORT_HEADER(options, false,
                      "Input %d points to an invalid variable(%d)", i,
                      header->getInput(i)->index);
      } else {
        int varId = header->getInput(i)->index - numPreDefinedVars;
        int varSize = header->getVar(varId)->num_elements *
                      CISATypeTable[header->getVar(varId)->getType()].typeSize;
        REPORT_HEADER(
            options, varSize == header->getInput(i)->size,
            "Input %d's size(%d) does not agree with its variable (V%d)'s", i,
            header->getInput(i)->size, varId + numPreDefinedVars);
        if (header->getInput(i)->size < irBuilder->getGRFSize()) {
          // check that input does not straddle GRF boundary
          auto beginGRF = header->getInput(i)->offset / irBuilder->getGRFSize();
          auto endGRF =
              (header->getInput(i)->offset + header->getInput(i)->size - 1) /
              irBuilder->getGRFSize();
          REPORT_HEADER(options, beginGRF == endGRF,
                        "Input %s is <1 GRF but straddles GRF boundary",
                        header->getInput(i)->dcl->getName());
        }
      }
      break;
    case INPUT_SAMPLER:
      REPORT_HEADER(options,
                    header->getInput(i)->index < header->getSamplerCount(),
                    "Input%d points to an invalid sampler index(%d)", i,
                    header->getInput(i)->index);
      break;
    case INPUT_SURFACE:
      if (header->getInput(i)->index - numPredSurf >=
              header->getSurfaceCount() ||
          header->getInput(i)->index < numPredSurf) {
        REPORT_HEADER(
            options, false,
            "Input%d points to an invalid/predefined surface index(%d)", i,
            header->getInput(i)->index);
      } else {
        int surfaceSize;
        surfaceSize =
            header->getSurface(header->getInput(i)->index - numPreDefinedSurfs)
                ->num_elements *
            CISATypeTable[ISA_TYPE_UD].typeSize;

        REPORT_HEADER(options, surfaceSize == header->getInput(i)->size,
                      "Input%d's size(%d) does not agree with its surface's", i,
                      header->getInput(i)->size);
      }
      break;
    default:
      REPORT_HEADER(options, false, "Input%d has invalid operand class(%d)", i,
                    header->getInput(i)->getInputClass());
    }
    /// TODO: Consider adding offset checks here.
  }

  verifyKernelAttributes();
}

void vISAVerifier::finalize() {
  for (const auto &iter : labelDefs) {
    if (!(iter.second)) {
      const label_info_t *lblInfo = header->getLabel(iter.first);
      if (lblInfo->kind != LABEL_FC) {
        REPORT_HEADER(options, false, "undefined label: %s",
                      header->getString(lblInfo->name_index));
      }
    }
  }
}

int vISAVerifier::verifyInstruction(const CISA_INST *inst) {
  size_t initialErrors = getNumErrors();
  ISA_Opcode opcode = (ISA_Opcode)inst->opcode;

  if (!(ISA_RESERVED_0 < opcode && opcode < ISA_NUM_OPCODE)) {
    REPORT_INSTRUCTION(options, false, "Invalid vISA opcode: %d", opcode);
    return VISA_FAILURE;
  }

  TARGET_PLATFORM instPlatform = CISA_INST_table[opcode].platf;
  if (instPlatform != ALL) {
    // We assume instructions are backward compatible
    // instructions that are not should have their own checks elsewhere
    REPORT_INSTRUCTION(options, irBuilder->getPlatform() >= instPlatform,
                       "vISA instruction not supported on this platform");
  }

  for (unsigned i = 0; i < inst->opnd_num; i++)
    verifyOperand(inst, i);

  if (hasExecSize(opcode)) {
    auto execSize = inst->getExecSize();
    REPORT_INSTRUCTION(options, execSize < EXEC_SIZE_ILLEGAL,
                       "vISA instruction uses an illegal execution size.");
    // check for illegal combinations of emask and execution size
    REPORT_INSTRUCTION(
        options,
        Get_VISA_Exec_Size(execSize) + getvISAMaskOffset(inst->getExecMask()) <=
            32,
        "vISA instruction has illegal combination of execution size and mask");
  }

  if (hasPredicate(opcode)) {
    uint16_t predicateNum = inst->pred.getId();
    REPORT_INSTRUCTION(options,
                       predicateNum < header->getPredCount() +
                                          COMMON_ISA_NUM_PREDEFINED_PRED,
                       "CISA instruction uses an illegal predicate value.");
  }

  switch (ISA_Inst_Table[opcode].type) {
  case ISA_Inst_SVM:
    verifyInstructionSVM(inst);
    break;
  case ISA_Inst_Mov:
    verifyInstructionMove(inst);
    break;
  case ISA_Inst_Sync:
    verifyInstructionSync(inst);
    break;
  case ISA_Inst_Flow:
    verifyInstructionControlFlow(inst);
    break;
  case ISA_Inst_Misc:
    verifyInstructionMisc(inst);
    break;
  case ISA_Inst_Arith:
    verifyInstructionArith(inst);
    break;
  case ISA_Inst_Logic:
    verifyInstructionLogic(inst);
    break;
  case ISA_Inst_Compare:
    verifyInstructionCompare(inst);
    break;
  case ISA_Inst_Address:
    verifyInstructionAddress(inst);
    break;
  case ISA_Inst_Sampler:
    verifyInstructionSampler(inst);
    break;
  case ISA_Inst_SIMD_Flow:
    verifyInstructionSIMDFlow(inst);
    break;
  case ISA_Inst_Data_Port:
    verifyInstructionDataport(inst);
    break;
  case ISA_Inst_LSC:
    verifyInstructionLsc(inst);
    break;
  default: {
    REPORT_INSTRUCTION(
        options, false,
        "Illegal or unimplemented CISA instruction (opcode, type): (%d, %d).",
        opcode, ISA_Inst_Table[opcode].type);
    return VISA_FAILURE;
  }
  }

  // Verify particular features
  verifyBFMixedMode(inst);

  return initialErrors == getNumErrors() ? VISA_SUCCESS : VISA_FAILURE;
}

void vISAVerifier::verifyInstructionShflIdx4(const CISA_INST *inst) {
  unsigned i = 0;

  auto execSize = inst->getExecSize();
  REPORT_INSTRUCTION(
      options, execSize == EXEC_SIZE_16 || execSize == EXEC_SIZE_32,
      "ISA_SHFL_IDX4 inst only supports execution size 16 and 32.");

  // Verify dst
  const raw_opnd &dst = getRawOperand(inst, i);
  verifyRawOperandType(inst, dst, isUDType);

  // Verify src0
  const vector_opnd &src0 = getVectorOperand(inst, ++i);
  REPORT_INSTRUCTION(options, src0.getOperandClass() == OPERAND_GENERAL,
                     "src0 in ISA_SHFL_IDX4 inst must be general.");
  VISA_Type src0Type = getVectorOperandType(header, src0);
  REPORT_INSTRUCTION(options,
                     src0Type == ISA_TYPE_UD,
                     "src0 datatype in ISA_SHFL_IDX4 inst must be UD.");
  uint16_t rowOffset0 = 0, colOffset0 = 0, vs0 = 0, w0 = 0, hs0 = 0;
  getRegion(src0, rowOffset0, colOffset0, vs0, w0, hs0);
  if (execSize == EXEC_SIZE_16)
    REPORT_INSTRUCTION(
        options,
        (vs0 == 1 && w0 == 1 && hs0 == 0 && colOffset0 == 0),
        "src0 in ISA_SHFL_IDX4 on SIMD16 must have region .0<1;1,0>.");
  if (execSize == EXEC_SIZE_32)
    REPORT_INSTRUCTION(options,
                       (vs0 == 0 && w0 == 16 && hs0 == 1 && colOffset0 == 0),
                       "src0 in ISA_SHFL_IDX4 on SIMD32 must have region "
                       ".0<0;16,1>.");

  // Verify src1
  const vector_opnd &src1 = getVectorOperand(inst, ++i);
  REPORT_INSTRUCTION(options, src1.getOperandClass() == OPERAND_GENERAL,
                     "src1 in ISA_SHFL_IDX4 inst must be general.");
  VISA_Type src1Type = getVectorOperandType(header, src1);
  REPORT_INSTRUCTION(options,
                     src1Type == ISA_TYPE_UW || src1Type == ISA_TYPE_UB,
                     "src1 datatype in ISA_SHFL_IDX4 inst must be UW or UB.");
  uint16_t rowOffset1 = 0, colOffset1 = 0, vs1 = 0, w1 = 0, hs1 = 0;
  getRegion(src1, rowOffset1, colOffset1, vs1, w1, hs1);
  if (src1Type == ISA_TYPE_UW)
    REPORT_INSTRUCTION(
        options,
        (vs1 == 2 && w1 == 1 && hs1 == 0 && (colOffset1 == 0 || colOffset1 == 1)),
        "src1 in ISA_SHFL_IDX4 must have region .(0|1)<2;1,0> for UW "
        "datatype.");
  if (src1Type == ISA_TYPE_UB)
    REPORT_INSTRUCTION(options,
                       (vs1 == 4 && w1 == 1 && hs1 == 0 &&
                        (colOffset1 == 0 || colOffset1 == 1 || colOffset1 == 2 ||
                         colOffset1 == 3)),
                       "src1 in ISA_SHFL_IDX4 must have region "
                       ".(0|1|2|3)<4;1,0> for UB datatype.");

  // verify source modifier
  REPORT_INSTRUCTION(options,
                     src0.getOperandModifier() == MODIFIER_NONE &&
                         src1.getOperandModifier() == MODIFIER_NONE,
                     "ISA_SHFL_IDX4 inst does not support source modifier.");
}

void vISAVerifier::verifyInstructionLfsr(const CISA_INST* inst) {
  // check the first operand which is FuncCtrl
  REPORT_INSTRUCTION(options,
                     getOperandType(inst, 0) == CISA_OPND_OTHER,
                     "first opnd in ISA_LFSR should be CISA_OPND_OTHER type");

  // check dst/src oprands class
  const vector_opnd &dst = getVectorOperand(inst, 1);
  const vector_opnd &src0 = getVectorOperand(inst, 2);
  const vector_opnd &src1 = getVectorOperand(inst, 3);
  Common_ISA_Operand_Class dstClass = dst.getOperandClass();
  Common_ISA_Operand_Class src0Class = src0.getOperandClass();
  Common_ISA_Operand_Class src1Class = src1.getOperandClass();
  REPORT_INSTRUCTION(
      options, dstClass == OPERAND_GENERAL && src0Class == OPERAND_GENERAL,
      "dst and src0 in ISA_LFSR should be general operand");
  REPORT_INSTRUCTION(
      options, src1Class == OPERAND_GENERAL || src1Class == OPERAND_IMMEDIATE,
      "src1 in ISA_LFSR should be general or immediate operand");

  // check source modifier and saturation
  VISA_Modifier dstMod = dst.getOperandModifier();
  VISA_Modifier src0Mod = src0.getOperandModifier();
  VISA_Modifier src1Mod = src1.getOperandModifier();
  REPORT_INSTRUCTION(options, dstMod == MODIFIER_NONE,
                     "saturation is not allowed for ISA_LFSR inst");
  REPORT_INSTRUCTION(options,
                     src0Mod == MODIFIER_NONE && src1Mod == MODIFIER_NONE,
                     "ISA_LFSR inst does not support source modifier.");

  // check dst/src operand datatype
  VISA_Type dstTy = getVectorOperandType(header, dst);
  VISA_Type src0Ty = getVectorOperandType(header, src0);
  VISA_Type src1Ty = getVectorOperandType(header, src1);
  REPORT_INSTRUCTION(options,
                     dstTy == ISA_TYPE_UD && src0Ty == ISA_TYPE_UD &&
                         src1Ty == ISA_TYPE_UD,
                     "operand datatype in ISA_LFSR inst must be UD.");

  // check regioning of dst
  uint16_t rowOffsetDst = 0, colOffsetDst = 0, vsDst = 0, wDst = 0, hsDst = 0;
  getRegion(dst, rowOffsetDst, colOffsetDst, vsDst, wDst, hsDst);
  REPORT_INSTRUCTION(
      options, hsDst == 1,
      "dst in ISA_LFSR must have region <1>.");

  // check regioning of src0
  uint16_t rowOffset0 = 0, colOffset0 = 0, vs0 = 0, w0 = 0, hs0 = 0;
  getRegion(src0, rowOffset0, colOffset0, vs0, w0, hs0);
  REPORT_INSTRUCTION(
      options,
      (vs0 == 0 && w0 == 1 && hs0 == 0) || (vs0 == 1 && w0 == 1 && hs0 == 0),
      "src0 in ISA_LFSR  must have region <1;1,0> or <0;1,0>.");

  // check regioning of src1
  uint16_t rowOffset1 = 0, colOffset1 = 0, vs1 = 0, w1 = 0, hs1 = 0;
  getRegion(src0, rowOffset1, colOffset1, vs1, w1, hs1);
  if (src1Class == OPERAND_GENERAL)
    REPORT_INSTRUCTION(
        options,
        (vs1 == 0 && w1 == 1 && hs1 == 0) || (vs1 == 1 && w1 == 1 && hs1 == 0),
        "src1 in ISA_LFSR must have region <1;1,0> or <0;1,0>.");
}

void vISAVerifier::verifyInstructionDnscl(const CISA_INST *inst) {
  // check the first 3 operands: conversion type, mode, rounding mode
  REPORT_INSTRUCTION(
      options,
      getOperandType(inst, 0) == CISA_OPND_OTHER &&
          getOperandType(inst, 1) == CISA_OPND_OTHER &&
          getOperandType(inst, 2) == CISA_OPND_OTHER,
      "first 3 operands in ISA_DNSCL should be CISA_OPND_OTHER type");

  // check dst/src oprands type
  const raw_opnd &dst = getRawOperand(inst, 3);
  const raw_opnd &src0 = getRawOperand(inst, 4);
  const raw_opnd &src1 = getRawOperand(inst, 5);
  const raw_opnd &src2 = getRawOperand(inst, 6);
  verifyRawOperandType(inst, dst, isUDType);
  verifyRawOperandType(inst, src0, isUDType);
  verifyRawOperandType(inst, src1, isUDType);
  verifyRawOperandType(inst, src2, isUDType);
}
#endif // IS_RELEASE_DLL
