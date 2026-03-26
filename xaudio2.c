static void xaudio2WavPlay(Platform *platform, IXAudio2SourceVoice **sourceVoice, Wav wav, bool loop) {
    if (platform->hasSound && wavIsValid(wav)) {
        IXAudio2 *xaudio = platform->xaudio;

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

        WAVEFORMATEX waveFormat = {0};
        {
            waveFormat.wFormatTag = wav.fmtChunk->formatTag;
            waveFormat.nChannels = wav.fmtChunk->channels;
            waveFormat.nSamplesPerSec = wav.fmtChunk->samplesPerSec;
            waveFormat.nAvgBytesPerSec = wav.fmtChunk->avgBytesPerSec;
            waveFormat.nBlockAlign = wav.fmtChunk->blockAlign;
            waveFormat.wBitsPerSample = wav.fmtChunk->bitsPerSample;
            waveFormat.cbSize = wav.fmtChunk->extensionSize;
        }

        HRESULT hr = IXAudio2_CreateSourceVoice(xaudio, sourceVoice, &waveFormat, 0, 1.0, 0, 0, 0);
        BOOL ok = SUCCEEDED(hr);

        if (ok) {
            hr = IXAudio2SourceVoice_SubmitSourceBuffer(
                *sourceVoice,
                &audioBuffer,
                0
            );
            ok = SUCCEEDED(hr);
        }

        if (ok) {
            IXAudio2SourceVoice_Start(*sourceVoice, 0, 0);
        }
    }
}

static void platformPlaySound(Platform *platform, Wav wav, PlatformSoundType soundType) {
    BASE_ASSERT(soundType > PlatformSoundType_None);
    BASE_ASSERT(soundType < PlatformSoundType_Count);
    bool isMusic = soundType == PlatformSoundType_Music;
    IXAudio2SourceVoice **sourceVoice = isMusic ? &platform->music : &platform->effects;

    xaudio2WavPlay(platform, sourceVoice, wav, isMusic);
}

static void platformPauseSound(Platform *platform, PlatformSoundType soundType) {
    bool isMusic = soundType == PlatformSoundType_Music;
    IXAudio2SourceVoice *sourceVoice = isMusic ? platform->music : platform->effects;

    if (sourceVoice) {
        IXAudio2SourceVoice_Stop(sourceVoice, 0, 0);
    }
}

static void platformResumeSound(Platform *platform, PlatformSoundType soundType) {
    bool isMusic = soundType == PlatformSoundType_Music;
    IXAudio2SourceVoice *sourceVoice = isMusic ? platform->music : platform->effects;

    if (sourceVoice) {
        IXAudio2SourceVoice_Start(sourceVoice, 0, 0);
    }
}

static void xaudio2Init(Platform *platform) {
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);

    bool ok = SUCCEEDED(hr);

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

        if (ok) {
            platform->hasSound = true;
        }
    }
}
