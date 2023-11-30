/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_SAA_HPP_
#define _IGA_SAA_HPP_

#include "MemManager.hpp"

#include <memory>

namespace iga {
template <class T> class std_arena_based_allocator {
protected:
  std::shared_ptr<MemManager> MemManager_ptr;

public:
  // for allocator_traits
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T &reference;
  typedef const T &const_reference;
  typedef T value_type;

  ~std_arena_based_allocator() = default;

  explicit std_arena_based_allocator(std::shared_ptr<MemManager> _other_ptr)
      : MemManager_ptr(_other_ptr) {}

  explicit std_arena_based_allocator()
      : MemManager_ptr(std::make_shared<MemManager>(4096)) {}

  explicit std_arena_based_allocator(const std_arena_based_allocator &other)
      : MemManager_ptr(other.MemManager_ptr) {}

  template <class U>
  std_arena_based_allocator(const std_arena_based_allocator<U> &other)
      : MemManager_ptr(other.MemManager_ptr) {}

  template <class U>
  std_arena_based_allocator &
  operator=(const std_arena_based_allocator<U> &other) {
    MemManager_ptr = other.MemManager_ptr;
    return *this;
  }

  template <class U> struct rebind {
    typedef std_arena_based_allocator<U> other;
  };

  template <class U> friend class std_arena_based_allocator;

  pointer allocate(size_type n, const void * = 0) {
    T *t = (T *)MemManager_ptr->alloc(n * sizeof(T));
    return t;
  }

  void deallocate(void *, size_type) {
    // No deallocation for arena allocator.
  }

  pointer address(reference x) const { return &x; }
  const_pointer address(const_reference x) const { return &x; }

  std_arena_based_allocator<T> &operator=(const std_arena_based_allocator &) {
    return *this;
  }

  void construct(pointer p, const T &val) { new ((T *)p) T(val); }
  void destroy(pointer p) { p->~T(); }

  size_type max_size() const { return size_t(-1); }

  inline bool operator==(const std_arena_based_allocator &) const {
    return true;
  }

  inline bool operator!=(const std_arena_based_allocator &a) const {
    return !operator==(a);
  }
};
} // namespace iga
#endif
