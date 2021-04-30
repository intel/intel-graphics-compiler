/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef SHADER_HASH_HPP
#define SHADER_HASH_HPP

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
