/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/



#ifndef _IGA_SAA_HPP_
#define _IGA_SAA_HPP_

#include "MemManager.hpp"

#include <memory>
using namespace std;
namespace iga
{
template <class T>
class std_arena_based_allocator
{
protected:
    std::shared_ptr<MemManager> MemManager_ptr;

public:

    //for allocator_traits
    typedef size_t    size_type;
    typedef ptrdiff_t difference_type;
    typedef T*        pointer;
    typedef const T*  const_pointer;
    typedef T&        reference;
    typedef const T&  const_reference;
    typedef T         value_type;

    explicit std_arena_based_allocator(std::shared_ptr<MemManager> _other_ptr)
        :MemManager_ptr(_other_ptr)
    {
    }

    explicit std_arena_based_allocator()
        :MemManager_ptr(nullptr)
    {
        //This implicitly calls MemManager constructor.
        MemManager_ptr = make_shared<MemManager>(4096);
    }

    explicit std_arena_based_allocator(const std_arena_based_allocator& other)
        : MemManager_ptr(other.MemManager_ptr)
    {}


    template <class U>
    std_arena_based_allocator(const std_arena_based_allocator<U>& other)
        : MemManager_ptr(other.MemManager_ptr)
    {}

    template <class U>
    std_arena_based_allocator& operator=(const std_arena_based_allocator<U>& other)
    {
        MemManager_ptr = other.MemManager_ptr;
        return *this;
    }

    template <class U>
    struct rebind { typedef std_arena_based_allocator<U> other; };

    template <class U> friend class std_arena_based_allocator;

    pointer allocate(size_type n, const void * = 0)
    {
        T* t = (T*)MemManager_ptr->alloc(n * sizeof(T));
        return t;
    }

    void deallocate(void*, size_type)
    {
        // No deallocation for arena allocator.
    }

    pointer           address(reference x) const { return &x; }
    const_pointer     address(const_reference x) const { return &x; }

    std_arena_based_allocator<T>&  operator=(const std_arena_based_allocator&)
    {
        return *this;
    }

    void              construct(pointer p, const T& val)
    {
        new ((T*)p) T(val);
    }
    void              destroy(pointer p) { p->~T(); }

    size_type         max_size() const { return size_t(-1); }

    inline bool operator==(const std_arena_based_allocator &) const { return true; }

    inline bool operator!=(const std_arena_based_allocator & a) const { return !operator==(a); }
};
}
#endif
