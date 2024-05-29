#include"ringBuffer.h"

#include<stdlib.h>
#include<string.h>


RingBuffer::RingBuffer()
{
    buffer=nullptr;
    bufferSize=0;
    write=0;
    read=0;
}

RingBuffer::~RingBuffer()
{
    freeBuffer();
}

bool RingBuffer::initBuffer(uint32_t size)
{
    //需要保证为2的次幂 取余运算转换为与运算 提升效率，即write%bufferSize
    if(!is_power_of_two(size)){
        if(size<2){
            size=2;
        }
        //向上取2的次幂
        int i=0;
        for(;size!=0;++i)
            size/=2;
        size=1U<<i;
    }
    std::lock_guard<std::mutex> lock(mutex);
    buffer=new uint8_t[size];
    if(buffer==nullptr)
        return false;

    memset(buffer,0,size);
    bufferSize=size;
    write=0;
    read=0;
    return true;
}

void RingBuffer::freeBuffer()
{
    std::lock_guard<std::mutex> lock(mutex);
    bufferSize=0;
    write=0;
    read=0;
    if(buffer!=nullptr){
        delete[] buffer;
        buffer=nullptr;
    }
}

void RingBuffer::resetBuffer()
{
    std::lock_guard<std::mutex> lock(mutex);
    write=0;
    read=0;
    memset(buffer,0,bufferSize);
}

bool RingBuffer::isEmpty()
{
    std::lock_guard<std::mutex> lock(mutex);
    return write==read;
}

bool RingBuffer::isFull()
{
    std::lock_guard<std::mutex> lock(mutex);
    return bufferSize==(write-read);
}

uint32_t RingBuffer::getReadableLen()
{
    std::lock_guard<std::mutex> lock(mutex);
    return write-read;
}

uint32_t RingBuffer::getRemainLen()
{
    std::lock_guard<std::mutex> lock(mutex);
    return bufferSize-(write-read);
}

uint32_t RingBuffer::getBufferSize()
{
    std::lock_guard<std::mutex> lock(mutex);
    return bufferSize;
}

uint32_t RingBuffer::writeBuffer(char *inBuf, uint32_t inSize)
{
    std::lock_guard<std::mutex> lock(mutex);
    if(buffer==nullptr||inBuf==nullptr||inSize==0){
        return -1;
    }

    //写入数据大小和缓冲区剩余空间大小 取最小值为最终写入大小
    inSize=Min(inSize,bufferSize-(write-read));
    //写数据如果写到末尾仍然是未写完的情况，那么回到头部继续写
    uint32_t len=Min(inSize,bufferSize-(write&(bufferSize-1)));
    //区间未写指针位置到缓冲区末端
    memcpy(buffer+(write&(bufferSize-1)),inBuf,len);
    //回到缓冲区头部继续写剩余数据
    memcpy(buffer,inBuf+len,inSize-len);

    //无符号溢出则为0
    write+=inSize;
    return inSize;

}

uint32_t RingBuffer::readBuffer(char *outBuf, uint32_t outSize)
{
    std::lock_guard<std::mutex> lock(mutex);

    if(buffer==nullptr||outBuf==nullptr||outSize==0)
        return -1;

    //读出数据大小和缓冲区刻度数据大小 取最小值作为最终读出大小
    outSize=Min(outSize,write-read);

    //读数据如果读到末尾仍未读完的情况，那么回到头部继续读
    uint32_t len=Min(outSize,bufferSize-(read&(bufferSize-1)));
    //区间为读指针位置到缓冲区末端
    memcpy(outBuf,buffer+(read&(bufferSize-1)),len);
    //回到缓冲区同步继续读剩余数据
    memcpy(outBuf+len,buffer,outSize-len);

    //无符号溢出为0
    read+=outSize;
    return outSize;
}
