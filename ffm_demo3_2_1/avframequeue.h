#ifndef AVFRAMEQUEUE_H
#define AVFRAMEQUEUE_H

#include"queue.h"
#include<mutex>

extern "C"{
#include"libavcodec/avcodec.h"
}

class MAVFrame
{
public:
    int serial;//用于标注frame的序号
    bool pos;//（只用于视频）如果为true，就是位置改变时使用关键帧但不播放的设计
    AVFrame* frame;
};

class AVFrameQueue
{
public:
    AVFrameQueue();
    ~AVFrameQueue();
    void Abort();
    int Size();
    int Push(MAVFrame* val);
    int Empty();

    MAVFrame* Pop(const int timeout);
    MAVFrame* Front();

    void Release();

private:


    Queue<MAVFrame*> queue;
};

#endif // AVFRAMEQUEUE_H
