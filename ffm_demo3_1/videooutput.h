#ifndef VIDEOOUTPUT_H
#define VIDEOOUTPUT_H

#include"avframequeue.h"
#include"avsyncclocker.h"
#include"playerstate.h"
#include<thread>


extern "C"{
#include"SDL.h"
#include"libavcodec/avcodec.h"
#include"libavutil/avutil.h"
#include"libavutil/imgutils.h"
#include"libswscale/swscale.h"
#include"libavutil/time.h"
}


class VideoOutput
{
public:
    VideoOutput(AVFrameQueue* frame_q,AVRational time_base_,PlayerState* p_s,int v_w,int v_h);
    ~VideoOutput();
    int Init();
    int MainLoop();
    void RefreshLoopWaitEvent(SDL_Event* event);
    void videoRefresh(double* remaining_time);
    void subtitleInit(void);
    void subtitleRefresh(void);
public:
    AVFrameQueue*frame_queue=NULL;
    AVRational time_base;//时间基
    AvSyncClocker* sync_clocker;//同步时钟
    SDL_Event event;//事件

    SDL_Window* win=NULL;//窗口
    SDL_Renderer* renderer=NULL;//渲染器
    SDL_Texture* texture=NULL;//纹理
    SDL_Rect rect;//纹理的感受野

    SDL_Texture* small_texture=NULL;//小覆盖图
    SDL_Rect small_rect;//小纹理的感受野

    int v_width=0;
    int v_height=0;


    SwsContext *sws_ctx=NULL;


    PlayerState* p_state;

};

#endif // VIDEOOUTPUT_H
