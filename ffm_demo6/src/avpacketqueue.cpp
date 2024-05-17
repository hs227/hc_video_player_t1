#include "avpacketqueue.h"

AVPacketQueue::AVPacketQueue()
{

}

AVPacketQueue::~AVPacketQueue()
{
    Abort();
}

void AVPacketQueue::Abort()
{
    Release();
    queue.Abort();

}

void AVPacketQueue::Release()
{
    //释放queue
    for(;;){
        AVPacket* pkt=nullptr;
        int size=queue.Size();
        int res=queue.Pop(pkt);
        if(size==0){
            break;
        }
        if(res==-1){

        }
        else if(res==-2){
            //empty
            break;
        }else{
            av_packet_free(&pkt);
        }
    }
}

int AVPacketQueue::Size()
{
    return queue.Size();
}

int AVPacketQueue::Push(AVPacket *val)
{
    //内存分配
    AVPacket* pkt=av_packet_alloc();
    //移动
    av_packet_move_ref(pkt,val);

    return queue.Push(pkt);
}

AVPacket *AVPacketQueue::Pop(const int timeout)
{
    AVPacket* pkt=nullptr;
    int res=queue.Pop(pkt,timeout);
    if(res<0){
        return nullptr;
    }
    return pkt;
}



