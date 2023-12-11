//
// Created by varjo on 11/25/2023.
//

#ifndef MYGAME_DEFINITIONS_H
#define MYGAME_DEFINITIONS_H
#include "../../../../../../../../AppData/Local/Android/Sdk/ndk/25.1.8937393/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/android/log.h"

#define LOGI(TAG,...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)
#define LOGD(TAG,...) __android_log_print(ANDROID_LOG_DEBUG,TAG,__VA_ARGS__)
#define LOGW(TAG,...) __android_log_print(ANDROID_LOG_WARNING,TAG,__VA_ARGS__)
#define LOGE(TAG,...) __android_log_print(ANDROID_LOG_ERROR, TAG,__VA_ARGS__)

#define NONCOPYABLE(className)                          \
    className(const className&) = delete;               \
    className& operator=(const className&) = delete;    \
    className(className&&) = delete;                    \
    className& operator=(className&&) = delete;

#endif //MYGAME_DEFINITIONS_H
