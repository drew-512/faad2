
#include <iterator>
#include <vector>
#include <algorithm>
#include <string>
#include <unordered_map>
#include "neaacdec.h"
#include "fmod.h"

#include "mov_parse.h"


typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;

using namespace std;

#define LOG(_format, ...)  { \
    FMOD_CODEC_LOG(c, FMOD_DEBUG_LEVEL_LOG, "***** M4A *****", _format "\n", ##__VA_ARGS__ );  \
}


#define MIN(a, b)   ((a) < (b) ? (a) : (b))
#define MAX(a, b)   ((a) > (b) ? (a) : (b))

#if BYTE_ORDER == BIG_ENDIAN
    #define BE32(_p)    *(uint32_t*)(_p)
    #define BE64(_p)    *(uint64_t*)(_p)
#else
    #define BE32(_p)    (uint32_t) (        \
            ((uint32_t)*(_p  ))<<24 |       \
            ((uint32_t)*(_p+1))<<16 |       \
            ((uint32_t)*(_p+2))<< 8 |       \
            ((uint32_t)*(_p+3))             \
    )

    #define BE64(_p)    (uint64_t) (        \
            ((uint64_t)*(_p  ))<<56 |       \
            ((uint64_t)*(_p+1))<<48 |       \
            ((uint64_t)*(_p+2))<<40 |       \
            ((uint64_t)*(_p+3))<<32 |       \
            ((uint64_t)*(_p+4))<<24 |       \
            ((uint64_t)*(_p+5))<<16 |       \
            ((uint64_t)*(_p+6))<< 8 |       \
            ((uint64_t)*(_p+7))             \
    )
#endif


class FaadDecoder {

    public:

        FaadDecoder() {
            
            ISO_Major = 0;
            ISO_Minor = 0;
            bzero(&FmodFormat, sizeof(FmodFormat));
            
            MediaData = {0, 0};
            Faad = NULL;
            FileSize = 0;
            
            atom_depth = 0;

            SampleRate = 0;
            Channels   = 0;
        }
        
        
        virtual ~FaadDecoder() {
            dealloc();
        }

        NeAACDecHandle          Faad;
        uint32_t                ISO_Major;
        uint32_t                ISO_Minor;
        string                  ISO_Brand;
        
        unsigned long           SampleRate;
        uint8_t                 Channels;
    
        FMOD_CODEC_WAVEFORMAT   FmodFormat;
            
        int64_t                 FileSize;
        vector<byte>            Moov;
        MOVSegment              MediaData;
        uint32_t                MediaStart[8];
        
        int                     atom_depth;  // TODO
        
        ByteSlice               _hotChunk;
        vector<uint8_t>         _hotChunkBuf;
        int64_t                 _hotChunkIdx;
        int64_t                 _endChunkIdx;
        
        NeAACDecFrameInfo       _frame;
        ByteSlice               _frameSamples;
        int64_t                 _readOffset;
        
        FMOD_RESULT             OpenDecoder(FMOD_CODEC_STATE* c);
        FMOD_RESULT             ReadSamples(FMOD_CODEC_STATE* c, ByteSlice* samples);
        FMOD_RESULT             SetPosition(FMOD_CODEC_STATE* c, int64_t pos, bool reset);
        
        void                    dealloc();
        
        
        FMOD_RESULT             loadChunk(FMOD_CODEC_STATE* c, int64_t readPos);

};

void FaadDecoder::dealloc() {
    if (Faad) {
        NeAACDecClose(Faad);
        Faad = NULL;
    }
    Moov.clear();
}


FMOD_RESULT FaadDecoder::OpenDecoder(FMOD_CODEC_STATE* c) {
    SetPosition(c, 0, true);
    if (Moov.size() == 0 || MediaData.len == 0) {
        return FMOD_ERR_FILE_BAD;
    }
    
    // TODO: parse X->Moov
    { }
    
    FMOD_RESULT err = loadChunk(c, 0);
    if (err) {
        return err;
    }
    
    
    // DOESNT WORK for ER_LC
    // mp4AudioSpecificConfig mp4;
    // bzero(&mp4, sizeof(mp4));
    // int err_format = NeAACDecAudioSpecificConfig(_hotChunk.ptr, _hotChunk.len, &mp4);
    // SampleRate = mp4.samplingFrequency;
    // Channels   = mp4.channelsConfiguration;
    // LOG("err_format=%d, mp4.objectType=%d, hz=%d, ch=%d", err_format, (int) mp4.objectTypeIndex, SampleRate, Channels);
  
    bool is_ER_LC = ((*_hotChunk.ptr) >> 3) == DRM_ER_LC;
    LOG("is_ER_LC: %d", (int) is_ER_LC);
    
    int init_err = -3773;
    if (is_ER_LC) {
        SampleRate = 44100; // TODO
        Channels = DRMCH_STEREO; // TODO
        init_err = NeAACDecInitDRM(&Faad, SampleRate, DRMCH_STEREO);
    } else {
        Faad = NeAACDecOpen();
        init_err = NeAACDecInit2(Faad, _hotChunk.ptr, _hotChunk.len, &SampleRate, &Channels);
    }
    
    LOG("Faad Init: %d", init_err);
    if (init_err) {
        return FMOD_ERR_INTERNAL;
    }

    LOG("NeAACDecInit2() OK: SampleRate=%d, Channels=%d", SampleRate, Channels);
    NeAACDecConfiguration* cfg = NeAACDecGetCurrentConfiguration(Faad);
    LOG("NeAACDecConfiguration: outputFormat=%d, defSampleRate=%d", cfg->outputFormat, cfg->defSampleRate);

    {
        FMOD_CODEC_WAVEFORMAT f;
        if (c->waveformat) {
            f = *c->waveformat;
            LOG("using c->waveformat %s", f.name ? f.name : "NULL");
        } else {
            bzero(&f, sizeof(f));
        }
        
        f.name         = ISO_Brand.c_str();
        f.format       = FMOD_SOUND_FORMAT_PCM16;
        f.channels     = Channels;
        f.frequency    = SampleRate;
        f.lengthbytes  = MediaData.len;
        f.lengthpcm    = SampleRate * 60;   // TODO: calculate this from mov atoms
        f.pcmblocksize = 0;                 // TODO
        
        FmodFormat = f;
        c->waveformat = &FmodFormat;
    }
    
    return FMOD_OK;
}

        
FMOD_RESULT FaadDecoder::SetPosition(FMOD_CODEC_STATE* c, int64_t pos, bool reset) {

    if (reset) {
        _hotChunkBuf.resize(MDAT_CHUNK_SIZE);
        _hotChunk.ptr = _hotChunkBuf.data();
        _hotChunk.cap = MDAT_CHUNK_SIZE;
        _hotChunk.len = 0;
        _hotChunkIdx = -1;
        _endChunkIdx = 0xFFFFFFFF;
        
        bzero(&_frame, sizeof(_frame));
        bzero(&_frameSamples, sizeof(_frameSamples));
        _readOffset = pos;
    
    } else if (pos != _readOffset) {
        _frameSamples.cap = 0;
        _frameSamples.len = 0;
        _readOffset = pos;
    
        if (Faad == NULL) {
            return FMOD_ERR_BADCOMMAND;
        }
    }
    return FMOD_OK;
}



// Loads the chunk containing the given read position (if not already loaded)
// readPos is the offset from the media data start (not the file stream)
FMOD_RESULT FaadDecoder::loadChunk(FMOD_CODEC_STATE* c, int64_t readPos) {
    int64_t chunkIdx = readPos >> MDAT_CHUNK_SHIFT;
    if (chunkIdx >= 0 && chunkIdx == _hotChunkIdx) { // already loaded
        return FMOD_OK;
    }
    
    int64_t chunkOfs = readPos - (chunkIdx << MDAT_CHUNK_SHIFT);
    int64_t chunkPos = MediaData.pos + chunkOfs;
    FMOD_RESULT err = c->functions->seek(c, (uint32_t) chunkPos, FMOD_CODEC_SEEK_METHOD_SET);
    if (err) {
        _hotChunkIdx = -1;
        return err;
    }

    err = c->functions->read(c, _hotChunk.ptr, _hotChunk.cap, &_hotChunk.len);
    if (err == FMOD_ERR_FILE_EOF && _hotChunk.len > 0) {
        err = FMOD_OK;
        _endChunkIdx = MIN(_endChunkIdx, chunkIdx);
    }
    
    if (err) {
        _hotChunkIdx = -1;
        return err;
    }
    
    LOG("loaded chunk %d @%d, len=%d", chunkIdx, chunkOfs, _hotChunk.len);
    _hotChunkIdx = chunkIdx;
    return FMOD_OK;
}




	
	
FMOD_RESULT FaadDecoder::ReadSamples(FMOD_CODEC_STATE* c, ByteSlice* samplesOut) {
    uint32_t bytesRemain = samplesOut->cap - samplesOut->len;

    while (1) {

        if (_frameSamples.len > 0) {
            int n = MIN(_frameSamples.len, bytesRemain);
            memcpy(samplesOut->ptr + samplesOut->len, _frameSamples.ptr, n);
            samplesOut->len   += n;
            _frameSamples.ptr += n;
            _frameSamples.len -= n;
            _frameSamples.cap -= n;
            bytesRemain       -= n;
        }
        
        LOG("bytesRemain=%d, readOffset=%d, hotChunkIdx=%d", bytesRemain, _readOffset, _hotChunkIdx);
        if (bytesRemain == 0) {
            return FMOD_OK;
        }

        FMOD_RESULT err = loadChunk(c, _readOffset);
        if (err) {
            return err;
        }
    
        uint32_t relPos = (uint32_t) (_readOffset - (_hotChunkIdx << MDAT_CHUNK_SHIFT));
        _frameSamples.cap = 0;
        _frameSamples.len = 0;
        LOG("relPos=%d, first=0x%X, toRead=%d", relPos, (int) *_hotChunk.ptr, _hotChunk.len - relPos);
        #pragma warning("below fails NeAACDecDecode() fails for modern DRM_ER_LC: 26")
        _frameSamples.ptr = (uint8_t*) NeAACDecDecode(Faad, &_frame, _hotChunk.ptr + relPos, _hotChunk.len - relPos); 
        if (_frameSamples.ptr == NULL || _frame.error) {
            LOG("NeAACDecDecode() failed: %d", _frame.error);
            return FMOD_ERR_FILE_BAD;
        }
        if (_frame.channels != Channels) {
            LOG("NeAACDecDecode() channels mismatch: %d != %d", _frame.channels, Channels);
            return FMOD_ERR_INTERNAL;
        }
        if (_frame.samplerate != SampleRate) {
            LOG("NeAACDecDecode() samplerate mismatch: %d != %d", _frame.samplerate, SampleRate);
            return FMOD_ERR_INTERNAL;
        }
        _readOffset += _frame.bytesconsumed;
        _frameSamples.cap =
        _frameSamples.len = _frame.samples * _frame.channels;
    }

}


static inline int read_atom(FMOD_CODEC_STATE* c, MOVAtom* atom) {
    byte buf[8];
    uint32_t bytesRead;
    
    FMOD_RESULT err = c->functions->read(c, buf, 8, &bytesRead);
    if (bytesRead < 8) {
        return FMOD_ERR_FILE_EOF;
    }
    atom->size = (int32_t) BE32(buf) - 8;
    atom->type = BE32(buf + 4);
    if (atom->size == -7) {  // 64 bit size signal
        c->functions->read(c, buf, 8, &bytesRead);
        if (bytesRead != 8) {
            return FMOD_ERR_FILE_ENDOFDATA;
        }
        atom->size = BE64(buf) - 16;
    }
    if (atom->size < 0) {
        return FMOD_ERR_FILE_BAD;
    }
    return 0;
}



#define SKIP(_atom)                                             \
    err = c -> functions -> seek(c, _atom.size, FMOD_CODEC_SEEK_METHOD_CURRENT); \
    if (err)                                                    \
        return err;                                             \
    

#define TELL(_pos)                                              \
    int err = c->functions->tell(c, &pos);                      \
    if (err) {                                                  \
        return err;                                             \
    }
    




static int mov_parse_ftyp(FMOD_CODEC_STATE* c, MOVAtom atom) {
    int brandLen = atom.size - 8;
    if (brandLen < 0 || atom.size > FTYP_LEN_MAX) {
        return FMOD_ERR_FORMAT;
    }
    
    CODEC_ENTER
    byte scrap[FTYP_LEN_MAX];
    uint32_t bytesRead = 0;
    
    c->functions->read(c, scrap, atom.size, &bytesRead);
    if (bytesRead != atom.size) {
        return FMOD_ERR_FILE_ENDOFDATA;
    }
    
    X->ISO_Major = BE32(scrap + 0);
    X->ISO_Minor = BE32(scrap + 4);
    if (X->ISO_Major != 'M4A ' && X->ISO_Major != 'mp42') {
        return FMOD_ERR_FORMAT;
    }

    if (brandLen > 0) {
        X -> ISO_Brand = string((char*)(scrap + 8), brandLen);
        LOG("ISO_Brand: %s", X -> ISO_Brand.c_str());
    }
    return 0;
}

static int mov_parse_moov(FMOD_CODEC_STATE* c, MOVAtom atom) {
    if (atom.size < 50) // ignore empties
        return 0;
    
    CODEC_ENTER
    
    uint32_t pos;
    TELL(pos);

    if (X->Moov.size() > 0) {
        LOG("duplicate moov at %d", pos);
        return FMOD_ERR_FORMAT;
    }
    LOG("moov: %d @ %d", atom.size, pos);
    X->Moov.resize(atom.size);
    byte* dst = X->Moov.data();
    uint32_t bytesRead;
    c->functions->read(c, dst, atom.size, &bytesRead);
    LOG("atom.size: %d, bytesRead: %d", atom.size, bytesRead);
    if (bytesRead != atom.size) {
        return FMOD_ERR_FILE_ENDOFDATA;
    }
    return 0;
}

static int mov_parse_mdat(FMOD_CODEC_STATE* c, MOVAtom atom) {
    if (atom.size < MDAT_MIN_LEN) // ignore empties
        return 0;
        
    CODEC_ENTER
    
    uint32_t pos;
    TELL(pos);
    
    if (X -> MediaData.pos) {
        LOG("duplicate mdat at %d", pos);
        return FMOD_ERR_FORMAT;
    }
    
    uint32_t bytesRead = sizeof(X->MediaStart);
    c->functions->read(c, &X->MediaStart, bytesRead, &bytesRead);
    X -> MediaData.pos = pos;
    X -> MediaData.len = atom.size;
        
    atom.size -= bytesRead;
    SKIP(atom)
    return 0;
}

static int mov_parse_skip(FMOD_CODEC_STATE* c, MOVAtom atom) {
    int err;
    SKIP(atom)
    return err;
}



static const MOVParseTableEntry s_MOV_parsers[] = {
    { MKTAG('s','k','i','p'), mov_parse_skip },
    { MKTAG('f','r','e','e'), mov_parse_skip },
    { MKTAG('m','o','o','v'), mov_parse_moov },
    { MKTAG('m','d','a','t'), mov_parse_mdat },
    { 0, NULL },
};




// The open callback function is used to read data from a file and decode it
FMOD_RESULT F_CALLBACK faac_open(FMOD_CODEC_STATE* c, FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO* userexinfo) {
    MOVAtom atom;
    int err = read_atom(c, &atom);
    if (err != 0 || atom.type != 'ftyp' || atom.size > FTYP_LEN_MAX) {
        return FMOD_ERR_FORMAT;
    }

    uint32_t fileSize = 0;
    c->functions->size(c, &fileSize);
    LOG("fileSize: %d", fileSize);
    if (fileSize == 0 && fileSize < FILE_SIZE_MIN) { // assume fileSize 0 denotes live stream?
        return FMOD_ERR_FILE_BAD;
    }

    FaadDecoder* X = new FaadDecoder();
    X -> FileSize = fileSize;
    c -> plugindata = X;
    
    // perform first and only once
    err = mov_parse_ftyp(c, atom);
    
    // parse pass: root level atoms
    while (!err) {
        err = read_atom(c, &atom);
        if (err) { 
            if (err == FMOD_ERR_FILE_EOF) { // allow live streams or truncated files to still play
                err = 0;
            }
            break;
        }
        
        bool handled = false;
        for (int i = 0; s_MOV_parsers[i].type && !handled; i++) {
            if (s_MOV_parsers[i].type == atom.type) {
                err = s_MOV_parsers[i].parse(c, atom);
                handled = true;
            }
        }
        if (!handled && err == 0) {
            err = mov_parse_skip(c, atom);
        }
        LOG("ATOM '%c%c%c%c' err: %d, handled: %d, ", (char)(atom.type >> 24), (char)(atom.type >> 16), (char)(atom.type >> 8), (char)atom.type, err, handled);
    }
    
    LOG("MediaData: %d bytes @ %d", X->MediaData.len, X->MediaData.pos);
    if (err == 0) {
        err = X -> OpenDecoder(c);
    }
    
    LOG("Err: %d", err);
    return (FMOD_RESULT) err;
};


// The close callback function is used to free decoded data
FMOD_RESULT F_CALLBACK faac_close(FMOD_CODEC_STATE* c) {
    CODEC_ENTER
    
    LOG("faac_close: %s", "closing");
    delete X;
    c->plugindata = X = NULL;
    return FMOD_OK;
};

// The read callback function is used to return decoded data
FMOD_RESULT F_CALLBACK faac_read(FMOD_CODEC_STATE* c, void* buffer, unsigned int sizebytes, unsigned int* bytesread) {
    CODEC_ENTER

    LOG("faac_read: block=%d", sizebytes);
    ByteSlice out;
    out.ptr = (uint8_t*) buffer;
    out.cap = sizebytes;
    out.len = 0;
    FMOD_RESULT err = X -> ReadSamples(c, &out);
    if (err) {
        return err;
    }
    *bytesread = out.len;
    return FMOD_OK;
};

// // The getlength callback function is used to return audio file length
// FMOD_RESULT F_CALLBACK faac_getlength(FMOD_CODEC_STATE* c, unsigned int* length, FMOD_TIMEUNIT lengthtype) {
//     ///length = 0;
//     return FMOD_ERR_UNINITIALIZED;  // TODO 
// };

// The setposition callback function is used to set playback position
FMOD_RESULT F_CALLBACK faac_setposition(FMOD_CODEC_STATE* c, int subsound, unsigned int position, FMOD_TIMEUNIT postype) {
    CODEC_ENTER

    // Set playback position
    //X ->_position = position;
    return FMOD_OK;
};

// The getposition callback function is used to get playback position
FMOD_RESULT F_CALLBACK faac_getposition(FMOD_CODEC_STATE* c, unsigned int* position, FMOD_TIMEUNIT postype) {
    CODEC_ENTER

    // Set playback position
    // *position = x->_position;
    return FMOD_OK;
};

// The soundcreated callback function is called when a sound is created
FMOD_RESULT F_CALLBACK faac_soundcreated(FMOD_CODEC_STATE* c, int subsound, FMOD_SOUND* sound)
{
    // Processing when sound is created
    return FMOD_OK;
};





// Codec information
FMOD_CODEC_DESCRIPTION gFaadCodecDesc = {
    FMOD_CODEC_PLUGIN_VERSION,    // Version number
    "FMOD MP4/AAC Codec",         // Codec name
    0x00010000,                   // Driver version number
    1,                            // Default As Stream
    FMOD_TIMEUNIT_PCMBYTES,       // Timeunit
    &faac_open,                   // open callback
    &faac_close,                  // close callback
    &faac_read,                   // read callback
    0, //&faac_getlength,              // getlength callback
    &faac_setposition,            // setposition callback
    0, // &faac_getposition,            // getposition callback  // TODO
    &faac_soundcreated,           // soundcreated callback
    0, //&faac_getWaveFormat           // getWaveFormatEx -- not needed if FMOD_CODEC_WAVEFORMAT is set in FMOD_CODEC_DESCRIPTION
};





DLL_EXPORT  FMOD_CODEC_DESCRIPTION* F_API FMODGetCodecDescription() {
    return &gFaadCodecDesc;
}










/*


        // Get tag information by brute force from /moov/udta/meta/ilst path
        // There might be a better way but this works for now
        // This processing is really not ideal
        if (memcmp(chunk->header, "moov", 4) == 0)
        {
            MP4HEADER* tmpHead = new MP4HEADER();
            vector <byte> tmpdata;
            uint32_t shift = 0;

            memcpy(tmpHead->size, chunk->data.data() + shift, 4);
            shift += 4;
            memcpy(tmpHead->header, chunk->data.data() + shift, 4);
            shift += 4;

            tmpHead->data.clear();
            tmpHead->data.resize(_get_size(tmpHead->size) - 8);
            // Save next level from moov data area
            memcpy(tmpHead->data.data(), chunk->data.data() + shift, _get_size(tmpHead->size) - 8);
            shift += (_get_size(tmpHead->size) - 8);

            // Skip until udta
            while (memcmp(tmpHead->header, "udta", 4) != 0)
            {
                memcpy(tmpHead->size, chunk->data.data() + shift, 4);
                shift += 4;
                memcpy(tmpHead->header, chunk->data.data() + shift, 4);
                shift += 4;
                if (_get_size(tmpHead->size) > 0)
                {
                    tmpdata.clear();
                    tmpdata.resize(_get_size(tmpHead->size) - 8);
                    memcpy(tmpdata.data(), chunk->data.data() + shift, _get_size(tmpHead->size) - 8);
                    shift += (_get_size(tmpHead->size) - 8);
                }
            }

            // If udta found
            if (memcmp(tmpHead->header, "udta", 4) == 0)
            {
                memcpy(tmpHead->size, tmpdata.data(), 4);
                memcpy(tmpHead->header, tmpdata.data() + 4, 4);
                tmpHead->data.resize(_get_size(tmpHead->size) - 8);
                memcpy(tmpHead->data.data(), tmpdata.data() + 8, _get_size(tmpHead->size) - 8);

                // Should have meta
                if (memcmp(tmpHead->header, "meta", 4) == 0)
                {
                    shift = 4;
                    memcpy(tmpHead->size, tmpHead->data.data() + shift, 4);
                    shift += 4;
                    memcpy(tmpHead->header, tmpHead->data.data() + shift, 4);
                    // There's a mysterious 4 bytes
                    shift += 4;
                    shift += (_get_size(tmpHead->size) - 8);

                    // Skip until ilst tag
                    while (memcmp(tmpHead->header, "ilst", 4) != 0)
                    {
                        memcpy(tmpHead->size, tmpHead->data.data() + shift, 4);
                        shift += 4;
                        memcpy(tmpHead->header, tmpHead->data.data() + shift, 4);
                        shift += 4;

                        // Break here as ilst data part becomes metadata
                        if (memcmp(tmpHead->header, "ilst", 4) == 0)
                            break;
                        shift += (_get_size(tmpHead->size) - 8);
                    }

                    // Should have ilst
                    if (memcmp(tmpHead->header, "ilst", 4) == 0)
                    {
                        // Until all needed tag info is retrieved
                        while (shift < tmpHead->data.size())
                        {
                            memcpy(tmpHead->size, tmpHead->data.data() + shift, 4);
                            shift += 4;
                            memcpy(tmpHead->header, tmpHead->data.data() + shift, 4);
                            shift += 4;
                            tmpdata.clear();
                            tmpdata.resize(_get_size(tmpHead->size) - 8);
                            memcpy(tmpdata.data(), tmpHead->data.data() + shift, _get_size(tmpHead->size) - 8);

                            // // ©ART tag
                            // if (tmpHead->header[0] == byte(0xA9) && tmpHead->header[1] == byte(0x41) && tmpHead->header[2] == byte(0x52) && tmpHead->header[3] == byte(0x54))
                            // {
                            //     string name = "ARTIST";
                            //     // Skip first 16 mysterious bytes in data part and reduce area by skip amount
                            //     x->artist.resize(_get_size(tmpHead->size) - 24);
                            //     memcpy(x->artist.data(), tmpdata.data() + 16, _get_size(tmpHead->size) - 24);
                            //     codec->functions->metadata(codec, FMOD_TAGTYPE_ID3V2, name.data(), x->artist.data(), x->artist.size(), FMOD_TAGDATATYPE_STRING_UTF8, 1);
                            // }
                            // // ©alb tag
                            // if (tmpHead->header[0] == byte(0xA9) && tmpHead->header[1] == byte(0x61) && tmpHead->header[2] == byte(0x6C) && tmpHead->header[3] == byte(0x62))
                            // {
                            //     string name = "ALBUM";
                            //     // Skip first 16 mysterious bytes in data part and reduce area by skip amount
                            //     x->album.resize(_get_size(tmpHead->size) - 24);
                            //     memcpy(x->album.data(), tmpdata.data() + 16, _get_size(tmpHead->size) - 24);
                            //     codec->functions->metadata(codec, FMOD_TAGTYPE_ID3V2, name.data(), x->album.data(), x->album.size(), FMOD_TAGDATATYPE_STRING_UTF8, 1);
                            // }
                            // // ©nam tag
                            // if (tmpHead->header[0] == byte(0xA9) && tmpHead->header[1] == byte(0x6E) && tmpHead->header[2] == byte(0x61) && tmpHead->header[3] == byte(0x6D))
                            // {
                            //     string name = "TITLE";
                            //     // Skip first 16 mysterious bytes in data part and reduce area by skip amount
                            //     x->title.resize(_get_size(tmpHead->size) - 24);
                            //     memcpy(x->title.data(), tmpdata.data() + 16, _get_size(tmpHead->size) - 24);
                            //     codec->functions->metadata(codec, FMOD_TAGTYPE_ID3V2, name.data(), x->title.data(), x->title.size(), FMOD_TAGDATATYPE_STRING_UTF8, 1);
                            // }
                            // shift += (_get_size(tmpHead->size) - 8);
                        }
                    }
                }
            }
            moov = true;
        }
    }

        */