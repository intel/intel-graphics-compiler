/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ElfReader.h"
#include "llvm/BinaryFormat/ELF.h"
#include <string.h>

namespace CLElfLib
{

/******************************************************************************\
 Constructor: RAIIElf::RAIIElf
\******************************************************************************/
RAIIElf::RAIIElf(CElfReader *pElf) : p(pElf) {}

/******************************************************************************\
 Destructor: RAIIElf::RAIIElf
\******************************************************************************/
RAIIElf::~RAIIElf() {
    if ( p != NULL )
    {
        CElfReader::Delete(p);
    }
}

/******************************************************************************\
 Constructor: CElfReader::CElfReader
\******************************************************************************/
CElfReader::CElfReader(
    const char* pElfBinary,
    const size_t elfBinarySize )
{
    m_pNameTable = NULL;
    m_nameTableSize = 0;
    m_pElfHeader = (SElfHeader*)pElfBinary;
    m_pBinary = pElfBinary;

    // get a pointer to the string table
    if( m_pElfHeader )
    {
        GetSectionData(
            m_pElfHeader->SectionNameTableIndex,
            m_pNameTable, m_nameTableSize );
    }
}

/******************************************************************************\
 Destructor: CElfReader::~CElfReader
\******************************************************************************/
CElfReader::~CElfReader()
{
}

/******************************************************************************\
 Member Function: CElfReader::Create
\******************************************************************************/
CElfReader* CElfReader::Create(
    const char* pElfBinary,
    const size_t elfBinarySize )
{
    CElfReader* pNewReader = NULL;

    if( IsValidElf( pElfBinary, elfBinarySize ) )
    {
        pNewReader = new CElfReader( pElfBinary, elfBinarySize );
    }

    return pNewReader;
}

/******************************************************************************\
 Member Function: CElfReader::Delete
\******************************************************************************/
void CElfReader::Delete(
    CElfReader* &pElfReader )
{
    if( pElfReader )
    {
        delete pElfReader;
        pElfReader = NULL;
    }
}

/******************************************************************************\
 Member Function: IsValidElf64
 Description:     Determines if a binary is in the ELF64 format checks for
                  invalid offsets.
\******************************************************************************/
bool CElfReader::IsValidElf(
    const void* pBinary,
    const size_t binarySize )
{
    bool retVal = false;
    SElfHeader* pElfHeader = NULL;
    SElfSectionHeader* pSectionHeader = NULL;
    size_t headerSize = sizeof(SElfHeader);
    char* pNameTable = NULL;
    char* pEnd = NULL;
    size_t ourSize = 0;
    size_t entrySize = 0;
    size_t indexedSectionHeaderOffset = 0;

    // validate header
    if( pBinary && ( binarySize >= headerSize ) )
    {
        // calculate a pointer to the end
        pEnd = (char*)pBinary + binarySize;
        pElfHeader = (SElfHeader*)pBinary;

        if( ( pElfHeader->Identity[ID_IDX_MAGIC0] == ELF_MAG0 ) &&
            ( pElfHeader->Identity[ID_IDX_MAGIC1] == ELF_MAG1 ) &&
            ( pElfHeader->Identity[ID_IDX_MAGIC2] == ELF_MAG2 ) &&
            ( pElfHeader->Identity[ID_IDX_MAGIC3] == ELF_MAG3 ) &&
            ( pElfHeader->Identity[ID_IDX_CLASS]  == EH_CLASS_64 ||
              pElfHeader->Identity[ID_IDX_CLASS]  == EH_CLASS_32 ) )
        {
            ourSize += pElfHeader->ElfHeaderSize;
            retVal = true;
        }
    }

    // validate sections
    if( retVal == true )
    {
        // get the section entry size
        entrySize = pElfHeader->SectionHeaderEntrySize;

        // get an offset to the name table
        if( pElfHeader->SectionNameTableIndex <
            pElfHeader->NumSectionHeaderEntries )
        {
            indexedSectionHeaderOffset =
                (size_t)pElfHeader->SectionHeadersOffset +
                ( pElfHeader->SectionNameTableIndex * entrySize );

            if( ( (char*)pBinary + indexedSectionHeaderOffset ) <= pEnd )
            {
                SElfSectionHeader* pNameTableSectionHeader = (SElfSectionHeader*)((char*)pBinary + indexedSectionHeaderOffset);
                pNameTable = (char*)pBinary + pNameTableSectionHeader->DataOffset;
            }
        }

        for (unsigned int i = 0; i < pElfHeader->NumSectionHeaderEntries; i++)
        {
            indexedSectionHeaderOffset = (size_t)pElfHeader->SectionHeadersOffset +
                (i * entrySize);

            // check section header offset
            if (((char*)pBinary + indexedSectionHeaderOffset) > pEnd)
            {
                retVal = false;
                break;
            }

            pSectionHeader = (SElfSectionHeader*)(
                (char*)pBinary + indexedSectionHeaderOffset);

            // check section data
            if (((char*)pBinary + pSectionHeader->DataOffset + pSectionHeader->DataSize) > pEnd)
            {
                retVal = false;
                break;
            }

            // check section name index
            if ((pNameTable + pSectionHeader->Name) > pEnd)
            {
                retVal = false;
                break;
            }

            // tally up the sizes
            ourSize += (size_t)pSectionHeader->DataSize;
            ourSize += (size_t)entrySize;
        }

        // Learnt from NEO driver for zebin, does not need check size match
        //if (ourSize != binarySize)
        //{
        //    retVal = false;
        //}
    }

    return retVal;
}

/******************************************************************************\
 Member Function: GetElfHeader
 Description:     Returns a pointer to the requested section header
\******************************************************************************/
const SElfHeader* CElfReader::GetElfHeader()
{
    return m_pElfHeader;
}

/******************************************************************************\
 Member Function: GetSectionHeader
 Description:     Returns a pointer to the requested section header
\******************************************************************************/
const SElfSectionHeader* CElfReader::GetSectionHeader(
    unsigned int sectionIndex )
{
    SElfSectionHeader* pSectionHeader = NULL;
    size_t indexedSectionHeaderOffset = 0;
    size_t entrySize = m_pElfHeader->SectionHeaderEntrySize;

    if( sectionIndex < m_pElfHeader->NumSectionHeaderEntries )
    {
        indexedSectionHeaderOffset = (size_t)m_pElfHeader->SectionHeadersOffset +
            ( sectionIndex * entrySize );

        pSectionHeader = (SElfSectionHeader*)(
                (char*)m_pElfHeader + indexedSectionHeaderOffset );
    }

    return pSectionHeader;
}

/******************************************************************************\
 Member Function: GetSectionHeader
 Description:     Returns a pointer to the requested section header
\******************************************************************************/
const SElfSectionHeader* CElfReader::GetSectionHeader(
    const char* pSectionName)
{
    const SElfSectionHeader* pSectionHeader = NULL;
    const char* pCurrentName = NULL;

    for (unsigned int i = 1; i < m_pElfHeader->NumSectionHeaderEntries; i++)
    {
        pCurrentName = GetSectionName(i);

        if (pSectionName && pCurrentName && (strcmp(pSectionName, pCurrentName) == 0))
        {
            pSectionHeader = GetSectionHeader(i);
            break;
        }
    }

    return pSectionHeader;
}

/******************************************************************************\
 Member Function: GetSectionData
 Description:     Returns a pointer to and size of the requested section's
                  data
\******************************************************************************/
E_RETVAL CElfReader::GetSectionData(
    const unsigned int sectionIndex,
    char* &pData,
    size_t &dataSize )
{
    E_RETVAL retVal = FAILURE;
    const SElfSectionHeader* pSectionHeader = GetSectionHeader( sectionIndex );

    if( pSectionHeader )
    {
        pData = (char*)m_pBinary + pSectionHeader->DataOffset;
        dataSize = ( size_t )pSectionHeader->DataSize;
        retVal = SUCCESS;
    }

    return retVal;
}

/******************************************************************************\
 Member Function: GetSectionData
 Description:     Returns a pointer to and size of the requested section's
                  data
\******************************************************************************/
E_RETVAL CElfReader::GetSectionData(
    const char* pName,
    char* &pData,
    size_t &dataSize )
{
    E_RETVAL retVal = FAILURE;
    const char* pSectionName = NULL;

    for( unsigned int i = 1; i < m_pElfHeader->NumSectionHeaderEntries; i++ )
    {
        pSectionName = GetSectionName( i );

        if( pSectionName && ( strcmp( pName, pSectionName ) == 0 ) )
        {
            GetSectionData( i, pData, dataSize );
            retVal = SUCCESS;
            break;
        }
    }

    return retVal;
}

/******************************************************************************\
 Member Function: GetSectionName
 Description:     Returns a pointer to a NULL terminated string
\******************************************************************************/
const char* CElfReader::GetSectionName(
    unsigned int sectionIndex )
{
    char* pName = NULL;
    const SElfSectionHeader* pSectionHeader = GetSectionHeader( sectionIndex );

    if( pSectionHeader )
    {
        pName = m_pNameTable + pSectionHeader->Name;
    }

    return pName;
}

} // namespace OclElfLib
