#ifndef VIDEOCONTEXT_H
#define VIDEOCONTEXT_H


#include"src/mainSDL.h"


#include<QVector>
#include<QFileInfo>
#include<QThread>

//用于保存打开的视频信息
class VideoContext
{
public:
    VideoContext(const QString& filepath_);
    ~VideoContext(){};
    // 获取纯文件名，不包含路径和后缀
    QString BaseName();
    // 获取纯文件名，包含后缀
    QString CompleteBaseName();

public:
    QString filepath;//完整的文件路径
    QFileInfo fileInfo;

};



class VideoContextManager
{
public:
    VideoContextManager();
    ~VideoContextManager(){};

    //添加一个新的视频信息
    void AddVideo(const QString& filepath);
    //视频文件是否打开过
    bool VExisted(const QString& filepath);
    //数组长度
    int VSize();
    //获得当前视频的时间s
    double GetPts(void);

    //打开视频
    void OpenVideo(const QString& filepath);
    //播放视频
    void PlayVideo(VideoInput v_in);

    //关闭视频
    void CloseVideo();

    //音量变化
    void VolumeChange(float volume_dis);

    //寻址(秒数)
    void Seek(int s);
    //变速
    void SpeedUpdate(int direct);
    //逐帧
    void Step(int s);


public:
    QVector<VideoContext> videoVec;//打开过的视频信息
    int videoVecIdx=-1;//正在播放的视频，-1代表未播放
    PlayerThread* playing=NULL;

public:
    bool pause_f=true;
    float volume=1.0f;
    float speed=1.0f;
    AvSyncClocker sync_clocker;
};



#endif // VIDEOCONTEXT_H
