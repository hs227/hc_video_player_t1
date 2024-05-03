#include <iostream>
#include"demuxthread.h"
#include"decodethread.h"
#include"audiooutput_v1.h"
#include"videooutput.h"

extern "C"{
#include"libavutil/avutil.h"
}


using namespace std;


#ifdef __MINGW32__
#undef main/* Prevents SDL from overriding main() */
#endif
int main(int argc,char* argv[])
{
    cout << "Hello World!" << endl;
    cout<<"ffmpeg version: "<<av_version_info()<<endl;

    if(argc<2){
        printf("arguments is less than 2[argc=%d]\n",argc);
        return 0;
    }

    AVPacketQueue audio_pkt_queue;
    AVPacketQueue video_pkt_queue;

    AVFrameQueue audio_fra_queue;
    AVFrameQueue video_fra_queue;


    //1.解复用
    //通过解复用，我们可以从视频文件中，读取需要的视频流和音频流
    DemuxThread demux1(&audio_pkt_queue,&video_pkt_queue);
    if(demux1.Init(argv[1])==-1){
        printf("demux_thread init failed\n");
        return 0;
    }
    if(demux1.Start()==-1){
        printf("demux_thread start failed\n");
        return 0;
    }

    //2.解码
    DecodeThread audio_decoder(&audio_pkt_queue,&audio_fra_queue);
    if(audio_decoder.Init(demux1.AudioCodecParameters())<0){
        printf("audio_decoder init failed.\n");
        return 0;
    }
    if(audio_decoder.Start()<0){
        printf("audio_decoder start failed.\n");
        return 0;
    }
    DecodeThread video_decoder(&video_pkt_queue,&video_fra_queue);
    if(video_decoder.Init(demux1.VideoCodecParameters())<0){
        printf("video_decoder init failed\n");
        return 0;
    }
    if(video_decoder.Start()<0){
        printf("video_decoder start failed\n");
        return 0;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    std::cout<<"Wake Up"<<endl;

    //Old Type audio输出
    // AudioParams audio_params;
    // memset(&audio_params,0,sizeof(AudioParams));
    // audio_params.channels=demux1.AudioCodecParameters()->channels;
    // audio_params.channel_layout=demux1.AudioCodecParameters()->channel_layout;
    // audio_params.fmt=(AVSampleFormat)demux1.AudioCodecParameters()->format;
    // audio_params.freq=demux1.AudioCodecParameters()->sample_rate;
    // audio_params.frame_size=demux1.AudioCodecParameters()->frame_size;

    // AudioOutput audio_out(audio_params,&audio_fra_queue);
    // if(audio_out.Init()<0){
    //     std::cout<<"audioOutput init Failed."<<std::endl;
    //     return 0;
    // }


    //New Type audio输出
    AudioOutput_v1 audio_out(&audio_fra_queue);
    if(audio_out.Init(demux1.AudioCodecParameters())<0){
        std::cout<<"audioOutput_v1 init Failed."<<std::endl;
        return 0;
    }


    // Old Type video输出
    VideoOutput video_out(&video_fra_queue,
                          800,
                          600);
    if(video_out.Init()<0){
        std::cout<<"videoOutput init Failed"<<std::endl;
        return 0;
    }
    video_out.MainLoop();

    //休眠5s
    //std::this_thread::sleep_for(std::chrono::milliseconds(30000));
    //std::cout<<"Wake Up"<<endl;

    demux1.Stop();
    audio_decoder.Stop();
    video_decoder.Stop();
    //audio_out.DeInit();








    return 0;
}
