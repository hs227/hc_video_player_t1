//成功绘制球体，成功加载球体的纹理
//实现坐标系，看球体纹理

#include <iostream>
#include"src/include/shader.h"

using namespace std;

extern "C"{
#include"src/include/glad.h"
#include"SDL.h"
#include"SDL_image.h"
#include"glm.hpp"
#include"glm/gtc/matrix_transform.hpp"
#include"glm/gtc/type_ptr.hpp"
}


static const float PI=glm::pi<float>();

unsigned int VAO,VBO,EBO;

std::vector<float>rectV;//顶点坐标(float*3)+纹理坐标(float*2)
std::vector<unsigned int>rectI;//顶点序列

static const int SCREEN_W=1280;
static const int SCREEN_H=1024;

//旋转速率
static const float rotate_rate=0.0001;

//camera loc
float c_loc_x=0,c_loc_y=0,c_loc_z=0;
static const float camera_mv_rate=0.2f;
//camera rotate
float c_ro_x=0,c_ro_y=0;
static const float camera_ro_rate=glm::radians(15.0f);



// openGL 绘制部分
void opengldraw(Shader* shader,unsigned int texture)
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);//设置清除色
    glClear(GL_COLOR_BUFFER_BIT);//使用清除色，清除颜色缓冲区，便于绘制新的内容
    glClear(GL_DEPTH_BUFFER_BIT);//深度缓存


    //设置并绑定纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    //坐标系
    glm::mat4 model=glm::mat4(1.0f);;//世界矩阵
    //model=glm::rotate(model,glm::pi<float>(),glm::vec3(0.0f,1.0f,0.0f));
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
    projection=glm::perspective(glm::radians(45.0f),(float)SCREEN_W/SCREEN_H,0.1f,100.0f);
    //projection=glm::translate(projection,glm::vec3(0.0f,0.0f,0.0f));

    shader->use();
    shader->SetMat4("model",model);
    shader->SetMat4("view",view);
    shader->SetMat4("projection",projection);

    // 绘制
    // 开启面剔除(只需要展示一个面，否则会有重合)
    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES,rectI.size(),GL_UNSIGNED_INT,0);

}


SDL_Window* sdl_window(void *  winHandler)
{
    if(winHandler==NULL){
        fprintf(stderr,"winHandler is NULL");
        return NULL;
    }
    //通过窗口句柄，将SDL嵌入对应对应窗口
    SDL_Window* window=SDL_CreateWindowFrom(winHandler);

    if(SDL_Init(SDL_INIT_VIDEO)!=0){
        fprintf(stderr,"SDL_video_init failed\n");
        return NULL;
    }


    //设置OpenGL属性
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);

    //通过已有窗口创建OpenGL上下文
    SDL_GLContext glContext=SDL_GL_CreateContext(window);
    return window;
}



//得到立方体的顶点信息
int CreateRect(std::vector<float>& rectVertices,std::vector<unsigned int>& rectIndices)
{
    const float length=1.0f;//x
    const float width=1.0f;//z
    const float height=1.0f;//y

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


int BindRect(const std::vector<float>& rectVertices,std::vector<unsigned int>& rectIndices)
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


void GenerateTexture(
    unsigned char** data,
    unsigned int* texture,
    int* width,int*height,int*channels)
{
    glGenTextures(1,texture);
    glBindTexture(GL_TEXTURE_2D,*texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    IMG_Init(IMG_INIT_PNG);

    //加载图片
    SDL_Surface* loaddedSurface=IMG_Load("video/out1.png");
    if(loaddedSurface==NULL){
        fprintf(stderr,"IMG load failed\n");
        return;
    }

    //获得信息
    *width=loaddedSurface->w;
    *height=loaddedSurface->h;
    *channels=loaddedSurface->format->BytesPerPixel;
    *data=new unsigned char[(*width)*(*height)*(*channels)];
    memcpy(*data,loaddedSurface->pixels,(*width)*(*height)*(*channels));

    //绑定
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,*width,*height,0,GL_RGB,GL_UNSIGNED_BYTE,*data);
    glGenerateMipmap(GL_TEXTURE_2D);


    //清理
    SDL_FreeSurface(loaddedSurface);
    IMG_Quit();
}

#ifdef __MINGW32__
#undef main/* Prevents SDL from overriding main() */
#endif
int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow("SDL+OpenGL",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          SCREEN_W, SCREEN_H, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    // 设置 SQL 版本和信息
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);//设置深度缓冲区大小
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);//OpenGL核心模式
    SDL_GLContext context=SDL_GL_CreateContext(window);//通过窗口创建OpenGL上下文

    SDL_GL_SetSwapInterval(1);//垂直同步，确保帧率不超过显示器的刷新率，避免画面撕裂
    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);//加载OpenGL函数指针

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);


    //加载Shader
    Shader shader("vertex.shader","fragment.shader");
    //开始处理立方体
    CreateRect(rectV,rectI);
    BindRect(rectV,rectI);

    //开始处理纹理
    unsigned char* data;
    unsigned int texture;
    int width,height,channels;
    //生成纹理
    GenerateTexture(&data,&texture,&width,&height,&channels);
    //将纹理与shader中的texture1绑定
    shader.use();

    shader.SetInt("texture1",0);





    //SDL的事件处理
    bool quit = false;
    SDL_Event event;
    while (quit == false) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                quit = true;
            if(event.type==SDL_KEYDOWN){
                if(event.key.keysym.sym==SDLK_w){
                    //上
                    c_loc_y-=camera_mv_rate;
                }else if(event.key.keysym.sym==SDLK_s){
                    //下
                    c_loc_y+=camera_mv_rate;
                }else if(event.key.keysym.sym==SDLK_a){
                    //左
                    c_loc_x+=camera_mv_rate;
                }else if(event.key.keysym.sym==SDLK_d){
                    //右
                    c_loc_x-=camera_mv_rate;
                }else if(event.key.keysym.sym==SDLK_f){
                    //前进
                    c_loc_z+=camera_mv_rate;
                }else if(event.key.keysym.sym==SDLK_g){
                    //后退
                    c_loc_z-=camera_mv_rate;
                }else if(event.key.keysym.sym==SDLK_p){
                    //复位
                    c_loc_x=c_loc_y=c_loc_z=0;
                    c_ro_x=c_ro_y=0;
                }else if(event.key.keysym.sym==SDLK_q){
                    //抬头
                    c_ro_x-=camera_ro_rate;
                }else if(event.key.keysym.sym==SDLK_e){
                    //低头
                    c_ro_x+=camera_ro_rate;
                }else if(event.key.keysym.sym==SDLK_z){
                    //左转
                    c_ro_y+=camera_ro_rate;
                }else if(event.key.keysym.sym==SDLK_c){
                    //右转
                    c_ro_y-=camera_ro_rate;
                }
            }
        }


        opengldraw(&shader,texture);//用于绘制OpenGL内容
        SDL_GL_SwapWindow(window);//刷新缓冲区到屏幕
        SDL_Delay(1000 / 60);//控制帧率60FPS
    }

    glDeleteVertexArrays(1,&VAO);
    glDeleteBuffers(1,&VBO);
    glDeleteBuffers(1,&EBO);
    delete[] data;//释放图片数据
    SDL_GL_DeleteContext(context);//释放OpenGL上下文
    SDL_DestroyWindow(window);//关闭窗口，释放窗口资源
    SDL_Quit();
    return 0;
}
