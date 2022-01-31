/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VCOPT_LIB_GENXCODEGEN_GENXOCLRUNTIMEINFO_H
#define VCOPT_LIB_GENXCODEGEN_GENXOCLRUNTIMEINFO_H

#include "vc/Support/BackendConfig.h"
#include "vc/Utils/GenX/KernelInfo.h"

#include "llvm/ADT/Optional.h"
#include "llvm/Pass.h"

#include "JitterDataStruct.h"
#include "RelocationInfo.h"

#include <cstdint>
#include <map>

#include "Probe/Assertion.h"

namespace llvm {
class Function;

class FunctionGroup;
class GenXSubtarget;

class KernelArgBuilder;

void initializeGenXOCLRuntimeInfoPass(PassRegistry &PR);

class GenXOCLRuntimeInfo : public ModulePass {
public:
  class KernelArgInfo {
    friend class KernelArgBuilder;

  public:
    enum class KindType {
      General,
      LocalSize,
      GroupCount,
      Buffer,
      SVM,
      Sampler,
      Image1D,
      Image1DArray,
      Image2D,
      Image2DArray,
      Image2DMediaBlock,
      Image3D,
      PrintBuffer,
      PrivateBase,
      ByValSVM,
      BindlessBuffer,
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
    KernelArgInfo() = default;

  public:
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
      case KindType::Image1DArray:
      case KindType::Image2D:
      case KindType::Image2DArray:
      case KindType::Image2DMediaBlock:
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

  // Symbols and reloacations are collected in zebin format. Later they are
  // translated into legacy format for patch token generation.
  using SymbolSeq = std::vector<vISA::ZESymEntry>;
  using RelocationSeq = std::vector<vISA::ZERelocEntry>;

  struct DataInfo {
    std::vector<uint8_t> Buffer;
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

  struct SectionInfo {
    DataInfo Data;
    // Symbols inside this section. Symbol offsets must be in bounds of
    // \p Data.
    SymbolSeq Symbols;
    // Relocations inside this section. "Inside" means that relocation/patching
    // happens inside this section a relocated symbol itself may refer to any
    // section, including the current one.
    RelocationSeq Relocations;

    void clear() {
      Data.clear();
      Symbols.clear();
      Relocations.clear();
    }
  };

  // Additional kernel info that are not provided by finalizer
  // but still required for runtime.
  struct KernelInfo {
    SectionInfo Func;
    // Duplicates Func.Relocations. Cannot unify it on VC side since the
    // duplication happens on Finalizer side.
    TableInfo LegacyFuncRelocations;
    std::string VISAAsm;

  private:
    std::string Name;

    bool UsesGroupId = false;
    bool UsesDPAS = false;
    int NumBarriers = 0;
    bool UsesReadWriteImages = false;
    bool SupportsDebugging = false;
    unsigned SLMSize = 0;
    unsigned ThreadPrivateMemSize = 0;
    unsigned StatelessPrivateMemSize = 0;
    bool DisableEUFusion = false;

    unsigned GRFSizeInBytes;

    using ArgInfoStorageTy = std::vector<KernelArgInfo>;
    using PrintStringStorageTy = std::vector<std::string>;
    ArgInfoStorageTy ArgInfos;
    PrintStringStorageTy PrintStrings;

  private:
    void setInstructionUsageProperties(const FunctionGroup &FG,
                                       const GenXBackendConfig &BC);
    void setMetadataProperties(vc::KernelMetadata &KM, const GenXSubtarget &ST);
    void setArgumentProperties(const Function &Kernel,
                               const vc::KernelMetadata &KM,
                               const GenXSubtarget &ST,
                               const GenXBackendConfig &BC);
    void setPrintStrings(const Module &KernelModule);

  public:
    using arg_iterator = ArgInfoStorageTy::iterator;
    using arg_const_iterator = ArgInfoStorageTy::const_iterator;
    using arg_size_type = ArgInfoStorageTy::size_type;

  public:
    // Creates kernel info for empty kernel.
    KernelInfo(const GenXSubtarget &ST);
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

    bool supportsDebugging() const { return SupportsDebugging; }

    // SIMD size is always set by igcmc to one. Preserve this here.
    unsigned getSIMDSize() const { return 1; }
    unsigned getSLMSize() const { return SLMSize; }

    // Deduced from actual function instructions.
    unsigned getTPMSize() const { return ThreadPrivateMemSize; }
    unsigned getStatelessPrivMemSize() const { return StatelessPrivateMemSize; }

    unsigned getGRFSizeInBytes() const { return GRFSizeInBytes; }

    // Deduced from actual function instructions.
    bool usesDPAS() const { return UsesDPAS; }
    // igcmc always sets this to zero. Preserve this here.
    unsigned getNumThreads() const { return 0; }

    int getNumBarriers() const { return NumBarriers; }
    bool usesReadWriteImages() const { return UsesReadWriteImages; }
    bool requireDisableEUFusion() const { return DisableEUFusion; }

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
  };

  using GTPinInfo = std::vector<char>;

  class CompiledKernel {
    KernelInfo CompilerInfo;
    FINALIZER_INFO JitterInfo;
    GTPinInfo GtpinInfo;
    std::vector<char> DebugInfo;

  public:
    CompiledKernel(KernelInfo &&KI, const FINALIZER_INFO &JI,
                   const GTPinInfo &GI, std::vector<char> DebugInfo);

    const KernelInfo &getKernelInfo() const { return CompilerInfo; }
    const FINALIZER_INFO &getJitterInfo() const { return JitterInfo; }
    const GTPinInfo &getGTPinInfo() const { return GtpinInfo; }
    const std::vector<uint8_t> &getGenBinary() const {
      return CompilerInfo.Func.Data.Buffer;
    }
    const std::vector<char> &getDebugInfo() const { return DebugInfo; }
  };

  struct ModuleInfoT {
    SectionInfo Constant;
    SectionInfo Global;
    // Real global string variables that are used in printf.
    // By design this can be filled only for zebin flow for now.
    // It should be possible to put all string variables into this section.
    // Though it would require merging \p Constant and \p ConstString for
    // oclbin on patch token generation side. So for now it isn't done this
    // way.
    SectionInfo ConstString;

    void clear() {
      Constant.clear();
      Global.clear();
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
  void print(raw_ostream &OS, const Module *M) const override;

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
