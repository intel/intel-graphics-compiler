/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SysUtils.hpp"
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include "Probe/Assertion.h"

using namespace std;

#ifndef S_ISDIR
#define S_ISDIR(flags) (((flags)&S_IFDIR)==S_IFDIR)
#endif

namespace IGC
{
    namespace SysUtils
    {
        //implementation details
        namespace
        {
            template <typename T>
            inline void MakeStrImpl(ostream& ss, T const& t)
            {
                ss << t;
            }

            template <typename T1, typename... T>
            inline void MakeStrImpl(ostream& ss, T1 const& t1, T const&... args)
            {
                MakeStrImpl(ss << t1, args...);
            }
        }

        //makes single string from args, using stringstream
        template <typename... T>
        string MakeStr(T const&... args)
        {
            if (sizeof...(args) == 0) // MakeStr() == ""
                return "";

            stringstream ss;
            MakeStrImpl(ss, args...); //see functions above
            return ss.str();
        }

        string GetProcessName()
        {
            string ret;

#if defined (__linux__)
            ifstream in(MakeStr("/proc/", getpid(), "/cmdline"), ios::in);
            IGC_ASSERT_MESSAGE(in.is_open(), "Can not open cmdline pseudo file for current process");

            //in cmdline arguments are separated with \0
            getline(in, ret, '\0');
            IGC_ASSERT_MESSAGE(in.good(), "Error reading from cmdline pseudo file");

#elif defined(_WIN64) || defined(_WIN32)
            ret.resize(MAX_PATH);
            DWORD size = ::GetModuleFileNameA(NULL, &ret[0], ret.size());
            IGC_ASSERT_MESSAGE(size < ret.size(), "Windows path can have MAX_PATH characters max");
            ret.resize(size);
#else
            ret = "unknownProcess";
#endif

            auto pos = ret.rfind('/');
            if (pos != ret.npos)
                ret = ret.substr(pos + 1);
            return ret;
        }

        bool CreateDir(string basedir, bool addprocessdir, bool addpid, string* full_path)
        {
            UnifyDirSeparators(basedir);

            if (addprocessdir)
            {
                if (!basedir.empty() && basedir.back() != '/')
                    basedir.push_back('/');

                basedir += GetProcessName();

                if (addpid)
                    basedir += MakeStr("_", GetProcessId());
            }

            if (basedir.empty())
            {
                if (full_path != nullptr)
                    *full_path = "";
                return true;
            }

            if (basedir.back() != '/')
                basedir.push_back('/');

            struct stat statbuf;

#ifdef _WIN32
            auto pos = basedir.find('/', (basedir.size() > 2 && basedir[1] == ':') ? 3 : 0); //on windows, ignore drive letter
#else
            auto pos = basedir.find('/', (basedir[0] == '/' ? 1 : 0)); //on linux, ignore root /
#endif

            while (pos != basedir.npos)
            {
                basedir[pos] = 0;

                if (stat(basedir.data(), &statbuf) != 0) //dir doesnt exist
                {
                    if (!CreateSingleDir(basedir.data())) //couldn't create
                        return false;
                }
                else if (!S_ISDIR(statbuf.st_mode)) //exists but isnt dir
                    return false;

                basedir[pos] = '/';
                pos = basedir.find('/', pos + 1);
            }

            if (full_path != nullptr)
                *full_path = basedir;

            return true;
        }
    }
}
