//
// Created by wlanjie on 16/5/30.
//

#include <jni.h>
#include <stdio.h>
#include "Utils.h"
#include "OpenFiles.h"

#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif
#define CLASS_NAME  "com/wlanjie/ffmpeg/library/FFmpeg"


void throw_not_open_file_exception(JNIEnv *env, const char *input_data_source) {
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }
    jclass illegal_argument_class = env->FindClass("java/lang/IllegalStateException");
    char error[255];
    snprintf(error, sizeof(error), "Cannot not open file %s\n", input_data_source);
    env->ThrowNew(illegal_argument_class, error);
    env->DeleteLocalRef(illegal_argument_class);
    OpenFiles::getInstance()->release();
}

void call_set_input_data_source_exception(JNIEnv *env) {
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }
    jclass illegal_argument_class = env->FindClass("java/lang/IllegalStateException");
    env->ThrowNew(illegal_argument_class, "Can't get inputDataSource, please call FFmpeg.setInputDataSource method");
    env->DeleteLocalRef(illegal_argument_class);
}

jint open_input_jni(JNIEnv *env, jobject object, jstring input_path) {
    int ret = 0;
    const char *input_data_source = env->GetStringUTFChars(input_path, NULL);
    //判断文件是否存在
    if (Utils::getInstance()->checkFileExist(env, input_data_source) < 0) {
        env->ReleaseStringUTFChars(input_path, input_data_source);
        env->DeleteLocalRef(object);
        OpenFiles::getInstance()->release();
        return -1;
    }
    InputFile *inputFile = dynamic_cast<InputFile *> (av_mallocz(sizeof(InputFile)));
    inputFile->inputDataSource = input_data_source;
    ret = OpenFiles::getInstance()->openInputFile(inputFile);
    if (ret < 0 || inputFile->ic) {
        throw_not_open_file_exception(env, input_data_source);
        env->ReleaseStringUTFChars(input_path, input_data_source);
        env->DeleteLocalRef(object);
        OpenFiles::getInstance()->release();
        return ret;
    }
    for (int i = 0; i < inputFile->ic->nb_streams; i++) {
        AVStream *st = inputFile->ic->streams[i];
        if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            Utils::getInstance()->setVideoWidth(env, object, st->codecpar->width);
            Utils::getInstance()->setVideoHeight(env, object, st->codecpar->height);
            Utils::getInstance()->setVideoRotation(env, object, FFmpeg::getInstance()->getRotation(st));
            break;
        }
    }
    env->ReleaseStringUTFChars(input_path, input_data_source);
    env->DeleteLocalRef(object);
    return ret;
}

jint compress(JNIEnv *env, jobject object, jstring output_path, jint new_width, jint new_height) {
    int ret = 0;
    const char *output_data_source = env->GetStringUTFChars(output_path, 0);
    OutputFile *outputFile = dynamic_cast<OutputFile *> (av_mallocz(sizeof(OutputFile)));
    outputFile->videoAVFilter = av_strdup("null");
    outputFile->audioAVFilter = av_strdup("anull");
    outputFile->outputDataSource = output_data_source;
    outputFile->newWidth = new_width;
    outputFile->newHeight = new_height;
    FFmpeg::getInstance()->setOutputFile(outputFile);
    ret = OpenFiles::getInstance()->openOutputFile(outputFile);
    if (ret < 0) {
        OpenFiles::getInstance()->release();
        env->ReleaseStringUTFChars(output_path, output_data_source);
        env->DeleteLocalRef(object);
        return ret;
    }
    ret = FFmpeg::getInstance()->transcode();
    if (ret < 0) {
        OpenFiles::getInstance()->release();
    }
//    release_media_source(&mediaSource);
    env->ReleaseStringUTFChars(output_path, output_data_source);
    env->DeleteLocalRef(object);
    return ret;
}

jint crop_jni(JNIEnv *env, jobject object, jstring output_path, jint x, jint y, jint width, jint height) {
    int ret = 0;
    const char *output_data_source = env->GetStringUTFChars(output_path, 0);
    char crop_avfilter[128];
    snprintf(crop_avfilter, sizeof(crop_avfilter), "crop=%d:%d:%d:%d", width, height, x, y);
    OutputFile *outputFile = dynamic_cast<OutputFile *> (av_mallocz(sizeof(OutputFile)));
    outputFile->videoAVFilter = av_strdup(crop_avfilter);
    outputFile->audioAVFilter = av_strdup("anull");
    outputFile->newWidth = width;
    outputFile->newHeight = height;
    FFmpeg::getInstance()->setOutputFile(outputFile);
    ret = OpenFiles::getInstance()->openOutputFile(outputFile);
    if (ret < 0) {
        OpenFiles::getInstance()->release();
        env->ReleaseStringUTFChars(output_path, output_data_source);
        env->DeleteLocalRef(object);
        return ret;
    }
    ret = FFmpeg::getInstance()->transcode();
//    release_media_source(&mediaSource);
    env->ReleaseStringUTFChars(output_path, output_data_source);
    env->DeleteLocalRef(object);
    return ret;
}

void release_ffmpeg(JNIEnv *env, jobject object) {
    OpenFiles::getInstance()->release();
    env->DeleteLocalRef(object);
}

void log_callback(void *ptr, int level, const char *fmt, va_list vl) {
    FILE *fp = fopen("/sdcard/av_log.txt", "a+");
    if (fp) {
        vfprintf(fp, fmt, vl);
        fflush(fp);
        fclose(fp);
    }
}


JNINativeMethod g_methods[] = {
        {"openInput", "(Ljava/lang/String;)I",  (void *) open_input_jni},
        {"compress", "(Ljava/lang/String;II)I", (void *) compress},
        {"crop", "(Ljava/lang/String;IIII)I",   (void *) crop_jni},
        {"release", "()V",                      (void *) release_ffmpeg},
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv *env = NULL;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_FALSE;
    }
    jclass clazz = env->FindClass(CLASS_NAME);
    env->RegisterNatives(clazz, g_methods, NELEM(g_methods));
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved) {

}
