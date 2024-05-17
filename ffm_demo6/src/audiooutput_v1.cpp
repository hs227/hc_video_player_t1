#include "audiooutput_v1.h"

AudioOutput_v1::AudioOutput_v1(AVFrameQueue* frame_q,AVRational time_base_,PlayerState* p_s)
    :frame_queue(frame_q),swr_ctx(NULL),deviceId(0),time_base(time_base_),p_state(p_s)
{
    memset(&desired_para,0,sizeof(desired_para));
    memset(&obtained_para,0,sizeof(obtained_para));

    sync_clocker=p_state->sync_clocker;
}

AudioOutput_v1::~AudioOutput_v1()
{

}

int ControlPlay(AudioOutput_v1* user,MAVFrame* audio_frame){
    int64_t srcPts=audio_frame->frame->pts*av_q2d(user->time_base)*1000;//ms

    int offset=0;//由于音频延迟（可以忽略也可以补偿）
    float speed=user->p_state->GetSpeed();//播放速度(pts变速)
    //float speed=1.0f;//(改成使用重采集实现播放速度变化)
    int64_t newPts=(srcPts-offset)/speed;
    int64_t base=user->sync_clocker->getSyncDrift();
    int sleepMs=newPts-base;

    //printf("Audio Frame pts=%lld;SyncTime pts=%lld\n",srcPts,base);

    //正常慢了(丢帧)
    if(sleepMs>=-1000&&sleepMs<-40){
        return 0;
    }
    //frame不属于该序列
    if(audio_frame->serial!=user->p_state->GetSerial()){
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
    // //快的过分了，就更新位置
    // if(sleepMs>=400&&sleepMs<100000){
    //     user->p_state->PlayPosChange(sleepMs/1000);
    //     printf("Audio Change the Pos:%d\n",sleepMs);
    // }
    // //慢的过分了，就更新位置
    // if(sleepMs>=-100000&&sleepMs<-400){
    //     user->p_state->PlayPosChange(sleepMs/1000);
    //     printf("Audio Change the Pos:%d\n",sleepMs);
    // }

    if(sleepMs>=100000||sleepMs<-100000)
        printf("Audio PTS failed too bad:%d\n",sleepMs);

    //正常播放
    return -1;
}


int Resample(AudioOutput_v1* user,AVFrame* frame,Uint8** out_data,int* audio_buf_size)
{
    //1.初始化重采样器(move to init)(如果使用重采集实现音频变速)
    if(user->p_state->GetSpeedFlag()==true){
        user->BuildSwrContext();
        user->p_state->SetSpeedFlag(false);
    }

    //2.执行音频重采样

    uint8_t** inData=frame->data;
    int inSamples=frame->nb_samples;
    uint8_t** outData=out_data;//未初始化
    int outSamples=(inSamples*user->obtained_para.spec_.freq)/(user->desired_para.spec_.freq/user->p_state->GetSpeed())+256;
    //计算输出缓冲区的总大小
    int outData_len=av_samples_get_buffer_size(NULL,user->obtained_para.spec_.channels,outSamples,
                                                        (AVSampleFormat)user->obtained_para.fmt,0);
    if(outData_len<0){
        printf("Resample process::outData_len calc failed\n");
        return -1;
    }

    //交错格式下内存分配(音道交错配置，因此为连续排列)
    //为整个缓冲区分配内存
    uint8_t* total_out_buffer=(uint8_t*)av_malloc(outData_len);
    //为每个通道分配内存区域
    if(total_out_buffer==NULL){
        printf("Resample process::total_out_buffer malloc failed\n");
        return -1;
    }
    for(int i=0;i<user->obtained_para.spec_.channels;++i){
        outData[i]=total_out_buffer+i*(outData_len/user->obtained_para.spec_.channels);
    }

    //planar格式下内存分配（各音道内存不连续）
    // for(int i=0;i<user->obtained_para.spec_.channels;++i){
    //     outData[i]=(uint8_t*)av_malloc(outData_len/user->obtained_para.spec_.channels);
    //     if(!outData[i]){
    //         printf("Resample process::av_malloc outData[%d]\n",i);
    //         return -1;
    //     }
    // }



    int res=swr_convert(user->swr_ctx,outData,outSamples,(const uint8_t **)inData,inSamples);
    if(res<0){
        printf("Resample process:swr_convert failed\n");
        return -1;
    }
    outSamples=res;//获得真实样本数
    if(audio_buf_size!=NULL){
        *audio_buf_size=av_samples_get_buffer_size(NULL,user->obtained_para.spec_.channels,outSamples,
                                                     (AVSampleFormat)user->obtained_para.fmt,0);
    }

    return 0;
}

//输出静音
// void  audioCallBack1(void *userdata, Uint8 * stream,int len)
// {
//     AudioOutput_v1* user_=(AudioOutput_v1*)userdata;
//     memset(stream,0,len);
// }

//不经过重采样的输出(单通道)
// void  audioCallBack2(void *userdata, Uint8 * stream,int len)
// {
//     AudioOutput_v1* user=(AudioOutput_v1*)userdata;
//     Uint8* stream_ptr=stream;
//     int each_len=0;
//     while(len>0){


//         AVFrame* frame=user->frame_queue->Pop(10);
//         if(!frame){
//             //没收到frame
//             break;
//         }
//         //收到frame，直接填入
//         int frame_size=av_samples_get_buffer_size(
//             NULL,frame->channels,frame->nb_samples,
//             (AVSampleFormat)frame->format,1);
//         each_len=frame_size;
//         if(each_len>len)
//             each_len=len;
//         //单channel输入(planar格式)
//         //如果是交错存储格式，则是全部channel输出
//         memcpy(stream_ptr,frame->data[0],each_len);
//         stream_ptr+=each_len;
//         len-=each_len;
//     }

//     //如果有还未填满的数据则填满静音
//     if(len>0){
//         memset(stream_ptr,0,len);
//     }
// }

//不经过重采样的输出(多通道)
// void  audioCallBack3(void *userdata, Uint8 * stream,int len)
// {

//     AudioOutput_v1* user=(AudioOutput_v1*)userdata;
//     Uint8* stream_ptr=stream;
//     int each_len=0;
//     while(len>0){
//         AVFrame* frame=user->frame_queue->Pop(10);
//         if(!frame){
//             //没收到frame
//             break;
//         }
//         //收到frame，直接填入
//         int frame_size=av_samples_get_buffer_size(
//             NULL,frame->channels,frame->nb_samples,
//             (AVSampleFormat)frame->format,1);
//         each_len=frame_size/frame->channels;
//         for(int i=0;i<frame->channels&&len>0;++i){
//             if(each_len>len)
//                 each_len=len;
//             //多channel输入
//             memcpy(stream_ptr,frame->data[i],each_len);
//             stream_ptr+=each_len;
//             len-=each_len;
//         }
//     }

//     //如果有还未填满的数据则填满静音
//     if(len>0){
//         memset(stream_ptr,0,len);
//     }
// }

//经过重采样的输出(多通道)
//planar->packet
void  audioCallBack4(void *userdata, Uint8 * stream,int len)
{
    AudioOutput_v1* user=(AudioOutput_v1*)userdata;
    Uint8* stream_ptr=stream;
    int each_len=0;

    MAVFrame* mFrame=nullptr;
    while(len>0){
        if(user->p_state->GetSpeedFlag()){
            //播放速度改变了
            printf("audioCallBack4:Why Change\n");
        }
        if(user->p_state->isPaused){
            //暂停中
            if(user->p_state->stepOne!=0){
                //目前只支持进一
                printf("audioCallBack4:Step\n");
            }else{
                break;
            }
        }
        if(user->p_state->GetPosFlag()!=0){
            //改位置中
            break;
        }
        mFrame=user->frame_queue->Front();
        if(!mFrame){
            //没收到frame
            break;
        }
        //时钟开始控制
        int ret=ControlPlay(user,mFrame);
        if(ret==0){
            //丢帧
            user->frame_queue->Pop(1);
            continue;
        }else if(ret>0){
            //延迟播放
            break;
        }

        //收到frame，重采样

        //resample后的frame->data
        uint8_t *resample_data[AV_NUM_DATA_POINTERS]={NULL};
        int resample_data_len;
        Resample(user,mFrame->frame,resample_data,&resample_data_len);

        //planar格式
        // each_len=resample_data_len/user->obtained_para.spec_.channels;
        // for(int i=0;i<user->obtained_para.spec_.channels&&len>0;++i){
        //     if(each_len>len)
        //         each_len=len;
        //     //多channel输入
        //     memcpy(stream_ptr,resample_data[i],each_len);
        //     av_free(resample_data[i]);
        //     stream_ptr+=each_len;
        //     len-=each_len;
        // }

        //packet格式(重采样后变成了packet格式)
        each_len=resample_data_len;
        if(each_len>len);
        memcpy(stream_ptr,resample_data[0],each_len);
        av_free(resample_data[0]);
        stream_ptr+=each_len;
        len-=each_len;

       mFrame=user->frame_queue->Pop(1);
        if(mFrame==NULL){
           continue;
       }
        av_frame_free(&mFrame->frame);
       delete mFrame;
        mFrame=NULL;

    }

    //如果有还未填满的数据则填满静音
    if(len>0){
        memset(stream_ptr,0,len);
    }


}




int AudioOutput_v1::Init(const AVCodecParameters* params)
{
    //1.初始化音频系统
    if(SDL_Init(SDL_INIT_AUDIO)<0){
        printf("SDL_AUDIO_INIT failed\n");
        return -1;
    }

    //2.设置理想音频参数
    //采样率
    desired_para.spec_.freq=params->sample_rate;
    //采样格式
    //desired_para.spec_.format=FmtAudioC::AudioFmtAV2SDL((AVSampleFormat)params->format);
    desired_para.spec_.format=AUDIO_S16SYS;
    //通道数(单通道还是立体声)
    desired_para.spec_.channels=params->channels;
    //样本数
    desired_para.spec_.samples=params->frame_size;
    //回调函数
    desired_para.spec_.callback=audioCallBack4;
    //回调函数的用户参数
    desired_para.spec_.userdata=this;


    //3.打开音频设备
    deviceId=SDL_OpenAudioDevice(NULL,0,&desired_para.spec_,&obtained_para.spec_,0);
    if(deviceId==0){
        printf("AudioDevice open failed:%s\n",SDL_GetError());
        return -1;
    }
    desired_para.channel_layout=params->channel_layout;
    desired_para.fmt=(AVSampleFormat)params->format;
    obtained_para.channel_layout=av_get_default_channel_layout(obtained_para.spec_.channels);
    obtained_para.fmt=AV_SAMPLE_FMT_S16;


    printf("AudioDevice Open:ID=%d\n",deviceId);
    printf("Desired_spec:\n");
    printf("De-freq:%d\n",desired_para.spec_.freq);
    printf("De-format:%d\n",desired_para.spec_.format);
    printf("De-fmt:%s\n",av_get_sample_fmt_name(desired_para.fmt));
    printf("De-channels:%d\n",desired_para.spec_.channels);
    printf("De-samples:%d\n",desired_para.spec_.samples);
    printf("De-channel_layout:%d\n",desired_para.channel_layout);
    printf("Obtained_spec:\n");
    printf("Ob-freq:%d\n",obtained_para.spec_.freq);
    printf("Ob-format:%d\n",obtained_para.spec_.format);
    printf("Ob-fmt:%s\n",av_get_sample_fmt_name(obtained_para.fmt));
    printf("Ob-channels:%d\n",obtained_para.spec_.channels);
    printf("Ob-samples:%d\n",obtained_para.spec_.samples);
    printf("Ob-channel_layout:%d\n",obtained_para.channel_layout);


    //init swr
    BuildSwrContext();







    return 0;
}

void AudioOutput_v1::DeInit()
{
    //5.退出音频系统
    SDL_CloseAudioDevice(deviceId);
    if(swr_ctx){
        swr_free(&swr_ctx);
    }

    //SDL_Quit();
}

void AudioOutput_v1::Play()
{
    //开始音频播放
    SDL_PauseAudioDevice(deviceId,0);
}

void AudioOutput_v1::Pause()
{
    //暂停音频播放
    SDL_PauseAudioDevice(deviceId,1);
}

int AudioOutput_v1::BuildSwrContext()
{
    static int buildTimes=0;
    std::cout<<"SwrContext BuildTImes= "<<buildTimes++<<"\n";
    //swr_init
    //分配内存
    if(swr_ctx){
        swr_free(&swr_ctx);
    }
    swr_ctx=swr_alloc();
    if(swr_ctx==NULL){
        printf("Init:swr_alloc failed\n");
        return -1;
    }
    //设置上下文参数
    int res=0;
    res=av_opt_set_int(swr_ctx,"in_channel_layout",desired_para.channel_layout,0);
    res=av_opt_set_int(swr_ctx,"out_channel_layout",obtained_para.channel_layout,0);
    res=av_opt_set_int(swr_ctx,"in_sample_rate",desired_para.spec_.freq,0);
    res=av_opt_set_int(swr_ctx,"out_sample_rate",obtained_para.spec_.freq,0);
    //AVSampleFormat
    res=av_opt_set_sample_fmt(swr_ctx,"in_sample_fmt",(AVSampleFormat)desired_para.fmt,0);
    res=av_opt_set_sample_fmt(swr_ctx,"out_sample_fmt",(AVSampleFormat)obtained_para.fmt,0);

    //初始化上下文
    if((res=swr_init(swr_ctx))<0){
        printf("Init::swr_init failed\n");
        char msg[128];
        av_strerror(res,msg,128);
        printf("ERROR:%s\n",msg);
        swr_free(&swr_ctx);
        return -1;
    }
}














