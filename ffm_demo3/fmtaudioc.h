#ifndef FMTAUDIOC_H
#define FMTAUDIOC_H


extern "C"{
#include"SDL.h"
#include"libavformat/avformat.h"
}


class FmtAudioC
{
public:
    static AVSampleFormat AudioFmtSDL2AV(const SDL_AudioFormat sdl_fmt);
    static SDL_AudioFormat AudioFmtAV2SDL(const AVSampleFormat av_fmt);
private:
    FmtAudioC(){}
    ~FmtAudioC(){}
};

#endif // FMTAUDIOC_H
