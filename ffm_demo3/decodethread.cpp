#include "decodethread.h"

#include<iostream>


DecodeThread::DecodeThread(AVPacketQueue* packet_q,AVFrameQueue* frame_q)
    :packet_queue(packet_q),frame_queue(frame_q)
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

int DecodeThread::Init(AVCodecParameters *para)
{
    if(!para){
        return -1;
    }
    //分配内存
    codec_ctx=avcodec_alloc_context3(NULL);
    //初始化
    int res=avcodec_parameters_to_context(codec_ctx,para);
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
    this->t=new std::thread(&DecodeThread::Run,this);
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
            //解码后：输出frame
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
