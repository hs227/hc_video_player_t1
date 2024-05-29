#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QVector>
#include<QResizeEvent>
#include<QKeyEvent>
#include<QTimer>
#include<QSlider>

#include"src/sdluser.h"
#include"src/mainSDL.h"
#include"videocontext.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void Init();



    //播放视频
    void PlayV();

    //关闭视频
    void CloseV();

    //音量改变
    void VolumeV(float v);

    //播放速度改变
    void SpeedV(float s);

    //进度改变
    void SeekV(float s);

    //逐帧
    void StepV(int s);


    //更新视频时长
    void UpdateVTime();
    //更新视频pts
    void UpdateVPTS();


    //Events
    void resizeEvent(QResizeEvent *event);
    void keyPressEvent(QKeyEvent* event);

private slots:
    void on_vOpenBtn_clicked();
    void on_vPlayBtn_clicked();
    void on_vCloseBtn_clicked();

public:
    SDLUser sdl_user;
private:
    Ui::MainWindow *ui;
    QTimer* timer;


    QString videoDefaultPath;//保存了默认视频文件夹路径
    VideoContextManager videoCxtManager;


    //为进度条
    bool slider_isDragging=false;
    double slider_lastValue=0;
};
#endif // MAINWINDOW_H
