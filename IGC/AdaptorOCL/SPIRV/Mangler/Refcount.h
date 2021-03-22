/*========================== begin_copyright_notice ============================

Copyright (c) 2016-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

#ifndef __REF_COUNT_H__
#define __REF_COUNT_H__

#include "Probe/Assertion.h"

namespace igc_SPIR {

template <typename T>
class RefCount{
public:
  RefCount(): m_refCount(0), m_ptr(0) {
  }

  RefCount(T* ptr): m_ptr(ptr) {
    m_refCount = new int(1);
  }

  RefCount(const RefCount<T>& other) {
    cpy(other);
  }

  ~RefCount() {
    if (m_refCount)
      dispose();
  }

  RefCount& operator=(const RefCount<T>& other) {
    if(this == &other)
      return *this;
    if (m_refCount)
      dispose();
    cpy(other);
    return *this;
  }

  void init(T* ptr) {
    IGC_ASSERT_MESSAGE(!m_ptr, "overrunning non NULL pointer");
    IGC_ASSERT_MESSAGE(!m_refCount, "overrunning non NULL pointer");
    m_refCount = new int(1);
    m_ptr = ptr;
  }

  bool isNull() const {
    return (!m_ptr);
  }

// Pointer access
  const T& operator*() const{
    sanity();
    return *m_ptr;
  }

  T& operator*() {
    sanity();
    return *m_ptr;
  }

  operator T*() {
    return m_ptr;
  }

  operator const T*() const{
    return m_ptr;
  }

  T* operator->() {
    return m_ptr;
  }

  const T* operator->() const{
    return m_ptr;
  }
private:
  void sanity() const{
    IGC_ASSERT_MESSAGE(m_ptr, "NULL pointer");
    IGC_ASSERT_MESSAGE(m_refCount, "NULL ref counter");
    IGC_ASSERT_MESSAGE(*m_refCount, "zero ref counter");
  }

  void cpy(const RefCount<T>& other) {
    m_refCount = other.m_refCount;
    m_ptr = other.m_ptr;
    if (m_refCount) ++*m_refCount;
  }

  void dispose() {
    sanity();
    if (0 == --*m_refCount) {
      delete m_refCount;
      delete m_ptr;
      m_ptr = 0;
      m_refCount = 0;
    }
  }

  int* m_refCount;
  T* m_ptr;
};// End RefCount

} // End SPIR namespace

#endif//__REF_COUNT_H__
