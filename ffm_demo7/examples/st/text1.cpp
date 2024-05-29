/* https://blog.csdn.net/u012983289/article/details/108319426

*/

#include<iostream>
#include<stdlib.h>
#include"stdint.h"

extern "C"{
#include"libavcodec/avcodec.h"
#include"libavformat/avformat.h"
#include"libavutil/avutil.h"
#include"SoundTouch.h"
#include"SoundTouchDLL.h"
}

typedef float SAMPLETYPE;
#define BUFF_SIZE 6720

static char* convBuff=nullptr;
int convBuffSize=0;

void* getConvBuffer(int sizeBytes)
{
    if(convBuffSize<sizeBytes){
        delete[] convBuff;

        convBuffSize=(sizeBytes+15)&(-8);
        convBuff=new char[convBuffSize];
    }
    return convBuff;
}

static inline short _swap16(short&wData)
{
    return wData;
}

int readHandle(float*buffer,int maxElems,FILE*fptr)
{
    int bytesPerSample=2;
    int numBytes=maxElems*bytesPerSample;
    char* temp=(char*)getConvBuffer(numBytes);
    numBytes=(int)fread(temp,1,numBytes,fptr);

    int numElems=numBytes/bytesPerSample;
    short*temp2=(short*)temp;
    double conv=1.0/32768.0;
    for(int i=0;i<numElems;++i){
        short value=temp2[i];
        buffer[i]=(float)(_swap16(value)*conv);
    }
    return numElems;
}

inline int saturate(float fvalue,float minval,float maxval)
{
    fvalue=std::min(maxval,fvalue);
    fvalue=std::max(minval,fvalue);

    return (int)fvalue;
}

void writeHandle(const float *buffer,int numElems,FILE* fptr)
{
    int numBytes;
    int bytePerSample=2;

    if(numElems<=0)
        return;

    numBytes=bytePerSample*numElems;
    // round bit up to aviod buffer overrun with 24bit-value assignment
    void* temp=getConvBuffer(numBytes+7);

    short* temp2=(short*)temp;
    for(int i=0;i<numElems;++i){
        short value=(short)saturate(buffer[i]*32768.0f,-32768.0f,32767.0f);
        temp2[i]=_swap16(value);
    }

    int res=(int)fwrite(temp,1,numBytes,fptr);
    if(res!=numBytes){
        fprintf(stderr,"Error write raw file.\n");
        exit(EXIT_FAILURE);
    }

}

void writeWavHeader(AVCodecContext* pCodecCtx,AVFormatContext* pFormatCtx,FILE* audioFile)
{
    // wav文件有44字节的wav头
    int8_t* data;
    uint32_t long_temp;
    uint16_t short_temp;
    uint16_t block_align;
    int bits=8;
    uint64_t fileSize;
    uint64_t audioDataSize;

    switch (pCodecCtx->sample_fmt)
    {
    case AV_SAMPLE_FMT_S16:
        bits = 16;
        break;
    case AV_SAMPLE_FMT_S32:
        bits = 32;
        break;
    case AV_SAMPLE_FMT_U8:
        bits = 8;
        break;
    default:
        bits = 16;
        break;
    }
    audioDataSize=(pFormatCtx->duration)*(bits/8)*(pCodecCtx->sample_rate)*(pCodecCtx->channels);
    fileSize=audioDataSize+36;
    data=(int8_t*)"RIFF";
    fwrite(data,sizeof(char),4,audioFile);
    fwrite(&fileSize,sizeof(int32_t),1,audioFile);
    data=(int8_t*)"WAVE";
    fwrite(data,sizeof(char),4,audioFile);
    data=(int8_t*)"fmt ";
    fwrite(data,sizeof(char),4,audioFile);
    long_temp=16;//子块大小
    fwrite(&long_temp,sizeof(int32_t),1,audioFile);
    short_temp=0x01;//音频格式
    fwrite(&short_temp,sizeof(int16_t),1,audioFile);
    short_temp=pCodecCtx->channels;
    fwrite(&short_temp,sizeof(int16_t),1,audioFile);
    long_temp=pCodecCtx->sample_rate;
    fwrite(&long_temp,sizeof(int32_t),1,audioFile);
    long_temp=(bits/8)*(pCodecCtx->channels);
    fwrite(&long_temp,sizeof(int32_t),1,audioFile);
    block_align=(bits/8)*(pCodecCtx->channels);
    fwrite(&block_align,sizeof(int16_t),1,audioFile);
    short_temp=bits;
    fwrite(&short_temp,sizeof(int16_t),1,audioFile);
    data=(int8_t*)"data";
    fwrite(data,sizeof(char),4,audioFile);
    fwrite(&audioDataSize,sizeof(int32_t),1,audioFile);


}








using namespace std;

int main()
{
    std::string filename ="c:\\a.wav";
    AVCodecContext* pCodecCtx;

    AVFormatContext* ifmt_ctx=NULL;
    int AudioStreamIndex=-1;
    int ret=-1;
    if((ret=avformat_open_input(&ifmt_ctx,filename.c_str(),NULL,NULL))<0){
        printf("cannot open input\n");
        return 0;
    }
    if((ret=avformat_find_stream_info(ifmt_ctx,NULL))<0){
        printf("cannot find input stream info\n");
        return 0;
    }
    // open the decoder
    for(int i=0;i<(int)ifmt_ctx->nb_streams;++i){
        if(ifmt_ctx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO){
            AudioStreamIndex=i;
            pCodecCtx->codec= avcodec_find_decoder(ifmt_ctx->streams[AudioStreamIndex]->codecpar->codec_id);

        }
    }

    float rateDelta=0.0;//变调变速
    float pitchDelta=5.0;//变调不变速
    float tempoDelta=0.0;//变速不变调+（加速 时长变短） -（减速 时长变长）
    HANDLE handle=soundtouch_createInstance();

    soundtouch_setSampleRate(handle,pCodecCtx->sample_rate);//输入采样率
    soundtouch_setChannels(handle,pCodecCtx->channels);//输入通道数
    soundtouch_setRateChange(handle,rateDelta);
    soundtouch_setPitchSemiTones(handle,pitchDelta);
    soundtouch_setTempoChange(handle,tempoDelta);
    std::string strOutFileNameAdd;

    if (rateDelta != 0.0)
    {
        if (rateDelta > 0.0)
            strOutFileNameAdd += "_rate+" + std::to_string(abs((int)rateDelta));
        else
            strOutFileNameAdd += "_rate-" + to_string(abs((int)rateDelta));
    }
    if (pitchDelta != 0)
    {
        if (pitchDelta > 0)
            strOutFileNameAdd += "_pitch+" + to_string(abs((int)pitchDelta));
        else
            strOutFileNameAdd += "_pitch-" + to_string((int)pitchDelta);
    }
    if (tempoDelta != 0)
    {
        if (tempoDelta > 0)
            strOutFileNameAdd += "_tempo+" + to_string(abs((int)tempoDelta));
        else
            strOutFileNameAdd += "_tempo-" + to_string(abs((int)tempoDelta));
    }


    FILE *pOut = NULL;
    char tmpName[100];
    sprintf_s(tmpName, "%s_%d_%dchannel_%s.wav", filename.c_str(), ifmt_ctx->streams[AudioStreamIndex]->codec->sample_rate, ifmt_ctx->streams[AudioStreamIndex]->codec->channels, strOutFileNameAdd.c_str());
    pOut = fopen(tmpName, "wb");
    if (AudioStreamIndex >= 0)
        pCodecCtx = ifmt_ctx->streams[AudioStreamIndex]->codec;

    //添加文件头
    writeWavHeader(pCodecCtx,ifmt_ctx,pOut);

    int nSamples=0;
    int nChannels=pCodecCtx->channels;
    int buffSizeSamples=BUFF_SIZE/nChannels;
    SAMPLETYPE sampleBuffer[BUFF_SIZE];

    FILE*fptr=fopen(filename.c_str(),"rb");
    fseek(fptr,44,SEEK_SET);//偏移44个字节，是因为wav的头部信息为44位
    while(feof(fptr)==0){
        int num=readHandle((float*)sampleBuffer,BUFF_SIZE,fptr);
        // Read a chunk of samples from the input file
        nSamples=num/nChannels;

        // Feed the samples into soundtouch process
        soundtouch_putSamples(handle,sampleBuffer,nSamples);

        do{
            nSamples=soundtouch_receiveSamples(handle,sampleBuffer,buffSizeSamples);
            writeHandle(sampleBuffer,nSamples*nChannels,pOut);
        }while(nSamples!=0);

    }




    soundtouch_flush(handle);
    do{
        nSamples=soundtouch_receiveSamples(handle,sampleBuffer,buffSizeSamples);
        writeHandle(sampleBuffer,nSamples*nChannels,pOut);
    }while(nSamples!=0);

    fclose(fptr);
    fclose(pOut);
    avcodec_close(pCodecCtx);
    avformat_close_input(&ifmt_ctx);
}


















