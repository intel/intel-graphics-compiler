/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <cinttypes>
#include <vector>

#include "cif/common/cif.h"

namespace CIF {

constexpr Version_t CompatibilityEncoderVersion = 1;
using CompatibilityDataHandle = const void *;

namespace Helpers {

template <typename EncodingBaseType> struct EncInterface {
  static constexpr size_t GetDescriptorSizeInEncodingBaseType() { return 3; }
  static constexpr size_t GetDescriptorSizeInBytes() {
    return GetDescriptorSizeInEncodingBaseType() * sizeof(EncodingBaseType);
  }

  static constexpr size_t GetInterfaceIdIdx() { return 0; }

  static constexpr size_t GetVersionIdx() { return 1; }

  static constexpr size_t GetChildrenCountIdx() { return 2; }

  static void Encode(EncodingBaseType *out, InterfaceId_t interfaceId, Version_t version,
                     EncodingBaseType childrenCount) {
    out[GetInterfaceIdIdx()] = interfaceId;
    out[GetVersionIdx()] = version;
    out[GetChildrenCountIdx()] = childrenCount;
  }

  static void Decode(const EncodingBaseType *in, InterfaceId_t &interfaceIdRet, Version_t &versionRet,
                     EncodingBaseType &childrenCountRet) {
    interfaceIdRet = in[GetInterfaceIdIdx()];
    versionRet = in[GetVersionIdx()];
    childrenCountRet = in[GetChildrenCountIdx()];
  }
};

template <typename EncodingBaseType> struct EncHeader {
  static constexpr size_t GetHeaderSizeInEncodingBaseType() { return 3; }

  static constexpr size_t GetHeaderSizeInBytes() {
    return GetHeaderSizeInEncodingBaseType() * sizeof(EncodingBaseType);
  }

  static constexpr size_t GetMagicIdx() { return 0; }

  static constexpr size_t GetVersionIdx() { return 1; }

  static constexpr size_t GetSizeIdx() { return 2; }

  static void Encode(EncodingBaseType *out, EncodingBaseType magic, EncodingBaseType version,
                     EncodingBaseType encodedInterfacesNum) {
    auto *header = out;
    header[GetMagicIdx()] = magic;
    header[GetVersionIdx()] = version;
    header[GetSizeIdx()] =
        GetHeaderSizeInBytes() + encodedInterfacesNum * EncInterface<EncodingBaseType>::GetDescriptorSizeInBytes();
  }

  static const EncodingBaseType *Decode(CompatibilityDataHandle handle, EncodingBaseType magic,
                                        EncodingBaseType version, EncodingBaseType &encodedInterfacesNumRet,
                                        EncodingBaseType &encodedEncoderVersionRet) {
    uint64_t invalidHeader[GetHeaderSizeInEncodingBaseType()];

    auto *header = reinterpret_cast<const EncodingBaseType *>(handle);
    if ((header == nullptr) || (header[GetMagicIdx()] != magic)) {
      invalidHeader[GetMagicIdx()] = InvalidInterface;
      invalidHeader[GetVersionIdx()] = InvalidVersion;
      invalidHeader[GetSizeIdx()] = 0;
      header = invalidHeader;
    }

    encodedEncoderVersionRet = header[GetVersionIdx()];
    encodedInterfacesNumRet = (header[GetSizeIdx()] - GetHeaderSizeInBytes());
    encodedInterfacesNumRet /= EncInterface<EncodingBaseType>::GetDescriptorSizeInBytes();

    if ((header[GetVersionIdx()] != version) || (header == invalidHeader)) {
      // incompatible encoder/decoders
      // we require strict compatibility (no backwards compatibility),
      // but this encoder should not change very often (hardly ever)
      return nullptr;
    }

    return header + GetHeaderSizeInEncodingBaseType();
  }
};

/// Node class that compatibility tree is made of
struct CompatibilityNode {
  /// Id of this interface
  InterfaceId_t InterfaceId = InvalidInterface;

  /// Version requested by the client
  Version_t RequestedVersion = InvalidVersion;

  /// Next child of parent node
  CompatibilityNode *NextSibling = nullptr;

  /// First child of this node
  CompatibilityNode *FirstChild = nullptr;

  /// Minimum version supported by server
  Version_t MinSupportedVersion = InvalidVersion;

  /// Maximum version supported by server
  Version_t MaxSupportedVersion = InvalidVersion;

  /// Adds a child node to this node (will update current children's nextSibling members)
  void AddChild(CompatibilityNode &nd) {
    if (FirstChild == nullptr) {
      FirstChild = &nd;
      return;
    }

    CompatibilityNode *currNd = FirstChild;
    while (currNd->NextSibling != nullptr) {
      currNd = currNd->NextSibling;
    }

    currNd->NextSibling = &nd;
  }

  /// Looks for child node with requested interfaceId
  CompatibilityNode *FindChild(InterfaceId_t interfaceId) {
    CompatibilityNode *currNd = FirstChild;
    while ((currNd != nullptr) && (currNd->InterfaceId != interfaceId)) {
      currNd = currNd->NextSibling;
    }
    return currNd;
  }

  /// Decodes the compatibility data into a tree of nodes
  /// First node in the returned vector is the root node
  template <typename EncodingBaseType, template <Version_t> class EntryPointInterface>
  static std::vector<CompatibilityNode> BuildTree(const EncodingBaseType *enc, uint64_t encWordsCount) {
    if ((enc == nullptr) ||
        (encWordsCount % EncInterface<EncodingBaseType>::GetDescriptorSizeInEncodingBaseType() != 0)) {
      assert(0);
      return std::vector<CompatibilityNode>{};
    }
    std::vector<CompatibilityNode> allNodes;
    CompatibilityNode dummyRootNode;
    allNodes.reserve(
        static_cast<size_t>(encWordsCount / EncInterface<EncodingBaseType>::GetDescriptorSizeInEncodingBaseType()));
    uint64_t currOffset = 0;
    BuildTreeImpl<EncodingBaseType>(enc, encWordsCount, currOffset, dummyRootNode, allNodes);
    PatchSupportedVersionFwd::Call<EntryPointInterface>(dummyRootNode);
    return allNodes;
  }

  template <typename InterfacesTree> static void PatchSupportedVersion(CompatibilityNode &parentNode) {
    InterfacesTree::template forwardToAll<PatchSupportedVersionFwd>(parentNode);
  }

protected:
  /// Helper forwarder struct - see PatchSupportedVersionFwd::Call for details
  struct PatchSupportedVersionFwd {
    /// Recursive worker for patching compatibility tree with data
    /// of interface versions supported by server
    template <template <Version_t> class Interface> static void Call(CompatibilityNode &parentNode) {
      CompatibilityNode *matchingNd = parentNode.FindChild(Interface<CIF::BaseVersion>::GetInterfaceId());
      if (matchingNd == nullptr) {
        return;
      }

      auto minVer = Interface<CIF::TraitsSpecialVersion>::GetOldestSupportedVersion();
      auto maxVer = Interface<CIF::TraitsSpecialVersion>::GetLatestSupportedVersion();
      matchingNd->MinSupportedVersion = minVer;
      matchingNd->MaxSupportedVersion = maxVer;

      Interface<CIF::TraitsSpecialVersion>::AllUsedInterfaces::template forwardToAll<PatchSupportedVersionFwd>(
          *matchingNd);
    }
  };

  /// Recursive worker for decoding compatibility data into a tree
  template <typename EncodingBaseType>
  static void BuildTreeImpl(const EncodingBaseType *enc, uint64_t encWordsCount, uint64_t &currOffset,
                            CompatibilityNode &parent, std::vector<CompatibilityNode> &nodesAllocator) {
    if (currOffset >= encWordsCount) {
      return; // no more data to decode
    }

    nodesAllocator.push_back(CompatibilityNode{});
    CompatibilityNode *nd = &*(--nodesAllocator.end());
    parent.AddChild(*nd);

    EncodingBaseType numChildren = 0;
    EncInterface<EncodingBaseType>::Decode(enc + currOffset, nd->InterfaceId, nd->RequestedVersion, numChildren);
    currOffset += EncInterface<EncodingBaseType>::GetDescriptorSizeInEncodingBaseType();
    for (EncodingBaseType childIt = 0; childIt < numChildren; ++childIt) {
      BuildTreeImpl(enc, encWordsCount, currOffset, *nd, nodesAllocator);
    }
  }
};

} // end namespace Helpers

/// Compatibility data is encoded in units of uint64_t
/// First value is InterfaceId of CompatibilityEncoder
/// Second value is version of CompatibilityEncoder that was used for encoding the data.
/// Third value is the size (in bytes) of the whole encoding (including id, version and this size).
/// Next, are encoded requirements of all interfaces that were used for encoding.
/// Encoding is based on DFS and has following layout:
///   | uint64_t CurrentInterfaceId | uint64_t VersionOfCurrentInterface | uint64_t NumChildInterfacesOfCurrent |
///   ENCODING OF CHILD INTERFACES | uint64_t NextInterfaceId ...
struct CompatibilityEncoder {
  using EncodingBaseType = uint64_t;
  using BackingStorage = std::vector<EncodingBaseType>;

  struct IncompatibilityData {
    InterfaceId_t Interface;
    Version_t RequestedVersion;
    Version_t MinVersionSupported;
    Version_t MaxVersionSupported;
  };

  CompatibilityEncoder() {}

  static constexpr InterfaceId_t GetIntefaceId() { return CIF::InterfaceIdCoder::Enc("CIF_COMP_ENC"); }

  template <template <Version_t> class EntryPointInterface>
  CompatibilityDataHandle Encode(const std::vector<InterfaceId_t> *interfacesToIgnore = nullptr) {
    const size_t sizeToReserve = Helpers::EncHeader<EncodingBaseType>::GetHeaderSizeInEncodingBaseType() +
                                 (Helpers::EncInterface<EncodingBaseType>::GetDescriptorSizeInEncodingBaseType() *
                                  2); // *2 - potential overestimate

    encodedData.clear();
    encodedData.reserve(sizeToReserve);

    // reserve space for header
    for (EncodingBaseType i = 0; i < Helpers::EncHeader<EncodingBaseType>::GetHeaderSizeInEncodingBaseType(); ++i) {
      encodedData.push_back(0);
    }

    // encode compatibility descriptors
    EncodeFwd::Call<EntryPointInterface>(encodedData, interfacesToIgnore);

    // encode header
    const size_t encodedInterfacesNum =
        (encodedData.size() - Helpers::EncHeader<EncodingBaseType>::GetHeaderSizeInEncodingBaseType()) /
        Helpers::EncInterface<EncodingBaseType>::GetDescriptorSizeInEncodingBaseType();
    Helpers::EncHeader<EncodingBaseType>::Encode(encodedData.data(), GetIntefaceId(), CompatibilityEncoderVersion,
                                                 encodedInterfacesNum);
    return encodedData.data();
  }

  template <template <Version_t> class EntryPointInterface>
  static bool GetFirstIncompatible(CompatibilityDataHandle externalEncoding, IncompatibilityData &ret) {
    uint64_t encodedInterfacesNum = 0;
    uint64_t encodedEncoderVersion = InvalidVersion;
    const EncodingBaseType *encodedInterfaces = Helpers::EncHeader<EncodingBaseType>::Decode(
        externalEncoding, GetIntefaceId(), CompatibilityEncoderVersion, encodedInterfacesNum, encodedEncoderVersion);

    if (encodedInterfaces == nullptr) {
      // incompatible encoder/decoders
      // we require strict compatibility, but this encoder should not change very often
      ret.Interface = GetIntefaceId();
      ret.RequestedVersion = encodedEncoderVersion;
      ret.MinVersionSupported = ret.MaxVersionSupported = CompatibilityEncoderVersion;
      return true;
    }

    {
      // TODO : Verify if this is actually better performant then full comparison
      // TODO : Maybe hash encodings in compile time instead?
      CompatibilityEncoder enc;
      auto thisSideEncoding = enc.Encode<EntryPointInterface>();
      auto sizeInBytes = encodedInterfacesNum * Helpers::EncInterface<EncodingBaseType>::GetDescriptorSizeInBytes() +
                         Helpers::EncInterface<EncodingBaseType>::GetDescriptorSizeInBytes();
      if (0 == std::memcmp(externalEncoding, thisSideEncoding, static_cast<uint32_t>(sizeInBytes))) {
        // fast path - exactly the same versions of interfaces
        return false;
      }
    }

    auto incomp = GetAllIncompatibleImpl<EntryPointInterface>(
        encodedInterfaces,
        encodedInterfacesNum * Helpers::EncInterface<EncodingBaseType>::GetDescriptorSizeInEncodingBaseType());
    if (incomp.size() == 0) {
      return false;
    }

    ret = *incomp.begin();
    return true;
  }

  static bool AreIdentical(CompatibilityDataHandle a, CompatibilityDataHandle b) {
    if (a == b) {
      return true;
    }

    if ((a == nullptr) || (b == nullptr)) {
      return false;
    }

    uint64_t encodedInterfacesNum = 0;
    uint64_t encodedEncoderVersion = InvalidVersion;

    Helpers::EncHeader<EncodingBaseType>::Decode(a, GetIntefaceId(), CompatibilityEncoderVersion, encodedInterfacesNum,
                                                 encodedEncoderVersion);

    auto sizeInBytes = encodedInterfacesNum * Helpers::EncInterface<EncodingBaseType>::GetDescriptorSizeInBytes() +
                       Helpers::EncInterface<EncodingBaseType>::GetDescriptorSizeInBytes();
    return 0 == std::memcmp(a, b, static_cast<size_t>(sizeInBytes));
  }

protected:
  struct EncodeFwd {
    template <template <Version_t> class Interface>
    static void Call(BackingStorage &storage, const std::vector<InterfaceId_t> *interfacesToIgnore = nullptr) {
      auto offset = storage.size();
      for (EncodingBaseType i = 0; i < Helpers::EncInterface<EncodingBaseType>::GetDescriptorSizeInEncodingBaseType();
           ++i) {
        storage.push_back(0);
      }

      auto requestedInterface = Interface<CIF::BaseVersion>::GetInterfaceId();
      auto requestedVersion = Interface<CIF::TraitsSpecialVersion>::GetLatestSupportedVersion();
      if (nullptr != interfacesToIgnore) {
        if (std::find(interfacesToIgnore->begin(), interfacesToIgnore->end(), requestedInterface) !=
            interfacesToIgnore->end()) {
          requestedVersion = AnyVersion;
        }
      }

      Helpers::EncInterface<EncodingBaseType>::Encode(
          storage.data() + offset, requestedInterface, requestedVersion,
          Interface<CIF::TraitsSpecialVersion>::AllUsedInterfaces::GetNumInterfaces());

      Interface<CIF::TraitsSpecialVersion>::AllUsedInterfaces::template forwardToAll<EncodeFwd>(storage,
                                                                                                interfacesToIgnore);
    }
  };

  template <template <Version_t> class EntryPointInterface>
  static std::vector<IncompatibilityData> GetAllIncompatibleImpl(const EncodingBaseType *enc, uint64_t encWordsCount) {
    std::vector<Helpers::CompatibilityNode> allNodes;
    allNodes = Helpers::CompatibilityNode::BuildTree<EncodingBaseType, EntryPointInterface>(enc, encWordsCount);

    std::vector<IncompatibilityData> ret;
    for (auto &nd : allNodes) {
      bool supported =
          (nd.RequestedVersion >= nd.MinSupportedVersion) & (nd.RequestedVersion <= nd.MaxSupportedVersion);
      supported |= (nd.RequestedVersion == AnyVersion);
      if (supported) {
        continue;
      }
      IncompatibilityData id;
      id.Interface = nd.InterfaceId;
      id.RequestedVersion = nd.RequestedVersion;
      id.MaxVersionSupported = nd.MaxSupportedVersion;
      id.MinVersionSupported = nd.MinSupportedVersion;
      ret.push_back(std::move(id));
    }

    return ret;
  }

  BackingStorage encodedData;
};

} // namespace CIF
