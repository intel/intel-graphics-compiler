/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This is a simplified array type container that is suitable for use in an
/// interface with the UMD.  We want to be able to mix and match debug/release
/// dlls with each other.  Each dll statically links the CRT.  While it would
/// be nice to use STL containers on the interface:
///
/// 1) Their layouts can differ between debug and release builds so, for
///    example, a std::vector place in an output structure by IGC in debug
///    could be interpreted differently by a release UMD and cause corruption.
///
/// 2) memory allocated in IGC must be freed in IGC (same with UMD).  Having
///    destructors in the containers could cause accidental deletion across
///    domains if we're not careful.
///
/// In this Array class, we force deletion on explicitly via the destroy()
/// method.  We also clearly don't have any #ifdef _DEBUG (or the like) that
/// would cause the memory layout to be different.
///
//===----------------------------------------------------------------------===//

#pragma once

#include <iterator>
#include <type_traits>
#include <assert.h>

namespace Interface {

template <typename T> class Array {
  // We should only be placing objects in here that don't do anything
  // interesting in the destructor to discourage accidental deletions.
  static_assert(std::is_trivially_destructible_v<T>, "Array contained type must be trivially destructible!");

private:
  // Standard iterator style interface
  class iterator {
  public:
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T *;
    using reference = T &;
    using iterator_category = std::random_access_iterator_tag;

  private:
    pointer Ptr;
    iterator(pointer Ptr) : Ptr(Ptr) {}

  public:
    reference operator*() const { return *Ptr; }
    pointer operator->() const { return Ptr; }
    bool operator==(const iterator &RHS) const { return Ptr == RHS.Ptr; }
    bool operator!=(const iterator &RHS) const { return !(*this == RHS); }
    iterator operator++(int) {
      iterator V = *this;
      ++*this;
      return V;
    }
    iterator &operator++() {
      Ptr++;
      return *this;
    }

    friend Array;
  };

  // const version
  class const_iterator {
  public:
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T *;
    using reference = const T &;
    using iterator_category = std::random_access_iterator_tag;

  private:
    pointer Ptr;
    const_iterator(pointer Ptr) : Ptr(Ptr) {}

  public:
    reference operator*() const { return *Ptr; }
    pointer operator->() const { return Ptr; }
    bool operator==(const const_iterator &RHS) const { return Ptr == RHS.Ptr; }
    bool operator!=(const const_iterator &RHS) const { return !(*this == RHS); }
    const_iterator operator++(int) {
      const_iterator V = *this;
      ++*this;
      return V;
    }
    const_iterator &operator++() {
      Ptr++;
      return *this;
    }

    friend Array;
  };

public:
  Array() = default;

  Array(uint32_t NumElts) {
    if (NumElts == 0)
      return;

    Start = allocate(NumElts);
    End = Start + NumElts;

    for (uint32_t i = 0; i < NumElts; i++)
      new (&Start[i]) typename iterator::value_type;
  }

  Array &operator=(const Array &) = delete;
  // no copying or copy assignment.  It would only make sense to have copy
  // assignment if we could delete the previous contents which may be unsafe.
  Array(const Array &) = delete;

  Array(Array &&RHS) {
    Start = RHS.Start;
    End = RHS.End;

    RHS.Start = nullptr;
    RHS.End = nullptr;
  }

  // move it around instead.
  Array &operator=(Array &&RHS) noexcept {
    Start = RHS.Start;
    End = RHS.End;

    RHS.Start = nullptr;
    RHS.End = nullptr;

    return *this;
  }

  // Convenince constructor to build your data up with the usual containers
  // prior to filling this up.
  template <typename InputIterator> Array(InputIterator first, InputIterator last) {
    size_t NumElts = std::distance(first, last);

    Start = allocate(NumElts);
    End = Start + NumElts;

    std::copy(first, last, this->begin());
  }

  size_t size() const { return End - Start; }

  // Only way to delete, explicitly.  Destructors are trivial so we don't
  // need to explicitly call them here.
  void destroy() {
    for (auto &v : *this) {
      v.destroy();
    }

    ::operator delete(Start);
  }

  iterator begin() { return iterator(Start); }
  iterator end() { return iterator(End); }

  const_iterator begin() const { return const_iterator(Start); }
  const_iterator end() const { return const_iterator(End); }

  const typename iterator::reference operator[](size_t i) const {
    assert((i < size()) && "out of bounds!");
    return Start[i];
  }
  typename iterator::reference operator[](size_t i) {
    assert((i < size()) && "out of bounds!");
    return Start[i];
  }

  // Useful for when we slightly over allocate to avoid having to do two
  // trips through a collection: one to the find the size and the other to
  // write the data.  Just shrink the size down after the fact.
  void trimSize(uint32_t NewSize) {
    if (NewSize > size())
      return;

    End = Start + NewSize;
  }

  bool empty() const { return Start == End; }

private:
  typename iterator::pointer Start = nullptr;
  typename iterator::pointer End = nullptr;

  inline typename iterator::pointer allocate(size_t NumElts) {
    return static_cast<typename iterator::pointer>(::operator new(NumElts * sizeof(typename iterator::value_type)));
  }

  template <typename A, typename B> friend class UnorderedMap;
};


} // namespace Interface
