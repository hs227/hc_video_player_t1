#ifndef DECODETHREAD_H
#define DECODETHREAD_H

#include"thread.h"
#include"avpacketqueue.h"
#include"avframequeue.h"
#include"avframebak.h"
#include"avsyncclocker.h"



class DecodeThread:public Thread
{
public:
    DecodeThread(AVPacketQueue* packet_q,AVFrameQueue* frame_q,
                 AVFrameBak* frame_bak_);
    ~DecodeThread();
    int Init(AVCodecParameters* para_,int stream_id_);
    int Start();
    int Stop();
    void Run();//示例

    void AudioDecodeRun();
    void VideoDecodeRun();
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

    //缓存管理器
    AVFrameBak* frame_bak;
    int frame_idx=0;
    int frame_num=0;
};


#endif // DECODETHREAD_H
