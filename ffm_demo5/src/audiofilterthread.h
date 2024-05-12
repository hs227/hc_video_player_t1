#ifndef AUDIOFILTERTHREAD_H
#define AUDIOFILTERTHREAD_H

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



class AudioFilterThread:public Thread
{
public:
    AudioFilterThread(AVFrameQueue* frame_q,AVFrameQueue* filt_frame_q,PlayerState* p_state_);
    ~AudioFilterThread();
    int Init(AVCodecParameters *para_,AVRational time_base_);
    int Start();
    int Stop();
    void Run();

    //音量变化，需要重新处理
    int VolumeUpdate();



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
    //特别设计用于音频的两个缓冲区
    AVFilterContext *abuffer_ctx=nullptr,*abuffersink_ctx=nullptr;


};

#endif // AUDIOFILTERTHREAD_H
