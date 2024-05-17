#include "videocontext.h"

VideoContext::VideoContext(const QString &filepath_)
    :filepath(filepath_)
{
    QFile file(filepath);
    fileInfo=QFileInfo(file);
    // // 获取纯文件名，不包含路径
    // QString baseName = fileInfo.baseName();
    // // 或者，如果你需要包括后缀，则可以使用
    // QString completeBaseName = fileInfo.fileName();
    // qDebug() << "Base name of the file (without extension):" << baseName;
    // qDebug() << "Complete base name of the file (with extension):" << completeBaseName;
}

QString VideoContext::BaseName()
{
    return fileInfo.baseName();
}

QString VideoContext::CompleteBaseName()
{
    return fileInfo.fileName();
}




VideoContextManager::VideoContextManager()
{

}

void VideoContextManager::AddVideo(const QString& filepath)
{
    VideoContext newVideoCxt(filepath);
    videoVec.push_front(newVideoCxt);
}


bool VideoContextManager::VExisted(const QString& filepath)
{
    QVector<VideoContext>::iterator it;
    for(it=videoVec.begin();it!=videoVec.end();++it){
        if(it->filepath==filepath){
            return true;
        }
    }
    return false;
}

int VideoContextManager::VSize()
{
    return videoVec.size();
}

double VideoContextManager::GetPts()
{
    if(playing){
        int64_t pts=sync_clocker.getSyncDrift();//ms
        pts/=1000;//s
        return pts;
    }
    return 0;
}

void VideoContextManager::OpenVideo(const QString &filepath)
{
    for(int i=0;i<videoVec.size();++i){
        if(videoVec[i].filepath==filepath){
            videoVecIdx=i;
            return ;
        }
    }

}

void VideoContextManager::PlayVideo(VideoInput v_in)
{
    //v_in首先由mainwindow实现初始化

    if(playing){
        //如果有视频正在播放(那么就是播放状态的改变)
        playing->PauseChange();
        pause_f=!pause_f;
        return;
    }

    //获得对应视频的信息
    //无法选取对应的视频
    if(videoVecIdx<0||videoVecIdx>=videoVec.size()){
        return;
    }
    VideoContext* v_ctx=&videoVec[videoVecIdx];
    playing=new PlayerThread();

    v_in.filepath=v_ctx->filepath.toStdString();
    v_in.volume=&this->volume;
    v_in.speed=&this->speed;
    v_in.clock_h=false;
    v_in.sync_clock=&this->sync_clocker;


    playing->Init(&v_in);
    playing->Start();

    //获得从playing中返回的数据
    while(playing->videoInput.clock_h==false){
        QThread::msleep(10);
    }



    //状态更新
    pause_f=false;//开始播放
}



void VideoContextManager::CloseVideo()
{
    if(!playing){
        qDebug()<<"no video is playing\n";
        return;
    }
    playing->EndPlay();
    playing->Stop();
    delete playing;
    playing=NULL;

    this->pause_f=true;
}

void VideoContextManager::VolumeChange(float volume_dis)
{
    static const float VOLUME_UP=5.0f;
    static const float VOLUME_DOWN=0.0f;

    volume+=volume_dis;
    volume=std::min(VOLUME_UP,volume);
    volume=std::max(VOLUME_DOWN,volume);

    if(playing){
        //如果有正在播放的视频，则改变音量
        playing->VolumeUpdate();
    }


}

void VideoContextManager::Seek(int s)
{
    if(playing){
        playing->Seek(s);
    }
}


float SpeedUp(float pSpeed)
{
    if(pSpeed<1.0f){
        pSpeed=1.0f;
    }else if(pSpeed<1.5f){
        pSpeed=1.5f;
    }else if(pSpeed<2.0f){
        pSpeed=2.0f;
    }else{
        pSpeed=2.0f;
    }
    return pSpeed;
}
float SpeedDown(float pSpeed)
{
    if(pSpeed>1.5f){
        pSpeed=1.5f;
    }else if(pSpeed>1.0f){
        pSpeed=1.0f;
    }else if(pSpeed>0.5f){
        pSpeed=0.5f;
    }else{
        pSpeed=0.5f;
    }
    return pSpeed;
}
void VideoContextManager::SpeedUpdate(int direct)
{
    float nSpeed;
    if(direct>0){
        nSpeed=SpeedUp(this->speed);
    }else if(direct<0){
        nSpeed=SpeedDown(this->speed);
    }else{
        return ;
    }
    this->speed=nSpeed;
    qDebug()<<"Speed Update:"<<this->speed;
    if(playing){
        playing->SpeedUpdate();
    }

}

void VideoContextManager::Step(int s)
{
    if(playing){
        playing->Step(s);
    }
}

void VideoContextManager::k_360_in(char key)
{
    if(playing){
        playing->k_360_in(key);
    }
}
