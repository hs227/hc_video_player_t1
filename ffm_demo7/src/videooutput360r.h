#ifndef VIDEOOUTPUT360R_H
#define VIDEOOUTPUT360R_H

#include"avframequeue.h"
#include"avsyncclocker.h"
#include"playerstate.h"
#include<thread>
#include<iostream>
#include"shader.h"


extern "C"{
#include"SDL.h"
#include"libavcodec/avcodec.h"
#include"libavutil/avutil.h"
#include"libavformat/avformat.h"
#include"libavutil/imgutils.h"
#include"libswscale/swscale.h"
#include"libavutil/time.h"
#include"ass/ass.h"
#include"SDL_ttf.h"

#include"glad.h"
#include"glm.hpp"
#include"glm/gtc/matrix_transform.hpp"
#include"glm/gtc/type_ptr.hpp"
}




//全景视频版VideoOutput(Rect)
class VideoOutput360R
{
public:
    VideoOutput360R(AVFrameQueue* frame_q,AVRational time_base_,PlayerState* p_s);
    ~VideoOutput360R();
    int Init();
    void DeInit();
    int MainLoop();
    void RefreshLoopWaitEvent();
    void videoRefresh(double* remaining_time);
    void ttfRefresh(long long v_pts);


    //openGL 绘制
    void openglDraw(unsigned int tex_y,unsigned int tex_u,unsigned int tex_v);
    int CreateRect(std::vector<float>& rectVertices,std::vector<unsigned int>&rectIndices);
    int BindRect(const std::vector<float>& rectVertices,std::vector<unsigned int>& rectIndices);
    void GenerateTexture(AVFrame* frame,unsigned int *tex_y,unsigned int *tex_u,unsigned int *tex_v);

public:
    AVFrameQueue*frame_queue=NULL;
    AVRational time_base;//时间基

    SDL_Window* win=NULL;//窗口

    SwsContext *sws_ctx=NULL;

    PlayerState* p_state=NULL;

    //ttf
    int fontSize;//ptx
    TTF_Font* font = NULL;


    //opengl
    SDL_GLContext glContext;
    //
    static const int Y_SEGMENTS=50;
    static const int X_SEGMENTS=50;

    unsigned int VAO,VBO,EBO;

    std::vector<float>rectV;//顶点坐标(float*3)+纹理坐标(float*2)
    std::vector<unsigned int>rectI;//顶点序列
    Shader* shader=NULL;//渲染器
    //camera loc
    float c_loc_x=0,c_loc_y=0,c_loc_z=0;
    const float camera_mv_rate=0.2f;
    //camera rotate
    float c_ro_x=0,c_ro_y=0;
    const float camera_ro_rate=(15.0f*glm::pi<float>())/180.0f;
};

#endif // VIDEOOUTPUT360R_H
