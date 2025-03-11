/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/debug/Debug.hpp"

#include "common/debug/TeeOutputStream.hpp"

#include "AdaptorCommon/customApi.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/SourceMgr.h"
#include "common/LLVMWarningsPop.hpp"

#if defined( _WIN32 ) || defined( _WIN64 )
#   include <io.h>
#   include <windows.h>
// Windows.h defines MemoryFence as _mm_mfence, but this conflicts with llvm::sys::MemoryFence
#undef MemoryFence
#endif
#include <sstream>
#include <string>
#include <exception>
#include <stdexcept>
#include <mutex>
#include "Probe/Assertion.h"

using namespace llvm;

#if defined _WIN32 || _WIN64
//#if defined( _DEBUG )
#include "psapi.h"

#ifdef IGC_METRICS__PROTOBUF_ATTACHED
#include <google/protobuf/message_lite.h>
#endif

int CatchAssert( int reportType, char *userMessage, int *retVal )
{
    if(IsDebuggerPresent())
    {
        *retVal = 1; // Break into the debugger or print a stack
    }
    else
    {
        IGC_ASSERT(0);
        *retVal = 0;
    }
    return true; // we always want to abort, return false pops up a window
}

BOOL WINAPI DllMain(
    _In_  HINSTANCE hinstDLL,
    _In_  DWORD fdwReason,
    _In_  LPVOID lpvReserved
)
{

    _CrtSetReportHook(CatchAssert);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
    _set_error_mode(_OUT_TO_STDERR);

    switch(fdwReason) {
    case DLL_PROCESS_DETACH:
        llvm_shutdown();

#ifdef IGC_METRICS__PROTOBUF_ATTACHED
        // Optional:  Delete all global objects allocated by libprotobuf.
        google::protobuf::ShutdownProtobufLibrary();
#endif
        break;

    case DLL_PROCESS_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_THREAD_ATTACH:
        break;
    }
    return TRUE;
}
#endif

namespace IGC
{
namespace Debug
{

#if defined _WIN32 || WIN64
namespace
{
    class DebugOutputStream
        : public raw_ostream
    {
    public:
        DebugOutputStream()
            : raw_ostream( true /* unbuffered */ )
        {
        }

        ~DebugOutputStream()
        {
            flush();
        }

        bool has_colors()
        {
            return false;
        }

        bool is_displayed()
        {
            return true;
        }

        virtual uint64_t current_pos() const
        {
            return 0;
        }

        size_t preferred_buffer_size() const
        {
            return 0;
        }

    private:
        virtual void write_impl(const char* Ptr, size_t Size)
        {
            std::stringstream ss;
            ss.write(Ptr, Size);
            OutputDebugStringA(ss.str().c_str());
        }
    };
} // end anonymous namespace
#endif

void Banner(raw_ostream & OS, std::string const& message)
{
#if _DEBUG
    if ( GetDebugFlag(DebugFlag::DUMPS) )
    {
        OS.changeColor( raw_ostream::YELLOW, true, false );
        OS << "-----------------------------------------\n";
        OS << message << "\n";
        OS << "-----------------------------------------\n";
        OS.resetColor();
    }
#endif
}

raw_ostream &ods()
{
#if defined(IGC_DEBUG_VARIABLES)
    if ( IGC_IS_FLAG_ENABLED(PrintToConsole) )
    {
#    if defined _WIN32 || WIN64
        {
            static DebugOutputStream dos;
            static TeeOutputStream tee( errs(), dos );
            if ( GetDebugFlag(DebugFlag::DUMP_TO_OUTS) )
            {
                return tee;
            }
            else
            {
                return dos;
            }
        }
#    else
        {
            if ( GetDebugFlag(DebugFlag::DUMP_TO_OUTS) )
            {
                return outs();
            }
            else
            {
                return nulls();
            }
        }
#    endif
    }
    else
#endif
    {
        return nulls();
    }
}


void Warning(
    const char*           pExpr,
    unsigned int          line,
    const char*           pFileName,
    std::string    const& message )
{
#if _DEBUG
    if ( GetDebugFlag(DebugFlag::DUMPS) )
    {
        ods().changeColor( raw_ostream::MAGENTA, true, false );
        ods() << pFileName << "(" << line << ") : ";
        ods() << "IGC Warning: (" << pExpr << ") " << message << "\n";
        ods().resetColor();
        ods().flush();
    }
#endif
}

namespace {
    void FatalErrorHandler(void *user_data, const char* reason, bool gen_crash_diag)
    {
        const std::string reasonStrWrapper(reason);
        (void)user_data;
        (void)reasonStrWrapper;
#if defined( _DEBUG )
#if defined( _WIN32 )
        OutputDebugStringA("LLVM Error: ");
        OutputDebugStringA(reasonStrWrapper.c_str());
        OutputDebugStringA("\n");
#endif
        fprintf( stderr, "%s", "LLVM Error: " );
        fprintf( stderr, "%s", reasonStrWrapper.c_str());
        fprintf( stderr, "%s", "\n");
        fflush( stderr );

        if ( reasonStrWrapper != "IO failure on output stream." )
        {
            exit(1);
        }
#endif
        // Throw exception to return control to an application,
        // it can try to gracefully process the error
        throw std::runtime_error("LLVM Error: " + reasonStrWrapper);
    }

    void ComputeFatalErrorHandler(const DiagnosticInfo &DI, void * /*Context*/) {
        // Skip non-error diag.
        if (DI.getSeverity() != DS_Error)
            return;
        std::string MsgStorage;
        raw_string_ostream Stream(MsgStorage);
        DiagnosticPrinterRawOStream DP(Stream);
        DI.print(DP);
        Stream.flush();

        std::string msg;
        msg += "\nerror: ";
        msg += MsgStorage;
        msg += "\nerror: backend compiler failed build.\n";
    }
}

void RegisterErrHandlers()
{
    static bool executed = false;
    if (!executed)
    {
        install_fatal_error_handler( FatalErrorHandler, nullptr );
        executed = true;
    }
}

void RegisterComputeErrHandlers(LLVMContext &C)
{
    C.setDiagnosticHandlerCallBack(ComputeFatalErrorHandler);
}

void ReleaseErrHandlers()
{
    // do nothing
}

static std::mutex stream_mutex;

void DumpLock()
{
    stream_mutex.lock();
}

void DumpUnlock()
{
    stream_mutex.unlock();
}

} // namespace Debug

int getPointerSize(llvm::Module &M) {
    return M.getDataLayout().getPointerSize();
}

} // namespace IGC
