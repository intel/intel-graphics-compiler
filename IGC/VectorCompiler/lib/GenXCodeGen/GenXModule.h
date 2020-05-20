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
//
/// GenXModule
/// ----------
///
/// GenXModule is a module pass whose purpose is to store information
/// about the module being written, such as the built kernels and functions.
///
/// A vISA kernel or function can call a *subroutine*, which can
/// then call further subroutines. All called subroutines are considered part of
/// the kernel or function, which means that a subroutine used by two different
/// kernels needs to have a copy in each. The two copies may be treated differently
/// by the backend passes, so there does actually need to be two copies of the
/// subroutine in the LLVM IR in the backend, one called by each kernel.
/// 
/// The GenXModule pass performs any necessary copying of subroutines, and
/// populates FunctionGroupAnalysis such that each kernel and its subroutines
/// make one FunctionGroup.
/// 
/// Subsequent passes are mostly FunctionGroupPasses, so they process one
/// FunctionGroup at a time.
///
/// GenXModule is also an analysis, preserved through subsequent passes to
/// GenXVisaWriter at the end, that is used to store each written vISA kernel.
///
/// **IR restriction**: After this pass, the lead function in a FunctionGroup is
/// a kernel (or function in the vISA sense), and other functions in the same
/// FunctionGroup are its subroutines.  A (non-intrinsic) call must be to a
/// function in the same FunctionGroup, and not the lead function.
/// 
//===----------------------------------------------------------------------===//
#ifndef GENXMODULE_H
#define GENXMODULE_H

#include "GenX.h"
#include "GenXBaling.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"

#include <inc/common/sku_wa.h>

#include <map>
#include <string>
#include <vector>

class VISABuilder;
class VISAKernel;

namespace llvm {
  class raw_pwrite_stream;
  class GenXSubtarget;

  namespace genx {

    // Stream : a class for streaming byte data, and then writing out to a
    // formatted_output_stream.
    class Stream {
      std::vector<unsigned char> V;
    public:
      void push_back(const void *Data, unsigned Size) {
        unsigned Pos = V.size();
        V.resize(Pos + Size);
        std::copy_n((const unsigned char *)Data, Size, V.begin() + Pos);
      }
      template<typename T> void push_back(T Val) { push_back(&Val, sizeof(Val)); }
      unsigned size() { return V.size(); }
      void write(raw_pwrite_stream &Out);
      void setData(unsigned Offset, const void *Data, unsigned Size) {
        assert(Offset + Size <= size());
        std::copy_n((const unsigned char *)Data, Size, V.begin() + Offset);
      }
    };

    // FuncWriter : a class to write the output for a GenX kernel or function
    class FuncWriter {
    public:
      FuncWriter() {}
      virtual ~FuncWriter() {}
      // isKernel : true if the Func is a kernel
      virtual bool isKernel() = 0;
      // setOffset : set the offset field in the header
      // For a kernel, it also sets the input_offset field in the header
      virtual void setOffset(uint32_t O) = 0;
      // get header/body size
      virtual unsigned getHeaderSize() = 0;
      virtual unsigned getBodySize() = 0;
      // write header/body
      virtual void writeHeader(raw_pwrite_stream &Out) = 0;
      virtual void writeBody(raw_pwrite_stream &Out) = 0;
    };

  } // end namespace genx


  //--------------------------------------------------------------------
  // GenXModule pass. Stores the information from various parts of the
  // GenX writing process
  class GenXModule : public ModulePass {
    typedef std::vector<genx::FuncWriter *> FuncWriters_t;
    FuncWriters_t FuncWriters;
    const GenXSubtarget *ST;
    LLVMContext *Ctx = nullptr;
    WA_TABLE *WaTable = nullptr;

    void collectFinalizerArgs(std::vector<const char*> &Owner) const;
    void clearFinalizerArgs(std::vector<const char*>& Owner) const;

    VISABuilder *CisaBuilder = nullptr;
    std::vector<const char*> CISA_Args;
    void InitCISABuilder();

    VISABuilder *VISAAsmTextReader = nullptr;
    std::vector<const char*> VISA_Args;
    void InitVISAAsmReader();

    bool InlineAsm = false;
    bool CheckForInlineAsm(Module &M) const;

    std::map<const Function *, VISAKernel *> VisaKernelMap;

  public:
    static char ID;
    explicit GenXModule() : ModulePass(ID) {}
    ~GenXModule() {
      clearFinalizerArgs(VISA_Args);
      clearFinalizerArgs(CISA_Args);
      for (unsigned i = 0; i != FuncWriters.size(); i++)
        delete FuncWriters[i];
    }
    virtual StringRef getPassName() const { return "GenX module"; }
    void getAnalysisUsage(AnalysisUsage &AU) const;
    bool runOnModule(Module &M);
    const GenXSubtarget *getSubtarget() { return ST; }
    // iterator for FuncWriters list
    typedef FuncWriters_t::iterator iterator;
    iterator begin() { return FuncWriters.begin(); }
    iterator end() { return FuncWriters.end(); }
    void push_back(genx::FuncWriter *VF) { FuncWriters.push_back(VF); }
    bool HasInlineAsm() const { return InlineAsm; }
    VISABuilder *GetCisaBuilder();
    VISABuilder *GetVISAAsmReader();
    void DestroyCISABuilder();
    void DestroyVISAAsmReader();
    LLVMContext &getContext();

    // Save and retrieve VISAKernels for given function.
    void saveVisaKernel(const Function *F, VISAKernel *Kernel) {
      assert(VisaKernelMap.count(F) == 0 && "Attempt to save kernel twice");
      VisaKernelMap[F] = Kernel;
    }
    // Valid only on GenXFinalizer stage until visa builder destructors called.
    VISAKernel *getVISAKernel(const Function *F) const {
      return VisaKernelMap.at(F);
    }
  };

  void initializeGenXModulePass(PassRegistry &);

} // end namespace llvm
#endif // ndef GENXMODULE_H
