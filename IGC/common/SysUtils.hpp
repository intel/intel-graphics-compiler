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

#if defined _WIN32
#include <Windows.h>
#include <cfgmgr32.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace IGC
{
    namespace SysUtils
    {
        inline unsigned int GetProcessId()
        {
#if defined _WIN32
            return (unsigned int)GetCurrentProcessId();
#else
            return (unsigned int)getpid();
#endif
        }

        inline bool CreateSingleDir(std::string const& dirpath)
        {
#if defined _WIN32
            return (CreateDirectoryA(dirpath.c_str(), NULL) == TRUE);
#elif defined __GNUC__
            return (mkdir(dirpath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0);
#else
            return false;
#endif
        }

        //changes \ to /
        inline void UnifyDirSeparators(std::string& path)
        {
            for (char& c : path)
                if (c == '\\')
                    c = '/';
        }

        //Returns process name, or throws exception if cannot be determinated
        std::string GetProcessName();

        //Tries to create directory 'basedir', all necessary directories will be created, like mkdir -p
        // if append_processdir == true, path is extedned with folder specific for running process
        // if both append_processdir and add_pid == true, appended folder's name will also contain process' pid
        // total 3 possibilities (assuming that running process is named 'example' with pid 0000):
        //  - CreateDir(dir, false, false) => will create just dir
        //  - CreateDir(dir, true, false) => will create 'dir/example'
        //  - CreateDir(dir, true, true) => will create 'dir/example_0000'
        //
        //If full_path != nullptr and function success, path to directory is copied to pointed string,
        // if function fails, full_path remains unaffected
        bool CreateDir(std::string basedir, bool append_processdir = false, bool add_pid = false, std::string* full_path = nullptr);
    }
}