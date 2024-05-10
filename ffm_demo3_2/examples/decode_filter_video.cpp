/*https://ffmpeg.org/doxygen/trunk/decode_filter_video_8c-example.html
 * Copyright (c) 2010 Nicolas George
 * Copyright (c) 2011 Stefano Sabatini
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * API example for decoding and filtering
 * @example decode_filter_video.c
 */

#define _XOPEN_SOURCE 600 // for usleep
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>

extern "C"{
#include"libavformat/avformat.h"
#include"libavcodec/avcodec.h"
#include"libavfilter/avfilter.h"
#include"libavutil/avutil.h"
#include"libavfilter/buffersink.h"
#include"libavfilter/buffersrc.h"
}

const char* filter_descr="scale=78:24,transpose=cclock";
/*other way:
 * scale=78:24 [scl]; [scl] transpose=cclock
 * // assumes "[in]" and "[out]" to be input outputs pads respectively
*/

static AVFormatContext* fmt_ctx;
static AVCodecContext* dec_ctx;
AVFilterContext* buffersink_ctx;
AVFilterContext* buffersrc_ctx;
AVFilterGraph* filter_graph;
static int video_stream_index=-1;
static int64_t last_pts=AV_NOPTS_VALUE;

static int open_input_file(const char* filename)
{
    const AVCodec* dec;
    int ret;

    if((ret=avformat_open_input(&fmt_ctx,filename,NULL,NULL))<0){
        fprintf(stderr,"cant open file[%s]\n",filename);
        return ret;
    }

    if((ret=avformat_find_stream_info(fmt_ctx,NULL))<0){
        fprintf(stderr,"cant find stream info\n");
        return ret;
    }

    // select the video stream
    if((ret=av_find_best_stream(fmt_ctx,AVMEDIA_TYPE_VIDEO,-1,-1,&dec,0))<0){
        fprintf(stderr,"cant select video stream\n");
        return ret;
    }
    video_stream_index=ret;

    //create decoding context
    if(!(dec_ctx=avcodec_alloc_context3(dec))){
        fprintf(stderr,"cant allocate decoder context \n");
        return AVERROR(ENOMEM);
    }
    avcodec_parameters_to_context(dec_ctx,fmt_ctx->streams[video_stream_index]->codecpar);
    // init the video decoder
    if((ret=avcodec_open2(dec_ctx,dec,NULL))<0){
        fprintf(stderr,"Cannot open video decoder\n");
        return ret;
    }

    return 0;
}

static int init_filters(const char* filters_descr)
{

    char args[512];
    int ret=0;
    const AVFilter* buffersrc=avfilter_get_by_name("buffer");
    const AVFilter* buffersink=avfilter_get_by_name("buffersink");
    AVFilterInOut* outputs=avfilter_inout_alloc();
    AVFilterInOut* inputs=avfilter_inout_alloc();
    AVRational time_base=fmt_ctx->streams[video_stream_index]->time_base;
    enum AVPixelFormat pix_fmts[]={AV_PIX_FMT_GRAY8,AV_PIX_FMT_NONE};

    filter_graph=avfilter_graph_alloc();
    if(!outputs||!inputs||!filter_graph){
        ret=AVERROR(ENOMEM);
        goto end;
    }

    // buffer video source: the decoded frames from the decoder will be inserted here
    snprintf(args,sizeof(args),
        "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             dec_ctx->width,dec_ctx->height,dec_ctx->pix_fmt,
             time_base.num,time_base.den,
             dec_ctx->sample_aspect_ratio.num,dec_ctx->sample_aspect_ratio.den);

    ret=avfilter_graph_create_filter(&buffersrc_ctx,buffersrc,"in",
                                       args,NULL,filter_graph);
    if(ret<0){
        fprintf(stderr,"init_filters: buffersrc_ctx create failed\n");
        goto end;
    }

    // buffer video sink: to terminate the filter chain
    ret=avfilter_graph_create_filter(&buffersink_ctx,buffersink,"out",
                                       NULL,NULL,filter_graph);
    if(ret<0){
        fprintf(stderr,"init_filters: buffersink_ctx create failed\n");
        goto end;
    }

    /* Set the endpoints for the filter graph. The filter_graph will
     * be linked to the graph described by filters_descr.
    */

    /* The buffer source output must be connected to the input pad of
     *  the first filter described by filters_descr; since the first
     *  filter input label is not specified, it is set to "in" by default
    */
    outputs->name=av_strdup("in");
    outputs->filter_ctx=buffersrc_ctx;
    outputs->pad_idx=0;
    outputs->next=NULL;
    /* The buffer sink input must be connected to the output pad of
     *  the last filter described by filters_descr; since the last
     *  filter output label is not specified, it is set to "out" by default
    */
    inputs->name=av_strdup("out");
    inputs->filter_ctx=buffersink_ctx;
    inputs->pad_idx=0;
    inputs->next=NULL;

    ret=avfilter_graph_parse_ptr(filter_graph,filters_descr,&inputs,&outputs,NULL);
    if(ret<0){
        goto end;
    }
    ret=avfilter_graph_config(filter_graph,NULL);
    if(ret<0){
        goto end;
    }

end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return -1;
}

static void display_frame(const AVFrame* frame,AVRational time_base)
{
    int x,y;
    uint8_t *p0,*p;
    int64_t delay;

    if(frame->pts!=AV_NOPTS_VALUE){
        if(last_pts!=AV_NOPTS_VALUE){
            // sleep roughly the right amount of time;
            // usleep is in microseconds, just like AV_TIME_BASE
            delay=av_rescale_q(frame->pts-last_pts,time_base,AV_TIME_BASE_Q);
            if(delay>0&&delay<1000000)
                usleep(delay);
        }
        last_pts=frame->pts;
    }

    // Trivial ASCII grayscale display
    p0=frame->data[0];
    puts("\033c");
    for(y=0;y<frame->height;++y){
        p=p0;
        for(x=0;x<frame->width;++x){
            putchar(" .-+#"[*(p++) / 52]);
        }
        putchar('\n');
        p0+=frame->linesize[0];
    }
    fflush(stdout);
}

int main(int argc,char** argv)
{
    int ret;
    AVPacket* packet;
    AVFrame* frame;
    AVFrame* filt_frame;

    if(argc!=2){
        fprintf(stderr,"Usage : %s file\n",argv[0]);
        return 1;
    }

    frame=av_frame_alloc();
    filt_frame=av_frame_alloc();
    packet=av_packet_alloc();
    if(!frame||!filt_frame||!packet){
        fprintf(stderr,"frame/filt_frame/packet alloc failed\n");
        return 1;
    }

    if((ret=open_input_file(argv[1]))<0){
        goto end;
    }
    if((ret=init_filters(filter_descr))<0){
        goto end;
    }

    // read all packets
    for(;;){
        if((ret=av_read_frame(fmt_ctx,packet))<0)
            break;
        if(packet->stream_index==video_stream_index){
            ret=avcodec_send_packet(dec_ctx,packet);
            if(ret<0){
                fprintf(stderr,"avocdec_send_packet failed\n");
                break;
            }
            while(ret>=0){
                ret=avcodec_receive_frame(dec_ctx,frame);
                if(ret==AVERROR(EAGAIN)||ret==AVERROR_EOF){
                    //frame receive finished
                    break;
                }else if(ret<0){
                    goto end;
                }
                frame->pts=frame->best_effort_timestamp;
                // push the decoded frame into the filtergraph
                if(av_buffersrc_add_frame_flags(buffersrc_ctx,frame,AV_BUFFERSRC_FLAG_KEEP_REF)<0){
                    fprintf(stderr,"failed to feed the frame into the buffersrc\n");
                    break;
                }
                // pull the filtered frame from the filtergraph
                for(;;){
                    ret=av_buffersink_get_frame(buffersink_ctx,filt_frame);
                    if(ret==AVERROR(EAGAIN)||ret==AVERROR_EOF){
                        break;
                    }
                    if(ret<0)
                        goto end;
                    display_frame(filt_frame,buffersink_ctx->inputs[0]->time_base);
                    av_frame_unref(filt_frame);
                }
                av_frame_unref(frame);
            }
        }
        av_packet_unref(packet);
    }

    if(ret==AVERROR_EOF){
        // signal EOF to the filtergraph
        if(av_buffersrc_add_frame_flags(buffersrc_ctx,NULL,0)<0){
            fprintf(stderr,"Error while close the filtergraph\n");
            goto end;
        }
        //pull remaining frames from the filtergraph
        for(;;){
            ret=av_buffersink_get_frame(buffersink_ctx,filt_frame);
            if(ret==AVERROR(EAGAIN)||ret==AVERROR_EOF){
                break;
            }
            if(ret<0){
                goto end;
            }
            display_frame(filt_frame,buffersink_ctx->inputs[0]->time_base);
            av_frame_unref(filt_frame);
        }
    }

end:
    avfilter_graph_free(&filter_graph);
    avcodec_free_context(&dec_ctx);
    av_frame_free(&frame);
    av_frame_free(&filt_frame);
    av_packet_free(&packet);

    if(ret<0&&ret!=AVERROR_EOF){
        fprintf(stderr,"Error occured: %s\n",av_err2str(ret));
        return 1;
    }
    return 0;
}








