//
// Created by pcl on 2021/5/19.
//

#ifndef FFMPEGEXERCISE_DECODERBASE_H
#define FFMPEGEXERCISE_DECODERBASE_H

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/time.h>
};

#include <thread>
#include "Decoder.h"

#define MAX_PATH 2048
#define DELAY_THRESHOLD 100 //ms

using namespace std;

enum DecoderState{
    STATE_UNKNOWN,
    STATE_DECODING,
    STATE_PAUSE,
    STATE_STOP
};

enum DecoderMsg{
    MSG_DECODER_INIT_ERROR,
    MSG_DECODER_READY,
    MSG_DECODER_DONE,
    MSG_DECODER_RENDER,
    MSG_DECODING_TIME
};

class DecoderBase : public Decoder{
public:
    DecoderBase(){}
    virtual ~DecoderBase(){};

    virtual void Start();

    virtual void Pause();

    virtual void Stop();

    virtual float GetDuration()
    {
        return m_Duration * 1.0f / 1000;
    }
    virtual void SeekToPosition(float position);

    virtual float GetCurrentPosition();

    virtual void ClearCache()
    {

    }

    virtual void SetMessageCallback(void* context, MessageCallback callback)
    {
        m_MsgContext = context;
        m_MsgCallback = callback;
    }
    //设置音视频同步的回调
    virtual void SetAVSyncCallback(void* context, AVSyncCallback callback)
    {
        m_AVDecoderContext = context;
        m_AVSyncCallback = callback;
    }

protected:
    void* m_MsgContext = nullptr;
    MessageCallback m_MsgCallback = nullptr;

    virtual int Init(const char* url,AVMediaType mediaType);
    virtual void UnInit();

    virtual void OnDecoderReady() = 0;
    virtual void OnDecoderDone() = 0;

    virtual void OnFrameAvailable(AVFrame* frame) = 0;

    AVCodecContext *GetCodecContext(){
        return m_AVCodecContext;
    }

private:
    int InitFFDecoder();
    void UnInitDecoder();

    void StartDecodingThread();

    void DecodingLoop();

    void UpdateTimeStamp();

    long AVSync();

    int DecodeOnePacket();

    static void DoAVDecoding(DecoderBase* decoder);

    AVFormatContext* m_AVFormatContext = nullptr;

    AVCodecContext* m_AVCodecContext = nullptr;

    AVCodec* m_AVCodec = nullptr;

    AVPacket* m_Packet = nullptr;

    AVFrame* m_Frame = nullptr;

    AVMediaType m_MediaType = AVMEDIA_TYPE_UNKNOWN;

    char m_Url[MAX_PATH] = {0};

    long m_CurTimeStamp = 0;

    long m_StartTimeStamp = -1;

    long m_Duration = 0;

    int m_StreamIndex = -1;

    mutex m_Mutex;

    condition_variable m_Cond;
    thread* m_Thread = nullptr;

    volatile float      m_SeekPosition = 0;
    volatile bool       m_SeekSuccess = false;
    //解码器状态
    volatile int  m_DecoderState = STATE_UNKNOWN;
    void* m_AVDecoderContext = nullptr;
    AVSyncCallback m_AVSyncCallback = nullptr;//用作音视频同步

};


#endif //FFMPEGEXERCISE_DECODERBASE_H
