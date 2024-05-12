#include <iostream>

#include"demuxthread.h"
#include"decodethread.h"
#include"audiooutput_v1.h"
#include"videooutput.h"
#include"playerstate.h"
#include"audiofilterthread.h"
#include"videofilterthread.h"
#include"libass_user.h"

extern "C"{
#include"libavutil/avutil.h"
#include"ass/ass.h"
#include"SDL_image.h"
#include"SDL_ttf.h"
}


using namespace std;

//打印各种库的信息，确定是否链接成功
void lib_version_info(void)
{
    cout<<"ffmpeg version: "<<av_version_info()<<endl;
    cout<<"libass version: "<<LIBASS_VERSION<<std::endl;
    cout<<"sdl version: "<<SDL_MAJOR_VERSION<<"."<<SDL_MINOR_VERSION<<std::endl;
    cout<<"sdl_image version: "<<SDL_IMAGE_MAJOR_VERSION<<"."<<SDL_IMAGE_MINOR_VERSION<<std::endl;
    cout<<"sdl_ttf version: "<<SDL_TTF_MAJOR_VERSION<<"."<<SDL_TTF_MINOR_VERSION<<std::endl;
}









#ifdef __MINGW32__
#undef main/* Prevents SDL from overriding main() */
#endif
int main(int argc,char* argv[])
{


    //判断库的链接是否正常
    //lib_version_info();
    //return 0;

    //参数打开视频
    if(argc<2){
        printf("arguments is less than 2[argc=%d]\n",argc);
        return 0;
    }


    //视频文件名
    std::string video_file=argv[1];

    //frame缓存管理器
    AVFrameBak bak_sys;
    //打开新的视频前都要删除旧视频的缓存
    bak_sys.clear_frame_bak();

    //同步时钟
    AvSyncClocker sync_clocker;

    AVPacketQueue audio_pkt_queue;
    AVFrameQueue audio_fra_queue;
    AVFrameQueue audio_lift_fra_queue;//filter后的audio frame
    AVPacketQueue video_pkt_queue;
    AVFrameQueue video_fra_queue;
    AVFrameQueue video_lift_fra_queue;//filter后的video frame
    AVPacketQueue subtitle_pkt_queue;

    //字幕使用系统
    LibassUser libass_sys;
    //在解复用器中完成初始化



    //播放器状态
    PlayerState playerState;
    playerState.bak_sys=&bak_sys;
    playerState.sync_clocker=&sync_clocker;
    playerState.audio_pkt_queue=&audio_pkt_queue;
    playerState.audio_fra_queue=&audio_fra_queue;
    playerState.audio_lift_fra_queue=&audio_lift_fra_queue;
    playerState.video_pkt_queue=&video_pkt_queue;
    playerState.video_fra_queue=&video_fra_queue;
    playerState.video_lift_fra_queue=&video_lift_fra_queue;
    playerState.libass_sys=&libass_sys;



    //解复用器
    DemuxThread demux1(&audio_pkt_queue,&video_pkt_queue,&subtitle_pkt_queue,&playerState);
    if(demux1.Init(video_file.c_str())==-1){
        printf("demux_thread init failed\n");
        return 0;
    }
    playerState.subtitle_timebase=demux1.SubtitleStreamTimebase();


    //字幕解码器
    DecodeThread subtitle_decoder(&subtitle_pkt_queue,NULL,NULL,&playerState);
    if(subtitle_decoder.Init(demux1.SubtitleCodecParameters(),demux1.SubtitleStreamIndex())<0&&
        libass_sys.type!=LibassUser::LU_Type::LU_NO_S){
        printf("subtitle_decoder init failed:%d\n",libass_sys.type);
        return 0;
    }
    //音频解码器
    DecodeThread audio_decoder(&audio_pkt_queue,&audio_fra_queue,
                               &bak_sys,&playerState);
    if(audio_decoder.Init(demux1.AudioCodecParameters(),demux1.AudioStreamIndex())<0){
        printf("audio_decoder init failed.\n");
        return 0;
    }
    //视频解码器
    DecodeThread video_decoder(&video_pkt_queue,&video_fra_queue,
                               &bak_sys,&playerState);
    if(video_decoder.Init(demux1.VideoCodecParameters(),demux1.VideoStreamIndex())<0){
        printf("video_decoder init failed\n");
        return 0;
    }


    //音频过滤器
    AudioFilterThread audio_filter(&audio_fra_queue,&audio_lift_fra_queue,&playerState);
    if(audio_filter.Init(demux1.AudioCodecParameters(),demux1.AudioStreamTimebase())<0){
        printf("audio_filter init failed\n");
        return 0;
    }
    //视频过滤器
    VideoFilterThread video_filter(&video_fra_queue,&video_lift_fra_queue,&playerState);
    if(video_filter.Init(demux1.VideoCodecParameters(),demux1.VideoStreamTimebase())<0){
        printf("video_filter init failed\n");
        return 0;
    }



    //音频播放器
    //New Type audio输出
    AudioOutput_v1 audio_out(&audio_lift_fra_queue,demux1.AudioStreamTimebase(),&playerState);
    if(audio_out.Init(demux1.AudioCodecParameters())<0){
        std::cout<<"audioOutput_v1 init Failed."<<std::endl;
        return 0;
    }

    //视频播放器
    // Old Type video输出
    VideoOutput video_out(&video_lift_fra_queue,demux1.VideoStreamTimebase(),&playerState);
    if(video_out.Init()<0){
        std::cout<<"videoOutput init Failed"<<std::endl;
        return 0;
    }




    //1.解复用
    //通过解复用，我们可以从视频文件中，读取需要的视频流和音频流
    if(demux1.Start()==-1){
        printf("demux_thread start failed\n");
        return 0;
    }


    //2.解码
    if(subtitle_decoder.Start()<0){
        printf("subtitle_decoder start failed.\n");
        return 0;
    }

    if(audio_decoder.Start()<0){
        printf("audio_decoder start failed.\n");
        return 0;
    }
    if(video_decoder.Start()<0){
        printf("video_decoder start failed.\n");
        return 0;
    }

    //3.滤镜
    if(audio_filter.Start()<0){
        printf("audio_filter start failed\n");
        return 0;
    }
    if(video_filter.Start()<0){
        printf("video_filter start failed\n");
        return 0;
    }

    //方便数据的准备
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout<<"Wake Up"<<endl;


    //开始计时
    sync_clocker.InitSync(AvSyncClocker::AVSYNC_TYPE::SYSTEM_TYPE);
    if(playerState.isPaused){
        //暂停
        sync_clocker.pauseSync(playerState.isPaused);
    }

    //开始播放
    audio_out.Play();
    video_out.MainLoop();



    demux1.Stop();
    audio_decoder.Stop();
    video_decoder.Stop();
    subtitle_decoder.Stop();
    audio_filter.Stop();
    video_filter.Stop();
    audio_out.DeInit();
    video_out.DeInit();
    libass_sys.DeInit();
    audio_pkt_queue.Abort();
    audio_fra_queue.Abort();
    audio_lift_fra_queue.Abort();//filter后的audio frame
    video_pkt_queue.Abort();
    video_fra_queue.Abort();
    video_lift_fra_queue.Abort();//filter后的video frame
    subtitle_pkt_queue.Abort();

    SDL_Quit();







    fprintf(stdout,"\nSuccess Exit\n");
    return 0;
}
