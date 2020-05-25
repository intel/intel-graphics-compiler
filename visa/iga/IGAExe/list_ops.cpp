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
#include "iga_main.hpp"

#include <sstream>


bool listOps(const Opts &opts, const std::string &opmn)
{
    bool hasError = false;
    auto ops = igax::OpSpec::enumerate(
        static_cast<igax::Platform>(opts.platform));
    std::stringstream ss;
    if (opmn.empty()) {
        // list a table of all ops
        // build header
        ss << std::setw(12) << std::left << "Mnemonic" << "  " << "Op" << "\n";
        for (const auto &op : ops) {
            ss << std::setw(12) << std::left <<
                op.menmonic() << "  " << op.name() << "\n";
        }
    } else {
        bool prefixSearch = opmn[opmn.size() - 1] == '*';
        bool foundOp = false;
        for (auto &op : ops) {
            std::string mn = op.menmonic();
            if (opmn == mn ||
                (prefixSearch &&
                    opmn.substr(0, opmn.size() - 1) == mn.substr(0, opmn.size() - 1)))
            {
                foundOp = true;
                ss << "Mnemonic: " << mn << "\n";
                ss << "Name:     " << op.name() << "\n";
                ss << op.description() << "\n\n";
            }
        }
        hasError = !foundOp;
        if (!foundOp) {
            fatalExitWithMessage("iga: %s: mnemonic not found for platform",
                opmn.c_str());
        }
    }
    writeText(opts, ss.str().c_str());
    return hasError;
}
