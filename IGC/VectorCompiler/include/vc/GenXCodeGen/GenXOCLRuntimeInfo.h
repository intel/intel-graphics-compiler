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

#ifndef VCOPT_LIB_GENXCODEGEN_GENXOCLRUNTIMEINFO_H
#define VCOPT_LIB_GENXCODEGEN_GENXOCLRUNTIMEINFO_H

#include "vc/GenXOpts/Utils/KernelInfo.h"
#include "vc/Support/BackendConfig.h"

#include "llvm/ADT/Optional.h"
#include "llvm/Pass.h"

#include "JitterDataStruct.h"
#include "RelocationInfo.h"

#include <map>

#include "Probe/Assertion.h"

namespace llvm {
class Function;

class FunctionGroup;
class GenXSubtarget;

void initializeGenXOCLRuntimeInfoPass(PassRegistry &PR);

class GenXOCLRuntimeInfo : public ModulePass {
public:
  class KernelArgInfo {
  public:
    enum class KindType {
      General,
      LocalSize,
      GroupCount,
      Buffer,
      SVM,
      Sampler,
      Image1D,
      Image2D,
      Image3D,
      PrintBuffer,
      PrivateBase,
      ByValSVM
    };

    enum class AccessKindType { None, ReadOnly, WriteOnly, ReadWrite };

  private:
    unsigned Index;
    KindType Kind;
    AccessKindType AccessKind;
    unsigned Offset;
    unsigned OffsetInArg; // Implicit arguments may be mapped to some part of an
                          // explicit argument. This field shows offset in the
                          // explicit arg.
    unsigned SizeInBytes;
    unsigned BTI;

  private:
    void translateArgDesc(genx::KernelMetadata &KM, unsigned ArgNo);

  public:
    KernelArgInfo(const Argument &Arg, genx::KernelMetadata &KM,
                  const DataLayout &DL);

    unsigned getIndex() const { return Index; }
    KindType getKind() const { return Kind; }
    AccessKindType getAccessKind() const { return AccessKind; }
    unsigned getOffset() const { return Offset; }
    unsigned getSizeInBytes() const { return SizeInBytes; }
    unsigned getBTI() const { return BTI; }
    unsigned getOffsetInArg() const { return OffsetInArg; }

    bool isImage() const {
      switch (Kind) {
      case KindType::Image1D:
      case KindType::Image2D:
      case KindType::Image3D:
        return true;
      default:
        return false;
      }
    }

    bool isResource() const {
      if (Kind == KindType::Buffer || Kind == KindType::SVM)
        return true;
      return isImage();
    }

    bool isWritable() const {
      IGC_ASSERT_MESSAGE(isResource(),
                         "Only resources can have writable property");
      return AccessKind != AccessKindType::ReadOnly;
    }
  };

  struct TableInfo {
    void *Buffer = nullptr;
    unsigned Size = 0;
    unsigned Entries = 0;
  };

  // This data partially duplicates KernelInfo data.
  // It exists due to OCLBinary to ZEBinary transition period.
  struct ZEBinaryInfo {
    struct SymbolsInfo {
      using ZESymEntrySeq = std::vector<vISA::ZESymEntry>;
      ZESymEntrySeq Functions;
      ZESymEntrySeq Globals;
      ZESymEntrySeq Constants;
      ZESymEntrySeq Local;
      // for now only function and local symbols are used
    };
    using ZERelocEntrySeq = std::vector<vISA::ZERelocEntry>;
    ZERelocEntrySeq Relocations;
    SymbolsInfo Symbols;
  };

  // Additional kernel info that are not provided by finalizer
  // but still required for runtime.
  struct KernelInfo {
    ZEBinaryInfo ZEBinInfo;

  private:
    std::string Name;

    bool UsesGroupId = false;


    // Jitter info contains similar field.
    // Whom should we believe?
    bool UsesBarriers = false;

    bool UsesReadWriteImages = false;

    unsigned SLMSize = 0;
    unsigned ThreadPrivateMemSize = 0;
    unsigned StatelessPrivateMemSize = 0;

    unsigned GRFSizeInBytes;

    using ArgInfoStorageTy = std::vector<KernelArgInfo>;
    using PrintStringStorageTy = std::vector<std::string>;
    ArgInfoStorageTy ArgInfos;
    PrintStringStorageTy PrintStrings;

    TableInfo ReloTable;
    TableInfo SymbolTable;

  private:
    void setInstructionUsageProperties(const FunctionGroup &FG,
                                       const GenXBackendConfig &BC);
    void setMetadataProperties(genx::KernelMetadata &KM,
                               const GenXSubtarget &ST);
    void setArgumentProperties(const Function &Kernel,
                               genx::KernelMetadata &KM);
    void setPrintStrings(const Module &KernelModule);

  public:
    using arg_iterator = ArgInfoStorageTy::iterator;
    using arg_const_iterator = ArgInfoStorageTy::const_iterator;
    using arg_size_type = ArgInfoStorageTy::size_type;

  public:
    // Creates kernel info for given function group.
    KernelInfo(const FunctionGroup &FG, const GenXSubtarget &ST,
               const GenXBackendConfig &BC);

    const std::string &getName() const { return Name; }

    // These are considered to always be true (at least in igcmc).
    // Preserve this here.
    bool usesLocalIdX() const { return true; }
    bool usesLocalIdY() const { return true; }
    bool usesLocalIdZ() const { return true; }

    // Deduced from actual function instructions.
    bool usesGroupId() const { return UsesGroupId; }

    // SIMD size is always set by igcmc to one. Preserve this here.
    unsigned getSIMDSize() const { return 1; }
    unsigned getSLMSize() const { return SLMSize; }

    // Deduced from actual function instructions.
    unsigned getTPMSize() const { return ThreadPrivateMemSize; }
    unsigned getStatelessPrivMemSize() const { return StatelessPrivateMemSize; }

    unsigned getGRFSizeInBytes() const { return GRFSizeInBytes; }


    bool usesBarriers() const { return UsesBarriers; }
    bool usesReadWriteImages() const { return UsesReadWriteImages; }

    // Arguments accessors.
    arg_iterator arg_begin() { return ArgInfos.begin(); }
    arg_iterator arg_end() { return ArgInfos.end(); }
    arg_const_iterator arg_begin() const { return ArgInfos.begin(); }
    arg_const_iterator arg_end() const { return ArgInfos.end(); }
    iterator_range<arg_iterator> args() { return {arg_begin(), arg_end()}; }
    iterator_range<arg_const_iterator> args() const {
      return {arg_begin(), arg_end()};
    }
    arg_size_type arg_size() const { return ArgInfos.size(); }
    bool arg_empty() const { return ArgInfos.empty(); }
    const PrintStringStorageTy &getPrintStrings() const { return PrintStrings; }
    TableInfo &getRelocationTable() { return ReloTable; }
    const TableInfo &getRelocationTable() const { return ReloTable; }
    TableInfo &getSymbolTable() { return SymbolTable; }
    const TableInfo &getSymbolTable() const { return SymbolTable; }
  };

  class GTPinInfo {
    std::vector<char> gtpinBuffer;
  public:
    GTPinInfo(std::vector<char>&& buf): gtpinBuffer(std::move(buf)) {}
    unsigned getGTPinBufferSize() const { return gtpinBuffer.size(); }
    const std::vector<char> &getGTPinBuffer() const { return gtpinBuffer; }
  };

  class CompiledKernel {
    KernelInfo CompilerInfo;
    FINALIZER_INFO JitterInfo;
    GTPinInfo GtpinInfo;
    std::vector<char> GenBinary;
    std::vector<char> DebugInfo;

  public:
    CompiledKernel(KernelInfo &&KI, const FINALIZER_INFO &JI,
                   const GTPinInfo &GI,
                   std::vector<char> GenBin,
                   std::vector<char> DebugInfo);

    const KernelInfo &getKernelInfo() const { return CompilerInfo; }
    const FINALIZER_INFO &getJitterInfo() const { return JitterInfo; }
    const GTPinInfo &getGTPinInfo() const { return GtpinInfo; }
    const std::vector<char> &getGenBinary() const { return GenBinary; }
    const std::vector<char> &getDebugInfo() const { return DebugInfo; }
  };

  struct DataInfoT {
    std::vector<char> Buffer;
    int Alignment = 0;
    // Runtime can allocate bigger zeroed out buffer, and fill only
    // the first part of it with the data from Buffer field. So there's no
    // need to fill Buffer with zero, one can just set AdditionalZeroedSpace,
    // and it will be additionally allocated. The size is in bytes.
    std::size_t AdditionalZeroedSpace = 0;

    void clear() {
      Buffer.clear();
      Alignment = 0;
      AdditionalZeroedSpace = 0;
    }
  };

  struct ModuleInfoT {
    DataInfoT ConstantData;
    DataInfoT GlobalData;

    void clear() {
      ConstantData.clear();
      GlobalData.clear();
    }
  };

  struct CompiledModuleT {
    using KernelStorageTy = std::vector<CompiledKernel>;
    ModuleInfoT ModuleInfo;
    KernelStorageTy Kernels;
    unsigned PointerSizeInBytes = 0;

    void clear() {
      ModuleInfo.clear();
      Kernels.clear();
      PointerSizeInBytes = 0;
    }
  };

public:
  using KernelStorageTy = CompiledModuleT::KernelStorageTy;

  using kernel_iterator = KernelStorageTy::iterator;
  using kernel_const_iterator = KernelStorageTy::const_iterator;
  using kernel_size_type = KernelStorageTy::size_type;

private:
  CompiledModuleT CompiledModule;

public:
  static char ID;

  GenXOCLRuntimeInfo() : ModulePass(ID) {
    initializeGenXOCLRuntimeInfoPass(*PassRegistry::getPassRegistry());
  }

  StringRef getPassName() const override { return "GenX OCL Runtime Info"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnModule(Module &M) override;

  void releaseMemory() override { CompiledModule.clear(); }

  // Move compiled kernels out of this pass.
  CompiledModuleT stealCompiledModule() { return std::move(CompiledModule); }

  // Kernel descriptor accessors.
  kernel_iterator kernel_begin() { return CompiledModule.Kernels.begin(); }
  kernel_iterator kernel_end() { return CompiledModule.Kernels.end(); }
  kernel_const_iterator kernel_begin() const {
    return CompiledModule.Kernels.begin();
  }
  kernel_const_iterator kernel_end() const {
    return CompiledModule.Kernels.end();
  }
  iterator_range<kernel_iterator> kernels() {
    return {kernel_begin(), kernel_end()};
  }
  iterator_range<kernel_const_iterator> kernels() const {
    return {kernel_begin(), kernel_end()};
  }
  kernel_size_type kernel_size() const { return CompiledModule.Kernels.size(); }
  bool kernel_empty() const { return CompiledModule.Kernels.empty(); }
};

ModulePass *
createGenXOCLInfoExtractorPass(GenXOCLRuntimeInfo::CompiledModuleT &Dest);
} // namespace llvm

#endif
