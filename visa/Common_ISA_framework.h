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
    char* name;
    bool  isInt;
    int   value;
    char* string_val;
    bool  attr_set;
};

typedef CISA_opnd VISA_opnd;

typedef struct _VISA_PredOpnd        : VISA_opnd {} VISA_PredOpnd;
typedef struct _VISA_RawOpnd         : VISA_opnd {} VISA_RawOpnd;
typedef struct _VISA_VectorOpnd      : VISA_opnd {} VISA_VectorOpnd;
typedef struct _VISA_LabelOpnd       : VISA_opnd {} VISA_LabelOpnd;
typedef struct _VISA_StateOpndHandle : VISA_opnd {} VISA_StateOpndHandle;

class VISAKernel;
class VISAKernelImpl;
namespace vISA
{
    class IR_Builder;
}

namespace CisaFramework
{

class  CisaInst;
class  CisaBinary;

class CisaInst
{
public:

    CisaInst(vISA::Mem_Manager &mem) :
        m_mem(mem),
        m_size(0)
    {
        memset(&m_cisa_instruction, 0, sizeof(CISA_INST));
        m_size = 1; // opcode size
        m_need_label_patch = false;
    }

    virtual ~CisaInst() { }

    CISA_INST             m_cisa_instruction;
    const VISA_INST_Desc* m_inst_desc;

    int createCisaInstruction(ISA_Opcode opcode, unsigned char exec_size, unsigned char modifier, unsigned short pred, VISA_opnd **opnd, int numOpnds, const VISA_INST_Desc* inst_desc);

    int getSize() const {return m_size;}

    void setLabelInfo(const std::string& label_name, bool is_func, bool needPatch)
    {
        m_label_name = label_name;
        m_is_function = is_func;
        m_need_label_patch = needPatch;
    }

    void setLabelIndex(int id)
    {
        m_cisa_instruction.opnd_array[0]->_opnd.other_opnd = id;
    }

    bool   needLabelPatch () const { return m_need_label_patch; }
    bool   isFuncLabel    () const { return m_is_function;      }
    std::string getLabelName   () const { return m_label_name;       }

    CISA_INST*      getCISAInst     () { return &m_cisa_instruction; }
    const VISA_INST_Desc* getCISAInstDesc () const { return m_inst_desc;}

    VISA_opnd * getOperand(unsigned index) const
    {
        return m_cisa_instruction.opnd_array[index];
    }

    unsigned short       getPredicate    () const { return m_cisa_instruction.pred;             }
    unsigned short       getPredicateNum () const { return getPredicate() & 0xfff;              }
    unsigned char        getOpcode       () const { return m_cisa_instruction.opcode;           }
    unsigned char        getModifier     () const { return m_cisa_instruction.modifier;         }
    unsigned             getOperandCount () const { return m_cisa_instruction.opnd_count;       }
    VISA_Exec_Size getExecSize     () const { return (VISA_Exec_Size)
                                                           (m_cisa_instruction.execsize & 0xF); }

    void *operator new(size_t sz, vISA::Mem_Manager& m){ return m.alloc(sz); }

private:
    char* m_inline_cisa;

    vISA::Mem_Manager &m_mem;
    short m_size;
    friend class vISA::IR_Builder;
    friend class CISA_IR_Builder;

    // during emition to determin what the labelID is.
    std::string m_label_name;
    bool m_is_function;
    bool m_need_label_patch;
};

class CisaBinary
{
public:

    CisaBinary(std::string filename, Options *options) :
        m_mem(4096),
        m_header_size(0),
        m_total_size(0),
        m_bytes_written_cisa_buffer(0),
        m_header_buffer(NULL),
        m_filename(filename),
        m_instId(0),
        m_options(options)
    {
        memset(&m_header, 0, sizeof(common_isa_header));
    }

    CisaBinary(Options *options) :
        m_mem(4096),
        m_filename(""),
        m_instId(0),
        m_options(options)
    {
        memset(&m_header, 0, sizeof(common_isa_header));

        m_header.num_kernels = 0;
        m_header.num_functions = 0;
        m_upper_bound_kernels = 0;
        m_upper_bound_functions = 0;

        m_header_size = 0;
        m_total_size = 0;
        m_bytes_written_cisa_buffer = 0;
        m_header_buffer = NULL;
    }

    virtual ~CisaBinary() { }

    void initCisaBinary(int numberKernels, int numberFunctions)
    {
        m_header.kernels = (kernel_info_t * ) m_mem.alloc(sizeof(kernel_info_t) * numberKernels);
        memset(m_header.kernels, 0, sizeof(kernel_info_t) * numberKernels);

        m_header.functions = (function_info_t * ) m_mem.alloc(sizeof(function_info_t) * numberFunctions);
        memset(m_header.functions, 0, sizeof(function_info_t) * numberFunctions);

        m_upper_bound_kernels = numberKernels;
        m_upper_bound_functions = numberFunctions;

        m_kernelOffsetLocationsArray = (int *)m_mem.alloc(sizeof(int)*numberKernels); //array to store offsets of where the offset of kernel is stored in isa header
        m_kernelInputOffsetLocationsArray = (int *)m_mem.alloc(sizeof(int)*numberKernels);
        m_krenelBinaryInfoLocationsArray = (int *)m_mem.alloc(sizeof(int)*numberKernels);

        m_functionOffsetLocationsArray = (int *)m_mem.alloc(sizeof(int)*numberFunctions); //array to store offsets of where the offset of kernel is stored in isa header

        genxBinariesSize = 0;
    }

    void setMagicNumber ( unsigned int  v ) { m_header.magic_number  = v; }
    void setMajorVersion( unsigned char v ) { m_header.major_version = v; }
    void setMinorVersion( unsigned char v ) { m_header.minor_version = v; }
    unsigned char getMajorVersion () const { return m_header.major_version; }
    unsigned char getMinorVersion () const { return m_header.minor_version; }
    unsigned int  getMagicNumber  () const { return m_header.magic_number;  }

    const common_isa_header& getIsaHeader() const { return m_header; }

    void initKernel( int kernelIndex, VISAKernelImpl * kernel );
    int finalizeCisaBinary();
    int dumpToFile(std::string binFileName);
    int dumpToStream(std::ostream *os);

    unsigned       getInstId() const { return m_instId; }
    void     incrementInstId() const { m_instId++;      }

    void *operator new(size_t sz, vISA::Mem_Manager& m){ return m.alloc(sz); }

    void isaDumpVerify(std::list<VISAKernelImpl *>, Options *options);
    void writeIsaAsmFile(std::string filename, std::string isaasmStr) const;

    unsigned long getHeaderSize(){ return m_header_size; }
    unsigned long getKernelVisaBinarySize(int i){ return m_header.kernels[i].size; }
    unsigned long getFunctionVisaBinarySize(int i){ return m_header.functions[i].size; }
    unsigned short getNumberKernels(){ return m_header.num_kernels; }
    unsigned short getNumberFunctions(){ return m_header.num_functions; }

    char * getVisaHeaderBuffer(){ return m_header_buffer; }
    char * getKernelVisaBinaryBuffer(int i){ return m_header.kernels[i].cisa_binary_buffer; }
    char * getFunctionVisaBinaryBuffer(int i){ return m_header.functions[i].cisa_binary_buffer; }

    std::string getFilename() const { return m_filename; }
    void setKernelVisaGenxBinaryBuffer(int i, void * buffer){ m_header.kernels[i].genx_binary_buffer = (char * )buffer; }
    void setKernelVisaGenxBinarySize(int i, unsigned short size){ m_header.kernels[i].binary_size = size; }

    void setFunctionsVisaGenxBinaryBuffer(int i, void * buffer){ m_header.functions[i].genx_binary_buffer = (char * )buffer; }
    void setFunctionsVisaGenxBinarySize(int i, unsigned short size){ m_header.functions[i].binary_size = size; }

    void patchKernel(int index, unsigned int genxBufferSize, void * buffer, int platform);
    void patchFunction(int index, unsigned genxBufferSize);

    Options *getOptions(){ return m_options; }

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
    unsigned long m_header_size;
    unsigned long m_total_size;
    unsigned long m_bytes_written_cisa_buffer;
    char * m_header_buffer;
    int m_upper_bound_kernels;
    int m_upper_bound_functions;

    std::string m_filename;
    mutable unsigned m_instId;
    Options *m_options;
};

}
#endif
