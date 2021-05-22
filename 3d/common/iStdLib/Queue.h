/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

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
