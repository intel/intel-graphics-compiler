/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "UserAddrSpaceMD.hpp"
#include "IGC/Probe/Assertion.h"

using namespace IGC;

UserAddrSpaceMD::UserAddrSpaceMD(llvm::LLVMContext *ctx) {
  this->ctx = ctx;
  user_addrspace_priv = ctx->getMDKindID("user_as_priv");
  user_addrspace_global = ctx->getMDKindID("user_as_global");
  user_addrspace_local = ctx->getMDKindID("user_as_local");
  user_addrspace_generic = ctx->getMDKindID("user_as_generic");
  user_addrspace_raystack = ctx->getMDKindID("user_as_raystack");

  dummyNode = llvm::MDNode::get(*ctx, llvm::MDString::get(*ctx, ""));
}

void UserAddrSpaceMD::Set(llvm::Instruction *inst, LSC_DOC_ADDR_SPACE type, llvm::MDNode *node) {
  if (inst) {
    llvm::MDNode *chosen_node = node != nullptr ? node : dummyNode;

    user_addrspace chosen_type = -1;

    switch (type) {
    case LSC_DOC_ADDR_SPACE::PRIVATE:
      chosen_type = user_addrspace_priv;
      break;
    case LSC_DOC_ADDR_SPACE::GLOBAL:
      chosen_type = user_addrspace_global;
      break;
    case LSC_DOC_ADDR_SPACE::LOCAL:
      chosen_type = user_addrspace_local;
      break;
    case LSC_DOC_ADDR_SPACE::GENERIC:
      chosen_type = user_addrspace_generic;
      break;
    case LSC_DOC_ADDR_SPACE::RAYSTACK:
      chosen_type = user_addrspace_raystack;
      break;
    default:
      IGC_ASSERT_MESSAGE(0, "Unsupport addrspace type");
      break;
    }
    inst->setMetadata(chosen_type, chosen_node);
  }
}

LSC_DOC_ADDR_SPACE UserAddrSpaceMD::Get(llvm::Instruction *inst) {
  LSC_DOC_ADDR_SPACE addrSpace = LSC_DOC_ADDR_SPACE::INVALID;

  if (inst) {
    if ([[maybe_unused]] auto md_addrspace_org = inst->getMetadata(user_addrspace_priv)) {
      addrSpace = LSC_DOC_ADDR_SPACE::PRIVATE;
    } else if ([[maybe_unused]] auto md_addrspace_org = inst->getMetadata(user_addrspace_global)) {
      addrSpace = LSC_DOC_ADDR_SPACE::GLOBAL;
    } else if ([[maybe_unused]] auto md_addrspace_org = inst->getMetadata(user_addrspace_local)) {
      addrSpace = LSC_DOC_ADDR_SPACE::LOCAL;
    } else if ([[maybe_unused]] auto md_addrspace_org = inst->getMetadata(user_addrspace_generic)) {
      addrSpace = LSC_DOC_ADDR_SPACE::GENERIC;
    } else if ([[maybe_unused]] auto md_addrspace_org = inst->getMetadata(user_addrspace_raystack)) {
      addrSpace = LSC_DOC_ADDR_SPACE::RAYSTACK;
    }
  }

  return addrSpace;
}

bool UserAddrSpaceMD::Has(llvm::Instruction *inst, LSC_DOC_ADDR_SPACE type) { return Get(inst) == type; }