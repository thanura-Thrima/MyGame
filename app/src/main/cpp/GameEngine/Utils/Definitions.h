#pragma once
#include "../../../../../../../../AppData/Local/Android/Sdk/ndk/25.1.8937393/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/android/log.h"

#define LOGI(TAG,...) __android_log_print(ANDROID_LOG_INFO,TAG.c_str(),__VA_ARGS__)
#define LOGD(TAG,...) __android_log_print(ANDROID_LOG_DEBUG,TAG.c_str(),__VA_ARGS__)
#define LOGW(TAG,...) __android_log_print(ANDROID_LOG_WARNING,TAG.c_str(),__VA_ARGS__)
#define LOGE(TAG,...) __android_log_print(ANDROID_LOG_ERROR, TAG.c_str(),__VA_ARGS__)

#define NONCOPYABLE(className)                          \
    className(const className&) = delete;               \
    className& operator=(const className&) = delete;    \
    className(className&&) = delete;                    \
    className& operator=(className&&) = delete;


typedef unsigned long uint64;
typedef unsigned int uint32;
typedef unsigned char uint8;

typedef float float32;
typedef double float64;

struct AppStruct{

};