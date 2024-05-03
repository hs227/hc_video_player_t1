TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt


SOURCES += \
        audiooutput.cpp \
        audiooutput_v1.cpp \
        avframequeue.cpp \
        avpacketqueue.cpp \
        decodethread.cpp \
        demuxthread.cpp \
        fmtaudioc.cpp \
        main.cpp \
        videooutput.cpp





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


HEADERS += \
    audiooutput.h \
    audiooutput_v1.h \
    avframequeue.h \
    avpacketqueue.h \
    decodethread.h \
    demuxthread.h \
    fmtaudioc.h \
    queue.h \
    thread.h \
    videooutput.h


