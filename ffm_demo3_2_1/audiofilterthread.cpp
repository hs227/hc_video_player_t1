#include "audiofilterthread.h"

AudioFilterThread::AudioFilterThread(AVFrameQueue* frame_q,AVFrameQueue* filt_frame_q,PlayerState* p_state_)
    :frame_queue(frame_q),filt_frame_queue(filt_frame_q),p_state(p_state_)
{

}

AudioFilterThread::~AudioFilterThread()
{
    if(t){
        Stop();
    }
    if(filter_graph){
        avfilter_graph_free(&filter_graph);
    }
}

int BuildFilterGraph(AVFilterGraph** filter_graph_,AVFilterContext** abuffer_ctx_,AVFilterContext** abuffersink_ctx_,
                     const AVCodecParameters* paras,const AVRational time_base,float volumeValue)
{
    //abuffer->volume->abuffersink

    AVFilterGraph* filter_graph=nullptr;
    AVFilterContext*abuffer_ctx=nullptr;
    AVFilterContext*abuffersink_ctx=nullptr;
    AVFilterContext*volume_ctx;
    const AVFilter*abuffer;
    const AVFilter*volume;
    const AVFilter*abuffersink;

    char options_str[256];
    uint8_t ch_layout[64];

    int err;

    //0.创建滤镜图
    filter_graph=avfilter_graph_alloc();
    if(!filter_graph){
        fprintf(stderr,"init_filter_graph:avfilter_graph_alloc failed\n");
        return AVERROR_FILTER_NOT_FOUND;
    }

    //1.创建abuffer filter
    //1.1获得abuffer filter，用于输入prime_frame
    abuffer=avfilter_get_by_name("abuffer");
    if(!abuffer){
        fprintf(stderr,"init_filter_graph:abuffer filter get failed.\n");
        return AVERROR_FILTER_NOT_FOUND;
    }
    abuffer_ctx=avfilter_graph_alloc_filter(filter_graph,abuffer,"src");
    if(!abuffer_ctx){
        fprintf(stderr,"init_filter_graph:abuffersrc ctx get failed.\n");
        return AVERROR(ENOMEM);
    }
    //1.2配置abuffer filter
    //配置法1获得音频通道配置信息
    av_channel_layout_describe(&paras->ch_layout,(char*)ch_layout,sizeof(ch_layout));
    av_opt_set(abuffer_ctx,"channel_layout",(char*)ch_layout,AV_OPT_SEARCH_CHILDREN);
    av_opt_set(abuffer_ctx,"sample_fmt",av_get_sample_fmt_name((AVSampleFormat)paras->format),AV_OPT_SEARCH_CHILDREN);
    av_opt_set_q(abuffer_ctx,"time_base",time_base,AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(abuffer_ctx,"sample_rate",paras->sample_rate,AV_OPT_SEARCH_CHILDREN);
    //1.3初始化abuffer filter
    //此处已经配置完成了
    err=avfilter_init_str(abuffer_ctx,NULL);
    if(err<0){
        fprintf(stderr,"init_filter_graph:abuffer_ctx init failed\n");
        return -1;
    }

    //2.创建volume filter
    //2.1获得volume filter
    volume=avfilter_get_by_name("volume");
    if(!volume){
        fprintf(stderr,"init_filter_graph:volume filter open failed\n");
        return -1;
    }
    volume_ctx=avfilter_graph_alloc_filter(filter_graph,volume,"volume");
    if(!volume_ctx){
        fprintf(stderr,"init_filter_graph:volume filter ctx get failed\n");
        return -1;
    }
    //2.2配置volume filter
    //配置法3 string
    snprintf(options_str,sizeof(options_str),"volume=%.2f",volumeValue);
    err=avfilter_init_str(volume_ctx,options_str);
    if(err<0){
        char msg[128];
        av_strerror(err,msg,sizeof(msg));
        fprintf(stderr,"init_filter_graph: volume_ctx init failed:%s\n",msg);
        return -1;
    }

    //3.创建abuffersink filter
    //3.1获得abuffersink filter
    abuffersink=avfilter_get_by_name("abuffersink");
    if(!abuffersink){
        fprintf(stderr,"init_filter_graph:abuffersink get failed\n");
        return -1;
    }
    abuffersink_ctx=avfilter_graph_alloc_filter(filter_graph,abuffersink,"sink");
    if(!abuffersink_ctx){
        fprintf(stderr,"init_filter_graph:abuffersink_ctx alloc failed\n");
        return -1;
    }
    //3.2配置abuffersink filter
    //abuffersink无需配置
    err=avfilter_init_str(abuffersink_ctx,NULL);
    if(err<0){
        fprintf(stderr,"init_filter_graph:abuffersink_ctx init failed\n");
        return -1;
    }

    //4.链接滤镜图
    // a simple linear chain
    err=avfilter_link(abuffer_ctx,0,volume_ctx,0);
    if(err>=0){
        err=avfilter_link(volume_ctx,0,abuffersink_ctx,0);
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
    *abuffer_ctx_=abuffer_ctx;
    *abuffersink_ctx_=abuffersink_ctx;


    return 0;
}


int AudioFilterThread::Init(AVCodecParameters *para_,AVRational time_base_)
{
    paras=para_;
    time_base=time_base_;

    int ret=BuildFilterGraph(&filter_graph,&abuffer_ctx,&abuffersink_ctx,paras,time_base,p_state->GetVolumeValue());
    return ret;
}

int AudioFilterThread::Start()
{
    this->t=new std::thread(&AudioFilterThread::Run,this);

    if(!t){
        std::cout<<"audio_filter_thread new failed."<<std::endl;
        return -1;
    }
    return 0;
}

int AudioFilterThread::Stop()
{
    return Thread::Stop();
}

void AudioFilterThread::Run()
{
    MAVFrame* mFrame=nullptr;
    AVFrame* filt_frame=av_frame_alloc();
    int times=0;// for print
    while(abort!=1){

        if(p_state->GetVolumeFlag()==true){
            //更新音量
            p_state->SetVolumeFlag(false);
            VolumeUpdate();
            printf("Update volume :%.2f\n",p_state->GetVolumeValue());
        }

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
            int ret=av_buffersrc_add_frame_flags(abuffer_ctx,mFrame->frame,AV_BUFFERSRC_FLAG_KEEP_REF);
            if(ret<0){
                fprintf(stderr,"audio_filter buffersrc add failed\n");
                break;
            }
            //pull fitered_frame
            for(;;){
                ret=av_buffersink_get_frame(abuffersink_ctx,filt_frame);
                if(ret>=0){
                    MAVFrame* mfFrame=new MAVFrame();
                    mfFrame->frame=filt_frame;
                    mfFrame->serial=p_state->GetSerial();
                    filt_frame_queue->Push(mfFrame);
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

int AudioFilterThread::VolumeUpdate()
{
    AVFilterGraph* n_filter_graph;
    //特别设计用于音频的两个缓冲区
    AVFilterContext *n_abuffer_ctx,*n_abuffersink_ctx;
    int ret=BuildFilterGraph(&n_filter_graph,&n_abuffer_ctx,&n_abuffersink_ctx,
                               paras,time_base,p_state->GetVolumeValue());
    if(ret>=0){
        avfilter_graph_free(&filter_graph);
        filter_graph=n_filter_graph;
        abuffer_ctx=n_abuffer_ctx;
        abuffersink_ctx=n_abuffersink_ctx;
    }
    return ret;
}


