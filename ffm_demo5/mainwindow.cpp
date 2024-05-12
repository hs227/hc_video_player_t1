#include "mainwindow.h"
#include "ui_mainwindow.h"

#include<QFileDialog>
#include<QMessageBox>
#include<QTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("mPlayer");
    ui->sdlWindow->setScaledContents(false);


    videoDefaultPath=QCoreApplication::applicationDirPath();
    videoDefaultPath+="/../video/";
    qDebug()<<"videoDefaultPath:"<<videoDefaultPath;



    if(sdl_user.Init((void*)ui->sdlWindow->winId())<0){
        qDebug()<<"sdl_user init failed\n";
        exit(EXIT_FAILURE);
    }

    timer=new QTimer(this);
    connect(timer,&QTimer::timeout,this,&MainWindow::UpdateVPTS);
    timer->setInterval(1000);
    timer->start();


    connect(ui->posSlider,&QSlider::sliderPressed,this,[this](){
        this->slider_isDragging=true;
        this->slider_lastValue=this->ui->posSlider->value();
    });
    connect(ui->posSlider,&QSlider::sliderReleased,this,[this](){
        double start=this->slider_lastValue;
        double end=this->ui->posSlider->value();
        double pos_change=end-start;//s
        this->SeekV(pos_change);
        this->slider_isDragging=false;
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::Init()
{

}



void MainWindow::PlayV()
{
    VideoInput input;
    input.sdl_user=&this->sdl_user;

    videoCxtManager.PlayVideo(input);

    //更新视频时间
    UpdateVTime();
    //更新位置时间
    UpdateVPTS();
}



void MainWindow::CloseV()
{
    videoCxtManager.CloseVideo();

}

void MainWindow::VolumeV(float v)
{
    videoCxtManager.VolumeChange(v);
    int nV=videoCxtManager.volume*100;
    ui->volumeLabel->setText(QString::fromStdString(std::to_string(nV)));
}

void MainWindow::SpeedV(float s)
{
    videoCxtManager.SpeedUpdate(s);
    ui->speedSpinBox->setValue(videoCxtManager.speed);
}

void MainWindow::SeekV(float s)
{
    //单位s
    videoCxtManager.Seek(s);
}

void MainWindow::StepV(int s)
{
    videoCxtManager.Step(s);
}

void MainWindow::UpdateVTime()
{
    int seconds=videoCxtManager.sync_clocker.GetDuration();
    int hour=seconds/(3600);
    seconds-=hour*3600;
    int minute=seconds/60;
    seconds-=minute*60;
    QString sumTime=QString("%1:%2:%3").arg(hour).arg(minute).arg(seconds);
    ui->posLabelB->setText(sumTime);
    ui->posSlider->setRange(0,videoCxtManager.sync_clocker.GetDuration());
    ui->posSlider->setValue(0);
}

void MainWindow::UpdateVPTS()
{
    if(!videoCxtManager.playing||videoCxtManager.pause_f){
        return;
    }
    int seconds=videoCxtManager.GetPts()*videoCxtManager.speed;
    int hour=seconds/(3600);
    seconds-=hour*3600;
    int minute=seconds/60;
    seconds-=minute*60;
    QString sumTime=QString("%1:%2:%3").arg(hour).arg(minute).arg(seconds);
    ui->posLabelA->setText(sumTime);
    if(!this->slider_isDragging){
        //进度条拖动时不发生变化
        ui->posSlider->setValue(videoCxtManager.GetPts()*videoCxtManager.speed);
    }
}




void MainWindow::on_vOpenBtn_clicked()
{
    if(this->videoCxtManager.playing){
        //还有视频在播放
        this->on_vCloseBtn_clicked();
    }


    //打开一个浏览窗口
    QString fileName=QFileDialog::getOpenFileName(this,"打开",
                                                    videoDefaultPath,
                                                    "*.mkv *.mp4");

    if(fileName.isEmpty()){
        QMessageBox::warning(this,"警告","请选择一个文件");
    }else{
        qDebug()<<"New Video Open:"<<fileName;

        //如果有视频还在播放，就立刻关闭
        this->CloseV();
        //判断是否打开过这个文件
        if(videoCxtManager.VExisted(fileName)==false){
            //没有视频信息，需要保存视频信息
            videoCxtManager.AddVideo(fileName);
        }
        //打开视频
        videoCxtManager.OpenVideo(fileName);
        //显示目前打开视频的名称
        ui->videoPathLabel->setText(fileName);
    }
}


void MainWindow::on_vPlayBtn_clicked()
{
    this->PlayV();
    ui->vPlayBtn->setText(QString(videoCxtManager.pause_f?"播放":"暂停"));
}


void MainWindow::on_vCloseBtn_clicked()
{
    //关闭正在播放的视频
    this->CloseV();
    ui->videoPathLabel->setText(QString("waiting"));
    ui->vPlayBtn->setText(QString(videoCxtManager.pause_f?"播放":"暂停"));
}


void MainWindow::resizeEvent(QResizeEvent *event)
{
    // 在这里处理窗口大小变化的逻辑
    // event->size() 返回新的窗口尺寸
    qDebug() << "窗口大小已更改为：" << event->size();


    // if(sdl_user.widget){
    //     sdl_user.widget->resize(this->centralWidget()->size().width(),this->centralWidget()->size().height()-40);
    // }


    if(videoCxtManager.playing){
       videoCxtManager.playing->WindowResize();
    }

}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    qDebug()<<"Get KET";
    switch(event->key()){
    case Qt::Key_Space:
        //空格键暂停和播放
        on_vPlayBtn_clicked();
        break;
    case Qt::Key_Up:
        //上键音量增大
        this->VolumeV(0.1f);
        break;
    case Qt::Key_Down:
        //下键音量减少
        this->VolumeV(-0.1f);
        break;
    case Qt::Key_Left:
        //左键后退
        this->SeekV(-10);
        break;
    case Qt::Key_Right:
        //右键
        if(videoCxtManager.pause_f){
            //逐帧
            this->StepV(1);
        }else{
            //快进
            this->SeekV(10);
        }
        break;
    case Qt::Key_M:
        //播放加速
        this->SpeedV(1);
        break;
    case Qt::Key_N:
        //播放减速
        this->SpeedV(-1);
        break;
    default:
        break;
    }


}
