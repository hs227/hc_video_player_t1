#include "avframequeue.h"

AVFrameQueue::AVFrameQueue()
{

}

AVFrameQueue::~AVFrameQueue()
{

}

void AVFrameQueue::Abort()
{
    Release();
    queue.Abort();
}

void AVFrameQueue::Release()
{
    for(;;){
        AVFrame* frame=nullptr;
        int res=queue.Pop(frame,1);
        if(res==-1){

        }
        else if(res==-2){
            //empty
            break;
        }else{
            av_frame_free(&frame);
        }
    }
}

int AVFrameQueue::Size()
{
    return queue.Size();
}

int AVFrameQueue::Push(AVFrame *val)
{
    //内存分配
    AVFrame* frame=av_frame_alloc();
    //移动
    av_frame_move_ref(frame,val);

    return queue.Push(frame);
}

int AVFrameQueue::Empty()
{
    int size=this->Size();
    return size==0;
}

AVFrame *AVFrameQueue::Pop(const int timeout)
{
    AVFrame* frame=nullptr;
    int res=queue.Pop(frame,timeout);
    if(res<0){
        return nullptr;
    }
    return frame;
}

AVFrame *AVFrameQueue::Front()
{
    AVFrame* frame=nullptr;
    int res=queue.Front(frame);
    if(res<0){
        return nullptr;
    }
    return frame;
}
