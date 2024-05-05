#include "demuxthread.h"

#include"vector"

#define DPP(x) do{                              \
std::cout<<#x<<":"<<x<<std::endl;   \
}while(0)                                              \


DemuxThread::DemuxThread(AVPacketQueue* audio_q,AVPacketQueue* video_q,AVPacketQueue* subtitle_q)
    :audio_queue(audio_q),video_queue(video_q),subtitle_queue(subtitle_q)
{

}

DemuxThread::~DemuxThread()
{
    this->Stop();
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
    int times=0;
    while(this->abort!=1){

        //延迟功能(控制速度)
        if(audio_queue->Size()>100||video_queue->Size()>100){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        //读取资源
        res=av_read_frame(ifmt_ctx,&pkt);
        if(res<0){
            if(times==0){
                char msg[128];
                av_strerror(res,msg,sizeof(msg));
                std::cout<<msg<<": "<<this->url<<std::endl;
            }else{
                printf("read frame finished:times=%d\n",times);
            }
            break;
        }
        if(pkt.stream_index==audio_index){
            //av_packet_unref(&pkt);
            audio_queue->Push(&pkt);
            //printf("times(%d):this is audio stream(%d)(size=%d)\n",times,pkt.stream_index,audio_queue->Size());
        }else if(pkt.stream_index==video_index){
            //av_packet_unref(&pkt);
            video_queue->Push(&pkt);
            //printf("times(%d):this is video stream(%d)(size=%d)\n",times,pkt.stream_index,video_queue->Size());
        }else if(pkt.stream_index==subtitle_index){
            //字幕流已经提前处理
            av_packet_unref(&pkt);
            //subtitle_queue->Push(&pkt);
            //printf("time(%d):this is subtitle stream(%d)(size=%d)\n",times,pkt.stream_index,subtitle_queue->Size());
        }else{
            //释放资源
            printf("time(%d): unused stream index:%d\n",times,pkt.stream_index);
            av_packet_unref(&pkt);
        }
        times++;

    }
}

int DemuxThread::ProcessSubtitle(AVFrameBak* bak_sys)
{
    if(this->subtitle_index<0){
        //no subtitle stream
        return 0;
    }


    //初始化解码器和上下文
    AVCodecContext* dec_ctx=avcodec_alloc_context3(NULL);
    int res=avcodec_parameters_to_context(dec_ctx,this->ifmt_ctx->streams[this->subtitle_index]->codecpar);
    if(res<0){
        char msg[128];
        av_strerror(res,msg,sizeof(msg));
        std::cout<<msg<<std::endl;
        return -1;
    }
    //寻找解码器
    const AVCodec* codec=avcodec_find_decoder(dec_ctx->codec_id);
    if(codec==NULL){
        return -1;
    }
    //打开解码器
    res=avcodec_open2(dec_ctx,codec,NULL);
    if(res<0){
        char msg[128];
        av_strerror(res,msg,sizeof(msg));
        std::cout<<msg<<std::endl;
        return -1;
    }


    //遍历读取所有的packet，并处理字幕packet
    AVPacket pkt;
    AVSubtitle sub;
    int pkt_idx=0;

    std::vector<AVSubtitle> sub_vec;


    while(av_read_frame(ifmt_ctx,&pkt)>=0){
        if(pkt.stream_index==this->subtitle_index){
            int got_subtitle;
            avcodec_decode_subtitle2(dec_ctx,&sub,&got_subtitle,&pkt);
            if(got_subtitle){
                sub_vec.push_back(sub);
                // //处理解码后的字母数据
                // std::string frame_name="sub_";
                // frame_name+=std::to_string(pkt_idx++);
                // bak_sys->save_sub_bak("subtitle",frame_name,&sub);
            }
        }
        av_packet_unref(&pkt);
    }

    //遍历处理subtitle
    fprintf(stdout, "[Events]\n");
    fprintf(stdout, "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n");
    for(AVSubtitle& sub:sub_vec){
        int size=sub.num_rects;
        for(int i=0;i<size;++i){
            AVSubtitleRect *rect=sub.rects[i];


        }
    }


    //清理资源
    avcodec_free_context(&dec_ctx);

    //重置流，便于后续音频视频流的处理
    if (avformat_seek_file(ifmt_ctx, -1, INT64_MIN, 0, INT64_MAX, 0) < 0) {
        printf("Error seeking.\n");
        return -1;
    }



    return 0;
}



AVCodecParameters *DemuxThread::AudioCodecParameters()
{
    if(audio_index==-1)
        return nullptr;
    return ifmt_ctx->streams[audio_index]->codecpar;
}

AVCodecParameters *DemuxThread::VideoCodecParameters()
{
    if(video_index==-1)
        return nullptr;
    return ifmt_ctx->streams[video_index]->codecpar;
}

AVCodecParameters *DemuxThread::SubtitleCodecParameters()
{
    if(subtitle_index==-1)
        return nullptr;
    return ifmt_ctx->streams[subtitle_index]->codecpar;
}

AVRational DemuxThread::AudioStreamTimebase()
{
    if(audio_index!=-1){
        return ifmt_ctx->streams[audio_index]->time_base;
    }
    return AVRational{0,0};
}

AVRational DemuxThread::VideoStreamTimebase()
{
    if(video_index!=-1){
        return ifmt_ctx->streams[video_index]->time_base;
    }
    return AVRational{0,0};
}

AVRational DemuxThread::SubtitleStreamTimebase()
{
    if(subtitle_index!=-1){
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
