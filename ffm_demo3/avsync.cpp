#include "avsync.h"

#include"math.h"

using namespace std::chrono;


AVSync::AVSync()
{

}

void AVSync::InitClock()
{
    SetClock(NAN);//数字对比是一个无效值
}

void AVSync::SetClockAt(double pts_, double time)
{
    pts=pts_;
    pts_drift=pts-time;


}

double AVSync::GetClock()
{
    double time=GetMicroseconds()/1000000.0;
    return pts+time;
}

void AVSync::SetClock(double pts_)
{
    double time=GetMicroseconds()/1000000.0;//us->s
    SetClockAt(pts,time);
}


time_t AVSync::GetMicroseconds()
{
    system_clock::time_point time_point_new=system_clock::now();
    system_clock::duration duration=time_point_new.time_since_epoch();

    time_t micros=duration_cast<microseconds>(duration).count();
    return micros;
}
