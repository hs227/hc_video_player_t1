QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    src/audiofilterthread.cpp \
    src/audiooutput_v1.cpp \
    src/avframebak.cpp \
    src/avframequeue.cpp \
    src/avpacketqueue.cpp \
    src/avsyncclocker.cpp \
    src/decodethread.cpp \
    src/demuxthread.cpp \
    src/libass_user.cpp \
    src/mainSDL.cpp \
    src/playerstate.cpp \
    src/sdluser.cpp \
    src/videofilterthread.cpp \
    src/videooutput.cpp \
    videocontext.cpp

HEADERS += \
    mainwindow.h \
    src/audiofilterthread.h \
    src/audiooutput_v1.h \
    src/avframebak.h \
    src/avframequeue.h \
    src/avpacketqueue.h \
    src/avsyncclocker.h \
    src/decodethread.h \
    src/demuxthread.h \
    src/libass_user.h \
    src/mainSDL.h \
    src/playerstate.h \
    src/queue.h \
    src/sdluser.h \
    src/thread.h \
    src/videofilterthread.h \
    src/videooutput.h \
    videocontext.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


win32 {
INCLUDEPATH +=$$PWD/ffmpeg/include
LIBS += $$PWD/ffmpeg/lib/avformat.lib \
              $$PWD/ffmpeg/lib/avcodec.lib \
              $$PWD/ffmpeg/lib/avdevice.lib \
              $$PWD/ffmpeg/lib/avfilter.lib \
              $$PWD/ffmpeg/lib/avutil.lib \
              $$PWD/ffmpeg/lib/postproc.lib \
              $$PWD/ffmpeg/lib/swresample.lib \
              $$PWD/ffmpeg/lib/swscale.lib

INCLUDEPATH +=$$PWD/SDL2/include
LIBS += $$PWD/SDL2/lib/x64/SDL2.lib
INCLUDEPATH +=$$PWD/SDL_ttf/include
LIBS += $$PWD/SDL_ttf/lib/x64/SDL2_ttf.lib

INCLUDEPATH +=$$PWD/libass/include
LIBS += $$PWD/libass/lib/x64/ass.lib
}



