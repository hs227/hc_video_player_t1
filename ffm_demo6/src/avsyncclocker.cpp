#include "avsyncclocker.h"

AvSyncClocker::AvSyncClocker()
    :type(AVSYNC_TYPE::AUDIO_TYPE),startTime(0),ptsDrift(0),
    lastPtsTime(0),lastPtsDrift(0)
{

}

AvSyncClocker::~AvSyncClocker()
{
}

void AvSyncClocker::InitSync(const AVSYNC_TYPE type_,float lastSpeed_)
{
    type=type_;
    lastPtsTime=0;
    lastPtsDrift=0;
    lastSpeed=lastSpeed_;

    //仅支持音频时钟和系统时钟
    if(type==AVSYNC_TYPE::SYSTEM_TYPE){
        startTime=av_gettime_relative()/1000;//ms
    }else{
        type=AVSYNC_TYPE::AUDIO_TYPE;
        setSync(0);
    }

}

void AvSyncClocker::setSync(int64_t ptsTime)
{
    //基本不影响，不加锁提高效率
    int64_t cur=av_gettime_relative()/1000;
    ptsDrift=ptsTime-cur;
}

int64_t AvSyncClocker::getSyncDrift()
{
    int64_t cur=av_gettime_relative()/1000;
    if(type==AVSYNC_TYPE::SYSTEM_TYPE){
        return cur-startTime;
    }
    return ptsDrift+cur;
}

AvSyncClocker::AVSYNC_TYPE AvSyncClocker::getSyncType()
{
    return type;
}

void AvSyncClocker::pauseSync(bool pause)
{
    if(pause){
        //暂停
        lastPtsTime=av_gettime_relative()/1000;
        if(type==AVSYNC_TYPE::AUDIO_TYPE){
            lastPtsDrift=ptsDrift;
        }
    }else{
        //播放
        int64_t offset=av_gettime_relative()/1000-lastPtsTime;
        if(type==AVSYNC_TYPE::SYSTEM_TYPE){
            startTime+=offset;
        }else{
            ptsDrift=lastPtsDrift-offset;
        }
    }
}




void AvSyncClocker::posChangeSync(int64_t posTime)
{
    if(type==AVSYNC_TYPE::SYSTEM_TYPE){
        //startTime+=av_gettime_relative()/1000-startTime-posTime;
        int64_t cur=av_gettime_relative()/1000;
        //int64_t tmp=cur-startTime;
        startTime+=(cur-startTime)-posTime;
        //tmp=cur-startTime;
        //printf("%d\n",tmp);
    }else{
        setSync(posTime);
    }
}

void AvSyncClocker::speedChangeSync(float speed)
{
    if(type==AVSYNC_TYPE::SYSTEM_TYPE){
        int64_t elapsed= av_gettime_relative()/1000-startTime;
        int offset=elapsed-(elapsed*this->lastSpeed/speed);
        startTime+=offset;


        // int64_t elapsed=av_gettime_relative()/1000-startTime;
        // startTime+=(elapsed/speed)*(speed-lastSpeed);
        //int64_t new_elapsed=av_gettime_relative()/1000-startTime;
        //printf("elapsed:%lld,n_elapsed:%lld\n",elapsed,new_elapsed);
        //printf("speed:%f,n_speed:%f\n",speed,lastSpeed);
    }else{
        int64_t cur=getSyncDrift()*this->lastSpeed/speed;
        setSync(cur);
    }
    this->lastSpeed=speed;
}

void AvSyncClocker::SetDuration(double d_s)
{
    this->duration_s=d_s;
}

double AvSyncClocker::GetDuration()
{
    return this->duration_s;
}
