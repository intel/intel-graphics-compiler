/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MDFrameWork.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Constants.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>
#include <llvm/ADT/MapVector.h>
#include "common/LLVMWarningsPop.hpp"

#include "StringMacros.hpp"
#include "common/igc_regkeys.hpp"

#include <optional>

using namespace llvm;

//(non-autogen)function prototypes
MDNode *CreateNode(unsigned char i, Module *module, StringRef name);
MDNode *CreateNode(int i, Module *module, StringRef name);
MDNode *CreateNode(unsigned i, Module *module, StringRef name);
MDNode *CreateNode(uint64_t i, Module *module, StringRef name);
MDNode *CreateNode(float f, Module *module, StringRef name);
MDNode *CreateNode(bool b, Module *module, StringRef name);
template <typename val> MDNode *CreateNode(const std::vector<val> &vec, Module *module, StringRef name);
template <typename val, size_t s> MDNode *CreateNode(const std::array<val, s> &arr, Module *module, StringRef name);
template <typename val> MDNode *CreateNode(const std::optional<val> &option, Module *module, StringRef name);
MDNode *CreateNode(Value *val, Module *module, StringRef name);
MDNode *CreateNode(StructType *Ty, Module *module, StringRef name);
template <typename Key, typename Value>
MDNode *CreateNode(const std::map<Key, Value> &FuncMD, Module *module, StringRef name);
template <typename Key, typename Value>
MDNode *CreateNode(const MapVector<Key, Value> &FuncMD, Module *module, StringRef name);
template <typename Key> MDNode *CreateNode(const std::set<Key> &FuncMD, Module *module, StringRef name);
template <typename Key> MDNode *CreateNode(const SetVector<Key> &FuncMD, Module *module, StringRef name);
MDNode *CreateNode(const std::string &s, Module *module, StringRef name);
MDNode *CreateNode(char *i, Module *module, StringRef name);

void readNode(bool &b, MDNode *node);
void readNode(float &x, MDNode *node);
void readNode(int &x, MDNode *node);
void readNode(uint64_t &x, MDNode *node);
void readNode(unsigned char &x, MDNode *node);
void readNode(Value *&val, MDNode *node);
void readNode(std::string &s, MDNode *node);
void readNode(char *&s, MDNode *node);

template <typename T> void readNode(std::vector<T> &vec, MDNode *node);
template <typename T, size_t s> void readNode(std::array<T, s> &arr, MDNode *node);
template <typename T> void readNode(std::optional<T> &option, MDNode *node);
void readNode(Function *&funcPtr, MDNode *node);
void readNode(GlobalVariable *&globalVar, MDNode *node);
void readNode(Type *&Ty, MDNode *node);
void readNode(StructType *&Ty, MDNode *node);

template <typename Key, typename Value> void readNode(std::map<Key, Value> &funcMD, MDNode *node);
template <typename Key, typename Value> void readNode(MapVector<Key, Value> &funcMD, MDNode *node);
template <typename Key> void readNode(std::set<Key> &funcMD, MDNode *node);
template <typename Key> void readNode(SetVector<Key> &funcMD, MDNode *node);

template <typename T> void readNode(T &t, MDNode *node, StringRef name);

// including auto-generated functions
#include "MDNodeFunctions.gen"
namespace IGC {
bool operator<(const ConstantAddress &a, const ConstantAddress &b) {
  if (a.bufId < b.bufId)
    return true;
  else if (a.bufId == b.bufId)
    return (a.eltId < b.eltId);

  return false;
}
} // namespace IGC

// non-autogen functions implementations below
MDNode *CreateNode(unsigned char i, Module *module, StringRef name) {
  Metadata *v[] = {
      MDString::get(module->getContext(), name),
      ValueAsMetadata::get(ConstantInt::get(Type::getInt8Ty(module->getContext()), i)),
  };
  MDNode *node = MDNode::get(module->getContext(), v);
  return node;
}

MDNode *CreateNode(int i, Module *module, StringRef name) {
  Metadata *v[] = {
      MDString::get(module->getContext(), name),
      ValueAsMetadata::get(ConstantInt::get(Type::getInt32Ty(module->getContext()), i)),
  };
  MDNode *node = MDNode::get(module->getContext(), v);
  return node;
}

MDNode *CreateNode(unsigned i, Module *module, StringRef name) {
  Metadata *v[] = {
      MDString::get(module->getContext(), name),
      ValueAsMetadata::get(ConstantInt::get(Type::getInt32Ty(module->getContext()), i)),
  };
  MDNode *node = MDNode::get(module->getContext(), v);
  return node;
}

MDNode *CreateNode(uint64_t i, Module *module, StringRef name) {
  Metadata *v[] = {
      MDString::get(module->getContext(), name),
      ValueAsMetadata::get(ConstantInt::get(Type::getInt64Ty(module->getContext()), i)),
  };
  MDNode *node = MDNode::get(module->getContext(), v);
  return node;
}

MDNode *CreateNode(const std::string &s, Module *module, StringRef name) {
  Metadata *v[] = {
      MDString::get(module->getContext(), name),
      MDString::get(module->getContext(), s),
  };
  MDNode *node = MDNode::get(module->getContext(), v);
  return node;
}

MDNode *CreateNode(float f, Module *module, StringRef name) {
  Metadata *v[] = {
      MDString::get(module->getContext(), name),
      ValueAsMetadata::get(ConstantFP::get(Type::getFloatTy(module->getContext()), f)),
  };

  MDNode *node = MDNode::get(module->getContext(), v);
  return node;
}

MDNode *CreateNode(bool b, Module *module, StringRef name) {
  Metadata *v[] = {MDString::get(module->getContext(), name),
                   ValueAsMetadata::get(ConstantInt::get(Type::getInt1Ty(module->getContext()), b ? 1 : 0))};

  MDNode *node = MDNode::get(module->getContext(), v);
  return node;
}

template <typename val> MDNode *CreateNode(const std::vector<val> &vec, Module *module, StringRef name) {
  std::vector<Metadata *> nodes;
  nodes.push_back(MDString::get(module->getContext(), name));
  size_t i = 0;
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    nodes.push_back(CreateNode(*(it), module, name.str() + "Vec[" + std::to_string(i++) + "]"));
    if (IGC_IS_FLAG_DISABLED(ShowFullVectorsInShaderDumps) && i > MAX_VECTOR_SIZE_TO_PRINT_IN_SHADER_DUMPS) {
      if (IGC_IS_FLAG_ENABLED(ShaderDumpEnable)) {
        std::string flagName = "ShowFullVectorsInShaderDumps";
#ifndef _WIN32
        flagName = "IGC_" + flagName;
#endif
        std::string warningMessage =
            "ShaderDumpEnable Warning! " + name.str() + "Vec[] has " + std::to_string(vec.size()) +
            " elements. Including first " + std::to_string(MAX_VECTOR_SIZE_TO_PRINT_IN_SHADER_DUMPS) +
            " items in ShaderDumps. To print all elements set " + flagName + " register flag to True. " +
            "ShaderOverride flag may not work properly without " + flagName + " enabled.";

        // Print warning once in console, print every time in LLVM ShaderDump
        static bool printWarningFirstTime = true;
        if (printWarningFirstTime) {
          fprintf(stderr, "\n%s\n\n", warningMessage.c_str());
          printWarningFirstTime = false;
        }
        nodes.push_back(CreateNode(false, module, warningMessage + " " + flagName + " currently equals"));
      }
      break;
    }
  }
  MDNode *node = MDNode::get(module->getContext(), nodes);
  return node;
}

template <typename val, size_t s> MDNode *CreateNode(const std::array<val, s> &vec, Module *module, StringRef name) {
  std::vector<Metadata *> nodes;
  nodes.push_back(MDString::get(module->getContext(), name));
  for (unsigned int i = 0; i < s; i++) {
    nodes.push_back(CreateNode(vec[i], module, name.str() + "Vec[" + std::to_string(i) + "]"));
  }
  MDNode *node = MDNode::get(module->getContext(), nodes);
  return node;
}

template <typename val> MDNode *CreateNode(const std::optional<val> &option, Module *module, StringRef name) {
  std::vector<Metadata *> nodes;
  nodes.push_back(MDString::get(module->getContext(), name));
  if (option.has_value())
    nodes.push_back(CreateNode(*option, module, name.str() + "Option"));
  else
    nodes.push_back(ValueAsMetadata::get(ConstantPointerNull::get(Type::getInt1PtrTy(module->getContext()))));

  MDNode *node = MDNode::get(module->getContext(), nodes);
  return node;
}

MDNode *CreateNode(Value *val, Module *module, StringRef name) {
  Metadata *v[] = {MDString::get(module->getContext(), name), ValueAsMetadata::get(val)};
  MDNode *node = MDNode::get(module->getContext(), v);
  return node;
}

MDNode *CreateNode(StructType *Ty, Module *module, StringRef name) {
  Metadata *v[] = {MDString::get(module->getContext(), name), ValueAsMetadata::get(UndefValue::get(Ty))};
  MDNode *node = MDNode::get(module->getContext(), v);
  return node;
}

template <typename Key, typename Value>
MDNode *CreateNode(const std::map<Key, Value> &FuncMD, Module *module, StringRef name) {
  std::vector<Metadata *> nodes;
  nodes.push_back(MDString::get(module->getContext(), name));
  int i = 0;
  for (auto it = FuncMD.begin(); it != FuncMD.end(); ++it) {
    nodes.push_back(CreateNode(it->first, module, name.str() + "Map[" + std::to_string(i) + "]"));
    nodes.push_back(CreateNode(it->second, module, name.str() + "Value[" + std::to_string(i++) + "]"));
  }
  MDNode *node = MDNode::get(module->getContext(), nodes);
  return node;
}

template <typename Key, typename Value>
MDNode *CreateNode(const MapVector<Key, Value> &FuncMD, Module *module, StringRef name) {
  std::vector<Metadata *> nodes;
  nodes.push_back(MDString::get(module->getContext(), name));
  int i = 0;
  for (auto it = FuncMD.begin(); it != FuncMD.end(); ++it) {
    // It is necessary to check that the function was not removed after the inline pass.
    // If this happens, the function may be trying to access invalid metadata.
    if (name == "FuncMD") {
      auto &functionList = module->getFunctionList();
      llvm::Module::FunctionListType::iterator funcIterator = std::find_if(
          functionList.begin(), functionList.end(), [&it](Function &f) { return &f == dyn_cast<Function>(it->first); });
      if (funcIterator == functionList.end()) {
        continue;
      }
    }

    nodes.push_back(CreateNode(it->first, module, name.str() + "Map[" + std::to_string(i) + "]"));
    nodes.push_back(CreateNode(it->second, module, name.str() + "Value[" + std::to_string(i++) + "]"));
  }

  MDNode *node = MDNode::get(module->getContext(), nodes);
  return node;
}

template <typename Key> MDNode *CreateNode(const std::set<Key> &FuncMD, Module *module, StringRef name) {
  std::vector<Metadata *> nodes;
  nodes.push_back(MDString::get(module->getContext(), name));
  int i = 0;
  for (auto it = FuncMD.begin(); it != FuncMD.end(); ++it) {
    nodes.push_back(CreateNode(*it, module, name.str() + "Set[" + std::to_string(i++) + "]"));
  }
  MDNode *node = MDNode::get(module->getContext(), nodes);
  return node;
}

template <typename Key> MDNode *CreateNode(const SetVector<Key> &FuncMD, Module *module, StringRef name) {
  std::vector<Metadata *> nodes;
  nodes.push_back(MDString::get(module->getContext(), name));
  int i = 0;
  for (auto it = FuncMD.begin(); it != FuncMD.end(); ++it) {
    nodes.push_back(CreateNode(*it, module, name.str() + "Set[" + std::to_string(i++) + "]"));
  }
  MDNode *node = MDNode::get(module->getContext(), nodes);
  return node;
}

void readNode(unsigned char &b, MDNode *node) {
  ValueAsMetadata *pVal = cast<ValueAsMetadata>(node->getOperand(1));
  b = (unsigned char)cast<ConstantInt>(pVal->getValue())->getZExtValue();
  return;
}

void readNode(char &b, MDNode *node) {
  ValueAsMetadata *pVal = cast<ValueAsMetadata>(node->getOperand(1));
  b = (char)cast<ConstantInt>(pVal->getValue())->getZExtValue();
  return;
}

void readNode(bool &b, MDNode *node) {
  ValueAsMetadata *pVal = cast<ValueAsMetadata>(node->getOperand(1));
  b = ((int)cast<ConstantInt>(pVal->getValue())->getZExtValue()) ? true : false;
  return;
}

void readNode(float &x, MDNode *node) {
  ValueAsMetadata *pVal = cast<ValueAsMetadata>(node->getOperand(1));
  x = (float)cast<ConstantFP>(pVal->getValue())->getValueAPF().convertToFloat();
  return;
}

void readNode(int &x, MDNode *node) {
  ValueAsMetadata *pVal = cast<ValueAsMetadata>(node->getOperand(1));
  x = (int)cast<ConstantInt>(pVal->getValue())->getZExtValue();
  return;
}

void readNode(std::string &s, MDNode *node) {
  s = cast<MDString>(node->getOperand(1))->getString().str();
  return;
}

void readNode(unsigned &x, MDNode *node) {
  ValueAsMetadata *pVal = cast<ValueAsMetadata>(node->getOperand(1));
  x = (unsigned)cast<ConstantInt>(pVal->getValue())->getZExtValue();
  return;
}

void readNode(uint64_t &x, MDNode *node) {
  ValueAsMetadata *pVal = cast<ValueAsMetadata>(node->getOperand(1));
  x = (uint64_t)cast<ConstantInt>(pVal->getValue())->getZExtValue();
  return;
}

void readNode(Value *&val, MDNode *node) { val = MetadataAsValue::get(node->getContext(), node->getOperand(1)); }

template <typename T> void readNode(std::vector<T> &vec, MDNode *node) {
  for (unsigned k = 1; k < node->getNumOperands(); k++) {
    T vecEle;
    readNode(vecEle, cast<MDNode>(node->getOperand(k)));
    vec.push_back(vecEle);
  }
  return;
}

template <typename T, size_t s> void readNode(std::array<T, s> &arr, MDNode *node) {
  for (unsigned k = 1; k < node->getNumOperands(); k++) {
    T vecEle;
    readNode(vecEle, cast<MDNode>(node->getOperand(k)));
    arr[k - 1] = std::move(vecEle);
  }
  return;
}

template <typename T> void readNode(std::optional<T> &option, MDNode *node) {
  // If it is a value, that's just the null pointer placeholder
  if (isa<ValueAsMetadata>(node->getOperand(1))) {
    option = std::nullopt;
  } else {
    // otherwise, read the contents
    T tmp;
    readNode(tmp, cast<MDNode>(node->getOperand(1)));
    option = tmp;
  }
}

void readNode(Function *&funcPtr, MDNode *node) {
  ValueAsMetadata *pVal = cast<ValueAsMetadata>(node->getOperand(1));
  Value *v = pVal->getValue();
  funcPtr = cast<Function>(v);
}

void readNode(GlobalVariable *&globalVar, MDNode *node) {
  ValueAsMetadata *pVal = cast<ValueAsMetadata>(node->getOperand(1));
  Value *v = pVal->getValue();
  globalVar = cast<GlobalVariable>(v);
}

void readNode(Type *&Ty, MDNode *node) {
  ValueAsMetadata *pVal = cast<ValueAsMetadata>(node->getOperand(1));
  Value *v = pVal->getValue();
  Ty = cast<Type>(v->getType());
}

void readNode(StructType *&Ty, MDNode *node) {
  ValueAsMetadata *pVal = cast<ValueAsMetadata>(node->getOperand(1));
  Value *v = pVal->getValue();
  Ty = cast<StructType>(v->getType());
}

template <typename Key, typename Value> void readNode(std::map<Key, Value> &keyMD, MDNode *node) {
  for (unsigned k = 1; k < node->getNumOperands(); k++) {
    std::pair<Key, Value> p;
    readNode(p.first, cast<MDNode>(node->getOperand(k++)));
    readNode(p.second, cast<MDNode>(node->getOperand(k)));
    keyMD.insert(p);
  }
  return;
}

template <typename Key, typename Value> void readNode(MapVector<Key, Value> &keyMD, MDNode *node) {
  for (unsigned k = 1; k < node->getNumOperands(); k++) {
    std::pair<Key, Value> p;
    readNode(p.first, cast<MDNode>(node->getOperand(k++)));
    readNode(p.second, cast<MDNode>(node->getOperand(k)));
    keyMD.insert(p);
  }
  return;
}

template <typename Key> void readNode(std::set<Key> &keyMD, MDNode *node) {
  for (unsigned k = 1; k < node->getNumOperands(); k++) {
    Key key;
    readNode(key, cast<MDNode>(node->getOperand(k)));
    keyMD.insert(key);
  }
  return;
}

template <typename Key> void readNode(SetVector<Key> &keyMD, MDNode *node) {
  for (unsigned k = 1; k < node->getNumOperands(); k++) {
    Key key;
    readNode(key, cast<MDNode>(node->getOperand(k)));
    keyMD.insert(key);
  }
  return;
}

template <typename T> void readNode(T &t, MDNode *node, StringRef name) {
  for (unsigned i = 1; i < node->getNumOperands(); i++) {
    MDNode *temp = cast<MDNode>(node->getOperand(i));
    if (cast<MDString>(temp->getOperand(0))->getString() == name) {
      readNode(t, temp);
      return;
    }
  }
}

void IGC::deserialize(IGC::ModuleMetaData &deserializeMD, const Module *module) {
  IGC::ModuleMetaData temp;
  deserializeMD = temp;
  NamedMDNode *root = module->getNamedMetadata("IGCMetadata");
  if (!root) {
    return;
  } // module has not been serialized with IGCMetadata yet
  MDNode *moduleRoot = root->getOperand(0);
  readNode(deserializeMD, moduleRoot);
}

void IGC::serialize(const IGC::ModuleMetaData &moduleMD, Module *module) {
  NamedMDNode *LLVMMetadata = module->getNamedMetadata("IGCMetadata");
  if (LLVMMetadata) {
    // clear old metadata if present
    LLVMMetadata->dropAllReferences();
  }
  LLVMMetadata = module->getOrInsertNamedMetadata("IGCMetadata");
  auto node = CreateNode(moduleMD, module, "ModuleMD");
  LLVMMetadata->addOperand(node);
}

bool IGC::isBindless(const IGC::FunctionMetaData &funcMD) {
  return funcMD.rtInfo.callableShaderType != RayGen || funcMD.rtInfo.isContinuation;
}

bool IGC::isContinuation(const IGC::FunctionMetaData &funcMD) { return funcMD.rtInfo.isContinuation; }

bool IGC::isCallStackHandler(const IGC::FunctionMetaData &funcMD) {
  return funcMD.rtInfo.callableShaderType == IGC::CallStackHandler;
}


int IGC::extractAnnotatedNumThreads(const IGC::FunctionMetaData &funcMD) {
  static constexpr const char *searchedString = "num-thread-per-eu";
  static constexpr auto searchedStringLength = std::char_traits<char>::length(searchedString);

  for (const auto &annotation : funcMD.UserAnnotations) {
    auto index = annotation.find(searchedString);
    if (index != std::string::npos) {
      std::string value = annotation.substr(index + searchedStringLength);

      // Remove whitespaces - if they are present
      value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());

      return value == "auto" ? 0 : std::stoi(value);
    }
  }

  return -1;
}
