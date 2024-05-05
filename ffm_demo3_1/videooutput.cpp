#include "videooutput.h"
#include <iostream>

VideoOutput::VideoOutput(AVFrameQueue* frame_q,AVRational time_base_,PlayerState* p_s,int v_w,int v_h)
    :frame_queue(frame_q),v_width(v_w),v_height(v_h),time_base(time_base_),p_state(p_s)
{
    sync_clocker=p_state->sync_clocker;
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

    //打开窗口
    win=SDL_CreateWindow("player",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,
                           v_width,v_height,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    if(!win){
        const char* msg=SDL_GetError();
        std::cout<<msg<<std::endl;
        return -1;
    }
    //打开渲染器
    renderer=SDL_CreateRenderer(win,-1,0);
    if(!renderer){
        const char* msg=SDL_GetError();
        std::cout<<msg<<std::endl;
        return -1;
    }
    //打开纹理
    texture=SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_IYUV,SDL_TEXTUREACCESS_STREAMING,
                                v_width,v_height);
    if(!texture){
        const char* msg=SDL_GetError();
        std::cout<<msg<<std::endl;
        return -1;
    }

    //准备观察野
    this->rect.x=0;
    this->rect.y=0;
    this->rect.w=v_width;
    this->rect.h=v_height;

    // small_texture=SDL_CreateTexture(renderer,
    //                                   SDL_PIXELFORMAT_IYUV,SDL_TEXTUREACCESS_STREAMING,
    //                                   v_width/2,v_height/2);
    // this->small_rect.x=0;
    // this->small_rect.y=0;
    // this->small_rect.w=v_width/2;
    // this->small_rect.h=v_height/2;


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
            }else if(event.key.keysym.sym==SDLK_SPACE){
                //空格键
                p_state->sync_clocker->pauseSync(p_state->isPaused);
                p_state->isPaused=!p_state->isPaused;
            }
            break;
        case SDL_QUIT:
            //退出
            return 0;
        case SDL_WINDOWEVENT:
            if(event.window.event==SDL_WINDOWEVENT_RESIZED||
                event.window.event==SDL_WINDOWEVENT_SIZE_CHANGED){
                //更新窗口
                v_width=event.window.data1;//新窗口宽
                v_height=event.window.data2;//新窗口高
                //更新观察野
                rect.w=v_width;
                rect.h=v_height;
                //更新上下文
                sws_freeContext(this->sws_ctx);
                this->sws_ctx=NULL;
                //更新纹理
                SDL_DestroyTexture(this->texture);
                this->texture=SDL_CreateTexture(renderer,
                                                  SDL_PIXELFORMAT_IYUV,SDL_TEXTUREACCESS_STREAMING,
                                                  v_width,v_height);
                if(!texture){
                    const char* msg=SDL_GetError();
                    std::cout<<msg<<std::endl;
                    return -1;
                }
            }
            break;
        default:
            break;
        }
    }
    return 0;
}




#define REFRESH_RATE 0.01
void VideoOutput::RefreshLoopWaitEvent(SDL_Event *event)
{
    double remaining_time=0.0;
    SDL_PumpEvents();
    //一旦有任何窗口事件，都不渲染画面
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


int ControlPlay(VideoOutput* user,AVFrame* video_frame){
    int64_t srcPts=video_frame->pts*av_q2d(user->time_base)*1000;//ms
    int offset=50;//移位
    int speed=1;//变速
    int64_t newPts=(srcPts-offset)/speed;
    int64_t base=user->sync_clocker->getSyncDrift();
    int sleepMs=srcPts-base;

    //太慢了(丢帧)
    if(sleepMs<-20){
        return 0;
    }

    //快了(睡眠)
    if(sleepMs>1){
        return sleepMs;
    }

    return -1;

}

void VideoOutput::videoRefresh(double *remaining_time)
{
    AVFrame* frame=NULL;
    if(p_state->isPaused){
        //暂停中
        return;
    }

    frame=frame_queue->Front();
    static int times=0;
    if(frame){
        printf("Video Frame pts=%d;SyncTime pts=%d\n",frame->pts,sync_clocker->getSyncDrift());
        int ret=ControlPlay(this,frame);
        if(ret==0){
            //丢帧
            this->frame_queue->Pop(1);
            *remaining_time=0;
            return;
        }else if(ret>0){
            //延迟播放
            return;
        }

        //渲染
        AVFrame* dst_frame=NULL;
        if(VideoPrepare(this,frame,&dst_frame)<0){
            frame=frame_queue->Pop(1);
            av_frame_free(&frame);
            return ;
        }
        //printf("Video Frame %d\n",times++);
        //printf("Src_V-fmt:%s\n",av_get_pix_fmt_name((AVPixelFormat)frame->format));
        //printf("Dst_V-fmt:%s\n",av_get_pix_fmt_name((AVPixelFormat)dst_frame->format));

        // SDL_UpdateYUVTexture(texture,&rect,
        //                      frame->data[0],frame->linesize[0],
        //                      frame->data[1],frame->linesize[1],
        //                      frame->data[2],frame->linesize[2]);
        SDL_UpdateYUVTexture(texture,&rect,
                             dst_frame->data[0],dst_frame->linesize[0],
                             dst_frame->data[1],dst_frame->linesize[1],
                             dst_frame->data[2],dst_frame->linesize[2]);

        // SDL_UpdateYUVTexture(small_texture,&small_rect,
        //                      frame->data[0],frame->linesize[0],
        //                      frame->data[1],frame->linesize[1],
        //                      frame->data[2],frame->linesize[2]);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer,texture,NULL,&rect);
        // SDL_RenderCopy(renderer,small_texture,NULL,&small_rect);
        SDL_RenderPresent(renderer);
        frame=frame_queue->Pop(1);
        av_frame_free(&frame);
        av_frame_free(&dst_frame);
    }

}


