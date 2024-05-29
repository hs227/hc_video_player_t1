#ifndef DEMUXTHREAD_H
#define DEMUXTHREAD_H

#include"thread.h"
#include"avpacketqueue.h"
#include"avframebak.h"
#include"playerstate.h"
#include<iostream>

extern "C"{
#include"libavformat/avformat.h"
#include"libavcodec/avcodec.h"
}

class DemuxThread:public Thread
{
public:
    DemuxThread(AVPacketQueue* audio_q,AVPacketQueue* video_q,AVPacketQueue* subtitle_q,PlayerState* p_state_);
    ~DemuxThread();
    int Init(const char* url_);
    int Start();
    int Stop();
    void Run();

    AVCodecParameters* AudioCodecParameters();
    AVCodecParameters* VideoCodecParameters();
    AVCodecParameters* SubtitleCodecParameters();

    AVRational AudioStreamTimebase();
    AVRational VideoStreamTimebase();
    AVRational SubtitleStreamTimebase();

    int AudioStreamIndex();
    int VideoStreamIndex();
    int SubtitleStreamIndex();

private:
    std::string url;//文件名
    AVPacketQueue* audio_queue=nullptr;//音频流
    AVPacketQueue* video_queue=nullptr;//视频流
    AVPacketQueue* subtitle_queue=nullptr;//字幕流

    AVFormatContext* ifmt_ctx=nullptr;//上下文
    int audio_index=-1;
    int video_index=-1;
    int subtitle_index=-1;

    PlayerState* p_state=nullptr;


};

#endif // DEMUXTHREAD_H
