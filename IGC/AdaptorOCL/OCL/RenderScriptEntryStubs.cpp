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
#include "RenderScriptEntry.h"
#include "Compiler/CodeGenPublic.h"

namespace RenderScript
{

// This file contains stubs for RSContext's member functions. It should be used for builds that
// don't need RenderScript (OCL Windows builds, etc.). See description in the header file for
// reference.

RSOptions::RSOptions(const std::string & AllOptionsString)
        : EnablePKF(false), EnableNativeFP64Full(false), EnableNativeFP64Relaxed(false)
{
}

RSContext::RSContext(IGC::CodeGenContext* ctx, const RSOptions & Opts)
    : m_AdaptationIsDone(false), m_Module(*ctx->getModule()), m_Options(Opts)
{
}

RSContext::~RSContext(){
}

bool RSContext::getScriptInfo(llvm::raw_ostream & O) const{
    return false;
}

bool RSContext::adaptModuleToOcl(const void * JitInfo, /*OUT*/ bool & IsRs2){
    return false;
}

bool RSContext::isRSEnabled(){
    return false;
}

llvm::FunctionPass * creatRsTypesResolverPass(){
    return nullptr;
}

}