#include "fifobuffer.h"



FifoBuffer::FifoBuffer()
{
    fifoBuffer=nullptr;
    fifoBufferLength=0;
    fifoBegin=0;
    fifoEnd=0;
}

FifoBuffer::~FifoBuffer()
{
    freeFifoBuffer();
}

bool FifoBuffer::initFifoBuffer(int bufferLength)
{
    if(bufferLength<0)
        return false;

    std::lock_guard<std::mutex> lock(fifoMutex);
    if(fifoBuffer!=nullptr)
        return false;
    fifoBuffer=new char[bufferLength];
    if(fifoBuffer==nullptr)
        return false;
    fifoBufferLength=bufferLength;
    memset(fifoBuffer,0,fifoBufferLength);

    fifoBegin=0;
    fifoEnd=0;
    dataNodeList.clear();
    return true;
}

void FifoBuffer::freeFifoBuffer()
{
    std::lock_guard<std::mutex> lock(fifoMutex);
    fifoBegin=0;
    fifoEnd=0;
    fifoBufferLength=0;
    dataNodeList.clear();

    if(fifoBuffer!=nullptr)
    {
        delete[] fifoBuffer;
        fifoBuffer=NULL;
    }
}

void FifoBuffer::resetFifoBuffer()
{
    std::lock_guard<std::mutex> lock(fifoMutex);
    fifoBegin=0;
    fifoEnd=0;
    dataNodeList.clear();
    memset(fifoBuffer,0,fifoBufferLength);
}

int FifoBuffer::getDataNodeSize()
{
    std::lock_guard<std::mutex> lock(fifoMutex);
    return dataNodeList.size();
}

int FifoBuffer::getFifoBufferLength()
{
    std::lock_guard<std::mutex> lock(fifoMutex);
    return fifoBufferLength;
}

int FifoBuffer::getRemainSpace()
{
    std::lock_guard<std::mutex> lock(fifoMutex);
    if(fifoBufferLength<=0)
        return fifoBufferLength;

    if(dataNodeList.size()<=0)
        return fifoBufferLength;

    int listLengthCount=0;
    std::list<DataNode>::iterator it;
    for(it=dataNodeList.begin();it!=dataNodeList.end();++it)
    {
        DataNode node=*it;
        listLengthCount+=node.end-node.begin;
    }

    return fifoBufferLength-listLengthCount;
}

bool FifoBuffer::pushData(char *data, int length)
{
    std::lock_guard<std::mutex> lock(fifoMutex);

    //条件判断
    if(fifoBufferLength<=0||fifoBuffer==NULL||data==NULL||length<=0||length>fifoBufferLength)
        return false;

    //检测剩余的空间是否足够存储，不够则重头开始存储
    if(fifoBufferLength-fifoEnd<length)
        fifoEnd=0;

    if(dataNodeList.size()>0)
    {
        int nStart=0;
        DataNode nodeFront=dataNodeList.front();
        nStart=nodeFront.begin;
        if(fifoEnd==0)
        {
            //是否可以重头开始
            if(nStart<length)
                return false;
        }else{
            if(nStart>=fifoEnd)
            {
                //剩余空间不足
                if((nStart-fifoEnd)<length)
                    return false;
            }else{
                //剩余空间不足
                if(fifoBufferLength-fifoEnd<length)
                    return false;

            }
        }
    }
    // 记录新的数据块
    DataNode node;
    node.begin=fifoEnd;
    node.end=node.begin+length;
    dataNodeList.push_back(node);

    memcpy(fifoBuffer+fifoEnd,data,length);
    fifoEnd+=length;
    return true;
}

char *FifoBuffer::popData(int *length)
{
    std::lock_guard<std::mutex> lock(fifoMutex);

    char*buffer=nullptr;
    if(dataNodeList.size()<=0)
    {
        *length=0;
        return buffer;
    }

    DataNode node;
    node=dataNodeList.front();

    //为了稳定性再次判断换回的节点是否有效
    int offset=node.end-node.begin;
    if(node.begin>=0&&
        node.end>0&&
        node.begin<fifoBufferLength&&
        node.end<=fifoBufferLength&&
        offset>0&&
        offset<=fifoBufferLength)
    {
        buffer=fifoBuffer+node.begin;
        *length=offset;
        return buffer;
    }else{
        buffer=NULL;
        *length=0;
        return buffer;
    }
}

bool FifoBuffer::popDelete()
{
    std::lock_guard<std::mutex> lock(fifoMutex);

    if(dataNodeList.size()<=0)
        return false;

    //删除数据块记录即可，表示该数据块区域可被再次使用
    dataNodeList.pop_back();
    if(dataNodeList.size()<=0)
    {
        fifoBegin=0;
        fifoEnd=0;
    }
    return true;
}
