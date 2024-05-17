#ifndef FIFOBUFFER_H
#define FIFOBUFFER_H

#include<list>
#include<mutex>
#include<string.h>

// 数据节点
struct DataNode
{
    DataNode()
    {
        begin=0;
        end=0;
    }
    //记录当前数据块的开始和结束位置
    int begin;
    int end;
};

//缓冲区类 实现队列存储数据块 只拷贝一次，提供给外部使用
class FifoBuffer
{
public:
    FifoBuffer();
    ~FifoBuffer();

    //初始缓冲区
    bool initFifoBuffer(int bufferLength);
    //释放缓冲区
    void freeFifoBuffer();
    //重置缓冲区（不需要重新申请内存）
    void resetFifoBuffer();

    //获取当前的数据块节点数量
    int getDataNodeSize();
    //获取缓冲区总长度
    int getFifoBufferLength();
    //获取缓冲区剩余空间大小
    int getRemainSpace();

    //数据写入缓冲区尾部 注：缓冲区满则写入失败
    bool pushData(char*data,int length);
    //返回缓冲区头部数据及长度供外部使用 需要popDelete释放数据块
    char* popData(int*length);
    //调用此函数删除头部数据块
    bool popDelete();

private:
    //线程安全锁
    std::mutex fifoMutex;

    //缓冲区对应变量
    char*fifoBuffer;
    int fifoBufferLength;
    int fifoBegin;
    int fifoEnd;

    //数据块节点
    std::list<DataNode> dataNodeList;

};

#endif // FIFOBUFFER_H
