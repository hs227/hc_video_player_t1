#include "videooutput360r.h"

VideoOutput360R::VideoOutput360R(AVFrameQueue* frame_q,AVRational time_base_,PlayerState* p_s)
    :frame_queue(frame_q),time_base(time_base_),p_state(p_s)
{

}

VideoOutput360R::~VideoOutput360R()
{

}

//根据窗口大小变化
static int FontSize_Change(int width)
{
    if(width<800){
        return 16;//ptx
    }else if(width<1024){
        return 24;//ptx
    }else if(width<1600){
        return 48;
    }
    return 60;
}

int VideoOutput360R::Init()
{
    //获得窗口
    this->win=p_state->vInput->sdl_user->win;

    int w,h;
    SDL_GetWindowSize(win,&w,&h);
    printf("%d,%d\n",w,h);
    p_state->vInput->sdl_user->v_width=w;
    p_state->vInput->sdl_user->v_height=h;


    //初始化OpenGL
    //设置OpenGL属性
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);//设置深度缓冲区大小
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);//OpenGL核心模式
    //通过已有窗口创建OpenGL上下文
    this->glContext=SDL_GL_CreateContext(this->win);
    SDL_GL_SetSwapInterval(1);//垂直同步，确保帧率不超过显示器的刷新率，避免画面撕裂
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))//加载OpenGL函数指针
    {
        std::cout << "Failed to initialization GLAD" << std::endl;
        exit(EXIT_FAILURE);
    }
    glViewport(0, 0, w, h);//更新视口
    glEnable(GL_DEPTH_TEST);//深度检测
    //加载Shader
    shader=new Shader("vertex.shader","fragment.shader");
    //开始处理球体
    CreateRect(rectV,rectI);
    BindRect(rectV,rectI);


    //初始化字体库
    // 初始化TTF库
    if (TTF_Init() == -1) {
        printf("Failed to initialize TTF: %s\n", TTF_GetError());
        return -1;
    }
    fontSize=FontSize_Change(p_state->vInput->sdl_user->v_width);
    font = TTF_OpenFont("msyh.ttf", fontSize);
    if (font == NULL) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return -1;
    }


    return 0;
}

void VideoOutput360R::DeInit()
{

    if(sws_ctx){
        sws_freeContext(sws_ctx);
        sws_ctx=NULL;
    }

    //释放opengl上下文
    glDeleteVertexArrays(1,&VAO);
    glDeleteBuffers(1,&VBO);
    glDeleteBuffers(1,&EBO);
    SDL_GL_DeleteContext(glContext);
    if(shader){
        delete shader;
        shader=NULL;
    }

    // 完成使用后，释放字体资源
    if(font){
        TTF_CloseFont(font);
        font=NULL;
    }
    // 清理TTF
    TTF_Quit();

    //SDL_Quit();
}

int VideoOutput360R::MainLoop()
{
    for(;;){

        if(p_state->vInput->abort_f){
            //线程退出
            return 0;
        }
        if(p_state->vInput->win_resize_f){
            //更新窗口大小

            int w,h;
            SDL_GetWindowSize(win,&w,&h);

            //更新窗口
            p_state->vInput->sdl_user->v_width=w;//新窗口宽
            p_state->vInput->sdl_user->v_height=h;//新窗口高

            //更新视口
            glViewport(0, 0, w, h);

            //更新字幕
            fontSize=FontSize_Change(p_state->vInput->sdl_user->v_width);
            TTF_CloseFont(font);
            font = TTF_OpenFont("msyh.ttf", fontSize);
            if (font == NULL) {
                printf("Failed to load font: %s\n", TTF_GetError());
                return -1;
            }


            //更新上下文
            sws_freeContext(this->sws_ctx);
            this->sws_ctx=NULL;
        }
        if(p_state->vInput->pause_f){
            //暂停或者播放
            p_state->isPaused=!p_state->isPaused;
            p_state->sync_clocker->pauseSync(p_state->isPaused);

            p_state->vInput->pause_f=false;
        }
        if(p_state->vInput->volume_f){
            //改变音量
            p_state->SetVolumeFlag(true);//audio_filter更新音量

            p_state->vInput->volume_f=false;
        }
        if(p_state->vInput->seek_f!=0){
            //寻址
            p_state->PlayPosChange(p_state->vInput->seek_f);
            p_state->vInput->seek_f=0;
        }
        if(p_state->vInput->speed_f){
            //变速
            //p_state->SetSpeedFlag(true);//audio_resample更新重采样器（重采集变速）
            p_state->SetSpeed(*p_state->vInput->speed);
            p_state->sync_clocker->speedChangeSync(p_state->GetSpeed());
            p_state->vInput->speed_f=false;
        }
        if(p_state->vInput->step_f!=0){
            //逐帧
            if(p_state->isPaused){
                //只有暂停才能逐帧
                p_state->StepUp();
            }
            p_state->vInput->step_f=0;
        }
        if(p_state->vInput->k_360_f!=0){
            //处理360特有的输入
            switch(p_state->vInput->k_360_f){
            case 'w':
                //上
                c_loc_y-=camera_mv_rate;
                break;
            case 's':
                //下
                c_loc_y+=camera_mv_rate;
                break;
            case 'a':
                //左
                c_loc_x+=camera_mv_rate;
                break;
            case 'd':
                //右
                c_loc_x-=camera_mv_rate;
                break;
            case 'f':
                //前进
                c_loc_z+=camera_mv_rate;
                break;
            case 'g':
                //后退
                c_loc_z-=camera_mv_rate;
                break;
            case 'q':
                //抬头
                c_ro_x-=camera_ro_rate;
                break;
            case 'e':
                //低头
                c_ro_x+=camera_ro_rate;
                break;
            case 'z':
                //左转
                c_ro_y-=camera_ro_rate;
                break;
            case 'c':
                //右转
                c_ro_y+=camera_ro_rate;
                break;
            case 'p':
                //复位
                c_loc_x=c_loc_y=c_loc_z=0;
                c_ro_x=c_ro_y=0;
                break;
            default:
                break;
            }
            p_state->vInput->k_360_f=0;
        }


        RefreshLoopWaitEvent();


    }
    return 0;
}




#define REFRESH_RATE 0.01
void VideoOutput360R::RefreshLoopWaitEvent()
{
    double remaining_time=REFRESH_RATE;

    //尝试刷新画面
    videoRefresh(&remaining_time);

    //控制帧率
    if(remaining_time>0.0){
        std::this_thread::sleep_for(std::chrono::milliseconds(int64_t(remaining_time*1000.0)));
    }

}

int VideoPrepare(VideoOutput360R* user,AVFrame* frame,AVFrame**dst_frame)
{
    *dst_frame=av_frame_alloc();
    (*dst_frame)->format=AV_PIX_FMT_YUV420P;
    (*dst_frame)->width=user->p_state->vInput->sdl_user->v_width;
    (*dst_frame)->height=user->p_state->vInput->sdl_user->v_height;
    if(av_frame_get_buffer(*dst_frame,0)<0){
        av_frame_free(dst_frame);
        printf("VideoPrepare:av_frame_get_buffer failed\n");
        return -1;
    }
    //创建SwsContext转换
    if(user->sws_ctx==NULL){
        user->sws_ctx=sws_getContext(
            frame->width,frame->height,(AVPixelFormat)frame->format,
            user->p_state->vInput->sdl_user->v_width,user->p_state->vInput->sdl_user->v_height,AV_PIX_FMT_YUV420P,
            SWS_BILINEAR,NULL,NULL,NULL);
        if(!user->sws_ctx){
            printf("VideoPrepare:sws_ctx is NULL.\n");
            return -1;
        }
    }



    //执行转换
    sws_scale(user->sws_ctx,frame->data,frame->linesize,
              0,frame->height,
              (*dst_frame)->data,(*dst_frame)->linesize);

    return 0;
}


int ControlPlay(VideoOutput360R* user,MAVFrame* video_frame){
    int64_t srcPts=video_frame->frame->pts*av_q2d(user->time_base)*1000;//ms

    int offset=50;//由于音频延迟（可以忽略也可以补偿）
    float speed=user->p_state->GetSpeed();//播放速度
    int64_t newPts=(srcPts-offset)/speed;
    int64_t base=user->p_state->sync_clocker->getSyncDrift();
    int sleepMs=newPts-base;

    if(srcPts==0){
        printf("srcPts:%d\n",srcPts);
    }

    // if(user->p_state->vInput->speed_f){
    printf("Video Frame pts=%lld;SyncTime pts=%lld\n",srcPts,base);
    //     user->p_state->vInput->speed_f=0;
    // }


    //正常慢了(丢帧)
    if(sleepMs>=-1000&&sleepMs<-40){
        return 0;
    }
    //不是这个序列
    if(video_frame->serial!=user->p_state->GetSerial()){
        return 0;
    }
    //解码但不播放
    if(video_frame->pos==false){
        return 0;
    }

    //快了(睡眠)
    if(sleepMs>1&&sleepMs<1000){
        return sleepMs;
    }

    if(user->p_state->stepOne){
        //在逐帧时，不能使用SEEK
        return -1;
    }
    //快的过分了，就更新位置
    if(sleepMs>=400&&sleepMs<100000){
        user->p_state->PlayPosChange(sleepMs/1000);
        printf("Video Change the Pos:%d\n",sleepMs);
        return 20;
    }
    //慢的过分了，就更新位置
    if(sleepMs>=-100000&&sleepMs<-400){
        user->p_state->PlayPosChange(sleepMs/1000);
        printf("Video Change the Pos:%d\n",sleepMs);
        return 20;
    }

    if(sleepMs>=100000||sleepMs<-100000)
        printf("Video PTS failed too bad:%d\n",sleepMs);






    //正常播放
    return -1;
}

void VideoOutput360R::videoRefresh(double *remaining_time)
{
    MAVFrame* mFrame=NULL;


    if(p_state->isPaused){
        //暂停中
        if(p_state->stepOne!=0){
            //目前只支持进一
            p_state->sync_clocker->pauseSync(false);
            printf("Step One Begin\n");
        }else{
            return;
        }
    }
    if(p_state->GetPosFlag()!=0){
        //改位置中
        return;
    }


    mFrame=frame_queue->Front();
    static int times=0;
    if(mFrame){
        if(p_state->stepOne!=0){
            printf("Step One Control\n");
        }
        int ret=ControlPlay(this,mFrame);
        if(ret==0){
            //丢帧
            this->frame_queue->Pop(1);
            *remaining_time=0;
            return;
        }else if(ret>0){
            //延迟播放
            return;
        }
        if(p_state->stepOne!=0){
            printf("Step One In\n");
        }




        //渲染
        AVFrame* dst_frame=NULL;
        if(VideoPrepare(this,mFrame->frame,&dst_frame)<0){
            mFrame=frame_queue->Pop(1);
            av_frame_free(&mFrame->frame);
            delete mFrame;
            return ;
        }
        printf("Video Frame %d\n",times++);
        printf("Src_V-fmt:%s\n",av_get_pix_fmt_name((AVPixelFormat)mFrame->frame->format));
        printf("Dst_V-fmt:%s\n",av_get_pix_fmt_name((AVPixelFormat)dst_frame->format));

        //开始处理纹理
        unsigned int tex_y,tex_u,tex_v;
        //生成纹理，
        GenerateTexture(dst_frame,&tex_y,&tex_u,&tex_v);
        shader->use();
        shader->SetInt("tex_y",0);
        shader->SetInt("tex_u",1);
        shader->SetInt("tex_v",2);

        openglDraw(tex_y,tex_u,tex_v);//用于绘制OpenGL内容
        SDL_GL_SwapWindow(this->win);//刷新缓冲区到屏幕
        //SDL_Delay(1000 / 60);//控制帧率60FPS




        mFrame=frame_queue->Pop(1);
        av_frame_free(&mFrame->frame);
        delete mFrame;
        av_frame_free(&dst_frame);



        if(p_state->stepOne!=0){
            //播放一帧
            printf("Step One End\n");
            p_state->sync_clocker->pauseSync(true);
            p_state->stepOne=0;
        }
    }

}

void VideoOutput360R::ttfRefresh(long long v_pts)
{


}

void VideoOutput360R::openglDraw(unsigned int tex_y,unsigned int tex_u,unsigned int tex_v)
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);//设置清除色
    glClear(GL_COLOR_BUFFER_BIT);//使用清除色，清除颜色缓冲区，便于绘制新的内容
    glClear(GL_DEPTH_BUFFER_BIT);//深度缓存


    //设置并绑定纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,tex_y);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,tex_u);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D,tex_v);

    //坐标系
    glm::mat4 model=glm::mat4(1.0f);;//世界矩阵
    model=glm::rotate(model,glm::radians(180.0f),glm::vec3(1.0f,0.0f,0.0f));

    //model=glm::translate(model,glm::vec3(0.0f,0.0f,0.0f));
    glm::mat4 view=glm::mat4(1.0f);;//观察矩阵
    //view=glm::rotate(view,)
    //view=glm::translate(view,glm::vec3(0.0f,0.0f,0.0f));
    view=glm::translate(view,glm::vec3(c_loc_x,c_loc_y,c_loc_z));
    view=glm::rotate(view,c_ro_x,glm::vec3(1.0f,0.0f,0.0f));
    view=glm::rotate(view,c_ro_y,glm::vec3(0.0f,1.0f,0.0f));
    //view=glm::rotate(model,(float)SDL_GetTicks()*rotate_rate,glm::vec3(1.0f,1.0f,.0f));
    glm::mat4 projection=glm::mat4(1.0f);;//透视矩阵
    projection=glm::perspective(glm::radians(120.0f),
                                  (float)(p_state->vInput->sdl_user->v_width/p_state->vInput->sdl_user->v_height),0.1f,100.0f);
    //projection=glm::translate(projection,glm::vec3(0.0f,0.0f,0.0f));

    shader->use();
    shader->SetMat4("model",model);
    shader->SetMat4("view",view);
    shader->SetMat4("projection",projection);

    // 绘制球
    // 开启面剔除(只需要展示一个面，否则会有重合)
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES,rectI.size(),GL_UNSIGNED_INT,0);
}

int VideoOutput360R::CreateRect(std::vector<float> &rectVertices, std::vector<unsigned int> &rectIndices)
{
    const float length=2.0f;//x
    const float width=2.0f;//z
    const float height=2.0f;//y

    //六个面(顺时针,从上到下)共是10个顶点（定点闭合）以及对应的纹理坐标
    //no.0
    rectVertices.push_back(-length);
    rectVertices.push_back(height);
    rectVertices.push_back(-width);
    rectVertices.push_back(0);
    rectVertices.push_back(1);
    //no.1
    rectVertices.push_back(length);
    rectVertices.push_back(height);
    rectVertices.push_back(-width);
    rectVertices.push_back(1.0/4.0);
    rectVertices.push_back(1);
    //no.2
    rectVertices.push_back(length);
    rectVertices.push_back(height);
    rectVertices.push_back(width);
    rectVertices.push_back(2.0/4.0);
    rectVertices.push_back(1);
    //no.3
    rectVertices.push_back(-length);
    rectVertices.push_back(height);
    rectVertices.push_back(width);
    rectVertices.push_back(3.0/4.0);
    rectVertices.push_back(1);
    //no.4(和no.0闭合)
    rectVertices.push_back(-length);
    rectVertices.push_back(height);
    rectVertices.push_back(-width);
    rectVertices.push_back(1);
    rectVertices.push_back(1);
    //no.5
    rectVertices.push_back(-length);
    rectVertices.push_back(-height);
    rectVertices.push_back(-width);
    rectVertices.push_back(0);
    rectVertices.push_back(0);
    //no.6
    rectVertices.push_back(length);
    rectVertices.push_back(-height);
    rectVertices.push_back(-width);
    rectVertices.push_back(1.0/4.0);
    rectVertices.push_back(0);
    //no.7
    rectVertices.push_back(length);
    rectVertices.push_back(-height);
    rectVertices.push_back(width);
    rectVertices.push_back(2.0/4.0);
    rectVertices.push_back(0);
    //no.8
    rectVertices.push_back(-length);
    rectVertices.push_back(-height);
    rectVertices.push_back(width);
    rectVertices.push_back(3.0/4.0);
    rectVertices.push_back(0);
    //no.9(和no.5重合)
    rectVertices.push_back(-length);
    rectVertices.push_back(-height);
    rectVertices.push_back(-width);
    rectVertices.push_back(1);
    rectVertices.push_back(0);

    //索引
    //后侧矩形
    //5-6-1
    rectIndices.push_back(5);
    rectIndices.push_back(6);
    rectIndices.push_back(1);
    //5-1-0
    rectIndices.push_back(5);
    rectIndices.push_back(1);
    rectIndices.push_back(0);
    //右侧矩形
    //6-7-2
    rectIndices.push_back(6);
    rectIndices.push_back(7);
    rectIndices.push_back(2);
    //6-2-1
    rectIndices.push_back(6);
    rectIndices.push_back(2);
    rectIndices.push_back(1);
    //前侧矩形
    //7-8-3
    rectIndices.push_back(7);
    rectIndices.push_back(8);
    rectIndices.push_back(3);
    //7-3-2
    rectIndices.push_back(7);
    rectIndices.push_back(3);
    rectIndices.push_back(2);
    //左侧矩形
    //8-9-4
    rectIndices.push_back(8);
    rectIndices.push_back(9);
    rectIndices.push_back(4);
    //8-4-3
    rectIndices.push_back(8);
    rectIndices.push_back(4);
    rectIndices.push_back(3);

}

int VideoOutput360R::BindRect(const std::vector<float> &rectVertices, std::vector<unsigned int> &rectIndices)
{
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);
    //绑定
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    // 将顶点数据绑定至当前默认的缓冲中
    glBufferData(GL_ARRAY_BUFFER, rectVertices.size() * sizeof(float), &rectVertices[0], GL_STATIC_DRAW);


    glGenBuffers(1,&EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, rectIndices.size() * sizeof(int), &rectIndices[0], GL_STATIC_DRAW);

    // 设置顶点属性指针
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    // 设置纹理属性指针
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    // 解绑VAO和VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return 0;

}


void VideoOutput360R::GenerateTexture(AVFrame* frame,unsigned int *tex_y,unsigned int *tex_u,unsigned int *tex_v)
{
    glGenTextures(1,tex_y);
    glBindTexture(GL_TEXTURE_2D,*tex_y);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, frame->linesize[0]);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RED,
                 frame->width,
                 frame->height,
                 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 frame->data[0]);



    glGenTextures(1,tex_u);
    glBindTexture(GL_TEXTURE_2D,*tex_u);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, frame->linesize[1]);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RED,
                 frame->width/2,
                 frame->height/2,
                 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 frame->data[1]);

    glGenTextures(1,tex_v);
    glBindTexture(GL_TEXTURE_2D,*tex_v);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, frame->linesize[2]);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RED,
                 frame->width / 2,
                 frame->height / 2,
                 0,
                 GL_RED,
                 GL_UNSIGNED_BYTE,
                 frame->data[2]);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);


}






