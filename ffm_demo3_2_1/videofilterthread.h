#ifndef VIDEOFILTERTHREAD_H
#define VIDEOFILTERTHREAD_H

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


class VideoFilterThread:public Thread
{
public:
    VideoFilterThread(AVFrameQueue* frame_q,AVFrameQueue* filt_frame_q,PlayerState* p_state_);
    ~VideoFilterThread();
    int Init(AVCodecParameters *para_,AVRational time_base_);
    int Start();
    int Stop();
    void Run();


private:
    //prime input
    AVFrameQueue* frame_queue;
    //filtered output
    AVFrameQueue* filt_frame_queue;

    //para
    AVCodecParameters * paras=nullptr;
    AVRational time_base;
    PlayerState* p_state=nullptr;

    AVFilterGraph* filter_graph=nullptr;

    AVFilterContext *buffer_ctx=nullptr,*buffersink_ctx=nullptr;


};

#endif // VIDEOFILTERTHREAD_H
