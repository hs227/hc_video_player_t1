#ifndef MAINSDL_H
#define MAINSDL_H


#include<iostream>
#include"sdluser.h"
#include"thread.h"
#include"avsyncclocker.h"

extern "C"{
#include"SDL.h"
}



//用于mainSDL的输入
//在线程运行前初始化
class VideoInput
{
//MainWindow
public:
    std::string filepath;
    SDLUser* sdl_user;
    //FLAGS
    unsigned int type_360_f;//是否是全景视频

//VideoCtxManager
public:
    float* volume;
    float* speed;
    AvSyncClocker* sync_clock;


//PlayerThread
public:
    //FLAGS
    bool abort_f;//线程退出
    bool win_resize_f;//窗口大小
    bool pause_f;//反制播放状态：暂停->播放;播放->暂停
    bool volume_f;//更新音量
    int seek_f;//寻址（正负）
    bool speed_f;//更新播放速度
    int step_f;//逐帧
    char k_360_f;//全景视频下特殊的输入
//Handle
public:
    bool clock_h;//用于sync_clock时候已经初始化完成的标识符
};







class PlayerThread : public Thread
{
public:
    explicit PlayerThread();
    ~PlayerThread();

    int Init(VideoInput* videoInput);
    int Start();
    int Stop();
    void Run();




    //Event
    //窗口尺寸
    void WindowResize();
    //暂停播放
    void PauseChange();
    //停止播放
    void EndPlay();
    //音量更新
    void VolumeUpdate();
    //寻址
    void Seek(int s);
    //倍速更新
    void SpeedUpdate();
    //逐帧
    void Step(int s);


    //360的特殊输入处理
    void k_360_in(char key);


public:
    VideoInput videoInput;
};






//播放器核心
int mainSDL(void* args);




#endif // MAINSDL_H
