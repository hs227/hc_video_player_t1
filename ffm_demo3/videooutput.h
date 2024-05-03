#ifndef VIDEOOUTPUT_H
#define VIDEOOUTPUT_H

#include"avframequeue.h"
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
    VideoOutput(AVFrameQueue* frame_q,int v_w,int v_h);
    ~VideoOutput();
    int Init();
    int MainLoop();
    void RefreshLoopWaitEvent(SDL_Event* event);
    void videoRefresh(double* remaining_time);
public:
    AVFrameQueue*frame_queue=NULL;
    SDL_Event event;//事件

    SDL_Window* win=NULL;//窗口
    SDL_Renderer* renderer=NULL;//渲染器
    SDL_Texture* texture=NULL;//纹理
    SDL_Rect rect;//纹理的感受野

    int v_width=0;
    int v_height=0;
    uint8_t* yuv_buf=NULL;
    int yuv_buf_size=0;

    //SDL_mutex* mutex=NULL;
    SwsContext *sws_ctx=NULL;


};

#endif // VIDEOOUTPUT_H
