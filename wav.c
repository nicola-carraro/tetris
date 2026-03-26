static bool wavIsValid(Wav wav) {
    bool result = wav.riffChunk && wav.fmtChunk && wav.data;

    return result;
}

static Wav wavParseFile(BaseReadResult file) {
    #if 0
    BASE_ASSERT(file.size);
    BASE_ASSERT(file.data);
    #endif

    Wav wav = {0};

    if (file.size > 0 && file.data) {
        uint8_t *bytes = (uint8_t*) file.data;

        WavRiffChunk *riffChunk = (WavRiffChunk *)bytes;
        wav.riffChunk = riffChunk;

        WavChunkHeader *chunkHeader = 0;
        for (uint64_t offset = sizeof(WavRiffChunk); (offset + sizeof(WavRiffChunk) - 4) < riffChunk->chunkSize && offset < file.size; offset += (chunkHeader->chunkSize + sizeof(WavChunkHeader))) {
            chunkHeader = (WavChunkHeader *) (bytes + offset);

            if (chunkHeader->chunkId == ' tmf') {
                WavFmtChunk *fmtChunk = (WavFmtChunk *)(bytes + offset + sizeof(WavChunkHeader));
                wav.fmtChunk = fmtChunk;
                #if 0
                platformDebugPrint("formatTag %u\n", fmtChunk->formatTag);
                platformDebugPrint("channels %u\n", fmtChunk->channels);
                platformDebugPrint("samplesPerSec %u\n", fmtChunk->samplesPerSec);
                platformDebugPrint("avgBytesPerSec %u\n", fmtChunk->avgBytesPerSec);
                platformDebugPrint("blockAlign %u\n", fmtChunk->blockAlign);
                platformDebugPrint("bitsPerSample %u\n", fmtChunk->bitsPerSample);
                platformDebugPrint("extensionSize %u\n", fmtChunk->extensionSize);
                platformDebugPrint("validBitsPerSample %u\n", fmtChunk->validBitsPerSample);
                platformDebugPrint("channelMask %u\n", fmtChunk->channelMask);
                #endif
            }

            if (chunkHeader->chunkId == 'atad') {
                void *data = (void *)(bytes + offset + sizeof(WavChunkHeader));
                wav.data = data;
                wav.dataSize = chunkHeader->chunkSize;
            }
        }
    }

    return wav;
}
