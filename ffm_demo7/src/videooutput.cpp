#include "videooutput.h"
#include <iostream>

VideoOutput::VideoOutput(AVFrameQueue* frame_q,AVRational time_base_,PlayerState* p_s)
    :frame_queue(frame_q),time_base(time_base_),p_state(p_s)
{
}

VideoOutput::~VideoOutput()
{

}



//根据窗口大小变化
static int FontSize_Change(int width)
{
    if(width<800){
        return 16;//ptx
    }else if(width<1024){
        return 24;//ptx
    }else if(width<1600){
        return 48;
    }
    return 60;
}

int VideoOutput::Init()
{

    //获得窗口
    this->win=p_state->vInput->sdl_user->win;

    int w,h;
    SDL_GetWindowSize(win,&w,&h);
    printf("%d,%d\n",w,h);
    p_state->vInput->sdl_user->v_width=w;
    p_state->vInput->sdl_user->v_height=h;


    //int a=p_state->vInput->sdl_user->widget->size().width();
    //int b=p_state->vInput->sdl_user->widget->size().height();

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
                                p_state->vInput->sdl_user->v_width,p_state->vInput->sdl_user->v_height);
    if(!texture){
        const char* msg=SDL_GetError();
        std::cout<<msg<<std::endl;
        return -1;
    }

    //准备观察野
    this->rect.x=0;
    this->rect.y=0;
    this->rect.w=p_state->vInput->sdl_user->v_width;
    this->rect.h=p_state->vInput->sdl_user->v_height;


    //初始化字体库
    // 初始化TTF库
    if (TTF_Init() == -1) {
        printf("Failed to initialize TTF: %s\n", TTF_GetError());
        return -1;
    }
    fontSize=FontSize_Change(p_state->vInput->sdl_user->v_width);
    font = TTF_OpenFont("msyh.ttf", fontSize);
    if (font == NULL) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return -1;
    }


    return 0;
}

void VideoOutput::DeInit()
{

    if(sws_ctx){
        sws_freeContext(sws_ctx);
        sws_ctx=NULL;
    }
    if(texture){
        SDL_DestroyTexture(texture);
        texture=NULL;
    }
    if(renderer){
        SDL_DestroyRenderer(renderer);
        renderer=NULL;
    }

    // if(win){
    //     SDL_DestroyWindow(win);
    //     win=NULL;
    // }


    // 完成使用后，释放字体资源
    if(font){
        TTF_CloseFont(font);
        font=NULL;
    }
    // 清理TTF
    TTF_Quit();

    //SDL_Quit();
}

int VideoOutput::MainLoop()
{
    for(;;){

        if(p_state->vInput->abort_f){
            //线程退出
            return 0;
        }
        if(p_state->vInput->win_resize_f){
            //更新窗口大小

            int w,h;
            SDL_GetWindowSize(win,&w,&h);

            //更新窗口
            p_state->vInput->sdl_user->v_width=w;//新窗口宽
            p_state->vInput->sdl_user->v_height=h;//新窗口高
            //更新观察野
            rect.w=p_state->vInput->sdl_user->v_width;
            rect.h=p_state->vInput->sdl_user->v_height;

            //更新字幕
            fontSize=FontSize_Change(p_state->vInput->sdl_user->v_width);
            TTF_CloseFont(font);
            font = TTF_OpenFont("msyh.ttf", fontSize);
            if (font == NULL) {
                printf("Failed to load font: %s\n", TTF_GetError());
                return -1;
            }


            //更新上下文
            sws_freeContext(this->sws_ctx);
            this->sws_ctx=NULL;
            //更新纹理
            SDL_DestroyTexture(this->texture);
            this->texture=SDL_CreateTexture(renderer,
                                              SDL_PIXELFORMAT_IYUV,SDL_TEXTUREACCESS_STREAMING,
                                              p_state->vInput->sdl_user->v_width,p_state->vInput->sdl_user->v_height);
            if(!texture){
                const char* msg=SDL_GetError();
                std::cout<<msg<<std::endl;
                return -1;
            }
        }
        if(p_state->vInput->pause_f){
            //暂停或者播放
            p_state->isPaused=!p_state->isPaused;
            p_state->sync_clocker->pauseSync(p_state->isPaused);

            p_state->vInput->pause_f=false;
        }
        if(p_state->vInput->volume_f){
            //改变音量
            p_state->SetVolumeFlag(true);//audio_filter更新音量

            p_state->vInput->volume_f=false;
        }
        if(p_state->vInput->seek_f!=0){
            //寻址
            p_state->PlayPosChange(p_state->vInput->seek_f);
            p_state->vInput->seek_f=0;
        }
        if(p_state->vInput->speed_f){
            //变速
            p_state->SetSpeedFlag(true);//audio_resample更新重采样器（重采集变速）
            p_state->SetSpeed(*p_state->vInput->speed);
            p_state->sync_clocker->speedChangeSync(p_state->GetSpeed());
            p_state->vInput->speed_f=false;
        }
        if(p_state->vInput->step_f!=0){
            //逐帧
            if(p_state->isPaused){
                //只有暂停才能逐帧
                p_state->StepUp();
            }
            p_state->vInput->step_f=0;
        }


        RefreshLoopWaitEvent();


    }
    return 0;
}




#define REFRESH_RATE 0.01
void VideoOutput::RefreshLoopWaitEvent()
{
    double remaining_time=REFRESH_RATE;

    //尝试刷新画面
    videoRefresh(&remaining_time);

    //控制帧率
    if(remaining_time>0.0){
        std::this_thread::sleep_for(std::chrono::milliseconds(int64_t(remaining_time*1000.0)));
    }

}

int VideoPrepare(VideoOutput* user,AVFrame* frame,AVFrame**dst_frame)
{
    *dst_frame=av_frame_alloc();
    (*dst_frame)->format=AV_PIX_FMT_YUV420P;
    (*dst_frame)->width=user->p_state->vInput->sdl_user->v_width;
    (*dst_frame)->height=user->p_state->vInput->sdl_user->v_height;
    if(av_frame_get_buffer(*dst_frame,0)<0){
        av_frame_free(dst_frame);
        printf("VideoPrepare:av_frame_get_buffer failed\n");
        return -1;
    }
    //创建SwsContext转换
    if(user->sws_ctx==NULL){
        user->sws_ctx=sws_getContext(
            frame->width,frame->height,(AVPixelFormat)frame->format,
            user->p_state->vInput->sdl_user->v_width,user->p_state->vInput->sdl_user->v_height,AV_PIX_FMT_YUV420P,
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


int ControlPlay(VideoOutput* user,MAVFrame* video_frame){
    int64_t srcPts=video_frame->frame->pts*av_q2d(user->time_base)*1000;//ms

    int offset=50;//由于音频延迟（可以忽略也可以补偿）
    float speed=user->p_state->GetSpeed();//播放速度
    int64_t newPts=(srcPts-offset)/speed;
    int64_t base=user->p_state->sync_clocker->getSyncDrift();
    int sleepMs=newPts-base;

    if(srcPts==0){
        printf("srcPts:%d\n",srcPts);
    }

    // if(user->p_state->vInput->speed_f){
    printf("Video Frame pts=%lld;SyncTime pts=%lld\n",srcPts,base);
    //     user->p_state->vInput->speed_f=0;
    // }


    //正常慢了(丢帧)
    if(sleepMs>=-1000&&sleepMs<-40){
        return 0;
    }
    //不是这个序列
    if(video_frame->serial!=user->p_state->GetSerial()){
        return 0;
    }
    //解码但不播放
    if(video_frame->pos==false){
        return 0;
    }

    //快了(睡眠)
    if(sleepMs>1&&sleepMs<1000){
        return sleepMs;
    }

    if(user->p_state->stepOne){
        //在逐帧时，不能使用SEEK
        return -1;
    }
    //快的过分了，就更新位置
    if(sleepMs>=400&&sleepMs<100000){
        user->p_state->PlayPosChange(sleepMs/1000);
        printf("Video Change the Pos:%d\n",sleepMs);
        return 20;
    }
    //慢的过分了，就更新位置
    if(sleepMs>=-100000&&sleepMs<-400){
        user->p_state->PlayPosChange(sleepMs/1000);
        printf("Video Change the Pos:%d\n",sleepMs);
        return 20;
    }

    if(sleepMs>=100000||sleepMs<-100000)
        printf("Video PTS failed too bad:%d\n",sleepMs);






    //正常播放
    return -1;
}

void VideoOutput::videoRefresh(double *remaining_time)
{
    MAVFrame* mFrame=NULL;


    if(p_state->isPaused){
        //暂停中
        if(p_state->stepOne!=0){
            //目前只支持进一
            p_state->sync_clocker->pauseSync(false);
            printf("Step One Begin\n");
        }else{
            return;
        }
    }
    if(p_state->GetPosFlag()!=0){
        //改位置中
        return;
    }


    mFrame=frame_queue->Front();
    static int times=0;
    if(mFrame){
        if(p_state->stepOne!=0){
            printf("Step One Control\n");
        }
        int ret=ControlPlay(this,mFrame);
        if(ret==0){
            //丢帧
            this->frame_queue->Pop(1);
            *remaining_time=0;
            return;
        }else if(ret>0){
            //延迟播放
            return;
        }
        if(p_state->stepOne!=0){
            printf("Step One In\n");
        }




        //渲染
        AVFrame* dst_frame=NULL;
        if(VideoPrepare(this,mFrame->frame,&dst_frame)<0){
            mFrame=frame_queue->Pop(1);
            av_frame_free(&mFrame->frame);
            delete mFrame;
            return ;
        }
        //printf("Video Frame %d\n",times++);
        //printf("Src_V-fmt:%s\n",av_get_pix_fmt_name((AVPixelFormat)frame->format));
        //printf("Dst_V-fmt:%s\n",av_get_pix_fmt_name((AVPixelFormat)dst_frame->format));


        SDL_UpdateYUVTexture(texture,&rect,
                             dst_frame->data[0],dst_frame->linesize[0],
                             dst_frame->data[1],dst_frame->linesize[1],
                             dst_frame->data[2],dst_frame->linesize[2]);


        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer,texture,NULL,&rect);
        //在视频渲染后渲染对应的字幕
        ttfRefresh(mFrame->frame->pts);





        SDL_RenderPresent(renderer);


        mFrame=frame_queue->Pop(1);
        av_frame_free(&mFrame->frame);
        delete mFrame;
        av_frame_free(&dst_frame);



        if(p_state->stepOne!=0){
            //播放一帧
            printf("Step One End\n");
            p_state->sync_clocker->pauseSync(true);
            p_state->stepOne=0;
        }
    }

}

void VideoOutput::ttfRefresh(long long v_pts)
{
    int index=0;
    static char msg1[512];
    static char msg2[512];
    char* buf[2];
    buf[0]=msg1;
    buf[1]=msg2;
    if(p_state->libass_sys->type==LibassUser::LU_Type::LU_NO_S){
        //没有字幕
        return;
    }


    ASS_Event* events=p_state->libass_sys->ass_track->events;
    const int event_count=p_state->libass_sys->ass_track->n_events;
    for(int i=0;i<event_count;++i){
        //获得单个事件
        const ASS_Event& event=events[i];
        if(v_pts>=event.Start&&
            v_pts<=(event.Start+event.Duration)){
            sprintf(buf[index++],"%s",event.Text);
            if(index==2)
                break;
        }
    }

    static SDL_Color textColor = {255, 0, 0, 255}; // 文本颜色（RGBA）
    for(int i=0;i<index;++i){
        SDL_Surface* textSurface = TTF_RenderUTF8_Blended(font, buf[i], textColor);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, textSurface);

        // 将字幕纹理作为覆盖层渲染到屏幕上
        SDL_Rect dstRect = {(p_state->vInput->sdl_user->v_width/2-textSurface->w/2),p_state->vInput->sdl_user->v_height-(i+1)*(10+fontSize),textSurface->w,textSurface->h}; // 根据需要调整字幕位置
        SDL_RenderCopy(renderer, texture, NULL, &dstRect);
    }

}









