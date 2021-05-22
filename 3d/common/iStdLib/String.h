/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#if defined _WIN32
#   pragma warning( push )
#   pragma warning( disable : 4996 )
#endif

#include "utility.h"
#include "Debug.h"
#include "Object.h"
#include "MemCopy.h"

#include <stdarg.h>
#include <stdio.h>

namespace iSTD
{

/*****************************************************************************\

Class:
    CString

Description:
    Abstraction class for string manipulation

\*****************************************************************************/
template<class CAllocatorType>
class CString : public CObject<CAllocatorType>
{
public:

    CString( void );
    CString( size_t growSize );
    CString( const CString& str );
    CString( const char* str );
    virtual ~CString( void );

    operator const char* () const;

    bool        operator == ( const CString& str );
    bool        operator == ( const char* str );

    CString&    operator = ( const CString& str );
    CString&    operator = ( const char* str );

    CString&    operator = ( const bool val );
    CString&    operator = ( const char val );
    CString&    operator = ( const short val );
    CString&    operator = ( const int val );
    CString&    operator = ( const long val );
    CString&    operator = ( const float val );
    CString&    operator = ( const unsigned char val );
    CString&    operator = ( const unsigned short val );
    CString&    operator = ( const unsigned int val );
    CString&    operator = ( const unsigned long val );

    const CString    operator + ( const CString& str ) const;
    const CString    operator + ( const char* str ) const;

    const CString    operator + ( const bool val ) const;
    const CString    operator + ( const char val ) const;
    const CString    operator + ( const short val ) const;
    const CString    operator + ( const int val ) const;
    const CString    operator + ( const long val ) const;
    const CString    operator + ( const float val ) const;
    const CString    operator + ( const unsigned char val ) const;
    const CString    operator + ( const unsigned short val ) const;
    const CString    operator + ( const unsigned int val ) const;
    const CString    operator + ( const unsigned long val ) const;

    void        operator += ( const CString& str );
    void        operator += ( const char* str );

    void        operator += ( const bool val );
    void        operator += ( const char val );
    void        operator += ( const short val );
    void        operator += ( const int val );
    void        operator += ( const long val );
    void        operator += ( const float val );
    void        operator += ( const unsigned char val );
    void        operator += ( const unsigned short val );
    void        operator += ( const unsigned int val );
    void        operator += ( const unsigned long val );

    void        Set( const char* str );
    void        SetFormatted( const char* str, ... );

    void        Append( const char* str );
    void        AppendFormatted( const char* str, ... );

    size_t      Length( void ) const;
    bool        IsEmpty( void ) const;

    void        DetachBuffer( void );

protected:

    char*       m_pString;
    size_t      m_Length;
    size_t      m_BufferSize;
    size_t      m_GrowSize;

    static const size_t s_cStringGrowSize = 128;
};

/*****************************************************************************\

Function:
    CString Constructor

Description:
    Initializes internal data

Input:
    none

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline CString<CAllocatorType>::CString( void )
    : CObject<CAllocatorType>()
{
    m_pString = NULL;
    m_Length = 0;
    m_BufferSize = 0;
    m_GrowSize = s_cStringGrowSize;
}

/*****************************************************************************\

Function:
    CString Constructor

Description:
    Initializes internal data

Input:
    size_t growSize

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline CString<CAllocatorType>::CString( size_t growSize )
    : CObject<CAllocatorType>()
{
    m_pString = NULL;
    m_Length = 0;
    m_BufferSize = 0;
    m_GrowSize = growSize;
}

template<class CAllocatorType>
inline CString<CAllocatorType>::CString( const CString<CAllocatorType>& str )
    : CObject<CAllocatorType>()
{
    m_pString = NULL;
    m_Length = 0;
    m_GrowSize = str.m_GrowSize;
    Set( str.m_pString );
}

template<class CAllocatorType>
inline CString<CAllocatorType>::CString( const char* str )
    : CObject<CAllocatorType>()
{
    m_pString = NULL;
    m_Length = 0;
    m_GrowSize = s_cStringGrowSize;
    Set( str );
}

/*****************************************************************************\

Function:
    CString Destructor

Description:
    Deletes internal data

Input:
    none

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline CString<CAllocatorType>::~CString( void )
{
    CAllocatorType::Deallocate( m_pString );
    m_pString = NULL;
    m_BufferSize = 0;
}

/*****************************************************************************\

Function:
    CString::operator const char* ()

Description:
    Casts CString object to const char*

Input:
    none

Output:
    const char*

\*****************************************************************************/
template<class CAllocatorType>
inline CString<CAllocatorType>::operator const char* () const
{
    return m_pString;
}

/*****************************************************************************\

Function:
    CString::operator ==

Description:
    Compares a CString to this CString

Input:
    const CString& str

Output:
    bool

\*****************************************************************************/
template<class CAllocatorType>
inline bool CString<CAllocatorType>::operator == (
    const CString<CAllocatorType>& str )
{
    if( str && m_pString )
    {
        return ( ::strcmp( m_pString, (const char*)str ) == 0 );
    }
    else
    {
        return false;
    }
}

/*****************************************************************************\

Function:
    CString::operator ==

Description:
    Compares a char* to this CString

Input:
    const char* str

Output:
    bool

\*****************************************************************************/
template<class CAllocatorType>
inline bool CString<CAllocatorType>::operator == (
    const char* str )
{
    if( str && m_pString )
    {
        return ( ::strcmp( m_pString, str ) == 0 );
    }
    else
    {
        return false;
    }
}

/*****************************************************************************\

Function:
    CString::operator =

Description:
    Sets a CString to this CString

Input:
    const CString& str

Output:
    CString&

\*****************************************************************************/
template<class CAllocatorType>
inline CString<CAllocatorType>& CString<CAllocatorType>::operator = (
    const CString<CAllocatorType>& str )
{
    if( str )
    {
        Set( (const char*)str );
    }

    return *this;
}

/*****************************************************************************\

Function:
    CString::operator =

Description:
    Sets a char* to this CString

Input:
    const char* str

Output:
    CString&

\*****************************************************************************/
template<class CAllocatorType>
inline CString<CAllocatorType>& CString<CAllocatorType>::operator = (
    const char* str )
{
    Set( str );
    return *this;
}

/*****************************************************************************\

Function:
    CString::operator =

Description:
    Sets a bool to this CString

Input:
    const bool val

Output:
    CString&

\*****************************************************************************/
template<class CAllocatorType>
inline CString<CAllocatorType>& CString<CAllocatorType>::operator = (
    const bool val )
{
    SetFormatted( "%d", val );
    return *this;
}

/*****************************************************************************\

Function:
    CString::operator =

Description:
    Sets a char to this CString

Input:
    const char val

Output:
    CString&

\*****************************************************************************/
template<class CAllocatorType>
inline CString<CAllocatorType>& CString<CAllocatorType>::operator = (
    const char val )
{
    SetFormatted( "%d", val );
    return *this;
}

/*****************************************************************************\

Function:
    CString::operator =

Description:
    Sets a short to this CString

Input:
    const short val

Output:
    CString&

\*****************************************************************************/
template<class CAllocatorType>
inline CString<CAllocatorType>& CString<CAllocatorType>::operator = (
    const short val )
{
    SetFormatted( "%d", val );
    return *this;
}

/*****************************************************************************\

Function:
    CString::operator =

Description:
    Sets a int to this CString

Input:
    const int val

Output:
    CString&

\*****************************************************************************/
template<class CAllocatorType>
inline CString<CAllocatorType>& CString<CAllocatorType>::operator = (
    const int val )
{
    SetFormatted( "%d", val );
    return *this;
}

/*****************************************************************************\

Function:
    CString::operator =

Description:
    Sets a long to this CString

Input:
    const long val

Output:
    CString&

\*****************************************************************************/
template<class CAllocatorType>
inline CString<CAllocatorType>& CString<CAllocatorType>::operator = (
    const long val )
{
    SetFormatted( "%d", val );
    return *this;
}

/*****************************************************************************\

Function:
    CString::operator =

Description:
    Sets a float to this CString

Input:
    const float val

Output:
    CString&

\*****************************************************************************/
template<class CAllocatorType>
inline CString<CAllocatorType>& CString<CAllocatorType>::operator = (
    const float val )
{
    double  d = val;
    SetFormatted( "%f", d );
    return *this;
}

/*****************************************************************************\

Function:
    CString::operator =

Description:
    Sets a unsigned char to this CString

Input:
    const unsigned char val

Output:
    CString&

\*****************************************************************************/
template<class CAllocatorType>
inline CString<CAllocatorType>& CString<CAllocatorType>::operator = (
    const unsigned char val )
{
    SetFormatted( "%u", val );
    return *this;
}

/*****************************************************************************\

Function:
    CString::operator =

Description:
    Sets a unsigned short to this CString

Input:
    const unsigned short val

Output:
    CString&

\*****************************************************************************/
template<class CAllocatorType>
inline CString<CAllocatorType>& CString<CAllocatorType>::operator = (
    const unsigned short val )
{
    SetFormatted( "%u", val );
    return *this;
}

/*****************************************************************************\

Function:
    CString::operator =

Description:
    Sets a unsigned int to this CString

Input:
    const unsigned int val

Output:
    CString&

\*****************************************************************************/
template<class CAllocatorType>
inline CString<CAllocatorType>& CString<CAllocatorType>::operator = (
    const unsigned int val )
{
    SetFormatted( "%u", val );
    return *this;
}

/*****************************************************************************\

Function:
    CString::operator =

Description:
    Sets a unsigned long to this CString

Input:
    const unsigned long val

Output:
    CString&

\*****************************************************************************/
template<class CAllocatorType>
inline CString<CAllocatorType>& CString<CAllocatorType>::operator = (
    const unsigned long val )
{
    SetFormatted( "%u", val );
    return *this;
}

/*****************************************************************************\

Function:
    CString::operator +

Description:
    Returns concatenation of two CStrings

Input:
    const CString& str

Output:
    const CString

\*****************************************************************************/
template<class CAllocatorType>
inline const CString<CAllocatorType> CString<CAllocatorType>::operator + (
    const CString<CAllocatorType>& str ) const
{
    CString<CAllocatorType> res( *this );

    if( str )
    {
        res.Append( (const char*)str );
    }

    return res;
}

/*****************************************************************************\

Function:
    CString::operator +

Description:
    Returns concatenation of CString and char*

Input:
    const char* str

Output:
    const CString

\*****************************************************************************/
template<class CAllocatorType>
inline const CString<CAllocatorType> CString<CAllocatorType>::operator + (
    const char* str ) const
{
    CString<CAllocatorType> res( *this );

    res.Append( str );

    return res;
}

/*****************************************************************************\

Function:
    CString::operator +

Description:
    Returns concatenation of CString and bool

Input:
    const bool val

Output:
    const CString

\*****************************************************************************/
template<class CAllocatorType>
inline const CString<CAllocatorType> CString<CAllocatorType>::operator + (
    const bool val ) const
{
    CString<CAllocatorType> res( *this );

    res.AppendFormatted( "%d", val );

    return res;
}

/*****************************************************************************\

Function:
    CString::operator +

Description:
    Returns concatenation of CString and char

Input:
    const char val

Output:
    const CString

\*****************************************************************************/
template<class CAllocatorType>
inline const CString<CAllocatorType> CString<CAllocatorType>::operator + (
    const char val ) const
{
    CString<CAllocatorType> res( *this );

    res.AppendFormatted( "%d", val );

    return res;
}

/*****************************************************************************\

Function:
    CString::operator +

Description:
    Returns concatenation of CString and short

Input:
    const short val

Output:
    const CString

\*****************************************************************************/
template<class CAllocatorType>
inline const CString<CAllocatorType> CString<CAllocatorType>::operator + (
    const short val ) const
{
    CString<CAllocatorType> res( *this );

    res.AppendFormatted( "%d", val );
    return res;
}

/*****************************************************************************\

Function:
    CString::operator +

Description:
    Returns concatenation of CString and int

Input:
    const int val

Output:
    const CString

\*****************************************************************************/
template<class CAllocatorType>
inline const CString<CAllocatorType> CString<CAllocatorType>::operator + (
    const int val ) const
{
    CString<CAllocatorType> res( *this );

    res.AppendFormatted( "%d", val );

    return res;
}

/*****************************************************************************\

Function:
    CString::operator +

Description:
    Returns concatenation of CString and long

Input:
    const long val

Output:
    const CString

\*****************************************************************************/
template<class CAllocatorType>
inline const CString<CAllocatorType> CString<CAllocatorType>::operator + (
    const long val ) const
{
    CString<CAllocatorType> res( *this );

    res.AppendFormatted( "%d", val );

    return res;
}

/*****************************************************************************\

Function:
    CString::operator +

Description:
    Returns concatenation of CString and float

Input:
    const float val

Output:
    const CString

\*****************************************************************************/
template<class CAllocatorType>
inline const CString<CAllocatorType> CString<CAllocatorType>::operator + (
    const float val ) const
{
    CString<CAllocatorType> res( *this );

    double  d = val;
    res.AppendFormatted( "%f", d );

    return res;
}

/*****************************************************************************\

Function:
    CString::operator +

Description:
    Returns concatenation of CString and unsigned char
Input:
    const unsigned char val

Output:
    const CString

\*****************************************************************************/
template<class CAllocatorType>
inline const CString<CAllocatorType> CString<CAllocatorType>::operator + (
    const unsigned char val ) const
{
    CString<CAllocatorType> res( *this );

    res.AppendFormatted( "%u", val );

    return res;
}

/*****************************************************************************\

Function:
    CString::operator +

Description:
    Returns concatenation of CString and unsigned short

Input:
    const unsigned short val

Output:
    const CString

\*****************************************************************************/
template<class CAllocatorType>
inline const CString<CAllocatorType> CString<CAllocatorType>::operator + (
    const unsigned short val ) const
{
    CString<CAllocatorType> res( *this );

    res.AppendFormatted( "%u", val );

    return res;
}

/*****************************************************************************\

Function:
    CString::operator +

Description:
    Returns concatenation of CString and unsigned int

Input:
    const unsigned int val

Output:
    const CString

\*****************************************************************************/
template<class CAllocatorType>
inline const CString<CAllocatorType> CString<CAllocatorType>::operator + (
    const unsigned int val ) const
{
    CString<CAllocatorType> res( *this );

    res.AppendFormatted( "%u", val );

    return res;
}

/*****************************************************************************\

Function:
    CString::operator +

Description:
    Returns concatenation of CString and unsigned long

Input:
    const unsigned long val

Output:
    const CString

\*****************************************************************************/
template<class CAllocatorType>
inline const CString<CAllocatorType> CString<CAllocatorType>::operator + (
    const unsigned long val ) const
{
    CString<CAllocatorType> res( *this );

    res.AppendFormatted( "%u", val );

    return res;
}

/*****************************************************************************\

Function:
    CString::operator +=

Description:
    Appends a CString to this CString

Input:
    const CString& str

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline void CString<CAllocatorType>::operator += (
    const CString<CAllocatorType>& str )
{
    if( str )
    {
        Append( (const char*)str );
    }
}

/*****************************************************************************\

Function:
    CString::operator +=

Description:
    Appends a char* to this CString

Input:
    const char* str

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline void CString<CAllocatorType>::operator += (
    const char* str )
{
    Append( str );
}

/*****************************************************************************\

Function:
    CString::operator +=

Description:
    Appends a bool to this CString

Input:
    const bool val

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline void CString<CAllocatorType>::operator += (
    const bool val )
{
    AppendFormatted( "%d", val );
}

/*****************************************************************************\

Function:
    CString::operator +=

Description:
    Appends a char to this CString

Input:
    const char val

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline void CString<CAllocatorType>::operator += (
    const char val )
{
    AppendFormatted( "%d", val );
}

/*****************************************************************************\

Function:
    CString::operator +=

Description:
    Appends a short to this CString

Input:
    const short val

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline void CString<CAllocatorType>::operator += (
    const short val )
{
    AppendFormatted( "%d", val );
}

/*****************************************************************************\

Function:
    CString::operator +=

Description:
    Appends a int to this CString

Input:
    const int val

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline void CString<CAllocatorType>::operator += (
    const int val )
{
    AppendFormatted( "%d", val );
}

/*****************************************************************************\

Function:
    CString::operator +=

Description:
    Appends a long to this CString

Input:
    const long val

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline void CString<CAllocatorType>::operator += (
    const long val )
{
    AppendFormatted( "%d", val );
}

/*****************************************************************************\

Function:
    CString::operator +=

Description:
    Appends a float to this CString

Input:
    const float val

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline void CString<CAllocatorType>::operator += (
    const float val )
{
    double  d = val;
    AppendFormatted( "%f", d );
}

/*****************************************************************************\

Function:
    CString::operator +=

Description:
    Appends a unsigned char to this CString

Input:
    const unsigned char val

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline void CString<CAllocatorType>::operator += (
    const unsigned char val )
{
    AppendFormatted( "%u", val );
}

/*****************************************************************************\

Function:
    CString::operator +=

Description:
    Appends a unsigned short to this CString

Input:
    const unsigned short val

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline void CString<CAllocatorType>::operator += (
    const unsigned short val )
{
    AppendFormatted( "%u", val );
}

/*****************************************************************************\

Function:
    CString::operator +=

Description:
    Appends a unsigned int to this CString

Input:
    const unsigned int val

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline void CString<CAllocatorType>::operator += (
    const unsigned int val )
{
    AppendFormatted( "%u", val );
}

/*****************************************************************************\

Function:
    CString::operator +=

Description:
    Appends a unsigned long to this CString

Input:
    const unsigned long val

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline void CString<CAllocatorType>::operator += (
    const unsigned long val )
{
    AppendFormatted( "%u", val );
}

/*****************************************************************************\

Function:
    CString::Set

Description:
    Sets the string to an unformatted string

Input:
    const char* str

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
void CString<CAllocatorType>::Set( const char* str )
{
    if( str )
    {
        const size_t length = ::strlen( str ) + 1;

        CAllocatorType::Deallocate( m_pString );
        m_pString = NULL;
        m_BufferSize = 0;

        m_pString = (char*)CAllocatorType::Allocate( length );
        ASSERT( m_pString );

        if( m_pString )
        {
            STRCPY( m_pString, length, str );
            m_Length = ::strlen( m_pString );
            m_BufferSize = length;
        }
    }
}

/*****************************************************************************\

Function:
    CString::SetFormatted

Description:
    Sets the string to a formatted string

Input:
    const char* str
    ...

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
void CString<CAllocatorType>::SetFormatted( const char* str, ... )
{
    if( str )
    {
        va_list args;
        
        va_start( args, str );
        const size_t length = ::_vscprintf( str, args ) + 1;
        va_end( args );
        
        CAllocatorType::Deallocate( m_pString );
        m_pString = NULL;
        m_BufferSize = 0;

        m_pString = (char*)CAllocatorType::Allocate( length );
        ASSERT( m_pString );

        if( m_pString )
        {
            va_start( args, str );
            VSNPRINTF( m_pString, length, length, str, args );
            va_end( args );

            m_Length = ::strlen( m_pString );
            m_BufferSize = length;
        }
    }
}

/*****************************************************************************\

Function:
    CString::Append

Description:
    Appends an unformatted string

Input:
    const char* str

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline void CString<CAllocatorType>::Append( const char* str )
{
    if( str )
    {
        if( m_pString )
        {
            size_t length = m_Length + ::strlen( str ) + 1;
            if( length > m_BufferSize )
            {
                // Once appended, it is likely to be appended again
                // so reserve m_GrowSize bytes extra
                length += m_GrowSize;
                char* temp = (char*)CAllocatorType::Allocate( length );
                ASSERT( temp );

                if( temp )
                {
                    STRCPY( temp, length, m_pString );
                    STRCAT( temp, length, str );
                    CAllocatorType::Deallocate( m_pString );
                    m_pString = temp;
                    m_Length = ::strlen( m_pString );
                    m_BufferSize = length;
                }
            }
            else
            {
                STRCAT( m_pString, m_BufferSize, str );
                m_Length = ::strlen( m_pString );
            }
        }
        else
        {
            Set( str );
        }
    }
}

/*****************************************************************************\

Function:
    CString::AppendFormatted

Description:
    Appends a formatted string

Input:
    const char* str
    ...

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
inline void CString<CAllocatorType>::AppendFormatted( const char* str, ... )
{
    if( str )
    {
        va_list args;
        
        va_start( args, str );
        size_t length = ::_vscprintf( str, args ) + 1;
        va_end( args );
        

        if( m_pString != NULL )
        {                      
            if( (m_Length + length) > m_BufferSize )
            {
                // Once appended, it is likely to be appended again
                // so reserve m_GrowSize bytes extra
                length += m_GrowSize;

                char* temp = (char*)CAllocatorType::Allocate( m_Length + length );
                ASSERT( temp );

                if( temp )
                {
                    MemCopy( temp, m_pString, m_Length );
                    
                    va_start( args, str );
                    VSNPRINTF( temp + m_Length, length, length - m_GrowSize, str, args );
                    va_end( args );
                    
                    char *oldStr = m_pString;
                    m_pString = temp;
                    m_BufferSize = m_Length + length;
                    m_Length += ::strlen( m_pString + m_Length );

                    CAllocatorType::Deallocate( oldStr );
                }
            }
            else
            {
                va_start( args, str );
                VSNPRINTF( m_pString + m_Length, m_BufferSize - m_Length, length, str, args );
                va_end( args );
                
                m_Length += ::strlen( m_pString + m_Length );
            }
        }
        else //if( m_pString != NULL )
        {
            m_BufferSize = 0;
            m_pString = (char*)CAllocatorType::Allocate( length );
            ASSERT( m_pString != NULL );

            if( m_pString )
            {
                va_start( args, str );
                VSNPRINTF( m_pString, length, length, str, args );
                va_end( args );
                
                m_Length = ::strlen( m_pString );
                m_BufferSize = length;
            }
        }
    }
}

/*****************************************************************************\

Function:
    CString::Length

Description:
    Returns the length of the string

Input:
    none

Output:
    size_t

\*****************************************************************************/
template<class CAllocatorType>
size_t CString<CAllocatorType>::Length( void ) const
{
    return m_Length;
}

/*****************************************************************************\

Function:
    CString::IsEmpty

Description:
    Returns true if, and only if, m_Length is 0.

Input:
    none

Output:
    true if string's length is 0, false if string's length is greater than 0. 

\*****************************************************************************/
template<class CAllocatorType>
bool CString<CAllocatorType>::IsEmpty( void ) const
{
    return ( m_Length == 0 );
}

/*****************************************************************************\

Function:
    CString::DetachBuffer

Description:
    Enforces the CString to forget about its previous buffer and do not try 
    to deallocate it. Use carefully and make sure that someone will deallocate
    this buffer.

Input:
    none

Output:
    none

\*****************************************************************************/
template<class CAllocatorType>
void CString<CAllocatorType>::DetachBuffer( void )
{
    m_pString = NULL;
}

} // iSTD
#if defined _WIN32
#   pragma warning ( pop )
#endif
