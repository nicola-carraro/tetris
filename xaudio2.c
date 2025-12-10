void xaudio2WavPlay(TtsTetris *tetris, IXAudio2SourceVoice **sourceVoice, Wav wav, bool loop) {
    if (tetris->hasSound) {
        IXAudio2 *xaudio = tetris->platform->xaudio;

        if (*sourceVoice != 0) {
            IXAudio2SourceVoice_DestroyVoice(*sourceVoice);
        }
        XAUDIO2_BUFFER audioBuffer = {0};
        {
            audioBuffer.Flags = XAUDIO2_END_OF_STREAM;
            audioBuffer.AudioBytes = wav.dataSize;
            audioBuffer.pAudioData = wav.data;
            audioBuffer.PlayBegin = 0;
            audioBuffer.PlayLength = 0;
            audioBuffer.LoopBegin = 0;
            audioBuffer.LoopLength = 0;
            audioBuffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;
            audioBuffer.pContext = 0;
        }

        WAVEFORMATEX musicWaveFormat = {0};
        {
            musicWaveFormat.wFormatTag = wav.fmtChunk->formatTag;
            musicWaveFormat.nChannels = wav.fmtChunk->channels;
            musicWaveFormat.nSamplesPerSec = wav.fmtChunk->samplesPerSec;
            musicWaveFormat.nAvgBytesPerSec = wav.fmtChunk->avgBytesPerSec;
            musicWaveFormat.nBlockAlign = wav.fmtChunk->blockAlign;
            musicWaveFormat.wBitsPerSample = wav.fmtChunk->bitsPerSample;
            musicWaveFormat.cbSize = wav.fmtChunk->extensionSize;
        }

        HRESULT hr = IXAudio2_CreateSourceVoice(xaudio, sourceVoice, &musicWaveFormat, 0, 1.0, 0, 0, 0);

        if (SUCCEEDED(hr)) {
            IXAudio2SourceVoice_SubmitSourceBuffer(
                *sourceVoice,
                &audioBuffer,
                0
            );
        }

        IXAudio2SourceVoice_Start(*sourceVoice, 0, 0);
    }
}

static void platformPlayMusic(TtsTetris *tetris, Wav wav) {
    xaudio2WavPlay(tetris, &tetris->platform->music, wav, 1);
}

static void platformPlaySound(TtsTetris *tetris, Wav wav) {
    xaudio2WavPlay(tetris, &tetris->platform->sound, wav, 0);
}

void xaudio2Pause(IXAudio2SourceVoice *sourceVoice) {
    IXAudio2SourceVoice_Stop(sourceVoice, 0, 0);
}

static BOOL xaudio2Init(TtsPlatform *platform) {
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    BOOL ok = SUCCEEDED(hr);

    if (ok) {
        hr = XAudio2Create(&platform->xaudio, 0,XAUDIO2_DEFAULT_PROCESSOR);
        ok = SUCCEEDED(hr);
    }

    if (ok) {
        hr = IXAudio2_CreateMasteringVoice(
            platform->xaudio,
            &platform->masteringVoice,
            XAUDIO2_DEFAULT_CHANNELS,
            XAUDIO2_DEFAULT_SAMPLERATE,
            0,
            0,
            0,
            0
        );
        ok = SUCCEEDED(hr);
    }

    return ok;
}
