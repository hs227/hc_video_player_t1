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

#include"ass/ass.h"
#include"SDL_ttf.h"
}


class VideoOutput
{
public:
    VideoOutput(AVFrameQueue* frame_q,AVRational time_base_,PlayerState* p_s);
    ~VideoOutput();
    int Init();
    int MainLoop();
    void RefreshLoopWaitEvent(SDL_Event* event);
    void videoRefresh(double* remaining_time);
    void ttfRefresh(long long v_pts);


public:
    AVFrameQueue*frame_queue=NULL;
    AVRational time_base;//时间基
    SDL_Event event;//事件

    SDL_Window* win=NULL;//窗口
    SDL_Renderer* renderer=NULL;//渲染器
    SDL_Texture* texture=NULL;//纹理
    SDL_Rect rect;//纹理的感受野

    SwsContext *sws_ctx=NULL;


    PlayerState* p_state;


    //ttf
    int fontSize;//ptx
    TTF_Font* font = NULL;

};

#endif // VIDEOOUTPUT_H
