#include "audiooutput_v1.h"

AudioOutput_v1::AudioOutput_v1(AVFrameQueue* frame_q)
    :frame_queue(frame_q),swr_ctx(NULL),deviceId(0)
{
    memset(&desired_para,0,sizeof(desired_para));
    memset(&obtained_para,0,sizeof(obtained_para));

}

AudioOutput_v1::~AudioOutput_v1()
{

}


int Resample(AudioOutput_v1* user,AVFrame* frame,Uint8** out_data,int* audio_buf_size)
{
    //1.初始化重采样器
    if(user->swr_ctx==NULL){
        //分配内存
        user->swr_ctx=swr_alloc();
        if(user->swr_ctx==NULL){
            printf("Resample Init:swr_alloc failed\n");
            return -1;
        }
        //设置上下文参数
        int res=0;
        res=av_opt_set_int(user->swr_ctx,"in_channel_layout",user->desired_para.channel_layout,0);
        res=av_opt_set_int(user->swr_ctx,"out_channel_layout",user->obtained_para.channel_layout,0);
        res=av_opt_set_int(user->swr_ctx,"in_sample_rate",user->desired_para.spec_.freq,0);
        res=av_opt_set_int(user->swr_ctx,"out_sample_rate",user->obtained_para.spec_.freq,0);
        //AVSampleFormat
        res=av_opt_set_sample_fmt(user->swr_ctx,"in_sample_fmt",(AVSampleFormat)user->desired_para.fmt,0);
        res=av_opt_set_sample_fmt(user->swr_ctx,"out_sample_fmt",(AVSampleFormat)user->obtained_para.fmt,0);

        //初始化上下文
        if((res=swr_init(user->swr_ctx))<0){
            printf("Resample Init::swr_init failed\n");
            char msg[128];
            av_strerror(res,msg,128);
            printf("ERROR:%s\n",msg);
            swr_free(&user->swr_ctx);
            return -1;
        }

    }

    //2.执行音频重采样

    uint8_t** inData=frame->data;
    int inSamples=frame->nb_samples;
    uint8_t** outData=out_data;//未初始化
    int outSamples=(inSamples*user->obtained_para.spec_.freq)/user->desired_para.spec_.freq+256;
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
void  audioCallBack1(void *userdata, Uint8 * stream,int len)
{
    AudioOutput_v1* user_=(AudioOutput_v1*)userdata;
    memset(stream,0,len);
}

//不经过重采样的输出(单通道)
void  audioCallBack2(void *userdata, Uint8 * stream,int len)
{
    AudioOutput_v1* user=(AudioOutput_v1*)userdata;
    Uint8* stream_ptr=stream;
    int each_len=0;
    while(len>0){
        AVFrame* frame=user->frame_queue->Pop(10);
        if(!frame){
            //没收到frame
            break;
        }
        //收到frame，直接填入
        int frame_size=av_samples_get_buffer_size(
            NULL,frame->channels,frame->nb_samples,
            (AVSampleFormat)frame->format,1);
        each_len=frame_size;
        if(each_len>len)
            each_len=len;
        //单channel输入(planar格式)
        //如果是交错存储格式，则是全部channel输出
        memcpy(stream_ptr,frame->data[0],each_len);
        stream_ptr+=each_len;
        len-=each_len;
    }

    //如果有还未填满的数据则填满静音
    if(len>0){
        memset(stream_ptr,0,len);
    }
}

//不经过重采样的输出(多通道)
void  audioCallBack3(void *userdata, Uint8 * stream,int len)
{

    AudioOutput_v1* user=(AudioOutput_v1*)userdata;
    Uint8* stream_ptr=stream;
    int each_len=0;
    while(len>0){
        AVFrame* frame=user->frame_queue->Pop(10);
        if(!frame){
            //没收到frame
            break;
        }
        //收到frame，直接填入
        int frame_size=av_samples_get_buffer_size(
            NULL,frame->channels,frame->nb_samples,
            (AVSampleFormat)frame->format,1);
        each_len=frame_size/frame->channels;
        for(int i=0;i<frame->channels&&len>0;++i){
            if(each_len>len)
                each_len=len;
            //多channel输入
            memcpy(stream_ptr,frame->data[i],each_len);
            stream_ptr+=each_len;
            len-=each_len;
        }
    }

    //如果有还未填满的数据则填满静音
    if(len>0){
        memset(stream_ptr,0,len);
    }
}

//经过重采样的输出(多通道)
//planar->packet
void  audioCallBack4(void *userdata, Uint8 * stream,int len)
{
    AudioOutput_v1* user=(AudioOutput_v1*)userdata;
    Uint8* stream_ptr=stream;
    int each_len=0;





    while(len>0){
        AVFrame* frame=user->frame_queue->Pop(10);
        if(!frame){
            //没收到frame
            break;
        }



        //收到frame，重采样

        //resample后的frame->data
        uint8_t *resample_data[AV_NUM_DATA_POINTERS]={NULL};
        int resample_data_len;
        Resample(user,frame,resample_data,&resample_data_len);

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




    //4.开始音频播放
    SDL_PauseAudioDevice(deviceId,0);



    return 0;
}

void AudioOutput_v1::DeInit()
{
    if(swr_ctx){
        swr_free(&swr_ctx);
    }

    //5.退出音频系统
    SDL_CloseAudioDevice(deviceId);
    SDL_Quit();
}













