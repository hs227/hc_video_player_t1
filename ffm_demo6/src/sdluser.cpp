#include "sdluser.h"

#include<iostream>


SDLUser::SDLUser()
{

}

SDLUser::~SDLUser()
{

}

int SDLUser::Init(void* winHandler)
{
    //输入检查
    if(winHandler==NULL){
        qDebug()<<"winHandler input is NULL\n";
        return -1;
    }


    //初始化SDL视频系统
    if(SDL_Init(SDL_INIT_VIDEO)<0){
        const char* msg=SDL_GetError();
        std::cout<<msg<<std::endl;
        return -1;
    }


    //打开窗口
    win=SDL_CreateWindowFrom(winHandler,SDL_WINDOW_OPENGL);
    if(!win){
        const char* msg=SDL_GetError();
        std::cout<<msg<<std::endl;
        return -1;
    }

    return 0;
}

void SDLUser::DeInit()
{
    SDL_DestroyWindow(win);
    SDL_Quit();
}
