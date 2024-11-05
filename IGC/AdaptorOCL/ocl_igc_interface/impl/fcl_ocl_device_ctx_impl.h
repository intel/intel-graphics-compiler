/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "ocl_igc_interface/fcl_ocl_device_ctx.h"
#include "ocl_igc_interface/impl/platform_impl.h"

#include <cinttypes>
#include <mutex>
#include <signal.h>
#include <setjmp.h>

#include "cif/common/cif.h"
#include "cif/export/cif_main_impl.h"
#include "cif/export/interface_creator.h"
#include "cif/export/muiltiversion.h"
#include "cif/export/pimpl_base.h"

#include "cif/macros/enable.h"
#include "OCLAPI/oclapi.h"

namespace IGC
{

CIF_DECLARE_INTERFACE_PIMPL(FclOclDeviceCtx) : CIF::PimplBase
{
    CIF_PIMPL_DECLARE_CONSTRUCTOR(CIF::Version_t version, CIF::ICIF *parentInterface)
    {
        platform.CreateImpl();
    }

    OCL_API_CALL FclOclTranslationCtxBase * CreateTranslationCtx(CIF::Version_t version, CodeType::CodeType_t inType, CodeType::CodeType_t outType);
    OCL_API_CALL FclOclTranslationCtxBase * CreateTranslationCtx(CIF::Version_t version, CodeType::CodeType_t inType, CodeType::CodeType_t outType, CIF::Builtins::BufferSimple* err);

    OCL_API_CALL PlatformBase * GetPlatformHandle(CIF::Version_t version)
    {
        return platform.GetVersion(version);
    }

    OCL_API_CALL CIF_PIMPL(Platform)* GetPlatformImpl()
    {
        return this->platform.GetImpl();
    }

    struct OCL_API_CALL MiscOptions
    {
        MiscOptions()
        {
            this->Clear();
        }

        void Clear()
        {
            OclApiVersion = 120;
        }

        // oclApiVersion format : version = major_revision*100 + minor_revision*10 +  sub_revision
        //                                    e.g. OCL2.1 = 210
        uint32_t OclApiVersion;
    } MiscOptions;

protected:
    CIF::Multiversion<Platform> platform;
};

CIF_DEFINE_INTERFACE_TO_PIMPL_FORWARDING_CTOR_DTOR(FclOclDeviceCtx);

}

#include "cif/macros/disable.h"

#if defined(NDEBUG)
#if defined(WIN32)
OCL_API_CALL int ex_filter(unsigned int code, struct _EXCEPTION_POINTERS* ep);

#define EX_GUARD_BEGIN                                                        \
__try                                                                         \
{                                                                             \

#define EX_GUARD_END                                                          \
}                                                                             \
__except (ex_filter(GetExceptionCode(), GetExceptionInformation()))           \
{                                                                             \
    return nullptr;                                                            \
}                                                                             \

#else
OCL_API_CALL void signalHandler(int sig, siginfo_t* info, void* ucontext);

#include "igc_signal_guard.h"

#define EX_GUARD_BEGIN                                                         \
  do {                                                                         \
    SET_SIG_HANDLER(SIGABRT)                                                   \
    SET_SIG_HANDLER(SIGFPE)                                                    \
    SET_SIG_HANDLER(SIGILL)                                                    \
    SET_SIG_HANDLER(SIGINT)                                                    \
    SET_SIG_HANDLER(SIGSEGV)                                                   \
    SET_SIG_HANDLER(SIGTERM)                                                   \
    int sig = setjmp(sig_jmp_buf);                                             \
    if (sig == 0) {

#define EX_GUARD_END                                                           \
    } else {                                                                   \
      return nullptr;                                                          \
    }                                                                          \
    REMOVE_SIG_HANDLER(SIGABRT)                                                \
    REMOVE_SIG_HANDLER(SIGFPE)                                                 \
    REMOVE_SIG_HANDLER(SIGILL)                                                 \
    REMOVE_SIG_HANDLER(SIGINT)                                                 \
    REMOVE_SIG_HANDLER(SIGSEGV)                                                \
    REMOVE_SIG_HANDLER(SIGTERM)                                                \
  } while (0);

#endif
#else
#define EX_GUARD_BEGIN
#define EX_GUARD_END
#endif
