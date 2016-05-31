//
// Created by wlanjie on 16/5/30.
//

#ifndef FFMPEG_FFMPEG_H
#define FFMPEG_FFMPEG_H

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/parseutils.h"
#include "libavfilter/avfilter.h"
#include "libavutil/bprint.h"
#include "libavutil/pixdesc.h"
#include "libavutil/eval.h"
#include "libavutil/display.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
};

#include <vector>

using namespace std;

#define SAFE_DELTE(p) do { delete (p); (p) = nullptr;} while (0)

typedef struct InputFile {
    const char *inputDataSource;
    AVFormatContext *ic;
} InputFile;

typedef struct OutputFile {
    const char *outputDataSource;
    AVFormatContext *oc;
    int newWidth;
    int newHeight;
    char *videoAVFilter;
    char *audioAVFilter;
} OutputFile;

typedef struct InputStream {
    AVStream *st;
    AVCodecContext *dec_ctx;
    struct AVCodec *dec;
    AVFilterContext *filter;
} InputStream;

typedef struct OutputStream {
    AVStream *st;
    AVCodecContext *enc_ctx;
    struct AVCodec *enc;
    AVFilterContext *filter;
    char *avfilter;
    int new_width;
    int new_height;
} OutputStream;

typedef struct FilterGraph {
    InputStream *ist;
    OutputStream *ost;
    AVFilterGraph *graph;
} FilterGraph;

class FFmpeg {
public:
    FFmpeg(void);
    virtual ~FFmpeg(void);
    static FFmpeg* getInstance();
    double getRotation(AVStream *st);
    void *grow_array(void *array, int elem_size, int *size, int new_size);
    int transcode();
    vector<InputStream> getInputStreams();
    vector<OutputStream> getOutputStreams();
    void addInputStream(InputStream *inputStream);
    void releaseInputStreams();
    void removeInputStream(InputStream *inputStream);
    void addOutputStream(OutputStream *outputStream);
    void releaseOutputStreams();
    void removeOutputStream(OutputStream *outputStream);
    void setInputFile(InputFile *inputFile);
    void setOutputFile(OutputFile *outputFile);
    OutputFile *getOutputFile();
    InputFile *getInputFile();

private:
    static FFmpeg *instance;
    vector<InputStream> inputStreams;
    vector<OutputStream> outputStreams;
    int encoder_write_frame(AVFrame *frame, int stream_index, int *got_frame);
    AVPacket init_packet();
    int flush_encoder(int stream_index);
    int filter_encoder_write_frame(int stream_index);
    OutputFile *outputFile;
    InputFile *inputFile;
};


#endif //FFMPEG_FFMPEG_H
