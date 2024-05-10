#include "fmtaudioc.h"




AVSampleFormat FmtAudioC::AudioFmtSDL2AV(const SDL_AudioFormat sdl_fmt)
{
    switch(sdl_fmt){
    case AUDIO_U8:

    case AUDIO_S8:
    case AUDIO_U16SYS:
    case AUDIO_S16SYS:
    case AUDIO_S32SYS:
    case AUDIO_F32SYS:
    default:
        break;
    }
    return AV_SAMPLE_FMT_S16;
}

SDL_AudioFormat FmtAudioC::AudioFmtAV2SDL(const AVSampleFormat av_fmt)
{
    SDL_AudioFormat res=AUDIO_S16SYS;
    switch(av_fmt){
    case AV_SAMPLE_FMT_U8:          ///< unsigned 8 bits
    case AV_SAMPLE_FMT_U8P:         ///< unsigned 8 bits, planar
        res=AUDIO_U8;break;
    case AV_SAMPLE_FMT_S16:         ///< signed 16 bits
    case AV_SAMPLE_FMT_S16P:        ///< signed 16 bits, planar
        res=AUDIO_S16SYS;break;
    case AV_SAMPLE_FMT_S32:         ///< signed 32 bits
    case AV_SAMPLE_FMT_S32P:        ///< signed 32 bits, planar
        res=AUDIO_S32SYS;break;
    case AV_SAMPLE_FMT_FLT:         ///< float
    case AV_SAMPLE_FMT_FLTP:        ///< float, planar
        res=AUDIO_F32SYS;break;
    case AV_SAMPLE_FMT_DBL:         ///< double
    case AV_SAMPLE_FMT_DBLP:        ///< double, planar
    case AV_SAMPLE_FMT_S64:         ///< signed 64 bits
    case AV_SAMPLE_FMT_S64P:        ///< signed 64 bits, planar
    default:
        break;
    }
    return res;
}
