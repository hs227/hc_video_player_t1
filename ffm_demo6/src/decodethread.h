#ifndef DECODETHREAD_H
#define DECODETHREAD_H

#include"thread.h"
#include"avpacketqueue.h"
#include"avframequeue.h"
#include"avframebak.h"
#include"avsyncclocker.h"
#include"playerstate.h"

extern "C"{
#include"libavfilter/avfilter.h"
#include"libavfilter/buffersink.h"
#include"libavfilter/buffersrc.h"
#include"libavutil/opt.h"
}



class DecodeThread:public Thread
{
public:
    DecodeThread(AVPacketQueue* packet_q,AVFrameQueue* frame_q,
                 AVFrameBak* frame_bak_,PlayerState* p_state_);
    ~DecodeThread();
    int Init(AVCodecParameters* para_,int stream_id_,AVRational timebase_);
    int Start();
    int Stop();


    void AudioDecodeRun();
    void VideoDecodeRun();
    void SubtitleDecodeRun();

    void Run();//示例
private:
    // 上下文
    AVCodecContext* codec_ctx=nullptr;
    // input
    AVPacketQueue* packet_queue=nullptr;
    // output
    AVFrameQueue* frame_queue=nullptr;
    //para
    AVCodecParameters * paras=nullptr;
    int stream_id=-1;
    AVRational timebase;

    //缓存管理器
    AVFrameBak* frame_bak;
    int frame_idx=0;
    int frame_num=0;

    PlayerState* p_state=nullptr;
};


#endif // DECODETHREAD_H
