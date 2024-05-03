#include "videooutput.h"
#include <iostream>

VideoOutput::VideoOutput(AVFrameQueue* frame_q,int v_w,int v_h)
    :frame_queue(frame_q),v_width(v_w),v_height(v_h)
{

}

VideoOutput::~VideoOutput()
{

}

int VideoOutput::Init()
{
    if(SDL_Init(SDL_INIT_VIDEO)<0){
        const char* msg=SDL_GetError();
        std::cout<<msg<<std::endl;
        return -1;
    }

    win=SDL_CreateWindow("player",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,
                           v_width,v_height,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    if(!win){
        const char* msg=SDL_GetError();
        std::cout<<msg<<std::endl;
        return -1;
    }

    renderer=SDL_CreateRenderer(win,-1,0);
    if(!renderer){
        const char* msg=SDL_GetError();
        std::cout<<msg<<std::endl;
        return -1;
    }

    texture=SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_IYUV,SDL_TEXTUREACCESS_STREAMING,
                                v_width,v_height);
    if(!texture){
        const char* msg=SDL_GetError();
        std::cout<<msg<<std::endl;
        return -1;
    }

    yuv_buf_size=v_width*v_height*1.5;
    yuv_buf=(uint8_t*)malloc(yuv_buf_size);






    return 0;
}

int VideoOutput::MainLoop()
{
    SDL_Event event;
    for(;;){
        RefreshLoopWaitEvent(&event);
        //读取事件
        switch(event.type){
        case SDL_KEYDOWN:
            if(event.key.keysym.sym==SDLK_ESCAPE){
                //ESC KEY
                return 0;
            }
            break;
        case SDL_QUIT:
            //退出
            return 0;
        default:
            break;
        }
    }
}




#define REFRESH_RATE 0.01
void VideoOutput::RefreshLoopWaitEvent(SDL_Event *event)
{
    double remaining_time=0.0;
    SDL_PumpEvents();
    while(!SDL_PeepEvents(event,1,SDL_GETEVENT,SDL_FIRSTEVENT,SDL_LASTEVENT)){
        if(remaining_time>0.0){
            std::this_thread::sleep_for(std::chrono::milliseconds(int64_t(remaining_time*1000.0)));
        }
        remaining_time=REFRESH_RATE;//控制刷新频率
        //尝试刷新画面
        videoRefresh(&remaining_time);
        SDL_PumpEvents();
    }
}

int VideoPrepare(VideoOutput* user,AVFrame* frame,AVFrame**dst_frame)
{
    *dst_frame=av_frame_alloc();
    (*dst_frame)->format=AV_PIX_FMT_YUV420P;
    (*dst_frame)->width=user->v_width;
    (*dst_frame)->height=user->v_height;
    if(av_frame_get_buffer(*dst_frame,0)<0){
        av_frame_free(dst_frame);
        printf("VideoPrepare:av_frame_get_buffer failed\n");
        return -1;
    }
    //创建SwsContext转换
    if(user->sws_ctx==NULL){
        user->sws_ctx=sws_getContext(
            frame->width,frame->height,(AVPixelFormat)frame->format,
            user->v_width,user->v_height,AV_PIX_FMT_YUV420P,
            SWS_BILINEAR,NULL,NULL,NULL);
        if(!user->sws_ctx){
            printf("VideoPrepare:sws_ctx is NULL.\n");
            return -1;
        }
    }



    //执行转换
    sws_scale(user->sws_ctx,frame->data,frame->linesize,
              0,frame->height,
              (*dst_frame)->data,(*dst_frame)->linesize);

    return 0;
}


void VideoOutput::videoRefresh(double *remaining_time)
{
    AVFrame* frame=NULL;
    frame=frame_queue->Front();
    static int times=0;
    if(frame){




        //渲染
        AVFrame* dst_frame=NULL;
        if(VideoPrepare(this,frame,&dst_frame)<0){
            frame=frame_queue->Pop(1);
            av_frame_free(&frame);
            return ;
        }
        printf("Video Frame %d\n",times++);
        printf("Src_V-fmt:%s\n",av_get_pix_fmt_name((AVPixelFormat)frame->format));
        printf("Dst_V-fmt:%s\n",av_get_pix_fmt_name((AVPixelFormat)dst_frame->format));
        rect.x=0;
        rect.y=0;
        rect.w=v_width;
        rect.h=v_height;
        // SDL_UpdateYUVTexture(texture,&rect,
        //                      frame->data[0],frame->linesize[0],
        //                      frame->data[1],frame->linesize[1],
        //                      frame->data[2],frame->linesize[2]);
        SDL_UpdateYUVTexture(texture,&rect,
                             dst_frame->data[0],dst_frame->linesize[0],
                             dst_frame->data[1],dst_frame->linesize[1],
                             dst_frame->data[2],dst_frame->linesize[2]);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer,texture,NULL,&rect);
        SDL_RenderPresent(renderer);
        frame=frame_queue->Pop(1);
        av_frame_free(&frame);
        av_frame_free(&dst_frame);
    }

}
