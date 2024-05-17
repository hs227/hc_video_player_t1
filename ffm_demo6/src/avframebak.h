//解码后frame数据的缓存管理器
//负责保存和清空
#ifndef AVFRAMEBAK_H
#define AVFRAMEBAK_H

#include<iostream>
#include<filesystem>
#include<functional>
#include<fstream>
#include"stdlib.h"

extern "C"{
#include"libavcodec/avcodec.h"


}

class AVFrameBak
{
public:
    AVFrameBak();
    ~AVFrameBak();

    //重新设置bak_root_path
    int set_bak_root_path(const std::filesystem::path& dir_path);
    int set_bak_root_path(const std::string& dir_path);

    //  清空全部的的frame_bak
    //  每次打开新的视频，都要删除旧视频的缓存
    void clear_frame_bak();
    //清空bak子文件夹
    void clear_frame_sub_bak(const std::filesystem::path& dir_path);

    //保存frame_bak
    //sub_dir:文件夹名,frame_name:frame_name+"id"+".bak"
    int save_frame_bak(const std::string& sub_dir,const std::string& frame_name,AVFrame* data);

    //读取frame_bak
    int load_frame_bak(const std::string& sub_dir,const std::string& frame_name,AVFrame* data);


    //字幕流相关
    int save_sub_bak(const std::string& sub_dir,const std::string& frame_name,AVSubtitle* data);



private:
    std::filesystem::path bak_root_path=std::filesystem::path("frame_bak");
};

#endif // AVFRAMEBAK_H
