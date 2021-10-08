/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef COMMON_ISA_FRAMEWORK
#define COMMON_ISA_FRAMEWORK

#include <map>
#include <list>
#include <vector>
#include <cstdio>
#include <string>
#include <sstream>
#include <fstream>

#include "VISADefines.h"
#include "Common_ISA.h"
#include "visa_igc_common_header.h"
#include "IsaDescription.h"
#include "Mem_Manager.h"

#define CISA_INVALID_ADDR_ID     -1
#define CISA_INVALID_PRED_ID     -1
#define CISA_INVALID_VAR_ID      ((unsigned)-1)
#define CISA_INVALID_SURFACE_ID  -1
#define CISA_INVALID_SAMPLER_ID  -1
#define INVALID_LABEL_ID         -1



// reserve p0 for the case of no predication
#define COMMON_ISA_NUM_PREDEFINED_PRED      1

#if 0
#define DEBUG_PRINT_SIZE(msg, value) {std::cout<< msg << value << std::endl; }
#define DEBUG_PRINT_SIZE_INSTRUCTION(msg, inst ,value) {std::cerr<< msg << ISA_Inst_Table[inst].str <<" : " << value << std::endl; }
#else
#define DEBUG_PRINT_SIZE(msg, value)
#define DEBUG_PRINT_SIZE_INSTRUCTION(msg, inst ,value)
#endif

struct attr_gen_struct {
    const char* name;
    bool  isInt;
    int   value;
    const char* string_val;
    bool  attr_set;
};

typedef struct _VISA_PredOpnd        : VISA_opnd {} VISA_PredOpnd;
typedef struct _VISA_RawOpnd         : VISA_opnd {} VISA_RawOpnd;
typedef struct _VISA_VectorOpnd      : VISA_opnd {} VISA_VectorOpnd;
typedef struct _VISA_LabelOpnd       : VISA_opnd {} VISA_LabelOpnd;
typedef struct _VISA_StateOpndHandle : VISA_opnd {} VISA_StateOpndHandle;

class VISAKernel;
class VISAKernelImpl;
class CISA_IR_Builder;

namespace CisaFramework
{

class CisaInst
{
public:

    CisaInst(vISA::Mem_Manager &mem) :
        m_mem(mem),
        m_size(0)
    {
        memset(&m_cisa_instruction, 0, sizeof(CISA_INST));
        m_size = 1; // opcode size
    }

    virtual ~CisaInst() { }

    CISA_INST             m_cisa_instruction;
    const VISA_INST_Desc* m_inst_desc;

    int createCisaInstruction(ISA_Opcode opcode, unsigned char exec_size, unsigned char modifier, PredicateOpnd pred, VISA_opnd **opnd, int numOpnds, const VISA_INST_Desc* inst_desc);

    int getSize() const {return m_size;}

    CISA_INST*      getCISAInst     () { return &m_cisa_instruction; }
    const VISA_INST_Desc* getCISAInstDesc () const { return m_inst_desc;}

    VISA_opnd * getOperand(unsigned index) const
    {
        return m_cisa_instruction.opnd_array[index];
    }

    unsigned char        getOpcode       () const { return m_cisa_instruction.opcode;           }
    unsigned char        getModifier     () const { return m_cisa_instruction.modifier;         }
    unsigned             getOperandCount () const { return m_cisa_instruction.opnd_count;       }
    VISA_Exec_Size getExecSize     () const { return (VISA_Exec_Size)
                                                           (m_cisa_instruction.execsize & 0xF); }

    void *operator new(size_t sz, vISA::Mem_Manager& m) {return m.alloc(sz); }

private:
    char* m_inline_cisa;

    vISA::Mem_Manager &m_mem;
    short m_size;
};

class CisaBinary
{
public:

    CisaBinary(CISA_IR_Builder* builder);

    virtual ~CisaBinary() { }

    void initCisaBinary(int numberKernels, int numberFunctions)
    {
        m_header.kernels = (kernel_info_t *) m_mem.alloc(sizeof(kernel_info_t) * numberKernels);
        memset(m_header.kernels, 0, sizeof(kernel_info_t) * numberKernels);

        m_header.functions = (function_info_t *) m_mem.alloc(sizeof(function_info_t) * numberFunctions);
        memset(m_header.functions, 0, sizeof(function_info_t) * numberFunctions);

        m_upper_bound_kernels = numberKernels;
        m_upper_bound_functions = numberFunctions;

        m_kernelOffsetLocationsArray = (int *)m_mem.alloc(sizeof(int)*numberKernels); //array to store offsets of where the offset of kernel is stored in isa header
        m_kernelInputOffsetLocationsArray = (int *)m_mem.alloc(sizeof(int)*numberKernels);
        m_krenelBinaryInfoLocationsArray = (int *)m_mem.alloc(sizeof(int)*numberKernels);

        m_functionOffsetLocationsArray = (int *)m_mem.alloc(sizeof(int)*numberFunctions); //array to store offsets of where the offset of kernel is stored in isa header

        genxBinariesSize = 0;
    }

    void setMagicNumber (unsigned int  v) { m_header.magic_number  = v; }
    void setMajorVersion(unsigned char v) { m_header.major_version = v; }
    void setMinorVersion(unsigned char v) { m_header.minor_version = v; }
    unsigned char getMajorVersion () const { return m_header.major_version; }
    unsigned char getMinorVersion () const { return m_header.minor_version; }
    unsigned int  getMagicNumber  () const { return m_header.magic_number;  }

    const common_isa_header& getIsaHeader() const { return m_header; }

    void initKernel(int kernelIndex, VISAKernelImpl * kernel);
    int finalizeCisaBinary();
    int dumpToFile(std::string binFileName);
    int dumpToStream(std::ostream *os);

    void *operator new(size_t sz, vISA::Mem_Manager& m) {return m.alloc(sz); }

    const VISAKernelImpl* getFmtKernelForISADump(const VISAKernelImpl* kernel,
                                                 const std::list<VISAKernelImpl *>& kernels) const;
    std::string isaDump(const VISAKernelImpl* kernel, const VISAKernelImpl* fmtKernel) const;
    int isaDump(const std::list<VISAKernelImpl *>&, const Options *options) const;
    void writeIsaAsmFile(std::string filename, std::string isaasmStr) const;

    unsigned long getKernelVisaBinarySize(int i) {return m_header.kernels[i].size; }
    unsigned long getFunctionVisaBinarySize(int i) {return m_header.functions[i].size; }
    unsigned short getNumberKernels() {return m_header.num_kernels; }
    unsigned short getNumberFunctions() {return m_header.num_functions; }

    char * getVisaHeaderBuffer() {return m_header_buffer; }

    void patchKernel(int index, unsigned int genxBufferSize, void * buffer, int platform);
    void patchFunction(int index, unsigned genxBufferSize);
    void patchFunctionWithGenBinary(int index, unsigned int genxBufferSize, char* buffer);

    Options *getOptions() {return m_options; }

private:
    /*
        Arrays that store locations (offset from beginning) in isa header buffer
        for offset field, and gen_binary_info data structure
        Will be used later to patch it when genx binaries are generated
    */
    int genxBinariesSize;
    int *m_kernelOffsetLocationsArray; //array to store offsets of where the offset of kernel is stored in isa header
    int *m_kernelInputOffsetLocationsArray;
    int *m_krenelBinaryInfoLocationsArray;

    int *m_functionOffsetLocationsArray; //array to store offsets of where the offset of kernel is stored in isa header
    unsigned long writeInToCisaHeaderBuffer(const void * value, int size);

    common_isa_header m_header;
    vISA::Mem_Manager m_mem;
    uint32_t m_header_size;
    uint32_t m_total_size;
    unsigned long m_bytes_written_cisa_buffer;
    char * m_header_buffer;
    int m_upper_bound_kernels;
    int m_upper_bound_functions;

    Options *m_options;

    CISA_IR_Builder* parent;
};

bool allowDump(const Options& options, const std::string& fileName);

}
#endif
