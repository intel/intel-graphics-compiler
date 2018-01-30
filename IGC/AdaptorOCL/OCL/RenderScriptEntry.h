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
#pragma once

#include <string>

namespace llvm
{

class Module;
class raw_ostream;
class FunctionPass;

}

namespace IGC
{
    class CodeGenContext;
}

namespace RenderScript
{

inline bool ContainsOption(const std::string & allOptions, const std::string & option)
{
    if((allOptions.size() == 0)||(option.size() == 0))
    {
        return false;
    }

    size_t pos = 0;
    while(pos < allOptions.size())
    {
        pos = allOptions.find(option, pos);
        if(std::string::npos == pos)
            return false; // not found

        if((0 != pos) && (' ' != allOptions[pos - 1]))
        {
            pos = pos + option.size(); // found only a substring - continue searching
            continue;
        }

        size_t delimPos = pos + option.size();
        if(delimPos >= allOptions.size())
            return true; // found as the last option in the sting

        char delim = allOptions[delimPos];

        bool isSubstring = (' ' != delim)&&('\0' != delim);

        if(isSubstring)
        {
            pos = pos + option.size();
            continue; // found only a substring - continue searching
        }
        else
        {
            return true; // option found (not a substring)
        }
    }

    return false;
}

struct RSOptions
{
    RSOptions(const std::string & AllOptionsString = "");
    bool EnablePKF;
    bool EnableNativeFP64Full;
    bool EnableNativeFP64Relaxed;
};

/// This class gives the OCL's part of IGC the interface to the RS-specific stuff.
/// For builds that don't require RS it will link against stubs (RenderScriptEntryStubs.cpp).
/// For builds that require RS (mainly on Android) it will link against the real RS implementation. 
class RSContext
{
public:
    RSContext(IGC::CodeGenContext* ctx, const RSOptions & Options);
    ~RSContext();
    
    /// Gets the RS script info need by the RS_UMD at the script creation stage
    bool getScriptInfo(llvm::raw_ostream & O) const;

    /// Adapts the RS module to a form that can be handled by the OCL passes
    /// Note : since we did not want the unneeded RS->OCL->IGC metadata translations
    ///        this method will produce the metadata in its final form - i.e. in IGC form
    bool adaptModuleToOcl(const void * JitInfo, /*OUT*/ bool & IsRs2);

    /// Returns true if RS is enabled for current IGC binary
    static bool isRSEnabled();

protected:
    RSContext(const RSContext & Rhs);
    RSContext & operator=(const RSContext & Rhs);

    bool m_AdaptationIsDone; // is the module already in the OCL form ?
    llvm::Module & m_Module; // the module to be worked on
    IGC::CodeGenContext* m_pCodeGenContext;
    RSOptions m_Options;     // compiler options
};

/// Creates the RsTypesSimplifier function pass if RS is enabled, returns nullptr otherwise
llvm::FunctionPass * creatRsTypesResolverPass();

}
