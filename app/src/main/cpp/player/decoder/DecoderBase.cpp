//
// Created by pcl on 2021/5/19.
//

#include "DecoderBase.h"
#include "LogUtil.h"

void DecoderBase::Start()
{
    if(m_Thread == nullptr)
    {
        StartDecodingThread();
    }
    else
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_DecoderState = STATE_DECODING;
        m_Cond.notify_all();
    }
}

void DecoderBase::Pause()
{
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_DecoderState = STATE_PAUSE;
}

void DecoderBase::Stop()
{
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_DecoderState = STATE_STOP;
    m_Cond.notify_all();
}

void DecoderBase::SeekToPosition(float position)
{
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_SeekPosition = position;
    m_DecoderState = STATE_DECODING;
    m_Cond.notify_all();
}

float DecoderBase::GetCurrentPosition()
{
    return  m_CurTimeStamp;
}

int DecoderBase::Init(const char *url, AVMediaType mediaType)
{
    strcpy(m_Url,url);
    m_MediaType = mediaType;
    return 0;
}

void DecoderBase::UnInit()
{
    if(m_Thread)
    {
        Stop();
        m_Thread->join();
        delete m_Thread;
        m_Thread = nullptr;
    }
}

int DecoderBase::InitFFDecoder()
{
    int result = -1;
    do{
        m_AVFormatContext = avformat_alloc_context();

        if(avformat_open_input(&m_AVFormatContext,m_Url,NULL,NULL) != 0)
        {
            LOGCATE("DecoderBase::InitFFDecoder avformat_open_input fail.");
            break;
        }

        if(avformat_find_stream_info(m_AVFormatContext,NULL) < 0)
        {
            LOGCATE("DecoderBase::InitFFDecoder avformat_find_stream_info fail.");
            break;
        }

        for (int i = 0; i < m_AVFormatContext->nb_streams; ++i)
        {
            if(m_AVFormatContext->streams[i]->codecpar->codec_type == m_MediaType)
            {
                m_StreamIndex = i;
                break;
            }
        }

        if(m_StreamIndex == -1)
        {
            LOGCATE("DecoderBase::InitFFDecoder Fail to find stream index.");
            break;
        }

        AVCodecParameters* codecParameters = m_AVFormatContext->streams[m_StreamIndex]->codecpar;

        m_AVCodec = avcodec_find_decoder(codecParameters->codec_id);
        if(m_AVCodec == nullptr)
        {
            LOGCATE("DecoderBase::InitFFDecoder avcodec_find_decoder fail.");
            break;
        }

        m_AVCodecContext = avcodec_alloc_context3(m_AVCodec);
        if(avcodec_parameters_to_context(m_AVCodecContext,codecParameters) != 0)
        {
            LOGCATE("DecoderBase::InitFFDecoder avcodec_parameters_to_context fail.");
            break;
        }

        AVDictionary *pAVDictionary = nullptr;
        av_dict_set(&pAVDictionary, "buffer_size", "1024000", 0);
        av_dict_set(&pAVDictionary, "stimeout", "20000000", 0);
        av_dict_set(&pAVDictionary, "max_delay", "30000000", 0);
        av_dict_set(&pAVDictionary, "rtsp_transport", "tcp", 0);

        result = avcodec_open2(m_AVCodecContext,m_AVCodec,&pAVDictionary);
        if(result < 0)
        {
            LOGCATE("DecoderBase::InitFFDecoder avcodec_open2 fail. result=%d", result);
            break;
        }

        result = 0;

        m_Duration = m_AVFormatContext->duration / AV_TIME_BASE * 1000; //us to ms

        m_Packet = av_packet_alloc();
        m_Frame = av_frame_alloc();

    }while (false);

    if(result != 0 && m_MsgContext && m_MsgCallback)
        m_MsgCallback(m_MsgContext,MSG_DECODER_INIT_ERROR,0);

    return result;
}

void DecoderBase::UnInitDecoder()
{
    LOGCATE("DecoderBase::UnInitDecoder");
    if(m_Frame != nullptr)
    {
        av_frame_free(&m_Frame);
        m_Frame = nullptr;
    }

    if(m_Packet != nullptr)
    {
        av_packet_free(&m_Packet);
        m_Packet = nullptr;
    }

    if(m_AVCodecContext != nullptr)
    {
        avcodec_close(m_AVCodecContext);
        avcodec_free_context(&m_AVCodecContext);
        m_AVCodecContext = nullptr;
        m_AVCodec = nullptr;
    }

    if(m_AVFormatContext != nullptr)
    {
        avformat_close_input(&m_AVFormatContext);
        avformat_free_context(m_AVFormatContext);
        m_AVFormatContext = nullptr;
    }

}

void DecoderBase::StartDecodingThread()
{
    m_Thread = new thread(DoAVDecoding,this);
}

void DecoderBase::DecodingLoop()
{
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_DecoderState = STATE_DECODING;
        lock.unlock();
    }

    for(;;)
    {
        while (m_DecoderState == STATE_PAUSE)
        {
            std::unique_lock<std::mutex> lock(m_Mutex);
            LOGCATE("DecoderBase::DecodingLoop waiting, m_MediaType=%d", m_MediaType);
            m_Cond.wait_for(lock, std::chrono::milliseconds(10));
            m_StartTimeStamp = GetSysCurrentTime() - m_CurTimeStamp;
        }

        if(m_DecoderState == STATE_STOP)
        {
            break;
        }

        if(m_StartTimeStamp == -1)
        {
            m_StartTimeStamp = GetSysCurrentTime();
        }

        if(DecodeOnePacket() != 0)
        {
            //??????????????????????????????
            std::unique_lock<std::mutex> lock(m_Mutex);
            m_DecoderState = STATE_PAUSE;
        }
    }
}

void DecoderBase::UpdateTimeStamp()
{
    std::unique_lock<std::mutex> lock(m_Mutex);
    if(m_Frame->pkt_dts != AV_NOPTS_VALUE)
    {
        m_CurTimeStamp = m_Frame->pkt_dts;
    }
    else if(m_Frame->pts != AV_NOPTS_VALUE)
    {
        m_CurTimeStamp = m_Frame->pts;
    }
    else
    {
        m_CurTimeStamp = 0;
    }

    m_CurTimeStamp = (int64_t)((m_CurTimeStamp * av_q2d(m_AVFormatContext->streams[m_StreamIndex]->time_base)) * 1000);

    if(m_SeekPosition > 0 && m_SeekSuccess)
    {
        m_StartTimeStamp = GetSysCurrentTime() - m_CurTimeStamp;
        m_SeekPosition = 0;
        m_SeekSuccess = false;
    }
}

long DecoderBase::AVSync()
{
    long curSysTime = GetSysCurrentTime();

    //??????????????????????????????????????????????????????
    long elapsedTime = curSysTime - m_StartTimeStamp;

    if(m_MsgContext && m_MsgCallback && m_MediaType == AVMEDIA_TYPE_AUDIO)
        m_MsgCallback(m_MsgContext,MSG_DECODING_TIME,m_CurTimeStamp * 1.0f / 1000);

    long delay = 0;

    //?????????????????????
    if(m_CurTimeStamp > elapsedTime) {
        //????????????
        auto sleepTime = static_cast<unsigned int>(m_CurTimeStamp - elapsedTime);//ms
        //??????????????????????????????
        sleepTime = sleepTime > DELAY_THRESHOLD ? DELAY_THRESHOLD :  sleepTime;
        av_usleep(sleepTime * 1000);
    }
    delay = elapsedTime - m_CurTimeStamp;


//    if(m_AVSyncCallback != nullptr && m_SeekPosition == 0) {
//        //?????????????????????,???????????????????????????,??????????????? m_AVSyncCallback ??????
//        long elapsedTime = m_AVSyncCallback(m_AVDecoderContext);
//        LOGCATE("DecoderBase::AVSync m_CurTimeStamp=%ld, elapsedTime=%ld", m_CurTimeStamp, elapsedTime);
//
//        if(m_CurTimeStamp > elapsedTime) {
//            //????????????
//            auto sleepTime = static_cast<unsigned int>(m_CurTimeStamp - elapsedTime);//ms
//            //??????????????????????????????
//            sleepTime = sleepTime > DELAY_THRESHOLD ? DELAY_THRESHOLD :  sleepTime;
//            av_usleep(sleepTime * 1000);
//        }
//        delay = elapsedTime - m_CurTimeStamp;
//    }

    return delay;
}

int DecoderBase::DecodeOnePacket() {
    LOGCATE("DecoderBase::DecodeOnePacket m_MediaType=%d", m_MediaType);
    if(m_SeekPosition > 0) {
        //seek to frame
        int64_t seek_target = static_cast<int64_t>(m_SeekPosition * 1000000);//??????
        int64_t seek_min = INT64_MIN;
        int64_t seek_max = INT64_MAX;
        int seek_ret = avformat_seek_file(m_AVFormatContext, -1, seek_min, seek_target, seek_max, 0);
        if (seek_ret < 0) {
            m_SeekSuccess = false;
            LOGCATE("BaseDecoder::DecodeOneFrame error while seeking m_MediaType=%d", m_MediaType);
        } else {
            if (-1 != m_StreamIndex) {
                avcodec_flush_buffers(m_AVCodecContext);
            }
            ClearCache();
            m_SeekSuccess = true;
            LOGCATE("BaseDecoder::DecodeOneFrame seekFrame pos=%f, m_MediaType=%d", m_SeekPosition, m_MediaType);
        }
    }
    int result = av_read_frame(m_AVFormatContext, m_Packet);
    while(result == 0) {
        if(m_Packet->stream_index == m_StreamIndex) {
//            UpdateTimeStamp(m_Packet);
//            if(AVSync() > DELAY_THRESHOLD && m_CurTimeStamp > DELAY_THRESHOLD)
//            {
//                result = 0;
//                goto __EXIT;
//            }

            if(avcodec_send_packet(m_AVCodecContext, m_Packet) == AVERROR_EOF) {
                //????????????
                result = -1;
                goto __EXIT;
            }

            //?????? packet ???????????? frame?
            int frameCount = 0;
            while (avcodec_receive_frame(m_AVCodecContext, m_Frame) == 0) {
                //???????????????
                UpdateTimeStamp();
                //??????
                AVSync();
                //??????
                LOGCATE("DecoderBase::DecodeOnePacket 000 m_MediaType=%d", m_MediaType);
                OnFrameAvailable(m_Frame);
                LOGCATE("DecoderBase::DecodeOnePacket 0001 m_MediaType=%d", m_MediaType);
                frameCount ++;
            }
            LOGCATE("BaseDecoder::DecodeOneFrame frameCount=%d", frameCount);
            //???????????? packet ??????????????????
            if(frameCount > 0) {
                result = 0;
                goto __EXIT;
            }
        }
        av_packet_unref(m_Packet);
        result = av_read_frame(m_AVFormatContext, m_Packet);
    }

__EXIT:
    av_packet_unref(m_Packet);
    return result;
}

void DecoderBase::DoAVDecoding(DecoderBase *decoder)
{
    do{
        if(decoder->InitFFDecoder() != 0)
        {
            break;
        }

        decoder->OnDecoderReady();
        decoder->DecodingLoop();
    }while (false);


    decoder->UnInitDecoder();
    decoder->OnDecoderDone();
}