#include "decodethread.h"
#include<iostream>


DecodeThread::DecodeThread(AVPacketQueue* packet_q,AVFrameQueue* frame_q,
                           AVFrameBak* frame_bak_,PlayerState* p_state_)
    :packet_queue(packet_q),frame_queue(frame_q),frame_bak(frame_bak_),p_state(p_state_)
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
    case AVMEDIA_TYPE_SUBTITLE://字幕流
        this->t=new std::thread(&DecodeThread::SubtitleDecodeRun,this);
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
    // AVFrame* frame=av_frame_alloc();
    // int times=0;
    // while(abort!=1){

    //     //延迟功能(控制速度)
    //     if(frame_queue->Size()>50){
    //         std::this_thread::sleep_for(std::chrono::milliseconds(10));
    //         continue;
    //     }

    //     AVPacket* pkt=packet_queue->Pop(10);
    //     if(pkt){
    //         //解码前：输入packet
    //         int res=avcodec_send_packet(codec_ctx,pkt);
    //         av_packet_free(&pkt);
    //         if(res<0){
    //             char msg[128];
    //             av_strerror(res,msg,sizeof(msg));
    //             std::cout<<msg<<std::endl;
    //             break;
    //         }
    //         //解码后：输出frame(注意一个packet可能有多个frame，所有用循环读取)
    //         for(;;){
    //             res=avcodec_receive_frame(codec_ctx,frame);
    //             if(res==0){
    //                 frame_queue->Push(frame);
    //                 times++;
    //                 //printf("times(%d):%s Frame(size=%d)\n",times,codec_ctx->codec->name,frame_queue->Size());
    //             }else if(AVERROR(EAGAIN)==res){
    //                 //output is not available in this state - user must
    //                 //                          try to send new input
    //                 break;
    //             }else{
    //                 abort=1;
    //                 char msg[128];
    //                 av_strerror(res,msg,sizeof(msg));
    //                 std::cout<<msg<<std::endl;
    //                 break;
    //             }
    //         }

    //     }
    //     else{
    //         //std::cout<<"not get packet"<<std::endl;
    //     }
    // }

}

void DecodeThread::AudioDecodeRun()
{
    AVFrame* frame=av_frame_alloc();
    int times=0;// for print

    int posNewPTS=0;
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
        if(p_state->adecoderPos_flag>0&&posNewPTS==0){
            avcodec_flush_buffers(codec_ctx);//清空解码器上下文缓存
            //改位置完成需要重新定位
            //posNewPTS=p_state->adecoderPos_flag;
            p_state->adecoderPos_flag=0;
        }

        //延迟功能(控制速度)
        if(frame_queue->Size()>10){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        AVPacket* pkt=packet_queue->Pop(10);
        if(pkt){


            //解码前：输入packet
            int res=avcodec_send_packet(codec_ctx,pkt);

            if(res<0){
                av_packet_free(&pkt);
                char msg[128];
                av_strerror(res,msg,sizeof(msg));
                std::cout<<msg<<std::endl;
                break;
            }

            if(posNewPTS>0){
                if(pkt->pts>=posNewPTS){
                    posNewPTS=0;
                }else{
                    avcodec_flush_buffers(codec_ctx);//清空解码器上下文缓存(AVFrame)
                    av_packet_free(&pkt);
                    continue;
                }
            }
            av_packet_free(&pkt);

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

                    MAVFrame* mFrame=new MAVFrame();
                    mFrame->frame=frame;
                    mFrame->serial=p_state->GetSerial();
                    frame_queue->Push(mFrame);
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

    int posNewPTS=0;

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
        if(p_state->vdecoderPos_flag>0&&posNewPTS==0){
            avcodec_flush_buffers(codec_ctx);//清空解码器上下文缓存
            //改位置完成需要重新定位
            //posNewPTS=p_state->vdecoderPos_flag;
            p_state->vdecoderPos_flag=0;
        }

        //延迟功能(控制速度)
        if(frame_queue->Size()>10){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        AVPacket* pkt=packet_queue->Pop(10);
        if(pkt){
            // if(posNewPTS>0){
            //     if(pkt->pts>=posNewPTS-800){
            //         if(pkt->flags&AV_PKT_FLAG_KEY){
            //             //符合时间的第一个关键帧
            //             printf("\nposNewPTS=%d:firstKey=%d\n",posNewPTS,pkt->pts);
            //             posNewPTS=0;
            //         }else{
            //             printf("Not key:%d\n",pkt->pts);
            //             av_packet_free(&pkt);
            //             continue;
            //         }
            //     }else{

            //         continue;
            //     }
            //     // if((pkt->pts>=posNewPTS-800)&&
            //     //     (pkt->flags&AV_PKT_FLAG_KEY)){
            //     //     //符合时间的第一个关键帧
            //     //     printf("\nposNewPTS=%d:firstKey=%d\n",posNewPTS,pkt->pts);
            //     //     posNewPTS=0;
            //     // }else{
            //     //     continue;
            //     // }

            // }




            //解码前：输入packet
            int res=avcodec_send_packet(codec_ctx,pkt);

            if(res<0){
                av_packet_free(&pkt);
                char msg[128];
                av_strerror(res,msg,sizeof(msg));
                std::cout<<msg<<std::endl;
                break;
            }


            // if(posNewPTS>0){
            //     if(pkt->pts>=posNewPTS){
            //         posNewPTS=0;
            //     }else{
            //         //解码但不播放
            //         avcodec_flush_buffers(codec_ctx);//清空解码器上下文缓存(AVFrame)
            //         av_packet_free(&pkt);
            //         continue;
            //     }
            // }
            av_packet_free(&pkt);


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

                    MAVFrame* mFrame=new MAVFrame();
                    mFrame->frame=frame;
                    mFrame->serial=p_state->GetSerial();
                    mFrame->pos=true;
                    if(posNewPTS>0){
                        if(frame->pts>=posNewPTS){
                            posNewPTS=0;

                        }else{
                            //解码但不播放
                            mFrame->pos=false;
                        }
                    }



                    frame_queue->Push(mFrame);
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

void DecodeThread::SubtitleDecodeRun()
{
    if(p_state->libass_sys->type!=LibassUser::LU_Type::LU_IN_S){
        //如果不是内封字幕
        return;
    }

    ass_process_codec_private(
        p_state->libass_sys->ass_track,(char*)codec_ctx->extradata,codec_ctx->extradata_size);



    AVFrame* frame=av_frame_alloc();
    int frame_idx=0;
    int times=0;// for print
    while(abort!=1){
        //字幕流由于其特殊性，不做速度控制

        AVPacket* pkt=packet_queue->Pop(10);
        if(pkt){
            double duration=pkt->duration*av_q2d(p_state->subtitle_timebase);
            double pts=pkt->pts*av_q2d(p_state->subtitle_timebase);

            AVSubtitle sub{};
            int got_sub_ptr=0;
            avcodec_decode_subtitle2(codec_ctx,&sub,&got_sub_ptr,pkt);
            if(got_sub_ptr){
                int num=sub.num_rects;
                for(int i=0;i<num;++i){
                    auto rect=sub.rects[i];
                    ass_process_chunk(p_state->libass_sys->ass_track,rect->ass,strlen(rect->ass),pts*1000,duration*1000);
                }
            }
        }
        else{
            //std::cout<<"not get packet"<<std::endl;
        }
    }

}




