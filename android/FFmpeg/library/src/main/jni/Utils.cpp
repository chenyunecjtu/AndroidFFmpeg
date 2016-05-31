//
// Created by wlanjie on 16/5/30.
//

#include "Utils.h"
#include <unistd.h>

Utils::Utils() {
    illegalArgumentClassName = "java/lang/IllegalArgumentException";
    mediaSourceObject = NULL;
    mediaSourceClass = NULL;
}

Utils::~Utils() {
    illegalArgumentClassName = NULL;
}

Utils* Utils::getInstance() {
    if (instance == NULL) {
        instance = new Utils();
    }
    return instance;
}

int Utils::checkFileExist(JNIEnv *env, const char *input_data_source) {
    if (input_data_source == NULL)
        return -1;
    if (strlen(input_data_source) == 0) {
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        }
        jclass illegal_argument_class = env->FindClass("java/lang/IllegalArgumentException");
        env->ThrowNew(illegal_argument_class, "must be before call FFmpeg.SetInputDataSource() method");
        env->DeleteLocalRef(illegal_argument_class);
        return -1;
    }
    if (access(input_data_source, 4) == -1) {
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        }
        jclass file_not_found_class = env->FindClass("java/io/FileNotFoundException");
        char error[255];
        snprintf(error, sizeof(error), "%s not found\n", input_data_source);
        env->ThrowNew(file_not_found_class, error);
        env->DeleteLocalRef(file_not_found_class);
        return -1;
    }
    return 0;
}

void Utils::throwIllegalArgumentException(JNIEnv *env, jclass illegalArgumentClass, const char *errorMessage) {
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }
    env->ThrowNew(illegalArgumentClass, errorMessage);
    env->DeleteLocalRef(illegalArgumentClass);
}

void Utils::getJniMethod(JNIEnv *env, jobject object) {
    jclass illegalArgumentClass = env->FindClass(illegalArgumentClassName);
    if (illegalArgumentClass == NULL) {
        return;
    }
    jclass ffmpegClass = env->GetObjectClass(object);
    if (ffmpegClass == NULL) {
        throwIllegalArgumentException(env, illegalArgumentClass, "Can't find FFmpeg class");
        env->DeleteLocalRef(ffmpegClass);
        return;
    }
    jfieldID mediaSourceFieldID = env->GetFieldID(ffmpegClass, "mediaSource", "Lcom/wlanjie/ffmpeg/library/MediaSource;");
    if (mediaSourceFieldID == NULL) {
        throwIllegalArgumentException(env, illegalArgumentClass, "Can't get FFmpeg.mediaSource Object");
        env->DeleteLocalRef(ffmpegClass);
        return;
    }
    mediaSourceObject = env->GetObjectField(object, mediaSourceFieldID);
    if (mediaSourceObject == NULL) {
        throwIllegalArgumentException(env, illegalArgumentClass, "Can't get FFmpeg.mediaSource Object");
        env->DeleteLocalRef(ffmpegClass);
        return;
    }
    mediaSourceClass = env->GetObjectClass(mediaSourceObject);
    if (mediaSourceClass == NULL) {
        throwIllegalArgumentException(env, illegalArgumentClass, "Can't get MeidaSource Object");
        env->DeleteLocalRef(mediaSourceObject);
        env->DeleteLocalRef(ffmpegClass);
    }
}

void Utils::setVideoHeight(JNIEnv *env, jobject object, int height) {
    if (mediaSourceClass == NULL || mediaSourceObject == NULL) {
        return;
    }
    jmethodID setVideoHeightMethodID = env->GetMethodID(mediaSourceClass, "setHeight", "(I)V");
    env->CallVoidMethod(mediaSourceObject, setVideoHeightMethodID, height);
    env->DeleteLocalRef(mediaSourceClass);
    env->DeleteLocalRef(mediaSourceObject);
}

void Utils::setVideoWidth(JNIEnv *env, jobject object, int width) {
    if (mediaSourceClass == NULL || mediaSourceObject == NULL) {
        return;
    }
    jmethodID setVideoWidthMethodID = env->GetMethodID(mediaSourceClass, "setWidth", "(I)V");
    env->CallVoidMethod(mediaSourceObject, setVideoWidthMethodID, width);
    env->DeleteLocalRef(mediaSourceClass);
    env->DeleteLocalRef(mediaSourceObject);
}

void Utils::setVideoRotation(JNIEnv *env, jobject object, double rotation) {
    if (mediaSourceClass == NULL || mediaSourceObject == NULL) {
        return;
    }
    jmethodID setVideoRotationMethodID = env->GetMethodID(mediaSourceClass, "setRotation", "(D)V");
    env->CallVoidMethod(mediaSourceObject, setVideoRotationMethodID, rotation);
    env->DeleteLocalRef(mediaSourceClass);
    env->DeleteLocalRef(mediaSourceObject);
}
