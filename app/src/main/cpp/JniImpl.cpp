//
// Created by pcl on 2021/5/19.
//

#include <cstdio>
#include <cstring>
#include <FFMediaPlayer.h>
#include <render/video/VideoGLRender.h>
#include <render/audio/OpenSLRender.h>
#include "util/LogUtil.h"
#include "jni.h"

extern "C" {
#include <libavcodec/version.h>
#include <libavcodec/avcodec.h>
#include <libavformat/version.h>
#include <libavutil/version.h>
#include <libavfilter/version.h>
#include <libswresample/version.h>
#include <libswscale/version.h>
};

#define NATIVE_CLASS_NAME "com/codefun/media/FFMediaPlayer"

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jlong JNICALL native_Init(JNIEnv *env, jobject obj, jstring jurl, jint renderType)
{
    const char* url = env->GetStringUTFChars(jurl, nullptr);
    FFMediaPlayer* player = new FFMediaPlayer();
    player->Init(env,obj, const_cast<char*>(url),renderType, nullptr);
    env->ReleaseStringUTFChars(jurl,url);

    return reinterpret_cast<jlong>(player);
}

JNIEXPORT void JNICALL native_Play(JNIEnv* env,jobject obj,jlong player_handle){
    if(player_handle != 0)
    {
        FFMediaPlayer* ffMediaPlayer = reinterpret_cast<FFMediaPlayer*>(player_handle);
        ffMediaPlayer->Play();
    }
}

JNIEXPORT void JNICALL native_SeekToPosition(JNIEnv* env,jobject obj,jlong player_handle,jfloat position)
{
    if(player_handle != 0)
    {
        FFMediaPlayer* ffMediaPlayer = reinterpret_cast<FFMediaPlayer*>(player_handle);
        ffMediaPlayer->SeekToPosition(position);
    }
}

JNIEXPORT void JNICALL native_Pause(JNIEnv* env,jobject obj,jlong player_handle)
{
    if(player_handle != 0)
    {
        FFMediaPlayer* ffMediaPlayer = reinterpret_cast<FFMediaPlayer*>(player_handle);
        ffMediaPlayer->Pause();
    }
}

JNIEXPORT void JNICALL native_Stop(JNIEnv* env,jobject obj,jlong player_handle)
{
    if(player_handle != 0)
    {
        FFMediaPlayer* ffMediaPlayer = reinterpret_cast<FFMediaPlayer*>(player_handle);
        ffMediaPlayer->Stop();
    }
}

JNIEXPORT void JNICALL native_UnInit(JNIEnv* env,jobject obj,jlong player_handle)
{
    if(player_handle != 0)
    {
        FFMediaPlayer* ffMediaPlayer = reinterpret_cast<FFMediaPlayer*>(player_handle);
        ffMediaPlayer->UnInit();
    }
}

JNIEXPORT jlong JNICALL native_GetMediaParams(JNIEnv* env,jobject obj,jlong player_handle,jint param_type)
{
    long value = 0;
    if(player_handle != 0)
    {
        FFMediaPlayer *ffMediaPlayer = reinterpret_cast<FFMediaPlayer *>(player_handle);
        value = ffMediaPlayer->GetMediaParams(param_type);
    }
    return value;
}

JNIEXPORT void JNICALL native_OnSurfaceCreated(JNIEnv* env,jclass clazz,jint render_type)
{
    VideoGLRender::GetInstance()->OnSurfaceCreated();
}

JNIEXPORT void JNICALL native_OnSurfaceChanged(JNIEnv* env,jclass clazz,jint rt,jint w,jint h)
{
    VideoGLRender::GetInstance()->OnSurfaceChanged(w,h);
}

JNIEXPORT void JNICALL native_OnDrawFrame(JNIEnv* env,jclass clazz,jint rt)
{
    VideoGLRender::GetInstance()->OnDrawFrame();
}

JNIEXPORT void JNICALL Test(JNIEnv* env,jclass clazz){

}

#ifdef __cplusplus
}
#endif

static JNINativeMethod g_NativeMethod[] = {
        {"native_Init",             "(Ljava/lang/String;I)J",        (void*)native_Init},
        {"native_Play",             "(J)V",                          (void*)native_Play},
        {"native_SeekToPosition",   "(JF)V",                         (void*)native_SeekToPosition},
        {"native_Pause",            "(J)V",                          (void*)native_Pause},
        {"native_Stop",             "(J)V",                          (void*)native_Stop},
        {"native_UnInit",           "(J)V",                          (void*)native_UnInit},
        {"native_GetMediaParams",   "(JI)J",                         (void*)native_GetMediaParams},
        {"native_OnSurfaceCreated", "(I)V",                          (void*)native_OnSurfaceCreated},
        {"native_OnSurfaceChanged", "(III)V",                        (void*)native_OnSurfaceChanged},
        {"native_OnDrawFrame",      "(I)V",                          (void*)native_OnDrawFrame}
};

static JNINativeMethod g_test[] = {
        {"native_Test","()V",(void*)Test}
};

static int RegisterNativeMethods(JNIEnv* env, const char* className,JNINativeMethod* methods,int methodNum)
{
    jclass clazz = env->FindClass(className);
    if(clazz == NULL)
    {
        LOGCATE("RegisterNativeMethods fail. clazz == NULL");
        return false;
    }

    if(env->RegisterNatives(clazz,methods,methodNum) < 0)
    {
        LOGCATE("RegisterNativeMethods fail");
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

static void UnregisterNativeMethods(JNIEnv* env, const char* className)
{
    jclass clazz = env->FindClass(className);
    if (clazz == NULL)
    {
        LOGCATE("UnregisterNativeMethods fail. clazz == NULL");
        return;
    }
    if (env != NULL)
    {
        env->UnregisterNatives(clazz);
    }
}

extern "C" jint JNI_OnLoad(JavaVM* jvm,void* p)
{
    jint jniRet = JNI_ERR;
    JNIEnv *env = NULL;
    if (jvm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK)
    {
        return jniRet;
    }

    jint regRet = RegisterNativeMethods(env, NATIVE_CLASS_NAME, g_NativeMethod,
                                        sizeof(g_NativeMethod) /
                                        sizeof(g_NativeMethod[0]));

//    jint regRet = RegisterNativeMethods(env, NATIVE_CLASS_NAME, g_test,
//                                        sizeof(g_test) /
//                                        sizeof(g_test[0]));
    if (regRet != JNI_TRUE)
    {
        return JNI_ERR;
    }


    return JNI_VERSION_1_6;
}

extern "C" void JNI_OnUnload(JavaVM *jvm, void *p)
{
    JNIEnv *env = NULL;
    if (jvm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK)
    {
        return;
    }


    UnregisterNativeMethods(env, NATIVE_CLASS_NAME);
}
