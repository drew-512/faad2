#pragma once


#include <iterator>
#include <vector>
#include <algorithm>
#include <string>


using namespace std;

#define MKTAG(a,b,c,d) (    \
    ((uint32_t)(a) << 24) | \
    ((uint32_t)(b) << 16) | \
    ((uint32_t)(c) <<  8) | \
    ((uint32_t)(d)      )   \
)


#if defined(__CYGWIN32__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WINAPI_FAMILY)
    #define DLL_EXPORT extern "C" __declspec(dllexport)
#else
    #define DLL_EXPORT extern "C"
#endif



#define CODEC_ENTER                                     \
    FaadDecoder* X = ((FaadDecoder*)c->plugindata);     \
    if (X == NULL)                                      \
        return FMOD_ERR_INTERNAL;                          



typedef struct MOVAtom {
    uint32_t    type;
    int64_t     size;  /* bytes remainming in atom segment                  */
} MOVAtom;

typedef struct MOVSegment {
    int64_t     pos;  /* start position of data                             */
    int64_t     len;  /* byte length                                        */
} MOVSegment;

typedef int        (*MOVParser)(FMOD_CODEC_STATE* c, MOVAtom atom);


typedef struct MOVParseTableEntry {
    uint32_t    type;
    MOVParser   parse;
} MOVParseTableEntry;



typedef struct ByteSlice {
    uint8_t*    ptr;
    uint32_t    cap;
    uint32_t    len;
} ByteSlice;





enum {
    FTYP_LEN_MAX     = 1024,
    
    MDAT_MIN_LEN     = 100,
    MDAT_CHUNK_SHIFT = 13,
    MDAT_CHUNK_SIZE  = 1 << MDAT_CHUNK_SHIFT,
    
    FILE_SIZE_MIN    = 3500,
};
