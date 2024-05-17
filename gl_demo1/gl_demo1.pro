TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main1.cpp \
        src/glad.c



INCLUDEPATH +=$$PWD/SDL2/include
LIBS += $$PWD/SDL2/lib/x64/SDL2.lib


INCLUDEPATH +=$$PWD/SDL_image/include
LIBS += $$PWD/SDL_image/lib/x64/SDL2_image.lib

INCLUDEPATH +=$$PWD/glm

HEADERS += \
    src/include/glad.h \
    src/include/shader.h





