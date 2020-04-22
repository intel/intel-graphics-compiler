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

#ifndef SHADER_HASH_HPP
#define SHADER_HASH_HPP

#pragma once

class ShaderHash
{
public:
    ShaderHash()
        : asmHash(0)
        , nosHash(0)
        , psoHash(0)
        , perShaderPsoHash(0)
        , rtlHash(0)
        , dcHash(0)
        , ltoHash(0)
        , stateHash(0)
    {}
    QWORD getAsmHash() const { return asmHash; }
    QWORD getNosHash() const { return nosHash; }
    QWORD getPsoHash() const { return psoHash; }
    QWORD getPerShaderPsoHash() const { return perShaderPsoHash; }

    QWORD asmHash;
    QWORD nosHash;
    QWORD psoHash;
    QWORD perShaderPsoHash;
    QWORD rtlHash;
    QWORD dcHash;
    QWORD ltoHash;
    QWORD stateHash;
};

#endif //SHADER_HASH_HPP
