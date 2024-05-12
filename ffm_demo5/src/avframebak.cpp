#include "avframebak.h"

AVFrameBak::AVFrameBak()
{

}

AVFrameBak::~AVFrameBak()
{

}

int AVFrameBak::set_bak_root_path(const std::filesystem::path &dir_path)
{
    if(!std::filesystem::exists(dir_path)){
        //该文件夹不存在
        return -1;
    }
    bak_root_path=dir_path;
    return 0;
}

int AVFrameBak::set_bak_root_path(const std::string &dir_path)
{
    return set_bak_root_path(std::filesystem::path(dir_path));
}


void AVFrameBak::clear_frame_bak()
{
    const std::filesystem::path& dir_path=bak_root_path;

    if (!std::filesystem::exists(dir_path)) {
        std::cerr << "Directory " << dir_path << " does not exist.\n";
        return;
    }
    //遍历清空所有子文件夹，然后删除文件夹
    for (const auto& entry : std::filesystem::directory_iterator(dir_path)){
        if(entry.is_directory()){
            try {
                clear_frame_sub_bak(entry.path());
                std::filesystem::remove(entry.path());
            } catch (const std::filesystem::filesystem_error& e) {
                std::cerr<<"Failed to remove the directory: "<<entry.path()
                          <<". Error :"<<e.what()<<std::endl;
            }
        }
    }

}

void AVFrameBak::clear_frame_sub_bak(const std::filesystem::path &sub_path)
{
    if (!std::filesystem::exists(sub_path)) {
        std::cerr << "Frame  " << sub_path << " does not exist.\n";
        return;
    }
    //清空子文件夹下的所有缓存bak文件
    for (const auto& entry : std::filesystem::directory_iterator(sub_path)) {
        try {
            std::filesystem::remove(entry.path());
        } catch (const std::filesystem::filesystem_error& err) {
            std::cerr<<"Failed to remove file: "<<entry.path()
                      <<". Error: "<<err.what()<<std::endl;
        }
    }
}

int AVFrameBak::save_frame_bak(const std::string& sub_dir,const std::string &frame_name,AVFrame* data)
{
    std::filesystem::path dir_path=bak_root_path;
    dir_path+="/"+sub_dir;
    //如果文件夹不存在则创建
    if(!std::filesystem::exists(dir_path)){
        try {
            std::filesystem::create_directories(dir_path);//可以创建多级文件夹
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr<<dir_path<<". Dict create failed\n";
            return -1;
        }
    }
    //创建文件，并将data写入保存
    std::filesystem::path file_path=dir_path;
    file_path+="/"+frame_name+".bak";

    std::ofstream out_file(file_path);
    if(!out_file.is_open()){
        std::cerr<<file_path<<". Open failed.\n";
        return -1;
    }
    //frame->metadata
    out_file.write((char*)data,sizeof(AVFrame));

    //frame->data
    int frame_size=av_samples_get_buffer_size(NULL,data->channels,data->nb_samples,(AVSampleFormat)data->format,1);
    if(frame_size>0){
        char tmp=((char*)data->data[0])[0];// DEBUG
        out_file.write((char*)&data->data[0],frame_size);
    }else{
        out_file.close();
        return -1;
    }
    out_file.close();
    return 0;

}

int AVFrameBak::load_frame_bak(const std::string &sub_dir, const std::string &frame_name, AVFrame *data)
{
    std::filesystem::path file_path=bak_root_path;
    file_path+="/"+sub_dir+"/"+frame_name+".bak";
    std::ifstream in_file(file_path);
    if(!in_file.is_open()){
        return -1;
    }
    //frame->metadata
    in_file.read((char*)data,sizeof(AVFrame));

    //frame->data
    int frame_size=av_samples_get_buffer_size(NULL,data->channels,data->nb_samples,(AVSampleFormat)data->format,1);
    if(frame_size>0){
        uint8_t* total_out_buffer=(uint8_t*)av_malloc(frame_size);
        //为每个通道分配内存区域
        if(total_out_buffer==NULL){
            in_file.close();
            return -1;
        }
        for(int i=0;i<data->channels;++i){
            data->data[i]=total_out_buffer+i*(frame_size/data->channels);
        }
    }else{
        in_file.close();
        return -1;
    }
    in_file.close();
    return 0;
}

int AVFrameBak::save_sub_bak(const std::string &sub_dir, const std::string &frame_name, AVSubtitle *data)
{
    std::filesystem::path dir_path=bak_root_path;
    dir_path+="/"+sub_dir;
    //如果文件夹不存在则创建
    if(!std::filesystem::exists(dir_path)){
        try {
            std::filesystem::create_directories(dir_path);//可以创建多级文件夹
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr<<dir_path<<". Dict create failed\n";
            return -1;
        }
    }

    //创建文件，并将data写入保存
    std::filesystem::path file_path=dir_path;
    file_path+="/"+frame_name+".bak";

    std::ofstream out_file(file_path);
    if(!out_file.is_open()){
        std::cerr<<file_path<<". Open failed.\n";
        return -1;
    }
    //sub->metadata
    //out_file.write((char*)data,sizeof(AVSubtitle));

    //sub->ass
    out_file.write((*data->rects)->ass,strlen((*data->rects)->ass));

    out_file.close();
    return 0;

}

