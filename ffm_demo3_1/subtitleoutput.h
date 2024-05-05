#ifndef SUBTITLEOUTPUT_H
#define SUBTITLEOUTPUT_H

#include"avpacketqueue.h"
#include"avsyncclocker.h"

extern "C"{

#include"ass/ass.h"
}

class SubtitleOutput
{
public:
    SubtitleOutput(AVPacketQueue* frame_q,AVRational time_base_,AvSyncClocker* sync_clocker_);
    ~SubtitleOutput();


public:
    //字幕流
    AVPacketQueue* pkt_queue;
    //时间基
    AVRational time_base;
    //同步时钟
    AvSyncClocker* sync_clocker;

    ASS_Library* libass=NULL;
    ASS_Renderer* ass_renderer=NULL;
    ASS_Track* ass_track=NULL;

};

#endif // SUBTITLEOUTPUT_H
