#include "avframequeue.h"

AVFrameQueue::AVFrameQueue()
{

}

AVFrameQueue::~AVFrameQueue()
{
    Abort();
}

void AVFrameQueue::Abort()
{
    Release();
    queue.Abort();
}

void AVFrameQueue::Release()
{

    for(;;){
        MAVFrame* frame=nullptr;
        int size=queue.Size();
        int res=queue.Pop(frame,1);
        if(size==0){
            break;
        }
        if(res==-1){

        }
        else if(res==-2){
            //empty
            break;
        }else{
            av_frame_free(&frame->frame);
            delete frame;
        }
    }
}

int AVFrameQueue::Size()
{
    return queue.Size();
}

int AVFrameQueue::Push(MAVFrame *val)
{
    MAVFrame* frame=new MAVFrame();
    frame->serial=val->serial;
    frame->pos=val->pos;
    //内存分配
    frame->frame=av_frame_alloc();
    //移动
    av_frame_move_ref(frame->frame,val->frame);

    return queue.Push(frame);
}

int AVFrameQueue::Empty()
{
    int size=this->Size();
    return size==0;
}

MAVFrame*AVFrameQueue::Pop(const int timeout)
{
    MAVFrame* frame=nullptr;
    int res=queue.Pop(frame,timeout);
    if(res<0){
        return nullptr;
    }
    return frame;
}

MAVFrame*AVFrameQueue::Front()
{
    MAVFrame* frame=nullptr;
    int res=queue.Front(frame);
    if(res<0){
        return nullptr;
    }
    return frame;
}


