#include "libass_user.h"
#include<fstream>
LibassUser::LibassUser() {}


//For libass_user::Use
SDL_Surface* ass_render_frame_to_surface(ASS_Image* images)
{

    // 创建一个足够大的SDL_Surface来容纳所有字幕图像
    // 注意，这里为了简化，假设所有字幕都是不重叠的，实际情况可能需要更复杂的布局算法
    int totalWidth = 0, maxHeight = 0;
    ASS_Image *image = images;
    while (image) {
        totalWidth += image->w; // 这里简单累加宽度，实际应考虑字幕间可能需要的间距
        maxHeight = std::max(maxHeight, image->h);
        image = image->next;
    }

    SDL_Surface* nsubtitleSurface = SDL_CreateRGBSurfaceWithFormat(0, totalWidth, maxHeight, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!nsubtitleSurface) {
        return NULL;
    }
    SDL_FillRect(nsubtitleSurface, NULL, SDL_MapRGBA(nsubtitleSurface->format, 0, 0, 0, 0)); // 清空Surface为透明

    // 重新遍历图像链表，将图像数据绘制到Surface
    Uint8* pixels = (Uint8*)nsubtitleSurface->pixels;
    int pitch = nsubtitleSurface->pitch;
    image = images;
    int currentX = 0;
    while (image) {
        // 注意：这里简化的假设图像可以直接按行复制，实际应用中需要根据图像的alpha通道进行混合处理
        for (int y = 0; y < image->h; ++y) {
            memcpy(pixels + currentX + y * pitch, image->bitmap + y * image->stride, image->w * sizeof(uint32_t));
        }
        currentX += image->w; // 移动到下一个图像的位置
        image = image->next;
    }

    // 清理ASS_Image链表
    //ass_free_images(images);
    return nsubtitleSurface;
}
//失败的
void LibassUser::Use()
{
    ASS_Library* libass=ass_library_init();
    ASS_Renderer* ass_renderer=ass_renderer_init(libass);
    ASS_Track* ass_track=ass_read_file(libass,"output_ass.ass","UTF-8");
    ass_set_fonts(ass_renderer,NULL,"Arial",ASS_FONTPROVIDER_AUTODETECT,NULL,0);
    ass_set_frame_size(ass_renderer,800,600);;
    ASS_Event* events=ass_track->events;
    int event_count=ass_track->n_events;

    std::ofstream pout;
    pout.open("ass out.txt");

    char msg[128];
    for(int i=0;i<event_count;++i){
        //获得单个时间
        const ASS_Event& event=events[i];
        sprintf(msg,"Start:%lld,End:%lld,Text:%s\n",
                event.Start,(event.Start+event.Duration),event.Text);
        pout<<msg;
    }
    pout.close();


    long long currentms=5000;//ms
    int isChange=0;// 与上一次生成比较，0：没有变化，1：位置变了，2: 内容变了
    ASS_Image* assimg=ass_render_frame(ass_renderer,ass_track,currentms,&isChange);
    if(!assimg){
        return;
    }

    // 初始化SDL
    SDL_Init(SDL_INIT_VIDEO);

    int v_w=640;
    int v_h=480;

    // 创建窗口
    SDL_Window *window = SDL_CreateWindow("Subtitles with SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          v_w, v_h, 0);
    SDL_Renderer *sdlRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* subtitleTexture=NULL;

    SDL_Surface* subtitleSurface=nullptr;
        //=SDL_CreateRGBSurfaceWithFormat(
        //0, v_w, v_h, 32, SDL_PIXELFORMAT_ARGB8888);
    // if(!subtitleSurface){
    //     return;
    // }
    // 确保Surface内容清空为透明
    //SDL_FillRect(subtitleSurface, NULL, SDL_MapRGBA(subtitleSurface->format, 0, 0, 0, 0));
    // 渲染字幕到Surface
    subtitleSurface=ass_render_frame_to_surface(assimg);

    // 如果之前有旧的字幕纹理，销毁它
    if (subtitleTexture) {
        SDL_DestroyTexture(subtitleTexture);
    }

    // 将渲染了字幕的Surface转换为Texture
    subtitleTexture = SDL_CreateTextureFromSurface(sdlRenderer, subtitleSurface);
    if (!subtitleTexture) {
        // 错误处理
    }

    // 清除之前的Surface
    SDL_FreeSurface(subtitleSurface);

    // 将视频帧渲染到屏幕
    //SDL_RenderCopy(sdlRenderer, frameTexture, NULL, NULL);

    // 将字幕纹理作为覆盖层渲染到屏幕上
    SDL_Rect dstRect = {0, 0, v_w, v_h}; // 根据需要调整字幕位置
    SDL_RenderCopy(sdlRenderer, subtitleTexture, NULL, &dstRect);

    // 更新屏幕
    SDL_RenderPresent(sdlRenderer);
    while(1){}
}

//成功的
void LibassUser::Use2()
{
    ASS_Library* libass=ass_library_init();
    ASS_Renderer* ass_renderer=ass_renderer_init(libass);
    ASS_Track* ass_track=ass_read_file(libass,"output_ass.ass","UTF-8");
    ass_set_fonts(ass_renderer,NULL,"mysh",ASS_FONTPROVIDER_AUTODETECT,NULL,0);
    ass_set_frame_size(ass_renderer,800,600);;
    ASS_Event* events=ass_track->events;
    int event_count=ass_track->n_events;

    std::ofstream pout;
    pout.open("ass out.txt");

    char msg[512];
    for(int i=0;i<event_count;++i){
        //获得单个时间
        const ASS_Event& event=events[i];
        sprintf(msg,"Start:%lld,End:%lld,Text:%s\n",
                event.Start,(event.Start+event.Duration),event.Text);
        pout<<msg;

    }
    pout.close();



    // 初始化TTF库
    if (TTF_Init() == -1) {
        printf("Failed to initialize TTF: %s\n", TTF_GetError());
        return;
    }

    // 初始化SDL
    SDL_Init(SDL_INIT_VIDEO);

    int v_w=640;
    int v_h=480;

    // 创建窗口
    SDL_Window *window = SDL_CreateWindow("Subtitles with SDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          v_w, v_h, 0);
    SDL_Renderer *sdlRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);


    int fontSize=16;//ptx
    TTF_Font* font = TTF_OpenFont("msyh.ttf", fontSize);
    if (font == NULL) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return;
    }


    long long currentTime=5000;
    int index=0;
    char msg2[512];
    char* buf[2];
    buf[0]=msg;
    buf[1]=msg2;


    for(int i=0;i<event_count;++i){
        //获得单个时间
        const ASS_Event& event=events[i];
        if(currentTime>=event.Start&&
            currentTime<=(event.Start+event.Duration)){
            sprintf(buf[index++],"%s",event.Text);
            if(index==2)
                break;
        }
    }


    SDL_Color textColor = {255, 0, 0, 255}; // 文本颜色（RGBA）
    for(int i=0;i<index;++i){
        SDL_Surface* textSurface = TTF_RenderUTF8_Blended(font, buf[i], textColor);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(sdlRenderer, textSurface);

        // 将字幕纹理作为覆盖层渲染到屏幕上
        SDL_Rect dstRect = {(v_w/2-textSurface->w/2),v_h*5/6-i*32,textSurface->w,textSurface->h}; // 根据需要调整字幕位置
        SDL_RenderCopy(sdlRenderer, texture, NULL, &dstRect);
    }
    // 更新屏幕
    SDL_RenderPresent(sdlRenderer);
    while(1){}
}

int LibassUser::Init(LU_Type flag,std::string* name)
{
    this->type=flag;
    if(type==LU_Type::LU_NO_S){
        //没有字幕
        return 0;
    }else if(type==LU_Type::LU_OUT_S){
        //使用外挂字幕
        if(!name){
            fprintf(stderr,"LibassUser::Init: name is NULL\n");
            return -1;
        }
        libass=ass_library_init();
        //ass_renderer=ass_renderer_init(libass);
        ass_track=ass_read_file(libass,(char*)name->c_str(),"UTF-8");
        //ass_set_fonts(ass_renderer,NULL,"mysh",ASS_FONTPROVIDER_AUTODETECT,NULL,0);
        //ass_set_frame_size(ass_renderer,800,600);;
        return 0;
    }else if(type==LU_Type::LU_IN_S){
        //使用内嵌字幕
        libass=ass_library_init();
        ass_track=ass_new_track(libass);
        return 0;
    }
    //错误的flag
    fprintf(stderr,"LibassUser::Init:wrong flag[%d]\n",flag);


    libass=ass_library_init();
    ass_renderer=ass_renderer_init(libass);




    //ass_track=ass_read_file(libass,"output_ass.ass","UTF-8");
}


