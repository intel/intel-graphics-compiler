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
#include "BitProcessor.hpp"
#include "../strings.hpp"

#include <stdio.h>
#include <stdarg.h>

using namespace iga;

void BitProcessor::warning(const char *patt, ...)
{
    va_list va;
    va_start(va, patt);
    warningAt(m_currInst ? m_currInst->getLoc() : m_currentPc, patt, va);
    va_end(va);
}
void BitProcessor::warningAt(const Loc& loc, const char *patt, ...)
{
    va_list va;
    va_start(va, patt);
    warningAt(loc, patt, va);
    va_end(va);
}
void BitProcessor::warningAt(const Loc& loc, const char *patt, va_list &ap)
{
    m_errorHandler.reportWarning(loc, vformatF(patt, ap));
}


void BitProcessor::error(const char *patt, ...)
{
    va_list va;
    va_start(va, patt);
    errorAt(m_currInst ? m_currInst->getLoc() : m_currentPc, patt, va);
    va_end(va);
}
void BitProcessor::errorAt(const Loc& loc, const char *patt, ...)
{
    va_list va;
    va_start(va, patt);
    errorAt(loc, patt, va);
    va_end(va);
}
void BitProcessor::errorAt(const Loc& loc, const char *patt, va_list &ap)
{
    m_errorHandler.reportError(loc, vformatF(patt, ap));
}


void BitProcessor::fatal(const char *patt, ...)
{
    va_list va;
    va_start(va, patt);
    fatalAt(m_currInst ? m_currInst->getLoc() : m_currentPc, patt, va);
    va_end(va);
}
void BitProcessor::fatalAt(const Loc& loc, const char *patt, ...)
{
    va_list va;
    va_start(va, patt);
    fatalAt(loc, patt, va);
    va_end(va);
}
void BitProcessor::fatalAt(const Loc& loc, const char *patt, va_list &ap)
{
    m_errorHandler.throwFatal(loc, vformatF(patt, ap));
}
