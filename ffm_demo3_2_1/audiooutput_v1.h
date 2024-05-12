#ifndef AUDIOOUTPUT_V1_H
#define AUDIOOUTPUT_V1_H

#include"avframequeue.h"
#include"playerstate.h"


extern "C"{
#include"SDL.h"
#include"libswresample/swresample.h"
#include"libavcodec/avcodec.h"
#include"libavutil/avutil.h"
#include"libavutil/channel_layout.h"
#include"libavutil/opt.h"
}

struct AudioOutputPara_v1{
    SDL_AudioSpec spec_;
    int64_t channel_layout;//通道布局
    enum AVSampleFormat fmt;
};

//通过SDL内部管理的线程，完成异步操作
class AudioOutput_v1
{
public:
    AudioOutput_v1(AVFrameQueue* frame_q,AVRational time_base_,PlayerState* p_s);
    ~AudioOutput_v1();
    int Init(const AVCodecParameters* params);
    void DeInit(void);
    //播放
    void Play(void);
    //暂停
    void Pause(void);


    int BuildSwrContext();


public:
    AVFrameQueue* frame_queue;

    SDL_AudioDeviceID deviceId;
    //理想的音频配置和实际获得的音频配置
    AudioOutputPara_v1 desired_para,obtained_para;
    //重采样器
    SwrContext* swr_ctx;
    //时间基
    AVRational time_base;
    //时钟
    AvSyncClocker* sync_clocker;


    PlayerState* p_state;
};

#endif // AUDIOOUTPUT_V1_H