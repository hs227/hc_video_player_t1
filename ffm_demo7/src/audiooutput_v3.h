#ifndef AUDIOOUTPUT_V3_H
#define AUDIOOUTPUT_V3_H


#include"avframequeue.h"
#include"playerstate.h"


extern "C"{
#include"SDL.h"
#include"libswresample/swresample.h"
#include"libavcodec/avcodec.h"
#include"libavutil/avutil.h"
#include"libavutil/channel_layout.h"
#include"libavutil/opt.h"
#include"src/sonic.h"
}





struct AudioOutputPara_v3{
    const AVCodecParameters* params;
    uint64_t out_chlayout;
    uint8_t out_channels;
    SDL_AudioFormat out_fmt;
    int out_sample_rate;
};



// sonic version
class AudioOutput_v3
{
public:
    AudioOutput_v3(AVFrameQueue* frame_q,AVRational time_base_,PlayerState* p_s);
    ~AudioOutput_v3();
    int Init(const AVCodecParameters* params);
    void DeInit(void);
    //播放
    void Play(void);
    //暂停
    void Pause(void);

    //更新音频设备
    int UpdateDevice();

    int BuildSwrCtx();
    int BuildSonicCtx();


public:
    AVFrameQueue* frame_queue;

    SDL_AudioDeviceID deviceId;

    AudioOutputPara_v3 config;
    //重采集器
    SwrContext* swr_ctx=NULL;

    sonicStream sonic=NULL;

    //时间基
    AVRational time_base;
    //时钟
    AvSyncClocker* sync_clocker;


    PlayerState* p_state;
};







#endif // AUDIOOUTPUT_V3_H
