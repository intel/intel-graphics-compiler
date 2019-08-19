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
#ifndef _IGA_DECODER_COMMON_H_
#define _IGA_DECODER_COMMON_H_



#define GED_DECODE_TO(FIELD, TRANS, DST) \
    do { \
      GED_RETURN_VALUE _status; \
      if(print_ged_debug) { \
          std::cout << "FIELD: " << #FIELD << std::endl; \
          GED_PrintFieldBitLocation(&m_currGedInst, GED_INS_FIELD_ ## FIELD); \
      } \
      DST = TRANS(GED_Get ## FIELD(&m_currGedInst, &_status)); \
      if (_status != GED_RETURN_VALUE_SUCCESS) { \
          handleGedDecoderError(__LINE__, #FIELD, _status); \
      } \
    } while (0)

#define GED_DECODE_RAW_TO(FIELD, DST) \
    do { \
      GED_RETURN_VALUE _status; \
      if(print_ged_debug) { \
          std::cout << "FIELD: " << #FIELD << std::endl; \
          GED_PrintFieldBitLocation(&m_currGedInst, GED_INS_FIELD_ ## FIELD); \
      } \
      DST = GED_Get ## FIELD(&m_currGedInst, &_status); \
      if (_status != GED_RETURN_VALUE_SUCCESS) { \
          handleGedDecoderError(__LINE__, #FIELD, _status); \
      } \
    } while (0)

#define GED_DECODE_RAW(GED_TYPE, ID, FIELD) \
        GED_TYPE ID; \
        GED_DECODE_RAW_TO(FIELD, ID);

#define GED_DECODE(IGA_TYPE, GED_TYPE, ID, FIELD) \
        GED_DECODE_RAW(GED_TYPE, GED_ ## ID, FIELD); \
        IGA_TYPE ID = GEDToIGATranslation::translate(GED_ ## ID);


#define GED_DECODE_RAW_TO_SRC(DST, TYPE, FIELD) \
    do { \
      GED_RETURN_VALUE _STATUS; \
      if(print_ged_debug) { \
          std::cout << "FIELD: " << #FIELD << std::endl; \
          GED_PrintFieldBitLocation(&m_currGedInst, GED_INS_FIELD_ ## FIELD); \
      } \
      DST = GED_Get ## FIELD(&m_currGedInst, &_STATUS); \
      if (_STATUS != GED_RETURN_VALUE_SUCCESS) { \
          handleGedDecoderError(__LINE__, #FIELD, _STATUS); \
      } \
    } while (0)

#define RETURN_GED_DECODE_RAW_TO_SRC(TYPE, FIELD) { \
        TYPE _DST; \
        GED_DECODE_RAW_TO_SRC(_DST, TYPE, FIELD); \
        return _DST; \
    }
// the extra namespace declaration is needed to make g++ happy
#define DEFINE_SOURCE_ACCESSOR(TYPE, FIELD, I) \
    namespace iga { \
        template <> TYPE DecoderBase::decodeSrc ## FIELD <SourceIndex::SRC##I>() { \
            RETURN_GED_DECODE_RAW_TO_SRC(TYPE, Src ## I ## FIELD); \
        } \
    }

/* #define DEFINE_SOURCE_ACCESSOR_INLINE(TYPE, FIELD, I) \
        template <> TYPE decodeSrc ## FIELD <SourceIndex::SRC##I>() { \
            RETURN_GED_DECODE_RAW_TO_SRC(TYPE, Src ## I ## FIELD); \
        }
*/
/* #define DEFINE_GED_SOURCE_ACCESSORS_INLINE_01(TYPE, FIELD) \
   DEFINE_SOURCE_ACCESSOR_INLINE(TYPE, FIELD, 0) \
    DEFINE_SOURCE_ACCESSOR_INLINE(TYPE, FIELD, 1)
*/
#define DEFINE_GED_SOURCE_ACCESSORS_01(TYPE, FIELD) \
    DEFINE_SOURCE_ACCESSOR(TYPE, FIELD, 0) \
    DEFINE_SOURCE_ACCESSOR(TYPE, FIELD, 1)

/* #define DEFINE_GED_SOURCE_ACCESSORS_INLINE_012(TYPE, FIELD) \
    DEFINE_SOURCE_ACCESSOR_INLINE(TYPE, FIELD, 0) \
    DEFINE_SOURCE_ACCESSOR_INLINE(TYPE, FIELD, 1) \
    DEFINE_SOURCE_ACCESSOR_INLINE(TYPE, FIELD, 2)
*/
#define DEFINE_GED_SOURCE_ACCESSORS_012(TYPE, FIELD) \
    DEFINE_SOURCE_ACCESSOR(TYPE, FIELD, 0) \
    DEFINE_SOURCE_ACCESSOR(TYPE, FIELD, 1) \
    DEFINE_SOURCE_ACCESSOR(TYPE, FIELD, 2)

#endif //_IGA_DECODER_COMMON_H_
