TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt


SOURCES += \
        audiofilterthread.cpp \
        audiooutput_v1.cpp \
        avframebak.cpp \
        avframequeue.cpp \
        avpacketqueue.cpp \
        avsyncclocker.cpp \
        decodethread.cpp \
        demuxthread.cpp \
        libass_user.cpp \
        main.cpp \
        playerstate.cpp \
        videofilterthread.cpp \
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
INCLUDEPATH +=$$PWD/SDL_image/include
LIBS += $$PWD/SDL_image/lib/x64/SDL2_image.lib
INCLUDEPATH +=$$PWD/SDL_ttf/include
LIBS += $$PWD/SDL_ttf/lib/x64/SDL2_ttf.lib

INCLUDEPATH +=$$PWD/libass/include
LIBS += $$PWD/libass/lib/x64/ass.lib




HEADERS += \
    audiofilterthread.h \
    audiooutput_v1.h \
    avframebak.h \
    avframequeue.h \
    avpacketqueue.h \
    avsyncclocker.h \
    decodethread.h \
    demuxthread.h \
    libass_user.h \
    playerstate.h \
    queue.h \
    thread.h \
    videofilterthread.h \
    videooutput.h


