/*========================== begin_copyright_notice ============================

Copyright (c) 2019-2021 Intel Corporation

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

#pragma once

#include "LinkedList.h"

namespace iSTD
{

/*****************************************************************************\
Template Parameters
\*****************************************************************************/
#define QueueTemplateList   class Type, class CAllocatorType
#define CQueueType          CQueue<Type,CAllocatorType>

/*****************************************************************************\

Class:
    CQueue

Description:
    Implements an linked-list-based queue

\*****************************************************************************/
template<QueueTemplateList>
class CQueue : public CLinkedListType
{
public:

    bool    Push( const Type element );
    Type    Pop( void );
    Type    Top( void ) const;
};

/*****************************************************************************\

Function:
    CQueue::Push

Description:
    Pushes an element on the queue

Input:
    const Type element

Output:
    bool - success or fail

\*****************************************************************************/
template<QueueTemplateList>
bool CQueueType::Push( const Type element )
{
    // Add element to the top of list
    return this->Add( element );
}

/*****************************************************************************\

Function:
    CQueue::Pop

Description:
    Pops an element off the queue

Input:
    none

Output:
    Type - element

\*****************************************************************************/
template<QueueTemplateList>
Type CQueueType::Pop( void )
{
    Type element = {0};

    if( this->IsEmpty() )
    {
        ASSERT(0);
    }
    else
    {
        // Get the last element on the list
        typename CQueue::CIterator end = this->End();
        --end;

        element = *end;

        // Remove the last element
        this->Remove( end );
    }

    return element;
}

/*****************************************************************************\

Function:
    CQueue::Top

Description:
    Returns the top element of the queue

Input:
    none

Output:
    Type - element

\*****************************************************************************/
template<QueueTemplateList>
Type CQueueType::Top( void ) const
{
    Type element = {0};

    if( this->IsEmpty() )
    {
        ASSERT(0);
    }
    else
    {
        // Get the last element on the list
        typename CQueueType::CConstIterator end = this->End();
        --end;

        element = *end;
    }

    return element;
}

} // iSTD
