#ifndef AVFRAMEQUEUE_H
#define AVFRAMEQUEUE_H

#include"queue.h"

extern "C"{
#include"libavcodec/avcodec.h"
}

class AVFrameQueue
{
public:
    AVFrameQueue();
    ~AVFrameQueue();
    void Abort();
    int Size();
    int Push(AVFrame* val);
    int Empty();

    AVFrame* Pop(const int timeout);
    AVFrame* Front();

private:
    void Release();
    Queue<AVFrame*> queue;
};

#endif // AVFRAMEQUEUE_H
