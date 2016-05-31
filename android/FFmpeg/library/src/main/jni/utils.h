//
// Created by wlanjie on 16/5/30.
//

#ifndef FFMPEG_UTILS_H
#define FFMPEG_UTILS_H


#include <jni.h>
#include <stdio.h>

class Utils {
public:
    Utils();
    ~Utils();
    static Utils* getInstance();
    int checkFileExist(JNIEnv *env, const char *input_data_source);
    void getJniMethod(JNIEnv *env, jobject object);
    void setVideoWidth(JNIEnv *env, jobject object, int width);
    void setVideoHeight(JNIEnv *env, jobject object, int height);
    void setVideoRotation(JNIEnv *env, jobject object, double rotation);

private:
    static Utils *instance;
    const char *illegalArgumentClassName;
    jobject mediaSourceObject;
    jclass mediaSourceClass;
    void throwIllegalArgumentException(JNIEnv *env, jclass illegalArgumentClass, const char *errorMessage);
};


#endif //FFMPEG_UTILS_H
