/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Object.h"
#include "Queue.h"
#include "MemCopy.h"


namespace iSTD
{

/*****************************************************************************\
Struct: IsArrayTypeSupported
\*****************************************************************************/
template<typename T>
struct IsArrayTypeSupported                     { enum { value = false }; };

template<>
struct IsArrayTypeSupported<bool>               { enum { value = true }; };

template<>
struct IsArrayTypeSupported<char>               { enum { value = true }; };

template<>
struct IsArrayTypeSupported<unsigned char>      { enum { value = true }; };

template<>
struct IsArrayTypeSupported<int>                { enum { value = true }; };

template<>
struct IsArrayTypeSupported<unsigned int>       { enum { value = true }; };

#ifndef __LP64__ // u/long on linux64 platform is 64-bit type and collides with U/INT64
template<>
struct IsArrayTypeSupported<long>               { enum { value = true }; };

template<>
struct IsArrayTypeSupported<unsigned long>      { enum { value = true }; };
#endif

template<>
struct IsArrayTypeSupported<INT64>              { enum { value = true }; };

template<>
struct IsArrayTypeSupported<UINT64>             { enum { value = true }; };

template<>
struct IsArrayTypeSupported<float>              { enum { value = true }; };

template<>
struct IsArrayTypeSupported<double>             { enum { value = true }; };

template<typename T>
struct IsArrayTypeSupported<T*>                 { enum { value = true }; };

/*****************************************************************************\
Template Parameters
\*****************************************************************************/
#define ArrayTemplateList   class Type, class CAllocatorType

#define CStaticArrayType            CStaticArray<Type,CAllocatorType>
#define CDynamicArrayType           CDynamicArray<Type,CAllocatorType>
#define CArrayType                  CArray<Type,CAllocatorType>

/*****************************************************************************\

Class:
    CStaticArray

Description:
    Implements a statically-sized array

\*****************************************************************************/
template<ArrayTemplateList>
class CStaticArray : public CObject<CAllocatorType>
{
public:

    CStaticArray( const DWORD maxSize );
    virtual ~CStaticArray( void );

    Type    GetElement( const DWORD index ) const;
    Type    GetElementUnsafe( const DWORD index ) const;
    bool    SetElement( const DWORD index, const Type& element );
    void    SetElementUnsafe( const DWORD index, const Type& element );

    DWORD   GetSize( void ) const;

    void    Clear( void );
    void    DebugPrint( void ) const;

    CStaticArrayType& operator= ( const CStaticArrayType &array );

    C_ASSERT( IsArrayTypeSupported<Type>::value == true );

protected:

    Type*   m_pArrayBuffer;
    DWORD   m_ActualSize;
};

/*****************************************************************************\

Function:
    CStaticArray Constructor

Description:
    Initializes the array

Input:
    const DWORD maxSize - maximum size of the array, in elements

Output:
    none

\*****************************************************************************/
template<ArrayTemplateList>
CStaticArrayType::CStaticArray( const DWORD maxSize )
    : CObject<CAllocatorType>()
{
    m_pArrayBuffer = (Type*)CAllocatorType::AlignedAllocate(
        maxSize * sizeof(Type),
        sizeof(Type) );

    if( m_pArrayBuffer )
    {
        SafeMemSet( m_pArrayBuffer, 0, (maxSize * sizeof(Type)) );

        m_ActualSize = maxSize;
    }
    else
    {
        m_ActualSize = 0;
    }
}

/*****************************************************************************\

Function:
    CStaticArray Destructor

Description:
    Frees all internal dynamic memory

Input:
    none

Output:
    none

\*****************************************************************************/
template<ArrayTemplateList>
CStaticArrayType::~CStaticArray( void )
{
    if( m_pArrayBuffer )
    {
        CAllocatorType::AlignedDeallocate( m_pArrayBuffer );
        m_pArrayBuffer = NULL;
    }

    m_ActualSize = 0;
}

/*****************************************************************************\

Function:
    CStaticArray::GetElement

Description:
    Returns the element at the index in the array

Input:
    const DWORD index - index of element to reference

Output:
    Type - value of element in array

\*****************************************************************************/
template<ArrayTemplateList>
Type CStaticArrayType::GetElement( const DWORD index ) const
{
    Type element;

    if( m_pArrayBuffer && ( index < m_ActualSize ) )
    {
        element = m_pArrayBuffer[ index ];
    }
    else
    {
        ASSERT(0);
        SafeMemSet( &element, 0, sizeof(Type) );
    }

    return element;
}

/*****************************************************************************\

Function:
    CStaticArray::GetElementUnsafe

Description:
    Returns the element at the index in the array

    Note that function does not check if index is in range! 

Input:
    const DWORD index - index of element to reference

Output:
    Type - value of element in array

\*****************************************************************************/
template<ArrayTemplateList>
Type CStaticArrayType::GetElementUnsafe( const DWORD index ) const
{
    ASSERT( m_pArrayBuffer && ( index < m_ActualSize ) );
    return m_pArrayBuffer[ index ];
}

/*****************************************************************************\

Function:
    CStaticArray::SetElement

Description:
    Sets the element at the index in the array to the given element

Input:
    const DWORD index - index of element to reference
    const Type& element - value of element to set

Output:
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<ArrayTemplateList>
bool CStaticArrayType::SetElement( const DWORD index, const Type& element )
{
    bool success = false;

    if( m_pArrayBuffer && ( index < m_ActualSize ) )
    {
        m_pArrayBuffer[ index ] = element;
        success = true;
    }

    ASSERT( success );
    return success;
}

/*****************************************************************************\

Function:
    CStaticArray::SetElementUnsafe

Description:
    Sets the element at the index in the array to the given element

    Note that function does not check if index is in range! 

Input:
    const DWORD index - index of element to reference
    const Type& element - value of element to set

Output:
    None

\*****************************************************************************/
template<ArrayTemplateList>
void CStaticArrayType::SetElementUnsafe( const DWORD index, const Type& element )
{
    ASSERT( m_pArrayBuffer && ( index < m_ActualSize ) );
    m_pArrayBuffer[ index ] = element;
}

/*****************************************************************************\

Function:
    CStaticArray::GetSize

Description:
    Returns the current number of elements in the array

Input:
    void

Output:
    DWORD - size of the array in elements

\*****************************************************************************/
template<ArrayTemplateList>
DWORD CStaticArrayType::GetSize( void ) const
{
    return m_ActualSize;
}

/*****************************************************************************\

Function:
    CStaticArray::Clear

Description:
    Clears the array

Input:
    none

Output:
    none

\*****************************************************************************/
template<ArrayTemplateList>
void CStaticArrayType::Clear( void )
{
    SafeMemSet( m_pArrayBuffer, 0, m_ActualSize * sizeof( Type ) );
}

/*****************************************************************************\

Function:
    CStaticArray::DebugPrint

Description:
    Prints the array to std output for debug only

Input:
    void

Output:
    void

\*****************************************************************************/
template<ArrayTemplateList>
void CStaticArrayType::DebugPrint( void ) const
{
#ifdef _DEBUG
    DPF( GFXDBG_STDLIB, "%s\n", __FUNCTION__ );
    DPF( GFXDBG_STDLIB, "\tAddress = %p\n", this );

    if( m_pArrayBuffer )
    {
        for( DWORD i = 0; i < GetSize(); i++ )
        {
            Type element = GetElement(i);
            DPF( GFXDBG_STDLIB, "\t\tElement[%u] = 0x%08x\n",
                i,
                *(DWORD*)&element );
        }
    }
#endif
}

/*****************************************************************************\

Function:
    CStaticArray::operator=

Description:
    Equal operator to copy an array

Input:
    const CStaticArrayType& array - array to copy

Output:
    *this

\*****************************************************************************/
template<ArrayTemplateList>
CStaticArrayType& CStaticArrayType::operator= ( const CStaticArrayType &array )
{
    if( m_pArrayBuffer && array.m_pArrayBuffer )
    {
        const DWORD copySize = Min( m_ActualSize, array.m_ActualSize );
        MemCopy( m_pArrayBuffer, array.m_pArrayBuffer, copySize * sizeof( Type ) );
    }

    return *this;
}

/*****************************************************************************\

Class:
    CDynamicArray

Description:
    Implements a dynamically-sized array

\*****************************************************************************/
template<ArrayTemplateList>
class CDynamicArray : public CObject<CAllocatorType>
{
public:

    CDynamicArray( const DWORD initSize );
    CDynamicArray( const DWORD initSize, const DWORD actualSize );
    virtual ~CDynamicArray( void );

    Type    GetElement( const DWORD index ) const;
    bool    SetElement( const DWORD index, const Type& element );
    bool    Insert( const DWORD index, const Type& element );

    Type    GetElementUnsafe( const DWORD index ) const;

    const Type& GetElementReference( const DWORD index) const;
    //when the index request in GetELementReference does not exist a
    //reference to m_elementNoExist is returned instead.
    //why is it not const? because that would force it to be an l-value in the
    //constructor but because it is a template type there is no universal value.
    Type m_elementNoExist;

    DWORD   GetSize( void ) const;
    bool    Resize( const DWORD size );
    bool    Shrink( const DWORD size );

    void    PreAllocate( const DWORD size );

    void    Delete( void );
    void    Clear( void );

    void    DebugPrint( void ) const;

    CDynamicArrayType& operator= ( const CDynamicArrayType &array );

    C_ASSERT( IsArrayTypeSupported<Type>::value == true );

protected:

    virtual void    CreateArray( const DWORD size );
    void    DeleteArray( void );

    DWORD   GetMaxSize( void ) const;
    bool    IsValidIndex( const DWORD index ) const;

    Type*   m_pArrayBuffer;

    DWORD   m_UsedSize;
    DWORD   m_ActualSize;

    DECL_DEBUG_MUTEX( m_InstanceNotThreadSafe )
};

/*****************************************************************************\

Function:
    CDynamicArray Constructor

Description:
    Initializes the array

Input:
    const DWORD initSize - initial size of the array, in elements

Output:
    none

\*****************************************************************************/
template<ArrayTemplateList>
CDynamicArrayType::CDynamicArray( const DWORD initSize )
    : CObject<CAllocatorType>()
{
    m_pArrayBuffer = NULL;

    m_UsedSize = 0;
    m_ActualSize = 0;

    //can't use the zero initializer, {0}, on template types
    memset((void*)&m_elementNoExist, 0, sizeof(Type));

    CreateArray( initSize );

    INIT_DEBUG_MUTEX( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CDynamicArray Destructor

Description:
    Frees all internal dynamic memory

Input:
    none

Output:
    none

\*****************************************************************************/
template<ArrayTemplateList>
CDynamicArrayType::~CDynamicArray( void )
{
    Delete();

    DELETE_DEBUG_MUTEX( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CDynamicArray::GetElement

Description:
    Returns the element at the index in the array

Input:
    const DWORD index - index of element to reference

Output:
    Type - value of element in array

\*****************************************************************************/
template<ArrayTemplateList>
Type CDynamicArrayType::GetElement( const DWORD index ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    Type element;

    if( m_pArrayBuffer && IsValidIndex( index ) )
    {
        element = m_pArrayBuffer[ index ];
    }
    else
    {
        SafeMemSet( &element, 0, sizeof(Type) );
    }

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return element;
}

/*****************************************************************************\

Function:
    CDynamicArray::GetElementUnsafe

Description:
    Returns the element at the index in the array

    Note that function does not check if index is in range! 

Input:
    const DWORD index - index of element to reference

Output:
    Type - value of element in array

\*****************************************************************************/
template<ArrayTemplateList>
Type CDynamicArrayType::GetElementUnsafe( const DWORD index ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    ASSERT( m_pArrayBuffer && IsValidIndex( index ) );

    Type element = m_pArrayBuffer[ index ];

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
    return element;
}

/*****************************************************************************\

Function:
    CDynamicArray::GetElementReference

Description:
    Returns the a const reference to the element at the index in the array. Do
    not attempt to retain pointers to the reference returned as it will be
    invalid after a resize operation which can happen automatically.

Input:
    const DWORD index - index of element to reference

Output:
    const Type* - reference to the element in the array. returns NULL if
                  element not found.

\*****************************************************************************/
template<ArrayTemplateList>
const Type& CDynamicArrayType::GetElementReference( const DWORD index ) const
{
    ACQUIRE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );

    if( m_pArrayBuffer && IsValidIndex( index ) )
    {
        return m_pArrayBuffer[ index ];
    }
    else
    {
        //returning the zeroed out structure is not the greatest, but the best
        //that can be done in this situation. when in debug builds the assert
        //will help point out that something bad is going on.
        ASSERT(m_pArrayBuffer && IsValidIndex( index ));
        return m_elementNoExist;
    }

    RELEASE_DEBUG_MUTEX_READ( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CDynamicArray::SetElement

Description:
    Sets the element at the index in the array to the given element

Input:
    const DWORD index - index of element to reference
    const Type& element - value of element to set

Output:
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<ArrayTemplateList>
bool CDynamicArrayType::SetElement( const DWORD index, const Type& element )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    bool success = false;

    // If the index is larger than the size of the array then grow the array
    if( !IsValidIndex( index ) )
    {
        CreateArray( index + 1 );
    }

    if( m_pArrayBuffer && IsValidIndex( index ) )
    {
        m_pArrayBuffer[ index ] = element;
        success = true;
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    ASSERT( success );
    return success;
}

/*****************************************************************************\

Function:
    CDynamicArray::Insert

Description:
    Sets the element at the index in the array to the given element and shifts 
    successors.

Input:
    const DWORD index - index at which the element should be inserted
    const Type& element - value of element to insert

Output:
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<ArrayTemplateList>
bool CDynamicArrayType::Insert( const DWORD index, const Type& element )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    const DWORD oldSize = GetSize();

    // Grow the array.
    if( index < oldSize )
    {
        CreateArray( oldSize + 1 );
        SafeMemMove( m_pArrayBuffer + index + 1, m_pArrayBuffer + index, ( oldSize - index )*sizeof( Type ) );
    }
    else
    {
        CreateArray( index + 1 );
    }

    bool success = SetElement( index, element );

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    ASSERT( success );
    return success;
}

/*****************************************************************************\

Function:
    CDynamicArray::GetSize

Description:
    Returns the current number of elements in the array

Input:
    void

Output:
    DWORD - size of the array in elements

\*****************************************************************************/
template<ArrayTemplateList>
DWORD CDynamicArrayType::GetSize( void ) const
{
    const DWORD size = m_UsedSize;
    return size;
}

/*****************************************************************************\

Function:
    CDynamicArray::Resize

Description:
    Resize the array

Input:
    const DWORD size - new size for the array

Output:
    bool

\*****************************************************************************/
template<ArrayTemplateList>
bool CDynamicArrayType::Resize( const DWORD size )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    bool success = true;

    const DWORD allocSize = size * sizeof( Type );

    // TODO: add Reallocate function to CAllocator
    Type* pArrayBuffer = (Type*)CAllocatorType::Allocate( allocSize );

    if( pArrayBuffer )
    {
        SafeMemSet( pArrayBuffer, 0, allocSize );

        if( m_pArrayBuffer )
        {
            // copy the old array to the new array
            const DWORD copySize = Min( size, m_ActualSize );
            MemCopy( pArrayBuffer, m_pArrayBuffer, copySize * sizeof( Type ) );
            DeleteArray();
        }

        m_pArrayBuffer = pArrayBuffer;
        m_ActualSize = size;
        m_UsedSize = size;
    }
    else
    {
        success = false;
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    return success;
}

/*****************************************************************************\

Function:
    CDynamicArray::Shrink

Description:
    Shrink the array

Input:
    const DWORD size - new size for the array

Output:
    bool

\*****************************************************************************/
template<ArrayTemplateList>
bool CDynamicArrayType::Shrink( const DWORD size )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    bool success = false;
    if( size < m_UsedSize )
    {
        success = true;
        m_UsedSize = size;
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    return success;
}

/*****************************************************************************\

Function:
    CDynamicArray::Delete

Description:
    Deletes the internal data

Input:
    void

Output:
    void

\*****************************************************************************/
template<ArrayTemplateList>
void CDynamicArrayType::Delete( void )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    DeleteArray();
    m_UsedSize = 0;

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CDynamicArray::Clear

Description:
    Zeros the internal data

Input:
    void

Output:
    void

\*****************************************************************************/
template<ArrayTemplateList>
void CDynamicArrayType::Clear( void )
{
    ACQUIRE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );

    SafeMemSet( m_pArrayBuffer, 0, m_ActualSize * sizeof( Type ) );

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
}

/*****************************************************************************\

Function:
    CDynamicArray::DebugPrint

Description:
    Prints the array to std output for debug only

Input:
    void

Output:
    void

\*****************************************************************************/
template<ArrayTemplateList>
void CDynamicArrayType::DebugPrint( void ) const
{
#ifdef _DEBUG
    DPF( GFXDBG_STDLIB, "%s\n", __FUNCTION__ );
    DPF( GFXDBG_STDLIB, "\tAddress = %p\n", this );

    if( m_pArrayBuffer )
    {
        for( DWORD i = 0; i < GetSize(); i++ )
        {
            Type element = GetElement(i);
            DPF( GFXDBG_STDLIB, "\t\tElement[%u] = 0x%08x\n",
                i,
                *(DWORD*)&element );
        }
    }
#endif
}

/*****************************************************************************\

Function:
    CDynamicArray::operator=

Description:
    Equal operator to copy an array

Input:
    const CDynamicArrayType& array - array to copy

Output:
    *this

\*****************************************************************************/
template<ArrayTemplateList>
CDynamicArrayType& CDynamicArrayType::operator= ( const CDynamicArrayType &array )
{
    ACQUIRE_DEBUG_MUTEX_WRITE (m_InstanceNotThreadSafe);

    // TO DO:
    // Not all cases guarantee to be l=r after assignment:
    //
    // 1. array.m_pArrayBuffer == NULL
    // Probably lack of:
    // m_UsedSize = 0;
    //
    // 2. array.m_UsedSize == 0
    // if (m_UsedSize < 0) -> always false, CreateArray not called
    // Probably lack of:
    // m_UsedSize = 0;
    //
    // 3. m_UsedSize >= array.m_UsedSize
    // First 'if' is false, CreateArray not called
    // Probably lack of:
    // m_UsedSize = array.m_UsedSize;

    if ( array.m_pArrayBuffer )
    {
        if (m_UsedSize < array.m_UsedSize)
        {
            CreateArray( array.m_UsedSize );
        }

        if (m_pArrayBuffer && (m_UsedSize >= array.m_UsedSize))
        {
            MemCopy (m_pArrayBuffer, array.m_pArrayBuffer, array.m_UsedSize*sizeof(Type));
        }
    }

    RELEASE_DEBUG_MUTEX_WRITE( m_InstanceNotThreadSafe );
    return *this;
}

/*****************************************************************************\

Function:
    CDynamicArray::CreateArray

Description:
    Creates the internal array structure of the specified size

Input:
    const DWORD size - number of elements

Output:
    void

\*****************************************************************************/
template<ArrayTemplateList>
void CDynamicArrayType::CreateArray( const DWORD size )
{
    if( size )
    {
        if( size > GetMaxSize() )
        {
            // Grow the array exponentially
            DWORD actualSize = GetMaxSize() * 2;

            if( size > actualSize )
            {
                // The minimum allocation size is 32 elements, and
                // the allocations size is in multiples of 32 elements
                actualSize = Round( Max<DWORD>( size, 32 ), 32 );
            }

            ASSERT( actualSize >= size );
            ASSERT( actualSize > m_ActualSize );

            const DWORD allocSize = actualSize * sizeof(Type);

            Type* pArrayBuffer = (Type*)CAllocatorType::Allocate( allocSize );

            if( pArrayBuffer )
            {
                SafeMemSet( pArrayBuffer, 0, allocSize );
                if( m_pArrayBuffer )
                {
                    MemCopy( pArrayBuffer, m_pArrayBuffer, m_UsedSize * sizeof(Type) );
                    DeleteArray();
                }

                m_pArrayBuffer = pArrayBuffer;
                m_ActualSize = actualSize;
                m_UsedSize = size;
            }
        }
        else
        {
            // Update the array length
            m_UsedSize = size;
        }
    }
}

/*****************************************************************************\

Function:
    CDynamicArray::PreAllocate

Description:
    Pre-Allocates more space

Input:
    const DWORD size - number of elements

Output:
    void

\*****************************************************************************/
template<ArrayTemplateList>
void CDynamicArrayType::PreAllocate( const DWORD size )
{
    if( size && ( size > GetMaxSize() ) )
    {
        // Grow the array exponentially
        DWORD actualSize = GetMaxSize() * 2;

        if( size > actualSize )
        {
            // The minimum allocation size is 32 elements, and
            // the allocations size is in multiples of 32 elements
            actualSize = Round( Max<DWORD>( size, 32 ), 32 );
        }

        ASSERT( actualSize >= size );
        ASSERT( actualSize > m_ActualSize );

        const DWORD allocSize = actualSize * sizeof(Type);

        Type* pArrayBuffer = (Type*)CAllocatorType::Allocate( allocSize );

        if( pArrayBuffer )
        {
            SafeMemSet( pArrayBuffer, 0, allocSize );
            if( m_pArrayBuffer )
            {
                MemCopy( pArrayBuffer, m_pArrayBuffer, m_UsedSize * sizeof(Type) );
                DeleteArray();
            }

            m_pArrayBuffer = pArrayBuffer;
            m_ActualSize = actualSize;
        }
    }
}

/*****************************************************************************\

Function:
    CDynamicArray::DeleteArray

Description:
    Deletes the internal array structure

Input:
    void

Output:
    void

\*****************************************************************************/
template<ArrayTemplateList>
void CDynamicArrayType::DeleteArray( void )
{
    if( m_pArrayBuffer )
    {
        CAllocatorType::Deallocate( m_pArrayBuffer );
        m_pArrayBuffer = NULL;
    }

    m_ActualSize = 0;
}

/*****************************************************************************\

Function:
    CDynamicArray::GetMaxSize

Description:
    Returns the maximum number of elements in the array

Input:
    void

Output:
    DWORD length

\*****************************************************************************/
template<ArrayTemplateList>
DWORD CDynamicArrayType::GetMaxSize( void ) const
{
    return m_ActualSize;
}

/*****************************************************************************\

Function:
    CDynamicArray::IsValidIndex

Description:
    Determines if the index is in the array

Input:
    const DWORD index

Output:
    bool

\*****************************************************************************/
template<ArrayTemplateList>
bool CDynamicArrayType::IsValidIndex( const DWORD index ) const
{
    return ( index < GetSize() );
}

/*****************************************************************************\

Class:
    CArray

Description:
    Implements a dynamically-sized array, with "free-index" tracking

\*****************************************************************************/
template<ArrayTemplateList>
class CArray : public CDynamicArray<Type,CAllocatorType>
{
public:

    CArray( const DWORD initSize );
    virtual ~CArray( void );

    bool    SetElement( const DWORD index, const Type& element );

    bool    Resize( const DWORD size );

    void    SetFreeIndex( const DWORD index );
    DWORD   GetFreeIndex( void );

    void    Delete( void );

protected:

    virtual void    CreateArray( const DWORD size );

    typedef CQueue<DWORD,CAllocatorType>    CFreeIndexQueue;

    void    DeleteFreeIndexQueue( void );

    CFreeIndexQueue*    m_pFreeIndexQueue;
    DWORD               m_FreeIndex;
};

/*****************************************************************************\

Function:
    CArray Constructor

Description:
    Initializes the array

Input:
    const DWORD initSize - initial size of the array, in elements

Output:
    none

\*****************************************************************************/
template<ArrayTemplateList>
CArrayType::CArray( const DWORD initSize )
    : CDynamicArrayType( initSize )
{
    m_pFreeIndexQueue = NULL;
    m_FreeIndex = 0;
}

/*****************************************************************************\

Function:
    CArray Destructor

Description:
    Frees all internal dynamic memory

Input:
    none

Output:
    none

\*****************************************************************************/
template<ArrayTemplateList>
CArrayType::~CArray( void )
{
    DeleteFreeIndexQueue();
}

/*****************************************************************************\

Function:
    CArray::SetElement

Description:
    Sets the element at the index in the array to the given element

Input:
    const DWORD index - index of element to reference
    const Type& element - value of element to set

Output:
    bool - SUCCESS or FAIL

\*****************************************************************************/
template<ArrayTemplateList>
bool CArrayType::SetElement( const DWORD index, const Type& element )
{
    bool success = CDynamicArrayType::SetElement( index, element );

    if( success )
    {
        m_FreeIndex = Max( index+1, m_FreeIndex );

#ifdef _DEBUG
        if( m_pFreeIndexQueue != NULL )
        {
            ASSERT( m_pFreeIndexQueue->Find( index ) == m_pFreeIndexQueue->End() );
        }
#endif
    }

    return success;
}

/*****************************************************************************\

Function:
    CArray::Resize

Description:
    Resize the array

Input:
    const DWORD size - new size for the array

Output:
    bool

\*****************************************************************************/
template<ArrayTemplateList>
bool CArrayType::Resize( const DWORD size )
{
    bool success = CDynamicArrayType::Resize( size );

    if( success )
    {
        DeleteFreeIndexQueue();
    }

    return success;
}

/*****************************************************************************\

Function:
    CArray::SetFreeIndex

Description:
    Sets the index as unused by the client

Input:
    const DWORD index - index of entry no longer used

Output:
    none

\*****************************************************************************/
template<ArrayTemplateList>
void CArrayType::SetFreeIndex( const DWORD index )
{
    if( !m_pFreeIndexQueue )
    {
        m_pFreeIndexQueue = new CFreeIndexQueue();
        ASSERT( m_pFreeIndexQueue );
    }

    if( m_pFreeIndexQueue )
    {
        m_pFreeIndexQueue->Push( index );
    }
}

/*****************************************************************************\

Function:
    CArray::GetFreeIndex

Description:
    Returns the index of unused entry

Input:
    void

Output:
    DWORD index

\*****************************************************************************/
template<ArrayTemplateList>
DWORD CArrayType::GetFreeIndex( void )
{
    DWORD index = 0;

    if( m_pFreeIndexQueue && !m_pFreeIndexQueue->IsEmpty() )
    {
        index = m_pFreeIndexQueue->Pop();
        ASSERT( index < m_FreeIndex );
    }
    else
    {
        index = m_FreeIndex++;

        if( m_pFreeIndexQueue )
        {
             ASSERT( ( m_pFreeIndexQueue->Find( index ) == m_pFreeIndexQueue->End() ) );
        }
    }

    ASSERT( index <= this->GetSize() );
    return index;
}


/*****************************************************************************\

Function:
    CArray::Delete

Description:
    Deletes the internal data

Input:
    void

Output:
    void

\*****************************************************************************/
template<ArrayTemplateList>
void CArrayType::Delete( void )
{
    CDynamicArrayType::Delete();

    DeleteFreeIndexQueue();
    m_FreeIndex = 0;
}

/*****************************************************************************\

Function:
    CArray::CreateArray

Description:
    Creates the internal array structure of the specified size

Input:
    const DWORD size - number of elements

Output:
    void

\*****************************************************************************/
template<ArrayTemplateList>
void CArrayType::CreateArray( const DWORD size )
{
    CDynamicArrayType::CreateArray( size );
}

/*****************************************************************************\

Function:
    CArray::DeleteFreeIndexQueue

Description:
    Deletes the internal free index queue

Input:
    void

Output:
    void

\*****************************************************************************/
template<ArrayTemplateList>
void CArrayType::DeleteFreeIndexQueue( void )
{
    if( m_pFreeIndexQueue )
    {
        while( !m_pFreeIndexQueue->IsEmpty() )
        {
            m_pFreeIndexQueue->Pop();
        }

        SafeDelete( m_pFreeIndexQueue );
    }
}

} // iSTD
