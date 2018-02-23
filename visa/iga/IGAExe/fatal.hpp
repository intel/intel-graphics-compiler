#ifndef FATAL_HPP
#define FATAL_HPP

#ifdef WIN32
#include <Windows.h>
#include <malloc.h>
#else
#include <alloca.h>
#endif
#ifdef _MSC_VER
#define VSCPRINTF(PAT,VA) \
    _vscprintf(PAT,VA)
#define VSNPRINTF(B,BLEN,...) \
    vsprintf_s(B, BLEN, __VA_ARGS__)
#else
#define VSCPRINTF(PAT,VA) \
    vsnprintf(NULL, 0, PAT, VA)
#define VSNPRINTF(B,BLEN,...) \
    vsprintf(B,__VA_ARGS__)
#endif

#include <cstdarg>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#if INTPTR_MAX == INT64_MAX
#define IGA_EXE "iga64"
#elif INTPTR_MAX == INT32_MAX
#define IGA_EXE "iga32"
#else
#define IGA_EXE "iga"
#endif

#ifdef _WIN32
#undef  NORETURN_DECLSPEC
#define NORETURN_DECLSPEC __declspec(noreturn)
#undef  NORETURN_ATTRIBUTE
#define NORETURN_ATTRIBUTE
#else
#undef  NORETURN_DECLSPEC
#define NORETURN_DECLSPEC
#undef  NORETURN_ATTRIBUTE
#define NORETURN_ATTRIBUTE __attribute__((noreturn))
#endif


NORETURN_DECLSPEC
static void NORETURN_ATTRIBUTE fatalExit()
{
#ifdef _WIN32
    if (IsDebuggerPresent()) {
        DebugBreak();
    }
#endif
    exit(EXIT_FAILURE);
}


static void fatalMessage(const char *str)
{
    std::string s = IGA_EXE;
    s += ": ";
    s += str;
    s += '\n';

    fputs(s.c_str(), stderr);
#ifdef _WIN32
    OutputDebugStringA(s.c_str());
#endif
}


NORETURN_DECLSPEC
static void NORETURN_ATTRIBUTE fatalExitWithMessage(const char *pat, ...)
{
    va_list ap;
    va_start(ap, pat);
    size_t ebuflen = VSCPRINTF(pat, ap) + 1;
    va_end(ap);
    char *buf = (char *)alloca(ebuflen);
    va_start(ap, pat);
    VSNPRINTF(buf, ebuflen, pat, ap);
    va_end(ap);
    buf[ebuflen - 1] = 0;

    fatalMessage(buf);
    fatalExit();
}

#endif