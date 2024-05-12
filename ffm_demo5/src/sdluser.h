#ifndef SDLUSER_H
#define SDLUSER_H

#include<QDebug>
#include<QWidget>

extern "C"{
#include"SDL.h"
}


class SDLUser
{
public:
    SDLUser();
    ~SDLUser();
    int Init(void* winHandler);
    void DeInit();

public:
    SDL_Window* win=NULL;//窗口
    QWidget* widget=NULL;
    int v_width;
    int v_height;
};

#endif // SDLUSER_H
