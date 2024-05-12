#ifndef AVSYNCCLOCKER_H
#define AVSYNCCLOCKER_H

extern "C"{
#include"SDL.h"
#include"libavutil/avutil.h"
#include"libavutil/time.h"
}


class AvSyncClocker
{
public:
    //视频时钟(not supported) 音频慢则加快播放或丢掉部分音频帧；音频快则放慢播放速度（基本不使用）
    //音频时钟(not supported) 视频慢则加快播放或丢到部分视频帧；视频快则延迟播放保持上一帧
    //系统时钟(supported) 根据外部时钟判断快慢音视频进行对应处理
    enum class AVSYNC_TYPE{
        VIDEO_TYPE=0,
        AUDIO_TYPE,
        SYSTEM_TYPE,
    };
public:
    AvSyncClocker();
    ~AvSyncClocker();

    void InitSync(const AVSYNC_TYPE type_=AVSYNC_TYPE::AUDIO_TYPE,float lastSpeed_=1.0f);

    //设置基准时钟事件ptsTime要求为ms
    //音频时钟使用，系统时钟以现实时间为基准不需要设置
    void setSync(int64_t ptsTime);

    //返回相对基准时钟的s时间偏移值
    int64_t getSyncDrift();
    //返回当前时钟类型
    AVSYNC_TYPE getSyncType();
    //暂停同步时钟
    void pauseSync(bool pause);

    //进度变化改变同步时钟
    void posChangeSync(int64_t posTime);

   //变速时钟
    void speedChangeSync(float speed);

    //秒s
    void SetDuration(double d_s);
    double GetDuration();

private:
    AVSYNC_TYPE type;

    //系统时钟方式 ms
    int64_t startTime;
    //同步时钟方式 ms
    int64_t ptsDrift;

    //最后更新时刻的时钟(用于暂停)
    int64_t lastPtsTime;
    int64_t lastPtsDrift;

    //用于变速的时钟
    float lastSpeed;

    //用于记录视频时长
    double duration_s=-1;

};

#endif // AVSYNCCLOCKER_H
