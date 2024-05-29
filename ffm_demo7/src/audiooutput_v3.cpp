#include "audiooutput_v3.h"


AudioOutput_v3::AudioOutput_v3(AVFrameQueue* frame_q,AVRational time_base_,PlayerState* p_s)
    :frame_queue(frame_q),deviceId(0),time_base(time_base_),p_state(p_s)
{
    sync_clocker=p_state->sync_clocker;
}

AudioOutput_v3::~AudioOutput_v3()
{

}

int ControlPlay(AudioOutput_v3* user,MAVFrame* audio_frame){
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


int Resample(AudioOutput_v3* user,AVFrame* frame,Uint8** out_data,int* audio_buf_size)
{
    //只负责格式转换

    //1.内存申请
    uint8_t** inData=frame->data;
    int inSamples=frame->nb_samples;
    uint8_t** outData=out_data;//未初始化
    int outSamples=(inSamples*user->config.out_sample_rate)/(user->config.params->sample_rate)+256;
    //计算输出缓冲区的总大小
    int outData_len=av_samples_get_buffer_size(NULL,user->config.out_channels,outSamples,
                                                 (AVSampleFormat)user->config.out_fmt,0);
    if(outData_len<0){
        printf("Resample process::outData_len calc failed\n");
        return -1;
    }

    //2.交错格式下内存分配(音道交错配置，因此为连续排列)
    //为整个缓冲区分配内存
    uint8_t* total_out_buffer=(uint8_t*)av_malloc(outData_len);
    //为每个通道分配内存区域
    if(total_out_buffer==NULL){
        printf("Resample process::total_out_buffer malloc failed\n");
        return -1;
    }
    for(int i=0;i<user->config.out_channels;++i){
        outData[i]=total_out_buffer+i*(outData_len/user->config.out_channels);
    }

    //3.重采样
    int res=swr_convert(user->swr_ctx,outData,outSamples,(const uint8_t **)inData,inSamples);
    if(res<0){
        printf("Resample process:swr_convert failed\n");
        return -1;
    }
    outSamples=res;//获得真实样本数
    if(audio_buf_size!=NULL){
        *audio_buf_size=av_samples_get_buffer_size(NULL,user->config.out_channels,outSamples,
                                                     (AVSampleFormat)user->config.out_fmt,0);
    }

    return outSamples;
}



int SonicPrepare(AudioOutput_v3* user,Uint8** in_data,uint8_t* out_data,int in_samples)
{
    //1.初始化sonic(move to init)(实现音频变速)
    if(user->p_state->GetSpeedFlag()==true){
        user->BuildSonicCtx();
        user->p_state->SetSpeedFlag(false);
    }

    const short* samples=(const short*)in_data[0];


    // Feed the samples
    sonicWriteShortToStream(user->sonic,samples,in_samples);

     int numSamples=(float)in_samples/user->p_state->GetSpeed();
    //int numSamples=1024;

    // produce the sample out
    int outSamples=sonicReadShortFromStream(user->sonic,(short*)out_data,numSamples);
    int new_buf_size=av_samples_get_buffer_size(NULL,user->config.out_channels,outSamples,
                                 (AVSampleFormat)user->config.out_fmt,0);
    if(new_buf_size<0)
        return 0;
    return new_buf_size;
}


//经过重采样的输出(多通道)
//planar->packet
static void  audioCallBack4(void *userdata, Uint8 * stream,int len)
{
    AudioOutput_v3* user=(AudioOutput_v3*)userdata;
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
        int samples=Resample(user,mFrame->frame,resample_data,&resample_data_len);
        //再通过sonic变速
        uint8_t so_data[8192]={0};
        int so_data_len;
        so_data_len=SonicPrepare(user,resample_data,so_data,samples);

        //packet格式(重采样后变成了packet格式)
        each_len=so_data_len;
        if(each_len>len)
            each_len=len;
        memcpy(stream_ptr,so_data,each_len);

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




int AudioOutput_v3::Init(const AVCodecParameters* params)
{
    //1.初始化音频系统
    if(SDL_Init(SDL_INIT_AUDIO)<0){
        printf("SDL_AUDIO_INIT failed\n");
        return -1;
    }

    //2.设置理想音频参数
    config.params=params;
    SDL_AudioSpec desired_spec,obtained_spec;

    //采样率
    desired_spec.freq=config.params->sample_rate;
    //采样格式
    desired_spec.format=AUDIO_S16SYS;
    //通道数(单通道还是立体声)
    desired_spec.channels=config.params->channels;
    //样本数
    desired_spec.samples=config.params->frame_size;
    //回调函数
    desired_spec.callback=audioCallBack4;
    //回调函数的用户参数
    desired_spec.userdata=this;


    //3.打开音频设备
    deviceId=SDL_OpenAudioDevice(NULL,0,&desired_spec,&obtained_spec,0);
    if(deviceId==0){
        printf("AudioDevice open failed:%s\n",SDL_GetError());
        return -1;
    }
    config.out_fmt=AV_SAMPLE_FMT_S16;
    config.out_channels=obtained_spec.channels;
    config.out_chlayout=av_get_default_channel_layout(obtained_spec.channels);
    config.out_sample_rate=obtained_spec.freq;


    printf("AudioDevice Open:ID=%d\n",deviceId);
    printf("Desired_spec:\n");
    printf("De-freq:%d\n",desired_spec.freq);
    printf("De-format:%d\n",desired_spec.format);
    printf("De-channels:%d\n",desired_spec.channels);
    printf("De-samples:%d\n",desired_spec.samples);
    printf("Obtained_spec:\n");
    printf("Ob-freq:%d\n",obtained_spec.freq);
    printf("Ob-format:%d\n",obtained_spec.format);
    printf("Ob-channels:%d\n",obtained_spec.channels);
    printf("Ob-samples:%d\n",obtained_spec.samples);


    //init swr
    BuildSwrCtx();
    //init sonic
    BuildSonicCtx();







    return 0;
}

void AudioOutput_v3::DeInit()
{
    //5.退出音频系统
    SDL_CloseAudioDevice(deviceId);
    //退出sonic
    sonicDestroyStream(sonic);
}

void AudioOutput_v3::Play()
{
    //开始音频播放
    SDL_PauseAudioDevice(deviceId,0);
}

void AudioOutput_v3::Pause()
{
    //暂停音频播放
    SDL_PauseAudioDevice(deviceId,1);
}

int AudioOutput_v3::UpdateDevice()
{
    //当播放倍速改变时，需要更新音频设备

    //1.暂停和关闭当前音频设备
    SDL_PauseAudioDevice(this->deviceId,1);
    SDL_CloseAudioDevice(this->deviceId);

    //2.新的音频规格
    //采样率
    SDL_AudioSpec desired_spec,obtained_spec;

    //采样率
    desired_spec.freq=config.params->sample_rate;
    //采样格式
    desired_spec.format=AUDIO_S16SYS;
    //通道数(单通道还是立体声)
    desired_spec.channels=config.params->channels;
    //样本数
    desired_spec.samples=config.params->frame_size;
    //回调函数
    desired_spec.callback=audioCallBack4;
    //回调函数的用户参数
    desired_spec.userdata=this;


    //3.打开新的音频设备
    deviceId=SDL_OpenAudioDevice(NULL,0,&desired_spec,&obtained_spec,0);
    if(deviceId==0){
        printf("AudioDevice open failed:%s\n",SDL_GetError());
        return -1;
    }
    config.out_fmt=AV_SAMPLE_FMT_S16;
    config.out_channels=obtained_spec.channels;
    config.out_chlayout=av_get_default_channel_layout(obtained_spec.channels);
    config.out_sample_rate=obtained_spec.freq;
}

int AudioOutput_v3::BuildSwrCtx()
{
    //由于不使用重采样实现变速，
    //在AOv3中，该函数只用于初始化
    //    static int buildTimes=0;
    //    std::cout<<"SwrContext BuildTImes= "<<buildTimes++<<"\n";

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
    res=av_opt_set_int(swr_ctx,"in_channel_layout",config.params->channel_layout,0);
    res=av_opt_set_int(swr_ctx,"out_channel_layout",config.out_chlayout,0);// AV_SAMPLE_FMT_S16
    res=av_opt_set_int(swr_ctx,"in_sample_rate",config.params->sample_rate,0);
    res=av_opt_set_int(swr_ctx,"out_sample_rate",config.out_sample_rate,0);
    //AVSampleFormat
    res=av_opt_set_sample_fmt(swr_ctx,"in_sample_fmt",(AVSampleFormat)config.params->format,0);
    res=av_opt_set_sample_fmt(swr_ctx,"out_sample_fmt",(AVSampleFormat)config.out_fmt,0);

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

int AudioOutput_v3::BuildSonicCtx()
{
    if(sonic!=NULL){
        sonicDestroyStream(sonic);
    }
    //仅处理倍速
    sonic=sonicCreateStream(config.out_sample_rate,config.out_channels);
    sonicSetSpeed(sonic,this->p_state->GetSpeed());
    sonicSetPitch(sonic,1.0);
    sonicSetRate(sonic,1.0);
}
















