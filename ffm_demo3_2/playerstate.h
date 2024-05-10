#ifndef PLAYERSTATE_H
#define PLAYERSTATE_H


#include"avframebak.h"
#include"avsyncclocker.h"
#include"libass_user.h"
#include<thread>
#include<mutex>

#include"avframequeue.h"
#include"avpacketqueue.h"


class PlayerState
{
public:
    PlayerState();

    //打印所有状态
    void PrintStates();

    //播放速度加速
    void SpeedUp();
    //播放速度减速
    void SpeedDown();
    float GetSpeed();
    bool GetSpeedFlag();
    void SetSpeedFlag(bool flag);

    //播放位置改变
    void PlayPosChange(int64_t newPos);
    void PlayPosUp();//快进
    void PlayPosBack();//后退
    void StepUp();
    void StepDown();
    int64_t GetPlayPos();
    int GetPosFlag();
    void SetPosFlag(int flag);


    //音量增大
    void VolumeUp();
    //音量减少
    void VolumeDown();

    float GetVolumeValue();
    void SetVolumeValue(float val);
    bool GetVolumeFlag();
    void SetVolumeFlag(bool flag);

    int GetSerial();
    void IncSerial(void);

public:
    //状态区
    int subtitle_index=-1;
    //true:暂停；false:播放
    bool isPaused;
    //0:没有step;>0:进1;<0:退1
    int stepOne;
    //播放速度:0.5X,1.0X,1.5X,2.X
    float pSpeed;
    bool speed_flag;
    //音量大小
    float volume_val=1.0f;
    std::mutex m_volume_val;
    bool volume_flag;
    std::mutex m_volume_flag;
    //播放位置改变
    int64_t playPosition;//时间：秒s
    //0:没有seek;>0:快进;<0:倒退
    int playPos_flag;
    int64_t adecoderPos_flag;
    int64_t vdecoderPos_flag;
    //序列号（用于播放改变时，丢弃那些不同序列的frame）
    int serial=0;
    //视频相关
    int v_width;
    int v_height;
public:
    //工具区
    AVFrameBak* bak_sys;
    AvSyncClocker* sync_clocker;
    LibassUser* libass_sys;



    AVPacketQueue* audio_pkt_queue;
    AVFrameQueue* audio_fra_queue;
    AVPacketQueue* video_pkt_queue;
    AVFrameQueue* video_fra_queue;

    AVFrameQueue* audio_lift_fra_queue;//filter后的audio frame
    AVFrameQueue* video_lift_fra_queue;//filter后的video frame

    AVRational subtitle_timebase;

};

#endif // PLAYERSTATE_H
