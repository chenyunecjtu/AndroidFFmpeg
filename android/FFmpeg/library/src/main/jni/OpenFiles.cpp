//
// Created by wlanjie on 16/5/30.
//

#include "OpenFiles.h"

OpenFiles::OpenFiles() {

}

OpenFiles::~OpenFiles() {

}

OpenFiles* OpenFiles::getInstance() {
    if (instance == NULL) {
        instance = new OpenFiles();
    }
    return instance;
}

int OpenFiles::openInputFile(InputFile *inputFile) {
    int ret = 0;
    FFmpeg::getInstance()->;
    AVFormatContext *ic = NULL;
    ret = avformat_open_input(&ic, inputFile->inputDataSource, NULL, NULL);
    if (ret < 0) {
        av_err2str(ret);
        return ret;
    }
    ret = avformat_find_stream_info(ic, NULL);
    if (ret < 0) {
        av_err2str(ret);
        return ret;
    }
    for (int i = 0; i < ic->nb_streams; ++i) {
        AVStream *st = ic->streams[i];
        InputStream *ist = dynamic_cast<InputStream *> (av_mallocz(sizeof(*ist)));
        ist->st = st;
        ist->dec = avcodec_find_decoder(st->codec->codec_id);
        ist->dec->capabilities |= CODEC_CAP_DELAY;
        ist->dec_ctx = avcodec_alloc_context3(ist->dec);
        ist->dec_ctx->active_thread_type |= FF_THREAD_FRAME;
        ret = avcodec_copy_context(ist->dec_ctx, st->codec);
        if (ret < 0) {
            av_freep(&ist);
            av_err2str(ret);
            return ret;
        }
        FFmpeg::getInstance()->addInputStream(ist);
    }
    inputFile->ic = ic;
    av_dump_format(ic, 0, inputFile->inputDataSource, 0);
    return ret;
}


int OpenFiles::get_buffer(AVCodecContext *s, AVFrame *frame, int flags) {
    return avcodec_default_get_buffer2(s, frame, flags);
}

void free_filter_graph(FilterGraph **graph, int graph_count) {
    for (int i = 0; i < graph_count; ++i) {
        FilterGraph *filterGraph = graph[i];
        avfilter_graph_free(&filterGraph->graph);
        av_freep(&filterGraph);
        av_freep(&graph);
    }
}

int OpenFiles::init_output() {
    int ret = 0;
    FilterGraph **graph = NULL;
    int graph_count = 0;
    int outputStreamSize = FFmpeg::getInstance()->getOutputStreams().size();
    for (int i = 0; i < outputStreamSize; ++i) {
        InputStream *ist = &FFmpeg::getInstance()->getInputStreams().at(i);
        OutputStream *ost = &FFmpeg::getInstance()->getOutputStreams().at(i);
        ost->st->discard = ist->st->discard;
        ost->enc_ctx->bits_per_raw_sample = ist->dec_ctx->bits_per_raw_sample;
        ost->enc_ctx->chroma_sample_location = ist->dec_ctx->chroma_sample_location;
        if (!ost->filter && (ost->enc_ctx->codec_type == AVMEDIA_TYPE_VIDEO || ost->enc_ctx->codec_type == AVMEDIA_TYPE_AUDIO)) {
            FilterGraph *filterGraph = Filters::getInstance()->init_filtergraph(ist, ost);
            if ((ret = Filters::getInstance()->configure_filtergraph(filterGraph)) < 0) {
                return ret;
            }
            graph[graph_count - 1] = filterGraph;
        }
        AVRational frame_rate = { 0, 0 };
        if (ost->enc_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
            frame_rate = av_buffersink_get_frame_rate(ost->filter);
            if (!frame_rate.num) {
                frame_rate = (AVRational) { 25, 1 };
            }
        }
        /**
         * 设置音频和视频的参数
         * 视频包括:宽,高,帧率,这里设置的是1秒25帧,25fps,
         * 音频包括:声道数量,采样率,声道类别(左右声道),
         */
        switch (ost->enc_ctx->codec_type) {
            case AVMEDIA_TYPE_VIDEO:
                ost->enc_ctx->pix_fmt = (enum AVPixelFormat) ost->filter->inputs[0]->format;
                ost->enc_ctx->width = ost->filter->inputs[0]->w;
                ost->enc_ctx->height = ost->filter->inputs[0]->h;
                AVRational time_base  = av_inv_q(frame_rate);
//                ost->st->avg_frame_rate = frame_rate;
                ost->enc_ctx->time_base = ist->dec_ctx->time_base;
                ost->enc_ctx->pix_fmt = ost->enc->pix_fmts[0];
                ost->enc_ctx->sample_aspect_ratio = ist->dec_ctx->sample_aspect_ratio;
                break;
            case AVMEDIA_TYPE_AUDIO:
                ost->enc_ctx->sample_fmt = (enum AVSampleFormat) ost->filter->inputs[0]->format;
                ost->enc_ctx->sample_rate = ost->filter->inputs[0]->sample_rate;
                ost->enc_ctx->channels = avfilter_link_get_channels(ost->filter->inputs[0]);
                ost->enc_ctx->channel_layout = ost->filter->inputs[0]->channel_layout;
                ost->enc_ctx->time_base = (AVRational) { 1, ost->enc_ctx->sample_rate };
                break;
            default:
                break;
        }
    }
    int inputStreamSize = FFmpeg::getInstance()->getInputStreams().size();
    AVDictionary *decoder_opts = NULL;
    av_dict_set(&decoder_opts, "threads", "auto", 0);
    //打开输入的编码器
    for (int i = 0; i < inputStreamSize; ++i) {
        InputStream *ist = &FFmpeg::getInstance()->getInputStreams().at(i);
        ist->dec_ctx->opaque = ist;
        ist->dec_ctx->get_buffer2 = get_buffer;
        if ((ret = avcodec_open2(ist->dec_ctx, ist->dec, &decoder_opts)) < 0) {
            av_err2str(ret);
            free_filter_graph(graph, graph_count);
            return ret;
        }
    }
    //打开输出的编码器
    AVDictionary *encoder_opts = NULL;
    av_dict_set(&encoder_opts, "threads", "auto", 0);
    for (int i = 0; i < outputStreamSize; ++i) {
        OutputStream *ost = &FFmpeg::getInstance()->getOutputStreams().at(i);
        if ((ret = avcodec_open2(ost->enc_ctx, ost->enc, &encoder_opts)) < 0) {
            free_filter_graph(graph, graph_count);
            av_err2str(ret);
            return ret;
        }
        if ((ret = avcodec_copy_context(ost->st->codec, ost->enc_ctx)) < 0) {
            free_filter_graph(graph, graph_count);
            av_err2str(ret);
            return ret;
        }
        ost->st->time_base = av_add_q(ost->enc_ctx->time_base, (AVRational) { 0, 1 });
        ost->st->codec->codec = ost->enc_ctx->codec;
    }
    //写文件头
    if ((ret = avformat_write_header(FFmpeg::getInstance()->getOutputFile()->oc, NULL)) < 0) {
        free_filter_graph(graph, graph_count);
        av_err2str(ret);
        return ret;
    }
    return ret;
}

OutputStream * OpenFiles::new_output_stream(AVFormatContext *oc, enum AVMediaType type, const char *codec_name, int source_index) {
    AVStream *st = avformat_new_stream(oc, NULL);
    if (!st) {
        return NULL;
    }
    OutputStream *ost = dynamic_cast<OutputStream *> (av_mallocz(sizeof(*ost)));
    AVCodec *enc = avcodec_find_encoder_by_name(codec_name);
    if (!enc) {
        av_log(NULL, AV_LOG_ERROR, "Can't found by encoder name %s", codec_name);
        return NULL;
    }
    AVCodecContext *enc_ctx = avcodec_alloc_context3(enc);
    ost->enc_ctx = enc_ctx;
    ost->enc = enc;
    ost->st = st;
    ost->st->codec->codec_type = type;
    ost->enc_ctx->codec_type = type;
//    ost->enc->capabilities |= CODEC_CAP_DELAY;
//    ost->enc_ctx->active_thread_type |= FF_THREAD_FRAME;
//    ost->enc_ctx->thread_count = 8;
    if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
        ost->enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    FFmpeg::getInstance()->addOutputStream(ost);
    return ost;
}

int OpenFiles::openOutputFile(OutputFile *outputFile) {
    int ret = 0;
    AVFormatContext *oc = NULL;
    ret = avformat_alloc_output_context2(&oc, NULL, NULL, outputFile->outputDataSource);
    if (ret < 0) {
        av_err2str(ret);
        return ret;
    }
    outputFile->oc = oc;
    int size = FFmpeg::getInstance()->getInputStreams().size();
    for (int i = 0; i < size; ++i) {
        InputStream *ist = &FFmpeg::getInstance()->getInputStreams().at(i);
        switch (ist->st->codec->codec_type) {
            case AVMEDIA_TYPE_VIDEO:
                if (av_guess_codec(oc->oformat, NULL, outputFile->outputDataSource, NULL, AVMEDIA_TYPE_VIDEO) != AV_CODEC_ID_NONE) {
                    OutputStream *ost = new_output_stream(oc, AVMEDIA_TYPE_VIDEO, "libx264", i);
                    if (ost == NULL) {
                        return AVERROR(ENOMEM);
                    }
                    ost->new_width = outputFile->newWidth;
                    ost->new_height = outputFile->newHeight;
//                    if (new_width > 0 && new_height > 0) {
//                        char video_size[10];
//                        snprintf(video_size, sizeof(video_size), "%dx%d", new_width, new_height);
//                        ret = av_parse_video_size(&ost->enc_ctx->width, &ost->enc_ctx->height,
//                                                  video_size);
//                        if (ret < 0) {
//                            av_free(video_size);
//                            av_err2str(ret);
//                            return ret;
//                        }
//                    }
                    ost->st->sample_aspect_ratio = ost->enc_ctx->sample_aspect_ratio;
                    ost->avfilter = outputFile->videoAVFilter;
                }
                break;
            case AVMEDIA_TYPE_AUDIO:
                if (av_guess_codec(oc->oformat, NULL, outputFile->outputDataSource, NULL, AVMEDIA_TYPE_AUDIO) != AV_CODEC_ID_NONE) {
                    OutputStream *ost = new_output_stream(oc, AVMEDIA_TYPE_AUDIO, "aac", i);
                    if (ost == NULL) {
                        return AVERROR(ENOMEM);
                    }
                    ost->avfilter = outputFile->audioAVFilter;
                }
                break;
            default:
                break;
        }
    }
    if (!(oc->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open2(&oc->pb, outputFile->outputDataSource, AVIO_FLAG_WRITE, NULL, NULL);
        if (ret < 0) {
            av_err2str(ret);
            return ret;
        }
    }
    if ((ret = init_output()) < 0) {
        return ret;
    }
    return ret;
}

void OpenFiles::release() {

}
