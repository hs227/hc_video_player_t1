#ifndef DEMUXTHREAD_H
#define DEMUXTHREAD_H

#include"thread.h"
#include"avpacketqueue.h"
#include<iostream>

extern "C"{
#include"libavformat/avformat.h"
#include"libavcodec/avcodec.h"
}

class DemuxThread:public Thread
{
public:
    DemuxThread(AVPacketQueue* audio_q,AVPacketQueue* video_q);
    ~DemuxThread();
    int Init(const char* url_);
    int Start();
    int Stop();
    void Run();


    AVCodecParameters* AudioCodecParameters();
    AVCodecParameters* VideoCodecParameters();

    AVRational AudioStreamTimebase();
    AVRational VideoStreamTimebase();

private:
    std::string url;//文件名
   AVPacketQueue* audio_queue=nullptr;//音频流
   AVPacketQueue* video_queue=nullptr;//视频流

    AVFormatContext* ifmt_ctx=nullptr;
    int audio_index=-1;
    int video_index=-1;
    int subtitle_index=-1;


};

#endif // DEMUXTHREAD_H
