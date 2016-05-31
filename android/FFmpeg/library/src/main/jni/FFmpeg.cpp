//
// Created by wlanjie on 16/5/30.
//

#include "FFmpeg.h"
#include "Utils.h"

FFmpeg::FFmpeg() {
}

FFmpeg::~FFmpeg() {

}

FFmpeg* FFmpeg::getInstance() {
    if (instance == NULL) {
        instance = new FFmpeg();
    }
    return instance;
}

vector<InputStream> FFmpeg::getInputStreams() {
    return inputStreams;
}

vector<OutputStream> FFmpeg::getOutputStreams() {
    return outputStreams;
}

void FFmpeg::addInputStream(InputStream *inputStream) {
    inputStreams.push_back(*inputStream);
}

void FFmpeg::releaseInputStreams() {
    inputStreams.clear();
    inputStreams.swap(inputStreams);
}

void FFmpeg::removeInputStream(InputStream *inputStream) {
    vector<InputStream>::iterator  iter;
    for (iter = inputStreams.begin(); iter != inputStreams.end();) {
        if (iter == inputStream) {
            inputStreams.erase(iter);
            break;
        }
        iter++;
    }
}

void FFmpeg::addOutputStream(OutputStream *outputStream) {
    outputStreams.push_back(*outputStream);
}

void FFmpeg::releaseOutputStreams() {
    outputStreams.clear();
    outputStreams.swap(outputStreams);
}

void FFmpeg::removeOutputStream(OutputStream *outputStream) {
    vector<OutputStream>::iterator  iter;
    for (iter = outputStreams.begin(); iter != outputStreams.end();) {
        if (iter == outputStream) {
            outputStreams.erase(iter);
            break;
        }
        iter++;
    }
}

void FFmpeg::setInputFile(InputFile *inputFile) {
    if (this->inputFile != inputFile) {
        this->inputFile = inputFile;
    }
}

void FFmpeg::setOutputFile(OutputFile *outputFile) {
    if (this->outputFile != outputFile) {
        this->outputFile = outputFile;
    }
}

InputFile* FFmpeg::getInputFile() {
    return inputFile;
}

OutputFile* FFmpeg::getOutputFile() {
    return outputFile;
}

double FFmpeg::getRotation(AVStream *st) {
    AVDictionaryEntry *rotate = av_dict_get(st->metadata, "rotate", NULL, 0);
    uint8_t *displaymatrix = av_stream_get_side_data(st, AV_PKT_DATA_DISPLAYMATRIX, NULL);
    double theta = 0;
    if (rotate && *rotate->value && strcmp(rotate->value, "0")) {
        char *tail;
        theta = av_strtod(rotate->value, &tail);
        if (*tail) {
            theta = 0;
        }
    }
    if (displaymatrix && !theta) {
        theta = -av_display_rotation_get((int32_t *) displaymatrix);
    }
    theta -= 360 * floor(theta / 360 + 0.9 / 360);
    if (fabs(theta - 90 * round(theta / 90)) > 2) {
        av_log(NULL, AV_LOG_WARNING, "Odd rotation angle.\n"
                "If you want to help, upload a sample "
                "of this file to ftp://upload.ffmpeg.org/incoming/ "
                "and contact the ffmpeg-devel mailing list. (ffmpeg-devel@ffmpeg.org)");
    }
    return theta;
}

void* FFmpeg::grow_array(void *array, int elem_size, int *size, int new_size) {

}


AVPacket FFmpeg::init_packet() {
    AVPacket packet;
    av_init_packet(&packet);
    packet.size = 0;
    packet.data = NULL;
    return packet;
}

int FFmpeg::encoder_write_frame(AVFrame *frame, int stream_index, int *got_frame) {
    int ret = 0;
    AVPacket packet = init_packet();
//    const OutputStream *ost = output_streams[stream_index];
    OutputStream *ost = &outputStreams.at(static_cast<unsigned int> (stream_index));
    int (*enc_func) (AVCodecContext *, AVPacket *, const AVFrame *, int *) =
            ost->st->codec->codec_type == AVMEDIA_TYPE_VIDEO ? avcodec_encode_video2 : avcodec_encode_audio2;
    int got_frame_local;
    if (!got_frame) {
        got_frame = &got_frame_local;
    }
    ret = enc_func(ost->enc_ctx, &packet, frame, got_frame);
    if (ret < 0) {
        av_packet_unref(&packet);
        return ret;
    }
    if (!(*got_frame)) {
        av_packet_unref(&packet);
        return 0;
    }
    packet.stream_index = stream_index;
    av_packet_rescale_ts(&packet, ost->st->codec->time_base, ost->st->time_base);
    ret = av_interleaved_write_frame(outputFile->oc, &packet);
    if (ret < 0) {
        av_packet_unref(&packet);
        return ret;
    }
    av_packet_unref(&packet);
    return ret;
}

int FFmpeg::flush_encoder(int stream_index) {
    int ret = 0;
//    const OutputStream *ost = output_streams[stream_index];
    OutputStream *ost = &outputStreams.at(static_cast<unsigned int> (stream_index));
    if (!(ost->enc_ctx->codec->capabilities & AV_CODEC_CAP_DELAY)) {
        return 0;
    }
    int got_frame;
    while (1) {
        ret = encoder_write_frame(NULL, stream_index, &got_frame);
        if (ret < 0) {
            break;
        }
        if (!got_frame) {
            return 0;
        }
    }
    return ret;
}

int FFmpeg::filter_encoder_write_frame(int stream_index) {
    int ret = 0;
//    const OutputStream *ost = output_streams[stream_index];
    OutputStream *ost = &outputStreams.at(static_cast<unsigned int> (stream_index));
    while (1) {
        AVFrame *frame = av_frame_alloc();
        if (!frame) {
            return AVERROR(ENOMEM);
        }
        ret = av_buffersink_get_frame_flags(ost->filter, frame, AV_BUFFERSINK_FLAG_NO_REQUEST);
        if (ret < 0) {
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                ret = 0;
            }
            av_frame_unref(frame);
            break;
        }
        frame->pict_type = AV_PICTURE_TYPE_NONE;
        ret = encoder_write_frame(frame, stream_index, NULL);
        if (ret < 0) {
            av_frame_unref(frame);
            return ret;
        }
        av_frame_unref(frame);
    }
    return ret;
}


int FFmpeg::transcode() {
    int ret = 0;
    int (*dec_func) (AVCodecContext *, AVFrame *, int *, const AVPacket *);
    AVPacket packet;
    int got_frame;
    while (av_read_frame(inputFile->ic, &packet) >= 0) {
        AVFrame *frame = av_frame_alloc();
//        InputStream *ist = input_streams[packet.stream_index];
        InputStream *ist = &inputStreams.at(static_cast<unsigned int> (packet.stream_index));
        if (!frame) {
            return AVERROR(ENOMEM);
        }
        av_packet_rescale_ts(&packet, ist->st->time_base, ist->st->codec->time_base);
        dec_func = ist->st->codec->codec_type == AVMEDIA_TYPE_VIDEO ? avcodec_decode_video2 : avcodec_decode_audio4;
        ret = dec_func(ist->dec_ctx, frame, &got_frame, &packet);
        if (ret < 0) {
            av_frame_unref(frame);
            return ret;
        }
        if (got_frame) {
            frame->pts = av_frame_get_best_effort_timestamp(frame);
            ret = av_buffersrc_add_frame_flags(ist->filter, frame, AV_BUFFERSRC_FLAG_PUSH);
            if (ret < 0) {
                av_frame_unref(frame);
                return ret;
            }
            ret = filter_encoder_write_frame(packet.stream_index);
            if (ret < 0) {
                return ret;
            }
        }
        av_packet_unref(&packet);
        av_frame_unref(frame);
    }
    for (int i = 0; i < inputFile->ic->nb_streams; ++i) {
        ret = filter_encoder_write_frame(i);
        if (ret < 0) {
            return ret;
        }
        ret = flush_encoder(i);
        if (ret < 0) {
            return ret;
        }
    }
    av_write_trailer(outputFile->oc);
    return ret;
}