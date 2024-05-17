#ifndef LOG_H
#define LOG_H

#include<stdio.h>
#include<time.h>
#include<stdarg.h>
#include<string>


// C++
static std::string getTimeString()
{
    time_t t=time(nullptr);
    char cTime[32];
    strftime(cTime,sizeof(cTime),"%Y-%m-%d %H:%S:%Z",localtime(&t));

    std::string sTime=cTime;
    return sTime;
}

//C
static char* getTimeChar()
{
    time_t t=time(nullptr);
    static char cTime[32];
    strftime(cTime,sizeof(cTime),"%Y-%m-%d %H:%S",localtime(&t));
    return cTime;
}

//提升兼容性 默认getTimeChar
#define INFO(format, ...)   fprintf(stdout, "%s [INFO] [%s:%d %s()] " format "\n", getTimeChar(), __FILE__, __LINE__, __func__ , ##__VA_ARGS__)
#define ERROR(format, ...)  fprintf(stderr, "%s [ERROR] [%s:%d %s()] " format "\n", getTimeChar(), __FILE__, __LINE__, __func__ , ##__VA_ARGS__)


enum LogLevel
{
    Debug=0,
    Info,
    Warn,
    Error,
    Fatal,
    LevelCount,
};

static const char* logLevel[LogLevel::LevelCount]={"Debug","Info","Warn","Error","Fatal"};


static void serialize(int level,const char* file,int line,const char* func,const char* format,...)
{
    time_t t=time(nullptr);
    char cTime[32]={0};
    strftime(cTime,sizeof(cTime),"%Y-%m-%d %H:%M:%S",localtime(&t));

    //前置信息
    char front[256]={0};
    snprintf(front,sizeof(front),"%s [%s] [%s:%d %s()]=>",cTime,logLevel[level],file,line,func);

    //具体信息
    va_list arglist;
    va_start(arglist,format);
    char buf[512]={0};
    vsnprintf(buf,sizeof(buf),format,arglist);
    printf("%d %d\n",front,buf);
    va_end(arglist);

}

#define LogInfo(format, ...)  serialize(LogLevel::Info, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LogError(format, ...) serialize(LogLevel::Error, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)











#endif // LOG_H
