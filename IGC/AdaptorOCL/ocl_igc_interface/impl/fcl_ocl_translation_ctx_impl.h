/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "ocl_igc_interface/fcl_ocl_translation_ctx.h"

#include <map>
#include <memory>
#include <mutex>
#include <utility>

#include "cif/builtins/memory/buffer/impl/buffer_impl.h"
#include "cif/export/pimpl_base.h"
#include "cif/helpers/error.h"

#include "ocl_igc_interface/impl/fcl_ocl_device_ctx_impl.h"
#include "ocl_igc_interface/impl/ocl_translation_output_impl.h"

#include "OCLFE/igd_fcl_mcl/headers/clang_tb.h"
#include "OCLFE/igd_fcl_mcl/headers/common_clang.h"

#include "cif/macros/enable.h"
#include "OCLAPI/oclapi.h"

namespace TC {}

namespace IGC {

CIF_DECLARE_INTERFACE_PIMPL(FclOclTranslationCtx) : CIF::PimplBase {
  OCL_API_CALL CIF_PIMPL_DECLARE_CONSTRUCTOR(CIF::Version_t version, CIF_PIMPL(FclOclDeviceCtx) * globalState,
                                             CodeType::CodeType_t inType, CodeType::CodeType_t outType)
      : globalState(CIF::Sanity::ToReferenceOrAbort(globalState)), inType(inType), outType(outType) {
    this->legacyInterface = CreateLegacyInterface(inType, outType, err);
    CIF::Sanity::NotNullOrAbort(this->legacyInterface);
    this->legacyInterface->SetOclApiVersion(globalState->MiscOptions.OclApiVersion);
  }

  OCL_API_CALL CIF_PIMPL_DECLARE_CONSTRUCTOR(CIF::Version_t version, CIF_PIMPL(FclOclDeviceCtx) * globalState,
                                             CodeType::CodeType_t inType, CodeType::CodeType_t outType,
                                             CIF::Builtins::BufferSimple * err)
      : globalState(CIF::Sanity::ToReferenceOrAbort(globalState)), inType(inType), outType(outType), err(err) {
    this->legacyInterface = CreateLegacyInterface(inType, outType, err);
    if (this->legacyInterface) {
      this->legacyInterface->SetOclApiVersion(globalState->MiscOptions.OclApiVersion);
    }
  }

  OCL_API_CALL CIF_PIMPL_DECLARE_DESTRUCTOR() override {
    if (this->legacyInterface != nullptr) {
      TC::CClangTranslationBlock::Delete(this->legacyInterface);
      this->legacyInterface = nullptr;
    }
  }

  OCL_API_CALL static bool SupportsTranslation(CodeType::CodeType_t inType, CodeType::CodeType_t outType) {
    static std::pair<CodeType::CodeType_t, CodeType::CodeType_t> supportedTranslations[] = {
        // from                 // to
        {CodeType::elf, CodeType::llvmBc},  {CodeType::elf, CodeType::llvmLl},  {CodeType::elf, CodeType::spirV},
        {CodeType::oclC, CodeType::llvmBc}, {CodeType::oclC, CodeType::llvmLl}, {CodeType::oclC, CodeType::spirV},
    };
    for (const auto &st : supportedTranslations) {
      if ((inType == st.first) && (outType == st.second)) {
        return true;
      }
    }
    return false;
  }

  OCL_API_CALL OclTranslationOutputBase *TranslateCM(
      CIF::Version_t outVersion, CIF::Builtins::BufferSimple * src, CIF::Builtins::BufferSimple * options,
      CIF::Builtins::BufferSimple * internalOptions, CIF::Builtins::BufferSimple * tracingOptions,
      uint32_t tracingOptionsCount);
  OCL_API_CALL OclTranslationOutputBase *Translate(
      CIF::Version_t outVersion, CIF::Builtins::BufferSimple * src, CIF::Builtins::BufferSimple * options,
      CIF::Builtins::BufferSimple * internalOptions, CIF::Builtins::BufferSimple * tracingOptions,
      uint32_t tracingOptionsCount) {
    if ((options != nullptr) && (options->GetSizeRaw() > 0) && (strstr(options->GetMemory<char>(), "-cmc"))) {
      assert(this->outType == CodeType::spirV);
      return TranslateCM(outVersion, src, options, internalOptions, tracingOptions, tracingOptionsCount);
    }
    // Create a copy of input arguments that can be modified
    TC::STB_TranslateInputArgs inputArgs;
    if (src != nullptr) {
      inputArgs.pInput = src->GetMemoryWriteable<char>();
      inputArgs.InputSize = static_cast<uint32_t>(src->GetSizeRaw());
    }
    if (options != nullptr) {
      inputArgs.pOptions = options->GetMemory<char>();
      inputArgs.OptionsSize = static_cast<uint32_t>(options->GetSizeRaw());
    }
    if (internalOptions != nullptr) {
      inputArgs.pInternalOptions = internalOptions->GetMemory<char>();
      inputArgs.InternalOptionsSize = static_cast<uint32_t>(internalOptions->GetSizeRaw());
    }
    if (tracingOptions != nullptr) {
      inputArgs.pTracingOptions = tracingOptions->GetMemoryRawWriteable();
    }
    inputArgs.TracingOptionsCount = tracingOptionsCount;

    TC::STB_TranslateOutputArgs outputArgs;
    bool success = legacyInterface->Translate(&inputArgs, &outputArgs);

    auto outputInterface =
        CIF::RAII::UPtr(CIF::InterfaceCreator<OclTranslationOutput>::CreateInterfaceVer(outVersion, outType));
    if (outputInterface == nullptr) {
      // OOM or unsupported
      return nullptr;
    }

    bool dataCopiedSuccessfuly = true;
    if (success) {
      dataCopiedSuccessfuly &=
          outputInterface->GetImpl()->AddWarning(outputArgs.ErrorString.data(), outputArgs.ErrorString.size());
      dataCopiedSuccessfuly &=
          outputInterface->GetImpl()->CloneDebugData(outputArgs.DebugData.data(), outputArgs.DebugData.size());
      dataCopiedSuccessfuly &=
          outputInterface->GetImpl()->SetSuccessfulAndCloneOutput(outputArgs.Output.data(), outputArgs.Output.size());
    } else {
      dataCopiedSuccessfuly &=
          outputInterface->GetImpl()->SetError(TranslationErrorType::FailedCompilation, outputArgs.ErrorString.data());
    }

    if (!dataCopiedSuccessfuly)
      return nullptr;

    return outputInterface.release();
  }

protected:
  std::string FclOpts{};
  std::string FclInternalOpts{};

public:
  OCL_API_CALL void GetFclOptions(CIF::Builtins::BufferSimple * opts) {
    assert(opts && opts->GetSizeRaw() == 0 && "GetFclOptions expects empty buffer");
    opts->Resize(FclOpts.size() + 1);
    strncpy((char *)opts->GetMemoryRawWriteable(), FclOpts.c_str(), FclOpts.size() + 1);
  }
  OCL_API_CALL void GetFclInternalOptions(CIF::Builtins::BufferSimple * opts) {
    assert(opts && opts->GetSizeRaw() == 0 && "GetFclInternalOptions expects empty buffer");
    opts->Resize(FclInternalOpts.size() + 1);
    strncpy((char *)opts->GetMemoryRawWriteable(), FclInternalOpts.c_str(), FclInternalOpts.size() + 1);
  }

protected:
  OCL_API_CALL static TC::CClangTranslationBlock *CreateLegacyInterface(
      CodeType::CodeType_t inType, CodeType::CodeType_t outType, CIF::Builtins::BufferSimple * err) {
    if (SupportsTranslation(inType, outType) == false) {
      return nullptr;
    }

    TC::STB_CreateArgs createArgs;
    createArgs.TranslationCode.Type.Input = TC::TB_DATA_FORMAT_OCL_TEXT;
    if (inType == CodeType::elf) {
      createArgs.TranslationCode.Type.Input = TC::TB_DATA_FORMAT_ELF;
    }
    createArgs.TranslationCode.Type.Output = TC::TB_DATA_FORMAT_LLVM_BINARY;
    if (outType == CodeType::llvmLl) {
      createArgs.TranslationCode.Type.Output = TC::TB_DATA_FORMAT_LLVM_TEXT;
    } else if (outType == CodeType::spirV) {
      createArgs.TranslationCode.Type.Output = TC::TB_DATA_FORMAT_SPIR_V;
    }

    TC::CClangTranslationBlock *legacyInterface = nullptr;
    TC::STB_TranslateOutputArgs outputArgs;
    bool success;
    success = TC::CClangTranslationBlock::Create(&createArgs, &outputArgs, legacyInterface);
    if (!success && err != nullptr) {
      const char *pErrorMsg = outputArgs.ErrorString.data();
      uint32_t pErrorMsgSize = outputArgs.ErrorString.size();
      err->PushBackRawBytes(pErrorMsg, pErrorMsgSize);
    }
    if (legacyInterface == nullptr) {
      return nullptr;
    }

    return legacyInterface;
  }

  CIF_PIMPL(FclOclDeviceCtx) & globalState;
  CodeType::CodeType_t inType = CodeType::undefined;
  CodeType::CodeType_t outType = CodeType::undefined;
  CIF::Builtins::BufferSimple *err = nullptr;
  TC::CClangTranslationBlock *legacyInterface = nullptr;
};

CIF_DEFINE_INTERFACE_TO_PIMPL_FORWARDING_CTOR_DTOR(FclOclTranslationCtx);

} // namespace IGC

#include "cif/macros/disable.h"
