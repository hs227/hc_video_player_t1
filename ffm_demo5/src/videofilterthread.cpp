#include "videofilterthread.h"

VideoFilterThread::VideoFilterThread(AVFrameQueue* frame_q,AVFrameQueue* filt_frame_q,PlayerState* p_state_)
    :frame_queue(frame_q),filt_frame_queue(filt_frame_q),p_state(p_state_)
{

}

VideoFilterThread::~VideoFilterThread()
{
    if(t){
        Stop();
    }
    if(filter_graph){
        avfilter_graph_free(&filter_graph);
    }
}

int BuildFilterGraph(AVFilterGraph** filter_graph_,AVFilterContext** buffer_ctx_,AVFilterContext** buffersink_ctx_,
                     const AVCodecParameters* paras,const AVRational time_base,const PlayerState* p_state)
{
    //buffer->drawtext->buffersink
    AVFilterGraph* filter_graph=nullptr;
    AVFilterContext*buffer_ctx=nullptr;
    AVFilterContext*buffersink_ctx=nullptr;
    AVFilterContext*drawtext_ctx=nullptr;
    const AVFilter*buffer;
    const AVFilter*buffersink;
    const AVFilter* drawtext;


    char args[256];

    int err;

    //0.创建滤镜图
    filter_graph=avfilter_graph_alloc();
    if(!filter_graph){
        fprintf(stderr,"init_filter_graph:avfilter_graph_alloc failed\n");
        return AVERROR_FILTER_NOT_FOUND;
    }

    //1.创建buffer filter
    //1.1获得buffer filter，用于输入prime_frame
    buffer=avfilter_get_by_name("buffer");
    if(!buffer){
        fprintf(stderr,"init_filter_graph:buffer filter get failed.\n");
        return AVERROR_FILTER_NOT_FOUND;
    }
    buffer_ctx=avfilter_graph_alloc_filter(filter_graph,buffer,"src");
    if(!buffer_ctx){
        fprintf(stderr,"init_filter_graph:buffer ctx get failed.\n");
        return AVERROR(ENOMEM);
    }
    //1.2配置buffer filter
    snprintf(args,sizeof(args),"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d",paras->width,paras->height,paras->format,time_base.num,time_base.den);
    //1.3初始化buffer filter
    err=avfilter_init_str(buffer_ctx,args);
    if(err<0){
        fprintf(stderr,"init_filter_graph:buffer_ctx init failed\n");
        return -1;
    }

    //2.创建drawtext filter
    //2.1获得drawtext filter
    drawtext=avfilter_get_by_name("drawtext");
    if(!drawtext){
        fprintf(stderr,"init_filter_graph:drawtext filter open failed\n");
        return -1;
    }
    drawtext_ctx=avfilter_graph_alloc_filter(filter_graph,drawtext,"drawtext");
    if(!drawtext_ctx){
        fprintf(stderr,"init_filter_graph:drawtext filter ctx get failed\n");
        return -1;
    }
    //2.2配置drawtext filter
    snprintf(args,sizeof(args),"fontsize=100:fontfile=FreeSerif.ttf:text='Logo':x=%d:y=%d: box=1: boxcolor=0x00000000",0,0);
    err=avfilter_init_str(drawtext_ctx,args);
    if(err<0){
        char msg[128];
        av_strerror(err,msg,sizeof(msg));
        fprintf(stderr,"init_filter_graph: drawtext_ctx init failed:%s\n",msg);
        return -1;
    }

    //3.创建buffersink filter
    //3.1获得buffersink filter
    buffersink=avfilter_get_by_name("buffersink");
    if(!buffersink){
        fprintf(stderr,"init_filter_graph:buffersink get failed\n");
        return -1;
    }
    buffersink_ctx=avfilter_graph_alloc_filter(filter_graph,buffersink,"sink");
    if(!buffersink_ctx){
        fprintf(stderr,"init_filter_graph:buffersink_ctx alloc failed\n");
        return -1;
    }
    //3.2配置buffersink filter
    err=avfilter_init_str(buffersink_ctx,NULL);
    if(err<0){
        fprintf(stderr,"init_filter_graph:buffersink_ctx init failed\n");
        return -1;
    }

    //4.链接滤镜图
    // a simple linear chain
    //err=avfilter_link(buffer_ctx,0,buffersink_ctx,0);
    err=avfilter_link(buffer_ctx,0,drawtext_ctx,0);

    if(err>=0){
        err=avfilter_link(drawtext_ctx,0,buffersink_ctx,0);
    }


    if(err<0){
        fprintf(stderr,"init_filter_graph: filters connect failed\n");
        return -1;
    }

    //5.配置滤镜图
    //手动连接，无需配置
    err=avfilter_graph_config(filter_graph,NULL);
    if(err<0){
        fprintf(stderr,"filter_graph config failed\n");
        return -1;
    }

    *filter_graph_=filter_graph;
    *buffer_ctx_=buffer_ctx;
    *buffersink_ctx_=buffersink_ctx;


    return 0;
}

int VideoFilterThread::Init(AVCodecParameters *para_, AVRational time_base_)
{
    paras=para_;
    time_base=time_base_;

    int ret=BuildFilterGraph(&filter_graph,&buffer_ctx,&buffersink_ctx,paras,time_base,p_state);
    return ret;
}

int VideoFilterThread::Start()
{
    this->t=new std::thread(&VideoFilterThread::Run,this);

    if(!t){
        std::cout<<"video_filter_thread new failed."<<std::endl;
        return -1;
    }
    return 0;
}

int VideoFilterThread::Stop()
{
    Thread::Stop();
    if(filter_graph){
        avfilter_graph_free(&filter_graph);
    }
    return 0;
}

void VideoFilterThread::Run()
{
    MAVFrame* mFrame=nullptr;
    AVFrame* filt_frame=av_frame_alloc();
    int times=0;// for print
    while(abort!=1){

        if(p_state->isPaused){
            //暂停中
            if(p_state->stepOne!=0){
                //目前只支持进一
            }else{
                continue;
            }

        }
        if(p_state->GetPosFlag()!=0){
            //改位置中
            continue;
        }

        //延迟功能(控制速度)
        if(filt_frame_queue->Size()>10){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        mFrame=frame_queue->Pop(10);
        if(mFrame){
            // push prime_frame
            int ret=av_buffersrc_add_frame_flags(buffer_ctx,mFrame->frame,AV_BUFFERSRC_FLAG_KEEP_REF);
            if(ret<0){
                fprintf(stderr,"audio_filter buffersrc add failed\n");
                break;
            }
            //pull fitered_frame
            for(;;){
                ret= av_buffersink_get_frame(buffersink_ctx,filt_frame);
                if(ret>=0){
                    MAVFrame* sinkmfFrame=new MAVFrame();
                    sinkmfFrame->frame=filt_frame;
                    sinkmfFrame->serial=p_state->GetSerial();
                    sinkmfFrame->pos=mFrame->pos;
                    filt_frame_queue->Push(sinkmfFrame);
                    times++;
                    //printf("times(%d):%s Frame(size=%d)\n",times,codec_ctx->codec->name,filt_frame_queue->Size());
                }else if(AVERROR(EAGAIN)==ret){
                    break;
                }else{
                    abort=1;
                    char msg[128];
                    av_strerror(ret,msg,sizeof(msg));
                    std::cout<<msg<<std::endl;
                    break;
                }
            }
            delete mFrame;
        }
        else{
            //std::cout<<"not get packet"<<std::endl;
        }
    }
}
