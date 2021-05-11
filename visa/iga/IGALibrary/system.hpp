/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include <ostream>
#include <string>

namespace iga
{
    bool IsStdoutTty();
    bool IsStderrTty();
    bool IsTty(const std::ostream &os);

    void SetStdinBinary();

    // bool LookupEnvironmentVariable(const char *key, std::string &value);
    // bool IsDirectory(const char *path);
    bool DoesFileExist(const char *path);

    // For older Windows console compatibility
    void EmitRedText(std::ostream &os, const std::string &s);
    void EmitGreenText(std::ostream &os, const std::string &s);
    void EmitYellowText(std::ostream &os, const std::string &s);

    bool DebuggerAttached();
}

#endif // SYSTEM_HPP
