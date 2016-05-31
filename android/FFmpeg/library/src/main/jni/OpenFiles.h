//
// Created by wlanjie on 16/5/30.
//

#ifndef FFMPEG_OPENFILES_H
#define FFMPEG_OPENFILES_H

#include "FFmpeg.h"
#include "Filters.h"

class OpenFiles {
public:
    OpenFiles();
    ~OpenFiles();
    static OpenFiles* getInstance();
    int openInputFile(InputFile *inputFile);
    int openOutputFile(OutputFile *outputFile);
    void release();

private:
    static OpenFiles* instance;
    int init_output();
    int get_buffer(AVCodecContext *s, AVFrame *frame, int flags);
    OutputStream *new_output_stream(AVFormatContext *oc, enum AVMediaType type, const char *codec_name, int source_index);
};


#endif //FFMPEG_OPENFILES_H
