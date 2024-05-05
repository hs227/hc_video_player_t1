#ifndef PLAYERSTATE_H
#define PLAYERSTATE_H


#include"avframebak.h"
#include"avsyncclocker.h"


class PlayerState
{
public:
    PlayerState();

    //打印所有状态
    void PrintStates();

public:
    //状态区
    //true:暂停；false:播放
    bool isPaused;


    //工具区
    AVFrameBak* bak_sys;
    AvSyncClocker* sync_clocker;
};

#endif // PLAYERSTATE_H
