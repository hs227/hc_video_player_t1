#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include"avframequeue.h"

extern "C"{
#include"SDL.h"
#include"libswresample/swresample.h"
}

struct AudioParams{
    int freq;//采样率
    int channels;//通道数
    int64_t channel_layout;//通道布局
    enum AVSampleFormat fmt;
    int frame_size;
};



class AudioOutput
{
public:
    AudioOutput(const AudioParams& audio_pra,AVFrameQueue* frame_q);
    ~AudioOutput();
    int Init();
    int DeInit();

public:
    AudioParams src_tgt;//解码后音频的参数
    AudioParams dst_tgt;//SDL实际输出的参数
    AVFrameQueue* frame_queue=nullptr;

    SwrContext* swr_ctx;
    uint8_t* audio_buf;
    uint8_t* audio_buf1;
    uint32_t audio_buf_size;
    uint32_t audio_buf1_size;
    uint32_t audio_buf_idx;
};

#endif // AUDIOOUTPUT_H
