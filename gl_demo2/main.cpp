//成功绘制球体，成功加载球体的纹理

#include <iostream>
#include"src/include/shader.h"

using namespace std;

extern "C"{
#include"src/include/glad.h"
#include"SDL.h"
#include"SDL_image.h"
}


static const GLfloat PI = 3.14159265358979323846f;
//将球横轴划分成50X50的网格
static const int Y_SEGMENTS=50;
static const int X_SEGMENTS=50;

unsigned int VAO,VBO,EBO;

std::vector<float>sphereV;//顶点坐标(float*3)+纹理坐标(float*2)
std::vector<unsigned int>sphereI;//顶点序列


// openGL 绘制部分
void opengldraw(Shader* shader,unsigned int texture)
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);//设置清除色
    glClear(GL_COLOR_BUFFER_BIT);//使用清除色，清除颜色缓冲区，便于绘制新的内容


    //设置并绑定纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    shader->use();
    // 绘制球
    // 开启面剔除(只需要展示一个面，否则会有重合)
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES,sphereI.size(),GL_UNSIGNED_INT,0);
    // 使用线框模式绘制
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //glDrawElements(GL_TRIANGLES, X_SEGMENTS * Y_SEGMENTS * 6, GL_UNSIGNED_INT, 0);
    // 点阵模式绘制
    //glPointSize(5);
    //glDrawElements(GL_POINTS, X_SEGMENTS*Y_SEGMENTS*6, GL_UNSIGNED_INT, 0);
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

//得到了球体的定点信息
int CreateSphere(std::vector<float>& sphereVertices,std::vector<unsigned int>&sphereIndices)
{

    //生成球的顶点
    for (int y = 0; y <= Y_SEGMENTS; y++)
    {
        for (int x = 0; x <= X_SEGMENTS; x++)
        {
            //三个顶点坐标
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
            float yPos = std::cos(ySegment * PI);
            float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

            sphereVertices.push_back(xPos);
            sphereVertices.push_back(yPos);
            sphereVertices.push_back(zPos);
            //两个纹理坐标
            float t_y=(float)y/(float)Y_SEGMENTS;
            float t_x=(float)x/(float)X_SEGMENTS;
            sphereVertices.push_back(t_x);
            sphereVertices.push_back(t_y);
        }
    }
    //根据球面上的点的坐标，构造三角形顶点的数组
    for (int i = 0; i < Y_SEGMENTS; i++)
    {
        for (int j = 0; j < X_SEGMENTS; j++)
        {
            sphereIndices.push_back(i * (X_SEGMENTS + 1) + j);
            sphereIndices.push_back((i + 1) * (X_SEGMENTS + 1) + j);
            sphereIndices.push_back((i + 1) * (X_SEGMENTS + 1) + j + 1);

            sphereIndices.push_back(i * (X_SEGMENTS + 1) + j);
            sphereIndices.push_back((i + 1) * (X_SEGMENTS + 1) + j + 1);
            sphereIndices.push_back(i * (X_SEGMENTS + 1) + j + 1);
        }
    }
    return 0;
}


int BindSphere(const std::vector<float>& sphereVertices,std::vector<unsigned int>& sphereIndices)
{

    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);
    //绑定
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    // 将顶点数据绑定至当前默认的缓冲中
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), &sphereVertices[0], GL_STATIC_DRAW);


    glGenBuffers(1,&EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(int), &sphereIndices[0], GL_STATIC_DRAW);

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
    SDL_Surface* loaddedSurface=IMG_Load("video/out.png");
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
                                          1280, 1024, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);


    // 设置 SQL 版本和信息
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);//设置深度缓冲区大小
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);//OpenGL核心模式
    SDL_GLContext context=SDL_GL_CreateContext(window);//通过窗口创建OpenGL上下文

    SDL_GL_SetSwapInterval(1);//垂直同步，确保帧率不超过显示器的刷新率，避免画面撕裂
    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);//加载OpenGL函数指针


    //加载Shader
    Shader shader("vertex.shader","fragment.shader");
    //开始处理球体

    CreateSphere(sphereV,sphereI);
    BindSphere(sphereV,sphereI);

    //开始处理纹理
    unsigned char* data;
    unsigned int texture;
    int width,height,channels;


    GenerateTexture(&data,&texture,&width,&height,&channels);

    shader.use();
    shader.SetInt("texture1",0);



    //SDL的事件处理
    bool quit = false;
    SDL_Event event;
    while (quit == false) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) quit = true;
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
