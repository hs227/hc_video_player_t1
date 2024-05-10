#include "playerstate.h"

#define BOOL_STR(x) ((x)?"TRUE":"FALSE")

PlayerState::PlayerState()
    :isPaused(false),stepOne(0),pSpeed(1.0f),volume_val(1.0f),volume_flag(false),
    playPos_flag(false),serial(0),
    bak_sys(NULL),sync_clocker(NULL),libass_sys(NULL),
    v_width(800),v_height(600),
    subtitle_timebase({0,0}),adecoderPos_flag(0),vdecoderPos_flag(0)
{}

void PlayerState::PrintStates()
{
    printf("PlayerState Info:\n");
    printf("Paused:%s\n",BOOL_STR(isPaused));
    printf("Speed:%.2fX\n",pSpeed);
    printf("Volume:%.2f\n",volume_val);

}

void PlayerState::SpeedUp()
{
    if(pSpeed<1.0f){
        pSpeed=1.0f;
    }else if(pSpeed<1.5f){
        pSpeed=1.5f;
    }else if(pSpeed<2.0f){
        pSpeed=2.0f;
    }else{
        pSpeed=2.0f;
    }

}

void PlayerState::SpeedDown()
{
    if(pSpeed>1.5f){
        pSpeed=1.5f;
    }else if(pSpeed>1.0f){
        pSpeed=1.0f;
    }else if(pSpeed>0.5f){
        pSpeed=0.5f;
    }else{
        pSpeed=0.5f;
    }
}

float PlayerState::GetSpeed()
{
    return pSpeed;
}

bool PlayerState::GetSpeedFlag()
{
    return speed_flag;
}

void PlayerState::SetSpeedFlag(bool flag)
{
    speed_flag=flag;
}

void PlayerState::PlayPosChange(int64_t newPos)
{
    // if(playPos_flag==false)
    //     playPos_flag=true;
    playPosition=newPos;
}

void PlayerState::PlayPosUp()
{

    int64_t newPos=sync_clocker->getSyncDrift();//ms
    newPos+=5000;//5s
    this->SetPosFlag(1);
    this->PlayPosChange(newPos);
}

void PlayerState::PlayPosBack()
{
    int64_t newPos=sync_clocker->getSyncDrift();
    newPos-=5000;//5s
    newPos=newPos<0?0:newPos;
    this->SetPosFlag(-1);
    this->PlayPosChange(newPos);
}

void PlayerState::StepUp()
{
    //进一
    this->stepOne=1;
}

void PlayerState::StepDown()
{
    //退一(关键帧)
    this->stepOne=1;
    this->PlayPosBack();
}

int64_t PlayerState::GetPlayPos()
{
    return playPosition;
}

int PlayerState::GetPosFlag()
{
    return playPos_flag;
}

void PlayerState::SetPosFlag(int flag)
{
    playPos_flag=flag;
}

void PlayerState::VolumeUp()
{
    static const float VOLUME_UP=5.0f;
    volume_val=std::min(VOLUME_UP,volume_val+0.1f);
}

void PlayerState::VolumeDown()
{
    static const float VOLUME_DOWN=0.0f;
    volume_val=std::max(VOLUME_DOWN,volume_val-0.1f);
}

float PlayerState::GetVolumeValue()
{
    std::lock_guard<std::mutex> lock(m_volume_val);
    return volume_val;
}

void PlayerState::SetVolumeValue(float val)
{
    std::lock_guard<std::mutex> lock(m_volume_val);
    volume_val=val;
}

bool PlayerState::GetVolumeFlag()
{
    //std::lock_guard<std::mutex> lock(m_volume_flag);
    return volume_flag;
}

void PlayerState::SetVolumeFlag(bool flag)
{
    std::lock_guard<std::mutex> lock(m_volume_flag);
    volume_flag=flag;
}

int PlayerState::GetSerial()
{
    return serial;
}

void PlayerState::IncSerial()
{
    serial++;
}
