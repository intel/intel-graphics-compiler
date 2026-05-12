/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _METADATA_H_
#define _METADATA_H_

#include <array>
#include <iostream>

namespace vISA {

//
// A metadata is simply a collection of (<key>, <value>) pairs that may be
// attached to an IR object (BB, Inst, Declare, etc.) Metadata is completely
// optional; vISA optimizations and transformations are not obliged to preserve
// them, and dropping them should not affect correctness. Metadata key is a
// Metadata::MDKey enum value, and only one value is allowed per key. Metadata
// value is represented by an MDNode abstract class, each subclass provides the
// actual implementation of the metadata value Currently there are only two
// types of metadata: MDString and MDLocation. Metadata memory management is
// performed by IR_Builder through the various allocaeMD* methods. An MDNode may
// be shared among muitiple IR objects, it's the user's responsiblity to ensure
// correct ownership and sharing behaviors. Each object has its own unqiue
// metadata map, however. Currently only G4_INST supports metadata through the
// setMetaData/getMetadata interface. OPEN: better management schemes for a
// MDNode's lifetime/ownership.
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

public:
  enum class MDKey : unsigned {
    InstComment = 0,
    InstLoc = 1,
    TokenLoc = 2,
    BlockFreqDigits = 3,
    BlockFreqScale = 4,
    Count
  };

private:
  std::array<MDNode *, static_cast<size_t>(MDKey::Count)> MDMap{};

public:
  explicit Metadata() = default;

  Metadata(const Metadata &md) = delete;

  virtual ~Metadata() {}

  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }

  void setMetadata(MDKey key, MDNode *value) {
    if (!value)
      return;
    MDMap[static_cast<size_t>(key)] = value;
  }

  MDNode *getMetadata(MDKey key) { return MDMap[static_cast<size_t>(key)]; }

  bool isMetadataSet(MDKey key) {
    return MDMap[static_cast<size_t>(key)] != nullptr;
  }

  void print(std::ostream &OS) const {
    static const char *names[] = {"comment", "location", "tokenlocation",
                                  "stats.blockFrequency.digits",
                                  "stats.blockFrequency.scale"};
    static_assert(std::size(names) == static_cast<size_t>(MDKey::Count),
                  "names[] must match MDKey enum");
    for (size_t i = 0; i < MDMap.size(); i++) {
      if (MDMap[i]) {
        OS << "\"" << names[i] << "\" : ";
        MDMap[i]->print(OS);
        OS << "\n";
      }
    }
  }

  void dump() const { print(std::cerr); }
};

} // namespace vISA

#endif
