#include "mainwindow.h"

#include <QApplication>
#include<QDebug>
#include"src/mainSDL.h"

extern "C"{
#include"libavutil/avutil.h"
}



#ifdef __MINGW32__
#undef main/* Prevents SDL from overriding main() */
#endif
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();



    // //创建一个新的子窗口始终保持在主窗口的上方
    // //这个子窗口用于SDL的嵌入
    // QWidget *centralWidget= new QWidget;
    // QPalette palette;
    // //palette.setColor(QPalette::Window, QColor(255, 0, 0)); // 设置背景色为红色
    // centralWidget->resize(w.centralWidget()->size().width(),w.centralWidget()->size().height()-60);
    // //centralWidget->setPalette(palette);
    // //centralWidget->setAutoFillBackground(true);
    // centralWidget->setWindowFlags(centralWidget->windowFlags() | Qt::WindowStaysOnTopHint|Qt::Tool);
    // //centralWidget->move(w.pos().x(),w.pos().y());
    // centralWidget->setParent(&w);
    // centralWidget->show();
    // w.sdl_user.widget=centralWidget;
    // // w.sdl_user.v_width=centralWidget->size().width()*1.2;
    // // w.sdl_user.v_height=centralWidget->size().height()*1.2;
    // //SDLUser
    // if(w.sdl_user.Init((void*)centralWidget->winId())<0){
    //     qDebug()<<"sdl_user init failed\n";
    //     exit(EXIT_FAILURE);
    // }





    //qDebug()<<av_version_info();





    return a.exec();
}
