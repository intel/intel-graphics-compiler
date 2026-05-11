/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// An arena based memory manager implementation.

// NOTE: Object requiring dword alignment is NOT supported.

#ifndef _MEM_MANAGER_H_
#define _MEM_MANAGER_H_

#include "Arena.h"
#include <memory>

namespace vISA {
class Mem_Manager {
public:
  Mem_Manager(size_t defaultArenaSize);
  ~Mem_Manager();

  Mem_Manager(const Mem_Manager &) = delete;
  Mem_Manager &operator=(const Mem_Manager &) = delete;

  void *alloc(size_t size) {
    return _arenaManager.AllocDataSpace(size, ArenaHeader::defaultAlign);
  }

  void *alloc(size_t size, std::align_val_t al) {
    return _arenaManager.AllocDataSpace(size, static_cast<size_t>(al));
  }

private:
  vISA::ArenaManager _arenaManager;
};

template <class T> class std_arena_based_allocator {
protected:
  std::shared_ptr<Mem_Manager> mem_manager_ptr;

public:
  // for allocator_traits
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T &reference;
  typedef const T &const_reference;
  typedef T value_type;

  ~std_arena_based_allocator() = default;

  explicit std_arena_based_allocator(std::shared_ptr<Mem_Manager> _other_ptr)
      : mem_manager_ptr(_other_ptr) {}

  explicit std_arena_based_allocator() : mem_manager_ptr(nullptr) {
    // This implicitly calls Mem_manager constructor.
    mem_manager_ptr = std::make_shared<Mem_Manager>(4096);
  }

  explicit std_arena_based_allocator(const std_arena_based_allocator &other)
      : mem_manager_ptr(other.mem_manager_ptr) {}

  template <class U>
  std_arena_based_allocator(const std_arena_based_allocator<U> &other)
      : mem_manager_ptr(other.mem_manager_ptr) {}

  template <class U>
  std_arena_based_allocator &
  operator=(const std_arena_based_allocator<U> &other) {
    mem_manager_ptr = other.mem_manager_ptr;
    return *this;
  }

  template <class U> struct rebind {
    typedef std_arena_based_allocator<U> other;
  };

  template <class U> friend class std_arena_based_allocator;

  pointer allocate(size_type n, const void * = 0) {
    T *t = (T *)mem_manager_ptr->alloc(n * sizeof(T));
    return t;
  }

  void deallocate(void *p, size_type) {
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

  bool operator==(const std_arena_based_allocator &) const { return true; }

  bool operator!=(const std_arena_based_allocator &a) const {
    return !operator==(a);
  }
};
} // namespace vISA
#endif
