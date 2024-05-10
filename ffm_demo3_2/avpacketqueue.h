#ifndef AVPACKETQUEUE_H
#define AVPACKETQUEUE_H

#include"queue.h"
#include<mutex>

extern "C"{
#include"libavcodec/avcodec.h"
}

class AVPacketQueue
{
public:
    AVPacketQueue();
    ~AVPacketQueue();
    void Abort();
    int Size();
    int Push(AVPacket* val);

    AVPacket* Pop(const int timeout);

    void Release();

private:

    Queue<AVPacket*> queue;
};

#endif // AVPACKETQUEUE_H
