/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _METADATA_H_
#define _METADATA_H_

#include <iostream>
#include <optional>

namespace vISA {

//
// A metadata is simply a collection of (<key>, <value>) pairs that may be
// attached to an IR object (BB, Inst, Declare, etc.) Metadata is completely
// optional; vISA optimizations and transformations are not obliged to preserve
// them, and dropping them should not affect correctness Metadata key is a
// string, and only one value is allowed per key for now. Metadata value is
// represented by an MDNode abstract class, each subclass provides the actual
// implementation of the metadata value Currently there are only two types of
// metadata: MDString and MDLocation. Metadata memory management is performed by
// IR_Builder through the various allocaeMD* methods. An MDNode may be shared
// among muitiple IR objects, it's the user's responsiblity to ensure correct
// ownership and sharing behaviors. Each object has its own unqiue metadata map,
// however. Currently only G4_INST supports metadata through the
// setMetaData/getMetadata interface. OPEN: use enum instead of string as
// metadata key. This would speed up lookup at the expense of some flexibility.
// OPEN: better management schemes for a MDNode's lifetime/ownership.
//

enum class MDType { String, SrcLoc, TokenLoc };

// forward declaration so that the asMD*() calls can work
class MDString;
class MDLocation;
class MDTokenLocation;

class MDNode {

  const MDType nodeType;

protected:
  MDNode(MDType ty) : nodeType(ty) {}

public:
  MDNode(const MDNode &node) = delete;

  void operator=(const MDNode &node) = delete;

  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }

  virtual ~MDNode() {}

  bool isMDString() const { return nodeType == MDType::String; }
  bool isMDLocation() const { return nodeType == MDType::SrcLoc; }
  bool isMDTokenLocation() const { return nodeType == MDType::TokenLoc; }

  const MDString *asMDString() const {
    return isMDString() ? reinterpret_cast<const MDString *>(this) : nullptr;
  }

  MDLocation *asMDLocation() const {
    return isMDLocation()
               ? reinterpret_cast<MDLocation *>(const_cast<MDNode *>(this))
               : nullptr;
  }

  MDTokenLocation *asMDTokenLocation() const {
    return isMDTokenLocation()
               ? reinterpret_cast<MDTokenLocation *>(const_cast<MDNode *>(this))
               : nullptr;
  }

  virtual void print(std::ostream &OS) const = 0;
  void dump() const { print(std::cerr); }
};

class MDString : public MDNode {
  const std::string data;

public:
  MDString(const std::string &str) : MDNode(MDType::String), data(str) {}

  MDString(const MDString &node) = delete;

  void operator=(const MDString &node) = delete;

  virtual ~MDString() = default;

  std::string getData() const { return data; }

  void print(std::ostream &OS) const { OS << "\"" << data << "\""; }
};

class MDLocation : public MDNode {
  int lineNo = -1;
  const char *srcFilename = nullptr;

public:
  MDLocation(int lineNo, const char *srcFilename)
      : MDNode(MDType::SrcLoc), lineNo(lineNo), srcFilename(srcFilename) {}

  MDLocation(const MDLocation &node) = delete;

  void operator=(const MDLocation &node) = delete;

  ~MDLocation() = default;

  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }
  int getLineNo() const { return lineNo; }
  const char *getSrcFilename() const { return srcFilename; }

  void print(std::ostream &OS) const {
    OS << "\"" << srcFilename << ":" << lineNo << "\"";
  }
};

class MDTokenLocation : public MDNode {
  std::vector<unsigned short> token;
  std::vector<unsigned> global_id;

public:
  MDTokenLocation(unsigned short _token, unsigned globalID)
      : MDNode(MDType::TokenLoc) {
    token.push_back(_token);
    global_id.push_back(globalID);
  }

  MDTokenLocation(const MDTokenLocation &node) = delete;

  void operator=(const MDTokenLocation &node) = delete;

  ~MDTokenLocation() = default;

  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }
  int getTokenLocationNum() const { return global_id.size(); }
  unsigned short getToken(int i) const { return token[i]; }
  unsigned getTokenLocation(int i) const { return global_id[i]; }
  void addTokenLocation(unsigned short _token, int globalID) {
    token.push_back(_token), global_id.push_back(globalID);
  }

  void print(std::ostream &OS) const {
    OS << token.back() << "." << global_id.back();
  }
};

class Metadata {

  std::unordered_map<std::string, MDNode *> MDMap;

public:
  explicit Metadata() {}

  Metadata(const Metadata &md) = delete;

  virtual ~Metadata() {}

  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }

  // it simply overwrites existing value for key if it already exists
  void setMetadata(const std::string &key, MDNode *value) {
    if (!value) {
      // do not allow nullptr value for now. ToDo: distinguish between nullptr
      // value vs. metadata not set?
      return;
    }
    MDMap[key] = value;
  }

  MDNode *getMetadata(const std::string &key) {
    auto iter = MDMap.find(key);
    return iter != MDMap.end() ? iter->second : nullptr;
  }

  bool isMetadataSet(const std::string &key) { return MDMap.count(key); }

  void print(std::ostream &OS) const {
    for (auto &&iter : MDMap) {
      OS << "\"" << iter.first << "\" : ";
      iter.second->print(OS);
      OS << "\n";
    }
  }

  void dump() const { print(std::cerr); }

  // list the known keys here to avoid typos
  inline static const std::string InstComment = "comment";
  inline static const std::string InstLoc = "location";
  inline static const std::string TokenLoc = "tokenlocation";
};

} // namespace vISA

#endif
