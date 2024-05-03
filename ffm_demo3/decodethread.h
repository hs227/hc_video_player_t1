#ifndef DECODETHREAD_H
#define DECODETHREAD_H

#include"thread.h"
#include"avpacketqueue.h"
#include"avframequeue.h"


class DecodeThread:public Thread
{
public:
    DecodeThread(AVPacketQueue* packet_q,AVFrameQueue* frame_q);
    ~DecodeThread();
    int Init(AVCodecParameters* para);
    int Start();
    int Stop();
    int Size();
    void Run();
private:
    AVCodecContext* codec_ctx=nullptr;
    AVPacketQueue* packet_queue=nullptr;
    AVFrameQueue* frame_queue=nullptr;
};

#endif // DECODETHREAD_H
