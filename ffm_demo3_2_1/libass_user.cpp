#include "libass_user.h"
#include<fstream>
LibassUser::LibassUser()
{

}

LibassUser::~LibassUser()
{

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
    return -1;
}

void LibassUser::DeInit()
{
    if(ass_renderer){
        ass_renderer_done(ass_renderer);
    }
    if(ass_track){
        ass_free_track(ass_track);
    }
    if(libass){
        ass_library_done(libass);
    }

}


