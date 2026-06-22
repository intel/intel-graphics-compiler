/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

class ShaderHashBase {
public:
  ShaderHashBase() : asmHash(0), perShaderPsoHash(0), stateHash(0), rtlHash(0) {}
  QWORD getAsmHash() const { return asmHash; }
  QWORD getPerShaderPsoHash() const { return perShaderPsoHash; }

  QWORD asmHash;
  QWORD perShaderPsoHash;
  QWORD stateHash;
  QWORD rtlHash;
};

class ShaderHash3D : public ShaderHashBase {
public:
  ShaderHash3D() : ShaderHashBase(), dcHash(0), ltoHash(0) {}
  ShaderHash3D(const ShaderHashBase &base) : ShaderHashBase(base), dcHash(0), ltoHash(0) {}

  QWORD dcHash;
  QWORD ltoHash;
};

class ShaderHash : public ShaderHash3D {
public:
  ShaderHash() : ShaderHash3D(), nosHash(0), psoHash(0) {}
  ShaderHash(const ShaderHash3D &spec) : ShaderHash3D(spec), nosHash(0), psoHash(0) {}
  ShaderHash(const ShaderHashBase &base) : ShaderHash3D(base), nosHash(0), psoHash(0) {}
  QWORD getNosHash() const { return nosHash; }
  QWORD getPsoHash() const { return psoHash; }

  bool is_set() const {
    return ((asmHash | nosHash | psoHash | perShaderPsoHash | rtlHash | dcHash | ltoHash | stateHash) != 0);
  }

  QWORD nosHash;
  QWORD psoHash;
};
