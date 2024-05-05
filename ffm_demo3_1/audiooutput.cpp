#include "audiooutput.h"

#include<iostream>
#include<cmath>

AudioOutput::AudioOutput(const AudioParams& audio_pra,AVFrameQueue* frame_q)
    :src_tgt(audio_pra),frame_queue(frame_q)
{

}

AudioOutput::~AudioOutput()
{

}

void fill_audio_pcm(void* udata,Uint8* stream,int len)
{
    //1.从frame queue读取解码后的PCM的数据，填充到stream
    //2.len=4000,但frame=6000，
    AudioOutput* is=(AudioOutput*)udata;
    int len1=0;
    int audio_size=0;

    while(len>0){
        if(is->audio_buf_idx>=is->audio_buf_size){
            is->audio_buf_idx=0;
            AVFrame* frame=is->frame_queue->Pop(10);
            if(frame){
                //读到数据
                //如何判断是否需要重采样(格式，采样率，通道布局)
                //同时只能进行一次重采样器的确定
                if(
                    ((frame->format!=is->dst_tgt.fmt)||
                     (frame->sample_rate!=is->dst_tgt.freq)||
                     (frame->channel_layout!=is->dst_tgt.channel_layout))&&
                    (!is->swr_ctx)
                    ){
                    is->swr_ctx=swr_alloc_set_opts(NULL,
                                                     is->dst_tgt.channel_layout,
                                                     (AVSampleFormat)is->dst_tgt.fmt,
                                                     is->dst_tgt.freq,
                                                     frame->channel_layout,
                                                     (AVSampleFormat)frame->format,
                                                     frame->sample_rate,
                                                     0,
                                                     NULL);
                    if(is->swr_ctx||swr_init(is->swr_ctx)<0){
                        printf("ReSample Failed.\n");
                        return ;
                    }
                }
                if(is->swr_ctx){
                    //重采样
                    uint8_t**in=(uint8_t**)frame->extended_data;
                    uint8_t**out=&is->audio_buf1;

                    int dst_samples=frame->nb_samples*is->dst_tgt.freq/frame->sample_rate+256;
                    int dst_bytes=av_samples_get_buffer_size(NULL,is->dst_tgt.channels,dst_samples,is->dst_tgt.fmt,0);
                    if(dst_bytes<0){
                        printf("av_samples_get_buffer_size failed\n");
                        return;
                    }
                    av_fast_malloc(is->audio_buf1,&is->audio_buf1_size,dst_bytes);

                    int len2=swr_convert(is->swr_ctx,out,dst_samples,(const uint8_t **)in,frame->nb_samples);
                    if(len2<0){
                        printf("swr_convert failed.\n");
                        return;
                    }
                    is->audio_buf=is->audio_buf1;
                    is->audio_buf_size=av_samples_get_buffer_size(NULL,is->dst_tgt.channels,len2,is->dst_tgt.fmt,1);


                }else{
                    //不需要重采样
                    audio_size=av_samples_get_buffer_size(NULL,
                                                            frame->channels,
                                                            frame->nb_samples,
                                                            (AVSampleFormat)frame->format,1);
                    av_fast_malloc(is->audio_buf1,&is->audio_buf1_size,audio_size);
                    is->audio_buf=is->audio_buf1;
                    is->audio_buf1_size=audio_size;
                    memcpy(is->audio_buf,frame->data[0],audio_size);
                }


                av_frame_free(&frame);
            }else{
                //未读到数据
                is->audio_buf=NULL;
                is->audio_buf_size=512;
            }
        }
        len1=is->audio_buf_size-is->audio_buf_idx;
        if(len1>len){
            len1=len;
        }
        if(!is->audio_buf){
            memset(stream,0,len1);
        }else{
            //真正拷贝数据
            memcpy(stream,is->audio_buf+is->audio_buf_idx,len1);
        }
        len-=len1;
        stream+=len1;
        is->audio_buf_idx+=len1;

    }
}

void fill_audio_pcm_v2(void* udata, Uint8* stream, int len)
{
    AudioOutput* is = static_cast<AudioOutput*>(udata);
    int filled = 0;
    int q_size=is->frame_queue->Size();

    while (filled < len && is->frame_queue->Size()>0) {
        AVFrame* frame = is->frame_queue->Pop(10);
        if (!frame) {
            continue; // 跳过无效帧，避免清空缓冲区，保持静默处理逻辑
        }

        int dst_samples, dst_bytes;
        uint8_t** out;
        if (
            (frame->format != is->dst_tgt.fmt ||
             frame->sample_rate != is->dst_tgt.freq ||
             av_get_default_channel_layout(frame->channels) != is->dst_tgt.channel_layout) &&
            (!is->swr_ctx || swr_is_initialized(is->swr_ctx) == 0)
            ) {
            if (is->swr_ctx) {
                swr_free(&is->swr_ctx);
            }
            is->swr_ctx = swr_alloc_set_opts(NULL,
                                             is->dst_tgt.channel_layout,
                                             is->dst_tgt.fmt,
                                             is->dst_tgt.freq,
                                             av_get_default_channel_layout(frame->channels),
                                             (AVSampleFormat)frame->format,
                                             frame->sample_rate,
                                             0,
                                             NULL);
            if (!is->swr_ctx || swr_init(is->swr_ctx) < 0) {
                av_frame_free(&frame);
                printf("ReSample initialization Failed.\n");
                return;
            }
        }

        dst_samples = frame->nb_samples * is->dst_tgt.freq / frame->sample_rate + 256;
        dst_bytes = av_samples_get_buffer_size(NULL, is->dst_tgt.channels, dst_samples, is->dst_tgt.fmt, 1);
        if (dst_bytes < 0) {
            av_frame_free(&frame);
            printf("av_samples_get_buffer_size failed\n");
            return;
        }

        // 确保audio_buf1有足够的空间
        av_fast_malloc(&is->audio_buf1, &is->audio_buf1_size, dst_bytes);

        uint8_t** in = (uint8_t**)frame->extended_data;
        out = &is->audio_buf1;
        int converted = swr_convert(is->swr_ctx, out, dst_samples, (const uint8_t**)in, frame->nb_samples);
        if (converted < 0) {
            av_frame_free(&frame);
            printf("swr_convert failed.\n");
            return;
        }

        is->audio_buf = is->audio_buf1;
        is->audio_buf_size = av_samples_get_buffer_size(NULL, is->dst_tgt.channels, converted, is->dst_tgt.fmt, 1);
        av_frame_free(&frame);

        // 复制数据到输出缓冲区
        int copy_len = fmin(len - filled, is->audio_buf_size - is->audio_buf_idx);
        memcpy(stream + filled, is->audio_buf + is->audio_buf_idx, copy_len);
        filled += copy_len;
        is->audio_buf_idx += copy_len;
    }

    // 若还有剩余空间未填满，填充静音数据
    if (filled < len) {
        memset(stream + filled, 0, len - filled);
    }
}


int AudioOutput::Init()
{
    //Audio System Init
    if(SDL_Init(SDL_INIT_AUDIO)<0){
        const char* msg=SDL_GetError();
        std::cout<<msg<<std::endl;
        return -1;
    }
    SDL_AudioSpec wanted_spec,real_spec;
    wanted_spec.channels=src_tgt.channels;
    wanted_spec.freq=src_tgt.freq;
    wanted_spec.format=AUDIO_S16SYS;
    wanted_spec.silence=0;
    wanted_spec.callback=fill_audio_pcm_v2;
    wanted_spec.userdata=this;
    wanted_spec.samples=src_tgt.frame_size;//采样数量

    int res=SDL_OpenAudio(&wanted_spec,&real_spec);
    if(res<0){
        printf("SDL_OpenAudio failed.\n");
        return -1;
    }
    dst_tgt.channels=real_spec.channels;
    dst_tgt.fmt=AV_SAMPLE_FMT_S16;
    dst_tgt.freq=real_spec.freq;
    dst_tgt.channel_layout=av_get_default_channel_layout(src_tgt.channels);
    dst_tgt.frame_size=src_tgt.frame_size;

    //打开音乐
    SDL_PauseAudio(0);

    return 0;
}

int AudioOutput::DeInit()
{
    //关闭音乐
    SDL_PauseAudio(1);
    SDL_CloseAudio();
    return 0;
}
