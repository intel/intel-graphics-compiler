/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "ocl_igc_interface/ocl_translation_output.h"

#include "cif/builtins/memory/buffer/impl/buffer_impl.h"
#include "cif/export/muiltiversion.h"
#include "cif/export/pimpl_base.h"

#include "cif/macros/enable.h"

namespace IGC{

struct TranslationErrorType
{
    using ErrorCode_t = uint64_t;
    using ErrorCodeCoder = CIF::Coder<ErrorCode_t>;

    static constexpr ErrorCode_t Success           = ErrorCodeCoder::Enc("E_SUCCESS");
    static constexpr ErrorCode_t Internal          = ErrorCodeCoder::Enc("E_INTERNAL");
    static constexpr ErrorCode_t Unused            = ErrorCodeCoder::Enc("E_UNUSED");
    static constexpr ErrorCode_t Unknown           = ErrorCodeCoder::Enc("E_UNKNOWN");
    static constexpr ErrorCode_t FailedCompilation = ErrorCodeCoder::Enc("E_FAIL_COMP");
    static constexpr ErrorCode_t InvalidInput      = ErrorCodeCoder::Enc("E_INV_INPUT");
    static constexpr ErrorCode_t UnhandledInput    = ErrorCodeCoder::Enc("E_UNH_INPUT");
};

CIF_DECLARE_INTERFACE_PIMPL(OclTranslationOutput) : CIF::PimplBase
{
    CIF_PIMPL_DECLARE_CONSTRUCTOR(CodeType::CodeType_t OutputType)
        : OutputType(OutputType), Error(TranslationErrorType::Unused)
    {
        BuildLog.CreateImpl();
        Output.CreateImpl();
        DebugData.CreateImpl();
    }

    bool Successful() const
    {
        return Error == TranslationErrorType::Success;
    }

    bool HasWarnings() const
    {
        return false;
    }

    CIF::Builtins::BufferBase * GetBuildLog(CIF::Version_t bufferVersion)
    {
        return BuildLog.GetVersion(bufferVersion);
    }

    CIF::Builtins::BufferBase * GetOutput(CIF::Version_t bufferVersion)
    {
        return Output.GetVersion(bufferVersion);
    }

    CIF::Builtins::BufferBase * GetDebugData(CIF::Version_t bufferVersion)
    {
        return DebugData.GetVersion(bufferVersion);
    }

    CodeType::CodeType_t GetOutputType() const
    {
        return OutputType;
    }

    bool SetError(TranslationErrorType::ErrorCode_t e, const char * errString = nullptr)
    {
        this->Error = e;

        if(errString == nullptr){
            return true;
        }

        auto len = strlen(errString);
        return BuildLog->PushBackRawBytes(errString, len + 1);
    }

    bool AddWarning(const std::string & warnString)
    {
        return AddWarning(warnString.c_str(), warnString.size());
    }

    bool AddWarning(const char * warn, size_t warnLength)
    {
        if((warn == nullptr) || (warnLength == 0)){
            return true;
        }

        if(BuildLog->GetSizeRaw() > 0){
            char * lastChar = reinterpret_cast<char*>(BuildLog->GetMemoryRawWriteable()) + BuildLog->GetSizeRaw() - 1;
            if(*lastChar == '\0'){
                *lastChar = '\n';
            }else{
                if(BuildLog->PushBackRawCopy('\n') == false){
                    return false;
                }
            }
        }
        if(BuildLog->PushBackRawBytes(warn, warnLength) == false){
            return false;
        }
        char * lastChar = reinterpret_cast<char*>(BuildLog->GetMemoryRawWriteable()) + BuildLog->GetSizeRaw() - 1;
        if(*lastChar != '\0'){
            return BuildLog->PushBackRawCopy('\0');
        }

        return true;
    }

    template<typename T>
    bool SetSuccessfulAndCloneOutput(const T * data, size_t size)
    {
        this->Error = TranslationErrorType::Success;
        return Output->PushBackRawBytes(data, size);
    }

    bool CloneDebugData(const char * data, size_t size)
    {
        return DebugData->PushBackRawBytes(data, size);
    }

protected:
    CIF::Multiversion<CIF::Builtins::Buffer> BuildLog;
    CIF::Multiversion<CIF::Builtins::Buffer> Output;
    CIF::Multiversion<CIF::Builtins::Buffer> DebugData;
    CodeType::CodeType_t OutputType;
    TranslationErrorType::ErrorCode_t  Error;
};

CIF_DEFINE_INTERFACE_TO_PIMPL_FORWARDING_CTOR_DTOR(OclTranslationOutput);

}

#include "cif/macros/disable.h"
