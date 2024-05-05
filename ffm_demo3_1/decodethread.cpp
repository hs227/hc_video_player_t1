#include "decodethread.h"
#include<iostream>


DecodeThread::DecodeThread(AVPacketQueue* packet_q,AVFrameQueue* frame_q,
                           AVFrameBak* frame_bak_)
    :packet_queue(packet_q),frame_queue(frame_q),frame_bak(frame_bak_)
{}

DecodeThread::~DecodeThread()
{
    if(t){
        Stop();
    }
    if(codec_ctx){
        avcodec_close(codec_ctx);
    }
}

int DecodeThread::Init(AVCodecParameters *para_,int stream_id_)
{
    if(!para_||stream_id_<0){
        return -1;
    }
    paras=para_;
    stream_id=stream_id_;

    //分配上下文内存
    codec_ctx=avcodec_alloc_context3(NULL);
    //初始化
    int res=avcodec_parameters_to_context(codec_ctx,para_);
    if(res<0){
        char msg[128];
        av_strerror(res,msg,sizeof(msg));
        std::cout<<msg<<std::endl;
        return -1;
    }
    //寻找解码器
    const AVCodec* codec=avcodec_find_decoder(codec_ctx->codec_id);
    if(codec==NULL){
        return -1;
    }
    //打开解码器
    res=avcodec_open2(codec_ctx,codec,NULL);
    if(res<0){
        char msg[128];
        av_strerror(res,msg,sizeof(msg));
        std::cout<<msg<<std::endl;
        return -1;
    }
    return 0;
}

int DecodeThread::Start()
{
    switch(this->paras->codec_type){
    case AVMEDIA_TYPE_VIDEO://视频流
        this->t=new std::thread(&DecodeThread::VideoDecodeRun,this);
        break;
    case AVMEDIA_TYPE_AUDIO://音频流
        this->t=new std::thread(&DecodeThread::AudioDecodeRun,this);
        break;
    default:
        t=NULL;
        break;
    }
//    this->t=new std::thread(&DecodeThread::Run,this);

    if(!t){
        std::cout<<"decode_thread new failed."<<std::endl;
        return -1;
    }
    return 0;
}

int DecodeThread::Stop()
{
    return Thread::Stop();
}


void DecodeThread::Run()
{
    AVFrame* frame=av_frame_alloc();
    int times=0;
    while(abort!=1){

        //延迟功能(控制速度)
        if(frame_queue->Size()>10){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        AVPacket* pkt=packet_queue->Pop(10);
        if(pkt){
            //解码前：输入packet
            int res=avcodec_send_packet(codec_ctx,pkt);
            av_packet_free(&pkt);
            if(res<0){
                char msg[128];
                av_strerror(res,msg,sizeof(msg));
                std::cout<<msg<<std::endl;
                break;
            }
            //解码后：输出frame(注意一个packet可能有多个frame，所有用循环读取)
            for(;;){
                res=avcodec_receive_frame(codec_ctx,frame);
                if(res==0){
                    frame_queue->Push(frame);
                    times++;
                    //printf("times(%d):%s Frame(size=%d)\n",times,codec_ctx->codec->name,frame_queue->Size());
                }else if(AVERROR(EAGAIN)==res){
                    //output is not available in this state - user must
                    //                          try to send new input
                    break;
                }else{
                    abort=1;
                    char msg[128];
                    av_strerror(res,msg,sizeof(msg));
                    std::cout<<msg<<std::endl;
                    break;
                }
            }

        }
        else{
            //std::cout<<"not get packet"<<std::endl;
        }
    }

}

void DecodeThread::AudioDecodeRun()
{
    AVFrame* frame=av_frame_alloc();
    int times=0;// for print
    while(abort!=1){

        //延迟功能(控制速度)
        if(frame_queue->Size()>10){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        AVPacket* pkt=packet_queue->Pop(10);
        if(pkt){
            //解码前：输入packet
            int res=avcodec_send_packet(codec_ctx,pkt);
            av_packet_free(&pkt);
            if(res<0){
                char msg[128];
                av_strerror(res,msg,sizeof(msg));
                std::cout<<msg<<std::endl;
                break;
            }
            //解码后：输出frame(注意一个packet可能有多个frame，所有用循环读取)
            for(;;){
                res=avcodec_receive_frame(codec_ctx,frame);
                if(res==0){
                    //先保存frame缓存
                    std::string dir_name("audio_");
                    dir_name+=std::to_string(stream_id);
                    std::string frame_name("frame_");
                    frame_name+=std::to_string(frame_idx);
                    frame_bak->save_frame_bak(dir_name,frame_name,frame);
                    //再输出
                    frame_idx++;
                    if(frame_num<frame_idx)
                        frame_num++;



                    frame_queue->Push(frame);
                    times++;
                    //printf("times(%d):%s Frame(size=%d)\n",times,codec_ctx->codec->name,frame_queue->Size());
                }else if(AVERROR(EAGAIN)==res){
                    //output is not available in this state - user must
                    //                          try to send new input
                    break;
                }else{
                    abort=1;
                    char msg[128];
                    av_strerror(res,msg,sizeof(msg));
                    std::cout<<msg<<std::endl;
                    break;
                }
            }

        }
        else{
            //std::cout<<"not get packet"<<std::endl;
        }
    }
}

void DecodeThread::VideoDecodeRun()
{
    AVFrame* frame=av_frame_alloc();
    int frame_idx=0;
    int times=0;// for print
    while(abort!=1){

        //延迟功能(控制速度)
        if(frame_queue->Size()>10){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        AVPacket* pkt=packet_queue->Pop(10);
        if(pkt){
            //解码前：输入packet
            int res=avcodec_send_packet(codec_ctx,pkt);
            av_packet_free(&pkt);
            if(res<0){
                char msg[128];
                av_strerror(res,msg,sizeof(msg));
                std::cout<<msg<<std::endl;
                break;
            }
            //解码后：输出frame(注意一个packet可能有多个frame，所有用循环读取)
            for(;;){
                res=avcodec_receive_frame(codec_ctx,frame);
                if(res==0){
                    //先保存frame缓存
                    std::string dir_name("video_");
                    dir_name+=std::to_string(stream_id);
                    std::string frame_name("frame_");
                    frame_name+=std::to_string(frame_idx++);
                    frame_bak->save_frame_bak(dir_name,frame_name,frame);
                    //再输出
                    frame_idx++;
                    if(frame_num<frame_idx)
                        frame_num++;


                    frame_queue->Push(frame);
                    times++;
                    //printf("times(%d):%s Frame(size=%d)\n",times,codec_ctx->codec->name,frame_queue->Size());
                }else if(AVERROR(EAGAIN)==res){
                    //output is not available in this state - user must
                    //                          try to send new input
                    break;
                }else{
                    abort=1;
                    char msg[128];
                    av_strerror(res,msg,sizeof(msg));
                    std::cout<<msg<<std::endl;
                    break;
                }
            }

        }
        else{
            //std::cout<<"not get packet"<<std::endl;
        }
    }
}

// void DecodeThread::SubtitleDecodeRun()
// {
//     AVFrame* frame=av_frame_alloc();
//     int frame_idx=0;
//     int times=0;// for print
//     while(abort!=1){

//         //延迟功能(控制速度)
//         if(frame_queue->Size()>10){
//             std::this_thread::sleep_for(std::chrono::milliseconds(10));
//             continue;
//         }

//         AVPacket* pkt=packet_queue->Pop(10);
//         if(pkt){
//             //解码前：输入packet
//             int res=avcodec_send_packet(codec_ctx,pkt);
//             av_packet_free(&pkt);
//             if(res<0){
//                 char msg[128];
//                 av_strerror(res,msg,sizeof(msg));
//                 std::cout<<msg<<std::endl;
//                 break;
//             }
//             //解码后：输出frame(注意一个packet可能有多个frame，所有用循环读取)
//             for(;;){
//                 res=avcodec_receive_frame(codec_ctx,frame);
//                 if(res==0){
//                     //先保存frame缓存
//                     std::string dir_name("subtitle_");
//                     dir_name+=std::to_string(stream_id);
//                     std::string frame_name("frame_");
//                     frame_name+=std::to_string(frame_idx++);
//                     frame_bak->save_frame_bak(dir_name,frame_name,frame);
//                     //再输出
//                     frame_idx++;
//                     if(frame_num<frame_idx)
//                         frame_num++;


//                     frame_queue->Push(frame);
//                     times++;
//                     //printf("times(%d):%s Frame(size=%d)\n",times,codec_ctx->codec->name,frame_queue->Size());
//                 }else if(AVERROR(EAGAIN)==res){
//                     //output is not available in this state - user must
//                     //                          try to send new input
//                     break;
//                 }else{
//                     abort=1;
//                     char msg[128];
//                     av_strerror(res,msg,sizeof(msg));
//                     std::cout<<msg<<std::endl;
//                     break;
//                 }
//             }

//         }
//         else{
//             //std::cout<<"not get packet"<<std::endl;
//         }
//     }

// }


