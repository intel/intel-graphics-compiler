/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This is a simplified unordered map container that is suitable for use in an
/// interface with the UMD. It uses the Array class for internal storage.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "Array.h"
#include <utility>

namespace Interface {

template <typename KeyT, typename ValueT>
class UnorderedMap {
private:
  Array<KeyT> Keys;
  Array<ValueT> Values;

public:
  using key_type = KeyT;
  using mapped_type = ValueT;
  using value_type = std::pair<const key_type, mapped_type>;
  using reference_type = std::pair<const key_type &, mapped_type &>;
  using const_reference_type = std::pair<const key_type &, const mapped_type &>;

  UnorderedMap() = default;

  void destroy() {
    Keys.destroy();
    Values.destroy();
  }

  size_t size() const { return Keys.size(); }
  bool empty() const { return Keys.empty(); }

  // Find index of key, or size() if not found
  size_t findIndex(const key_type& key) const {
    for (size_t i = 0; i < Keys.size(); ++i) {
      if (Keys[i] == key)
        return i;
    }
    return Keys.size();
  }

  // Returns pointer to value if found, nullptr otherwise
  mapped_type* find(const key_type& key) {
    size_t idx = findIndex(key);
    if (idx < Values.size())
      return &Values[idx];
    return nullptr;
  }
  const mapped_type* find(const key_type& key) const {
    size_t idx = findIndex(key);
    if (idx < Values.size())
      return &Values[idx];
    return nullptr;
  }

  // Insert or assign
  void insert(const key_type& key, const mapped_type& value) {
    size_t idx = findIndex(key);
    if (idx < Keys.size()) {
      Values[idx] = value;
    } else {
      // Grow arrays by 1
      Array<KeyT> newKeys(Keys.size() + 1);
      Array<ValueT> newValues(Values.size() + 1);
      for (size_t i = 0; i < Keys.size(); ++i) {
        newKeys[i] = Keys[i];
        newValues[i] = Values[i];
      }
      newKeys[Keys.size()] = key;
      newValues[Values.size()] = value;
      Keys.destroy();
      Values.destroy();
      Keys = std::move(newKeys);
      Values = std::move(newValues);
    }
  }

  // Insert or assign
  void insert(const key_type& key, mapped_type&& value) {
    size_t idx = findIndex(key);
    if (idx < Keys.size()) {
      Values[idx] = std::move(value);
    } else {
      uint32_t currSize = Keys.size();

      Array<KeyT> oldKeys = std::move(Keys);
      Array<ValueT> oldValues = std::move(Values);

      Keys = Array<KeyT>(currSize + 1);
      Values = Array<ValueT>(currSize + 1);

      for (uint32_t i = 0; i < currSize; ++i) {
        Keys[i] = std::move(oldKeys[i]);
        Values[i] = std::move(oldValues[i]);
      }

      Keys[currSize] = key;
      Values[currSize] = std::move(value);

      oldKeys.destroy();
      oldValues.destroy();
    }
  }

  // operator[]
  mapped_type& operator[](const key_type& key) {
    size_t idx = findIndex(key);
    if (idx < Values.size())
      return Values[idx];
    // Insert default value
    insert(key, std::move(mapped_type()));
    return Values[Values.size() - 1];
  }

  struct iterator {

  private:
    using keyIt = typename Array<KeyT>::iterator;
    using ValueIt = typename Array<ValueT>::iterator;

    keyIt m_k;
    ValueIt m_v;

  public:

    iterator(keyIt k, ValueIt v) : m_k(k), m_v(v) {}
    reference_type operator*() const { return reference_type(*m_k, *m_v); }
    iterator &operator++() {
      m_k++;
      m_v++;
      return *this;
    }

    bool operator==(const iterator &other) const { return m_k == other.m_k && m_v == other.m_v; }
    bool operator!=(const iterator &other) const { return !(*this == other); }
  };

  iterator begin() { return iterator(Keys.begin(), Values.begin()); }
  iterator end() { return iterator(Keys.end(), Values.end()); }

  struct const_iterator {

  private:
    using keyIt = typename Array<KeyT>::const_iterator;
    using ValueIt = typename Array<ValueT>::const_iterator;

    keyIt m_k;
    ValueIt m_v;

  public:
    const_iterator(keyIt k, ValueIt v) : m_k(k), m_v(v) {}
    const_reference_type operator*() const { return const_reference_type(*m_k, *m_v); }
    const_iterator &operator++() {
      m_k++;
      m_v++;
      return *this;
    }

    bool operator==(const const_iterator &other) const { return m_k == other.m_k && m_v == other.m_v; }
    bool operator!=(const const_iterator &other) const { return !(*this == other); }
  };

  const_iterator begin() const { return const_iterator(Keys.begin(), Values.begin()); }
  const_iterator end() const { return const_iterator(Keys.end(), Values.end()); }
};

} // namespace Interface
