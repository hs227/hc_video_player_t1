#include "demuxthread.h"

#include"vector"

#include<filesystem>


#define DPP(x) do{                              \
std::cout<<#x<<":"<<x<<std::endl;   \
}while(0)                                              \


DemuxThread::DemuxThread(AVPacketQueue* audio_q,AVPacketQueue* video_q,AVPacketQueue* subtitle_q,PlayerState* p_state_)
    :audio_queue(audio_q),video_queue(video_q),subtitle_queue(subtitle_q),p_state(p_state_)
{

}

DemuxThread::~DemuxThread()
{
    if(this->abort!=1){
        this->Stop();
    }
}

int DemuxThread::Init(const char *url_)
{
    this->url=url_;

    //1.分配解复用器内存
    this->ifmt_ctx=avformat_alloc_context();
    if(ifmt_ctx==nullptr){
        std::cout<<"ifmt_ctx alloc faile :"<<this->url<<std::endl;
        return -1;
    }
    //2.初始化，打开文件
    int res=avformat_open_input(&this->ifmt_ctx,this->url.c_str(),NULL,NULL);
    if(res<0){
        char msg[128];
        av_strerror(res,msg,sizeof(msg));
        std::cout<<msg<<": "<<this->url<<std::endl;
        return -1;
    }
    //3.读取信息
    res=avformat_find_stream_info(this->ifmt_ctx,NULL);
    if(res<0){
        char msg[128];
        av_strerror(res,msg,sizeof(msg));
        std::cout<<msg<<": "<<this->url<<std::endl;
        return -1;
    }
    //打印信息
    av_dump_format(this->ifmt_ctx,0,this->url.c_str(),0);

    //获取视频流，音频流，字幕流等
    audio_index=av_find_best_stream(ifmt_ctx,AVMediaType::AVMEDIA_TYPE_AUDIO,-1,-1,NULL,0);
    video_index=av_find_best_stream(ifmt_ctx,AVMediaType::AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);
    subtitle_index=av_find_best_stream(ifmt_ctx,AVMediaType::AVMEDIA_TYPE_SUBTITLE,-1,-1,NULL,0);
    DPP(audio_index);
    DPP(video_index);
    DPP(subtitle_index);


    //当前只处理同时视频流和音频流同时存在的情况
    if(audio_index<0||video_index<0){
        std::cout<<"video type not supportted."<<std::endl;
        return -1;
    }

    //字幕流
    //优先使用内挂字幕
    if(subtitle_index>0){
        //存在内挂字幕
        p_state->libass_sys->Init(LibassUser::LU_Type::LU_IN_S);
    }else{
        //是否存在外挂字幕
        std::string filename=url;
        size_t dot_pos = filename.rfind('.'); // rfind会从字符串的末尾开始寻找'.'
        if(dot_pos==std::string::npos){
            //filename+='.ass'
           printf("in subtitle process: url is wrong:%s\n",filename.c_str());
           return -1;
        }else{
            filename.replace(dot_pos,std::string::npos,".ass");
        }
        std::filesystem::path path=filename;
        if(std::filesystem::exists(path)){
            //外挂字幕文件存在
            p_state->libass_sys->Init(LibassUser::LU_Type::LU_OUT_S,&filename);

        }else{
            //没有字幕
            p_state->libass_sys->Init(LibassUser::LU_Type::LU_NO_S);
        }
    }



    return 0;
}

int DemuxThread::Start()
{
    this->t=new std::thread(&DemuxThread::Run,this);
    if(!t){
        std::cout<<"demux_thread new failed."<<std::endl;
        return -1;
    }
    return 0;
}

int DemuxThread::Stop()
{
    this->Thread::Stop();
    //关闭文件
    avformat_close_input(&ifmt_ctx);
    return 0;
}

void DemuxThread::Run()
{
    int res=0;
    AVPacket pkt;
    int loops=0;

    int64_t posTime=0;
    while(this->abort!=1){

        if(p_state->GetPosFlag()!=0){
            //位置移动了
            int flags=p_state->GetPosFlag();
            if(flags>0){
                //快进
                //flags=AVSEEK_FLAG_BACKWARD;//用于保证seek从流的开始向后寻找最接近但不超过的关键帧
                flags=0;
            }else if(flags<0){
                //后退
                flags=AVSEEK_FLAG_BACKWARD;
            }else{
                fprintf(stderr,"DemuxThread::Run:No way the PosFlag\n");
                return ;
            }
            //跳转播放时间 单位微秒
            int64_t timestamp=p_state->GetPlayPos()*1000;
            if(ifmt_ctx->start_time!=AV_NOPTS_VALUE)
                timestamp+=ifmt_ctx->start_time;
            res=avformat_seek_file(ifmt_ctx,-1,INT64_MIN,timestamp,INT64_MAX,flags);
            if(res<0){
                printf("Position seek failed\n");
            }else{
                //清空所有队列
                p_state->audio_pkt_queue->Release();
                p_state->audio_fra_queue->Release();
                p_state->audio_lift_fra_queue->Release();
                p_state->video_pkt_queue->Release();
                p_state->video_fra_queue->Release();
                p_state->video_lift_fra_queue->Release();
                //调整时钟

                p_state->sync_clocker->posChangeSync(p_state->GetPlayPos());
                posTime=p_state->sync_clocker->getSyncDrift();
                p_state->IncSerial();
                p_state->adecoderPos_flag=posTime;
                p_state->vdecoderPos_flag=posTime;

            }
            p_state->SetPosFlag(0);
        }
        if(p_state->isPaused){
            //暂停中
            if(p_state->stepOne!=0){
                //目前只支持进一
            }else{
                continue;
            }
        }

        //延迟功能(控制速度)
        if(audio_queue->Size()>100||video_queue->Size()>100){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        //读取资源
        res=av_read_frame(ifmt_ctx,&pkt);
        if(res<0){
            char msg[128];
            av_strerror(res,msg,sizeof(msg));
            std::cout<<msg<<": "<<this->url<<std::endl;
            continue;
        }


        if(pkt.stream_index==audio_index){
            //av_packet_unref(&pkt);
            audio_queue->Push(&pkt);
            //printf("times(%d):this is audio stream(%d)(size=%d)\n",loops,pkt.stream_index,audio_queue->Size());
        }else if(pkt.stream_index==video_index){
            //av_packet_unref(&pkt);
            video_queue->Push(&pkt);
            //printf("times(%d):this is video stream(%d)(size=%d)\n",loops,pkt.stream_index,video_queue->Size());
        }else if(pkt.stream_index==subtitle_index){
            if(p_state->libass_sys->type==LibassUser::LU_Type::LU_IN_S){
                subtitle_queue->Push(&pkt);//内封字幕
            }else{
                av_packet_unref(&pkt);
            }
            //printf("time(%d):this is subtitle stream(%d)(size=%d)\n",loops,pkt.stream_index,subtitle_queue->Size());
        }else{
            //释放资源
failed:
            printf("time(%d): unused stream index:%d\n",loops,pkt.stream_index);
            av_packet_unref(&pkt);
        }
        loops++;

    }
}



AVCodecParameters *DemuxThread::AudioCodecParameters()
{
    if(audio_index<0)
        return nullptr;
    return ifmt_ctx->streams[audio_index]->codecpar;
}

AVCodecParameters *DemuxThread::VideoCodecParameters()
{
    if(video_index<0)
        return nullptr;
    return ifmt_ctx->streams[video_index]->codecpar;
}

AVCodecParameters *DemuxThread::SubtitleCodecParameters()
{
    if(subtitle_index<0)
        return nullptr;
    return ifmt_ctx->streams[subtitle_index]->codecpar;
}

AVRational DemuxThread::AudioStreamTimebase()
{
    if(audio_index>=0){
        return ifmt_ctx->streams[audio_index]->time_base;
    }
    return AVRational{0,0};
}

AVRational DemuxThread::VideoStreamTimebase()
{
    if(video_index>=0){
        return ifmt_ctx->streams[video_index]->time_base;
    }
    return AVRational{0,0};
}

AVRational DemuxThread::SubtitleStreamTimebase()
{
    if(subtitle_index>=0){
        return ifmt_ctx->streams[subtitle_index]->time_base;
    }
    return AVRational{0,0};
}

int DemuxThread::AudioStreamIndex()
{
    return audio_index;
}

int DemuxThread::VideoStreamIndex()
{
    return video_index;
}

int DemuxThread::SubtitleStreamIndex()
{
    return subtitle_index;
}
