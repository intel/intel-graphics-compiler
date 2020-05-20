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

#include "FunctionGroup.h"
#include "JitterDataStruct.h"
#include "vc/GenXOpts/Utils/KernelInfo.h"
#include "llvm/ADT/Optional.h"
#include "llvm/Pass.h"
#include <cassert>
#include <map>

namespace llvm {
class Function;
class GenXSubtarget;

void initializeGenXOCLRuntimeInfoPass(PassRegistry &PR);

// This is an immutable pass to allow it creation once in the beginning of
// pipeline since creating it before actual place of need (cisa builder)
// will invalidate every other analyses required by builder.
class GenXOCLRuntimeInfo : public ImmutablePass {
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
      PrivateBase
    };

    enum class AccessKindType { None, ReadOnly, WriteOnly, ReadWrite };

  private:
    unsigned Index;
    KindType Kind;
    AccessKindType AccessKind;
    unsigned Offset;
    unsigned SizeInBytes;
    unsigned BTI;

  private:
    void translateArgDesc(genx::KernelMetadata &KM);

  public:
    KernelArgInfo(const Argument &Arg, genx::KernelMetadata &KM,
                  const DataLayout &DL);

    unsigned getIndex() const { return Index; }
    KindType getKind() const { return Kind; }
    AccessKindType getAccessKind() const { return AccessKind; }
    unsigned getOffset() const { return Offset; }
    unsigned getSizeInBytes() const { return SizeInBytes; }
    unsigned getBTI() const { return BTI; }

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
  };

  struct TableInfo {
    void *Buffer = nullptr;
    unsigned Size = 0;
    unsigned Entries = 0;
  };

  // Additional kernel info that are not provided by finalizer
  // but still required for runtime.
  struct KernelInfo {
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
    void setInstructionUsageProperties(FunctionGroup &FG,
                                       const GenXSubtarget &ST);
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
    KernelInfo(FunctionGroup &FG, const GenXSubtarget &ST);

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


  class CompiledKernel {
    KernelInfo CompilerInfo;
    FINALIZER_INFO JitterInfo;
    std::string GenBinary;

  public:
    CompiledKernel(KernelInfo &&KI, const FINALIZER_INFO &JI,
                   ArrayRef<char> GenBin);

    const KernelInfo &getKernelInfo() const { return CompilerInfo; }
    const FINALIZER_INFO &getJitterInfo() const { return JitterInfo; }
    const std::string &getGenBinary() const { return GenBinary; }
  };

public:
  using KernelStorageTy = std::vector<CompiledKernel>;

  using kernel_iterator = KernelStorageTy::iterator;
  using kernel_const_iterator = KernelStorageTy::const_iterator;
  using kernel_size_type = KernelStorageTy::size_type;

private:
  KernelStorageTy Kernels;

public:
  static char ID;

  GenXOCLRuntimeInfo() : ImmutablePass(ID) {
    initializeGenXOCLRuntimeInfoPass(*PassRegistry::getPassRegistry());
  }

  // Save kernel info and jit info for given function in this pass.
  void saveCompiledKernel(CompiledKernel &&KD) {
    Kernels.push_back(std::move(KD));
  }

  // Move compiled kernels out of this pass.
  KernelStorageTy stealCompiledKernels() { return std::move(Kernels); }

  // Kernel descriptor accessors.
  kernel_iterator kernel_begin() { return Kernels.begin(); }
  kernel_iterator kernel_end() { return Kernels.end(); }
  kernel_const_iterator kernel_begin() const { return Kernels.begin(); }
  kernel_const_iterator kernel_end() const { return Kernels.end(); }
  iterator_range<kernel_iterator> kernels() {
    return {kernel_begin(), kernel_end()};
  }
  iterator_range<kernel_const_iterator> kernels() const {
    return {kernel_begin(), kernel_end()};
  }
  kernel_size_type kernel_size() const { return Kernels.size(); }
  bool kernel_empty() const { return Kernels.empty(); }
};

ModulePass *createGenXOCLInfoExtractorPass(
    std::vector<GenXOCLRuntimeInfo::CompiledKernel> &Dest);
} // namespace llvm

#endif
