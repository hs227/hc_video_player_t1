#ifndef LIBASS_USER_H
#define LIBASS_USER_H

#include<iostream>

extern "C"{
#include"ass/ass.h"
#include"SDL.h"
#include"SDL_ttf.h"
}

class LibassUser
{
public:
    enum class LU_Type{
        //没有字幕
        LU_NO_S=0,
        //外挂字幕
        LU_OUT_S,
        //内挂字幕
        LU_IN_S,
    };


public:
    LibassUser();

    //两个示例代码
    void Use();
    void Use2();

    int Init(LU_Type flag,std::string* name=NULL);




public:
    ASS_Library* libass=NULL;
    ASS_Renderer* ass_renderer=NULL;
    ASS_Track* ass_track=NULL;

    LU_Type type;
    std::string ass_filename;

};

#endif // LIBASS_USER_H
