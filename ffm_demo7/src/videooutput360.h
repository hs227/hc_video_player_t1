#ifndef VIDEOOUTPUT360_H
#define VIDEOOUTPUT360_H

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




//全景视频版VideoOutput
class VideoOutput360
{
public:
    VideoOutput360(AVFrameQueue* frame_q,AVRational time_base_,PlayerState* p_s);
    ~VideoOutput360();
    int Init();
    void DeInit();
    int MainLoop();
    void RefreshLoopWaitEvent();
    void videoRefresh(double* remaining_time);
    void ttfRefresh(long long v_pts);


    //openGL 绘制
    void openglDraw(unsigned int tex_y,unsigned int tex_u,unsigned int tex_v);
    int CreateSphere(std::vector<float>& sphereVertices,std::vector<unsigned int>&sphereIndices);
    int BindSphere(const std::vector<float>& sphereVertices,std::vector<unsigned int>& sphereIndices);
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

    std::vector<float>sphereV;//顶点坐标(float*3)+纹理坐标(float*2)
    std::vector<unsigned int>sphereI;//顶点序列
    Shader* shader=NULL;//渲染器
    //camera loc
    float c_loc_x=0,c_loc_y=0,c_loc_z=0;
    const float camera_mv_rate=0.2f;
    //camera rotate
    float c_ro_x=0,c_ro_y=0;
    const float camera_ro_rate=(15.0f*glm::pi<float>())/180.0f;
};

#endif // VIDEOOUTPUT360_H
