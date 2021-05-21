//
// Created by pcl on 2021/5/19.
//

#ifndef FFMPEGEXERCISE_DECODER_H
#define FFMPEGEXERCISE_DECODER_H

typedef void (*MessageCallback)(void*,int, float);
typedef long (*AVSyncCallback)(void*);

class Decoder{
public:
    virtual void Start() = 0;
    virtual void Pause() = 0;
    virtual void Stop() = 0;
    virtual float GetDuration() = 0;
    virtual void SeekToPosition(float position) = 0;
    virtual float GetCurrentPosition() = 0;
    virtual void SetMessageCallback(void* context, MessageCallback callback) = 0;
};

#endif //FFMPEGEXERCISE_DECODER_H
