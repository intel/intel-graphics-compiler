/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// Currently VectorCompiler uses a special type of LLVM-SPIRV-Translator
// in a form of shared library called "SPIRVDLL"
// It is expected to move from this solution in favour of original Khronos
// LLVMSPIRVLib library.
// This file was created for the purpose of smooth transit between these two
// library versions.
//
//===----------------------------------------------------------------------===//

#include <sstream>

#include "SPIRVWrapper.h"

#include "Probe/Assertion.h"
#include "vc/Support/Status.h"
#include "llvm/IR/Verifier.h"

#ifdef IGC_VECTOR_USE_KHRONOS_SPIRV_TRANSLATOR
#include "LLVMSPIRVLib.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#else // IGC_VECTOR_USE_KHRONOS_SPIRV_TRANSLATOR
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Process.h"
#if defined(__linux__)
#include <dlfcn.h>
#endif // __linux__
#if defined(_WIN32)
#include <Windows.h>
#include "inc/common/DriverStore.h"
#endif // _WIN32
#endif // IGC_VECTOR_USE_KHRONOS_SPIRV_TRANSLATOR

using namespace llvm;

namespace {

using SpirvReadVerifyType = int(
    const char *pIn, size_t InSz, const uint32_t *SpecConstIds,
    const uint64_t *SpecConstVals, unsigned SpecConstSz,
    void (*OutSaver)(const char *pOut, size_t OutSize, void *OutUserData),
    void *OutUserData, void (*ErrSaver)(const char *pErrMsg, void *ErrUserData),
    void *ErrUserData);

#ifdef IGC_VECTOR_USE_KHRONOS_SPIRV_TRANSLATOR

int spirvReadVerify(const char *pIn, size_t InSz, const uint32_t *SpecConstIds,
                    const uint64_t *SpecConstVals, unsigned SpecConstSz,
                    void (*OutSaver)(const char *pOut, size_t OutSize,
                                     void *OutUserData),
                    void *OutUserData,
                    void (*ErrSaver)(const char *pErrMsg, void *ErrUserData),
                    void *ErrUserData) {
  llvm::LLVMContext Context;
  llvm::StringRef SpirvInput = llvm::StringRef(pIn, InSz);
  std::istringstream IS(SpirvInput.str());
  std::unique_ptr<llvm::Module> M;
  {
    llvm::Module *SpirM;
    std::string ErrMsg;
#if LLVM_VERSION_MAJOR > 7
    SPIRV::TranslatorOpts Opts;
    Opts.enableAllExtensions();
    Opts.setFPContractMode(SPIRV::FPContractMode::On);
    Opts.setDesiredBIsRepresentation(SPIRV::BIsRepresentation::SPIRVFriendlyIR);
    // Add specialization constants
    for (unsigned i = 0; i < SpecConstSz; ++i)
      Opts.setSpecConst(SpecConstIds[i], SpecConstVals[i]);

    // This returns true on success...
    bool Status = llvm::readSpirv(Context, Opts, IS, SpirM, ErrMsg);
#else
    if (SpecConstSz != 0) {
      std::ostringstream OSS;
      OSS << "spirv_read_verify: Specialization constants are not supported in "
             "this translator version (700) "
          << ErrMsg;
      ErrSaver(OSS.str().c_str(), ErrUserData);
      return -1;
    }
    // This returns true on success...
    bool Status = llvm::readSpirv(Context, IS, SpirM, ErrMsg);
#endif
    if (!Status) {
      std::ostringstream OSS;
      OSS << "spirv_read_verify: readSpirv failed: " << ErrMsg;
      ErrSaver(OSS.str().c_str(), ErrUserData);
      return -1;
    }
    Status = llvm::verifyModule(*SpirM);
    if (Status) {
      ErrSaver("spirv_read_verify: verify Module failed", ErrUserData);
      return -1;
    }
    M.reset(SpirM);
  }

  llvm::SmallVector<char, 16> CloneBuffer;
  llvm::raw_svector_ostream CloneOstream(CloneBuffer);
  WriteBitcodeToFile(*M, CloneOstream);
  IGC_ASSERT(CloneBuffer.size() > 0);

  OutSaver(CloneBuffer.data(), CloneBuffer.size(), OutUserData);
  return 0;
}

Expected<SpirvReadVerifyType *> getSpirvReadVerifyFunction() {
  return &spirvReadVerify;
}

#else // IGC_VECTOR_USE_KHRONOS_SPIRV_TRANSLATOR

// Get appropriate path to SPIRV DLL library for subsequent loading.
std::string findSpirvDLL() {
#if defined(_WIN64)
  // TODO: rename to SPIRVDLL64.dll when binary components are fixed.
  static constexpr char *SpirvLibName = "SPIRVDLL.dll";
#elif defined(_WIN32)
  static constexpr char *SpirvLibName = "SPIRVDLL32.dll";
#else
  static constexpr char *SpirvLibName = "libSPIRVDLL.so";
#endif

  auto EnvSpirv = llvm::sys::Process::GetEnv("VC_SPIRVDLL_DIR");
  if (EnvSpirv) {
    SmallString<32> Path;
    llvm::sys::path::append(Path, EnvSpirv.getValue(), SpirvLibName);
    return std::string{Path.str()};
  }

// Return standard path for SPIRVDLL for current build.
// Linux: plain library name.
// Windows: library name prefixed with current module
// location (installed driver location).
#if defined(_WIN32)
  // Expand libname to full driver path on windows.
  char TmpPath[MAX_PATH] = {};
  GetDependencyPath(TmpPath, SpirvLibName);
  return TmpPath;
#else
  return SpirvLibName;
#endif
}

Expected<SpirvReadVerifyType *> getSpirvReadVerifyFunction() {
  constexpr char *SpirvReadVerifyName = "spirv_read_verify_module";

  const std::string SpirvLibPath = findSpirvDLL();
#if defined(__linux__)
  // Hack to workaround cmoc crashes during loading of SPIRV library
  static auto DeepBindHack =
      dlopen(SpirvLibPath.c_str(), RTLD_NOW | RTLD_DEEPBIND);
#endif // __linux__

  std::string ErrMsg;
  using DL = sys::DynamicLibrary;
  DL DyLib = DL::getPermanentLibrary(SpirvLibPath.c_str(), &ErrMsg);
  if (!DyLib.isValid())
    return make_error<vc::DynLoadError>(ErrMsg);

  auto *SpirvReadVerify = reinterpret_cast<SpirvReadVerifyType *>(
      DyLib.getAddressOfSymbol(SpirvReadVerifyName));
  if (!SpirvReadVerify)
    return make_error<vc::SymbolLookupError>(SpirvLibPath, SpirvReadVerifyName);
  return SpirvReadVerify;
}

#endif // IGC_VECTOR_USE_KHRONOS_SPIRV_TRANSLATOR

} // namespace

Expected<std::vector<char>>
vc::translateSPIRVToIR(ArrayRef<char> Input, ArrayRef<uint32_t> SpecConstIds,
                       ArrayRef<uint64_t> SpecConstValues) {
  IGC_ASSERT(SpecConstIds.size() == SpecConstValues.size());
  auto OutSaver = [](const char *pOut, size_t OutSize, void *OutData) {
    auto *Vec = reinterpret_cast<std::vector<char> *>(OutData);
    Vec->assign(pOut, pOut + OutSize);
  };
  auto ErrSaver = [](const char *pErrMsg, void *ErrData) {
    auto *ErrStr = reinterpret_cast<std::string *>(ErrData);
    *ErrStr = pErrMsg;
  };
  std::string ErrMsg;
  std::vector<char> Result;
  auto SpirvReadVerifyFunctionExp = getSpirvReadVerifyFunction();
  if (!SpirvReadVerifyFunctionExp)
    return SpirvReadVerifyFunctionExp.takeError();
  auto *SpirvReadVerifyFunction = SpirvReadVerifyFunctionExp.get();

  int Status = SpirvReadVerifyFunction(
      Input.data(), Input.size(), SpecConstIds.data(), SpecConstValues.data(),
      SpecConstValues.size(), OutSaver, &Result, ErrSaver, &ErrMsg);

  if (Status != 0)
    return make_error<vc::BadSpirvError>(ErrMsg);
  return {std::move(Result)};
}
