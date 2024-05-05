#include "subtitleoutput.h"

SubtitleOutput::SubtitleOutput(AVPacketQueue* pkt_q,AVRational time_base_,AvSyncClocker* sync_clocker_)
    :pkt_queue(pkt_q),time_base(time_base_),sync_clocker(sync_clocker_)
{

}

SubtitleOutput::~SubtitleOutput()
{

}




