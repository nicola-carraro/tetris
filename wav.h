typedef struct WavWavGuid {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t  data4[8];
} WavGuid;

#pragma pack(push, 1)
typedef struct {
    uint16_t  formatTag;
    uint16_t  channels;
    uint32_t  samplesPerSec;
    uint32_t  avgBytesPerSec;
    uint16_t  blockAlign;
    uint16_t  bitsPerSample;
    uint16_t  extensionSize;
    uint16_t  validBitsPerSample;
    uint32_t  channelMask;
    WavGuid subFormat;
} WavFmtChunk;
#pragma pack(pop)

typedef struct {
    uint32_t chunkId;
    uint32_t chunkSize;
    uint32_t waveId;
} WavRiffChunk;

typedef struct {
    WavRiffChunk   *riffChunk;
    WavFmtChunk *fmtChunk;
    void        *data;
    uint32_t         dataSize;
} Wav;

typedef struct {
    uint32_t chunkId;
    uint32_t chunkSize;
} WavChunkHeader;

typedef union {
    uint32_t  n;
    char s[4];
} WavTag;
