#include "videooutput.h"
#include <iostream>

VideoOutput::VideoOutput(AVFrameQueue* frame_q,AVRational time_base_,PlayerState* p_s)
    :frame_queue(frame_q),time_base(time_base_),p_state(p_s)
{
}

VideoOutput::~VideoOutput()
{

}

static bool encodeJPGFromAVFrame(AVFrame *frame, int width, int height);


//根据窗口大小变化
int FontSize_Change(int width)
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
    if(SDL_Init(SDL_INIT_VIDEO)<0){
        const char* msg=SDL_GetError();
        std::cout<<msg<<std::endl;
        return -1;
    }

    //打开窗口
    win=SDL_CreateWindow("player",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,
                           p_state->v_width,p_state->v_height,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
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
                                p_state->v_width,p_state->v_height);
    if(!texture){
        const char* msg=SDL_GetError();
        std::cout<<msg<<std::endl;
        return -1;
    }

    //准备观察野
    this->rect.x=0;
    this->rect.y=0;
    this->rect.w=p_state->v_width;
    this->rect.h=p_state->v_height;


    //初始化字体库
    // 初始化TTF库
    if (TTF_Init() == -1) {
        printf("Failed to initialize TTF: %s\n", TTF_GetError());
        return -1;
    }
    fontSize=FontSize_Change(p_state->v_width);
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
    }
    if(texture){
        SDL_DestroyTexture(texture);
        texture=NULL;
    }
    if(renderer){
        SDL_DestroyRenderer(renderer);
        renderer=NULL;
    }

    if(win){
        SDL_DestroyWindow(win);
        win=NULL;
    }


    // 完成使用后，释放字体资源
    if(font){
    TTF_CloseFont(font);
    }
    // 清理TTF
    TTF_Quit();

    //SDL_Quit();
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
                //空格键(暂停和播放)
                p_state->isPaused=!p_state->isPaused;
                p_state->sync_clocker->pauseSync(p_state->isPaused);
            }else if(event.key.keysym.sym==SDLK_UP){
                //上方向键音量增大
                p_state->VolumeUp();
                p_state->SetVolumeFlag(true);
            }else if(event.key.keysym.sym==SDLK_DOWN){
                //下方向键音量减小
                p_state->VolumeDown();
                p_state->SetVolumeFlag(true);
            }else if(event.key.keysym.sym==SDLK_LEFT){
                //左方向键
                if(p_state->isPaused==true){
                    // 暂停情况下后退一帧
                    p_state->PlayPosBack();
                }else{
                    //播放情况下后退
                    p_state->PlayPosBack();
                }
            }else if(event.key.keysym.sym==SDLK_RIGHT){
                //右方向键快进
                if(p_state->isPaused==true){
                    // 暂停情况下前进一帧
                    p_state->StepUp();
                }else{
                    //播放情况下快进
                    p_state->PlayPosUp();
                }
            }else if(event.key.keysym.sym==SDLK_m){
                //m键加速
                p_state->SpeedUp();
                printf("Speed:%.2f\n",p_state->pSpeed);
            }else if(event.key.keysym.sym==SDLK_n){
                //n键减速
                p_state->SpeedDown();
                printf("Speed:%.2f\n",p_state->pSpeed);
            }else if(event.key.keysym.sym==SDLK_t){
                //截图
                this->isCUT=true;
            }else if(event.key.keysym.sym==SDLK_LESS){
                //<键上一个视频
                std::cout << "Less than key (<) pressed." << std::endl;
                p_state->VolumeDown();
                p_state->SetVolumeFlag(true);
            }else if(event.key.keysym.sym==SDLK_GREATER){
                //>键下一个视频
                std::cout << "Greater than key (>) pressed." << std::endl;
                p_state->VolumeUp();
                p_state->SetVolumeFlag(true);
            }
            break;
        case SDL_QUIT:
            //退出
            return 0;
        case SDL_WINDOWEVENT:
            if(event.window.event==SDL_WINDOWEVENT_RESIZED||
                event.window.event==SDL_WINDOWEVENT_SIZE_CHANGED){

                //更新窗口
                p_state->v_width=event.window.data1;//新窗口宽
                p_state->v_height=event.window.data2;//新窗口高
                //更新观察野
                rect.w=p_state->v_width;
                rect.h=p_state->v_height;

                //更新字幕
                fontSize=FontSize_Change(event.window.data1);
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
                                                  p_state->v_width,p_state->v_height);
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
    (*dst_frame)->width=user->p_state->v_width;
    (*dst_frame)->height=user->p_state->v_height;
    if(av_frame_get_buffer(*dst_frame,0)<0){
        av_frame_free(dst_frame);
        printf("VideoPrepare:av_frame_get_buffer failed\n");
        return -1;
    }
    //创建SwsContext转换
    if(user->sws_ctx==NULL){
        user->sws_ctx=sws_getContext(
            frame->width,frame->height,(AVPixelFormat)frame->format,
            user->p_state->v_width,user->p_state->v_height,AV_PIX_FMT_YUV420P,
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
    float speed=user->p_state->pSpeed;//播放速度
    int64_t newPts=(srcPts-offset)/speed;
    int64_t base=user->p_state->sync_clocker->getSyncDrift();
    int sleepMs=newPts-base;

    //太慢了(丢帧)
    if(sleepMs<-20||video_frame->serial!=user->p_state->GetSerial()){
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
    if(sleepMs>1&&sleepMs<2000){
        return sleepMs;
    }

    //快的过分了，就更新位置


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
        //t3=p_state->sync_clocker->getSyncDrift();
        printf("Video Frame pts=%d;SyncTime pts=%d\n",mFrame->frame->pts,p_state->sync_clocker->getSyncDrift());
        int ret=ControlPlay(this,mFrame);
        //printf("T1=%d,T2=%d,T3=%d\n",t1,t2,t3);
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

        if(isCUT){
            encodeJPGFromAVFrame(mFrame->frame,rect.w,rect.h);
            isCUT=false;
        }

        mFrame=frame_queue->Pop(1);
        av_frame_free(&mFrame->frame);
        delete mFrame;
        av_frame_free(&dst_frame);



        if(p_state->stepOne!=0){
            //播放一帧

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
        SDL_Rect dstRect = {(p_state->v_width/2-textSurface->w/2),p_state->v_height-(i+1)*(10+fontSize),textSurface->w,textSurface->h}; // 根据需要调整字幕位置
        SDL_RenderCopy(renderer, texture, NULL, &dstRect);
    }

}



/*
 * @功能  AVFrame保存为本地jpg图片
 * @参数  frame  yuv420P格式的AVFrame
 *        width  frame图像宽
 *        height frame图像高
 * @返回  true成功 false失败
 */
static bool encodeJPGFromAVFrame(AVFrame *frame, int width, int height)
{
    static int jpg_name_idx=0;
    if(frame == nullptr)
        return false;

    //按时间命名jpg文件
    std::string path=std::to_string(jpg_name_idx++);
    path+=".jpg";

    //创建输出上下文
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    int ret = avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, path.c_str());
    if (ret < 0)
    {
        fprintf(stderr,"avformat_alloc_output_context2 error");
        return false;
    }

    //由输出文件jpg格式自动推导编码器类型
    AVCodecContext *pCodecCtx = avcodec_alloc_context3(NULL);
    pCodecCtx->codec_id = pFormatCtx->oformat->video_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt =AV_PIX_FMT_YUV420P;
    pCodecCtx->width = width;
    pCodecCtx->height = height;
    pCodecCtx->time_base = AVRational{1, 25};
    pCodecCtx->gop_size = 25;
    pCodecCtx->max_b_frames = 0;
    //重要参数：压缩效果、码率  设置为高质量
    pCodecCtx->qcompress = 1.0;
    pCodecCtx->bit_rate = 1024 * 1024 * 3;

    //寻找并打开编码器
    const AVCodec *pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if (!pCodec)
    {
        fprintf(stderr,"avcodec_find_encoder() Failed");
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        avcodec_free_context(&pCodecCtx);
        return false;
    }

    ret = avcodec_open2(pCodecCtx, pCodec, NULL);
    if (ret < 0)
    {
        fprintf(stderr,"avcodec_open2() Failed");
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        avcodec_free_context(&pCodecCtx);
        return false;
    }

    //进行编码
    ret = avcodec_send_frame(pCodecCtx, frame);
    if (ret < 0)
    {
        fprintf(stderr,"avcodec_send_frame() Failed");
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        avcodec_free_context(&pCodecCtx);
        return false;
    }

    //得到编码jpeg数据pkt 由外部使用者释放
    AVPacket *pkt = av_packet_alloc();
    ret = avcodec_receive_packet(pCodecCtx, pkt);
    if (ret < 0)
    {
        fprintf(stderr,"avcodec_receive_packet() Failed");
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        avcodec_free_context(&pCodecCtx);
        return false;
    }

    //创建流生成本地jpg文件
    AVStream *videoStream = avformat_new_stream(pFormatCtx, 0);
    if (videoStream == NULL)
    {
        fprintf(stderr,"avformat_new_stream() Failed");
        avcodec_free_context(&pCodecCtx);
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        av_packet_free(&pkt);
        return false;
    }

    //写文件头、数据体、文件尾
    avformat_write_header(pFormatCtx, NULL);
    av_write_frame(pFormatCtx, pkt);
    av_write_trailer(pFormatCtx);

    //释放资源
    av_packet_free(&pkt);
    avformat_close_input(&pFormatCtx);
    avformat_free_context(pFormatCtx);
    avcodec_free_context(&pCodecCtx);

    return true;
}






