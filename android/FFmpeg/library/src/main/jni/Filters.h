//
// Created by wlanjie on 16/5/30.
//

#ifndef FFMPEG_FILTERS_H
#define FFMPEG_FILTERS_H

#include "FFmpeg.h"

class Filters {
public:
    Filters();
    ~Filters();
    int configure_filtergraph(FilterGraph *graph);
    FilterGraph* init_filtergraph(InputStream *ist, OutputStream *ost);
    static Filters* getInstance();
private:
    static Filters *instance;
    AVFilterContext* get_transpose_filter(AVFilterContext *link_filter_context, AVFilterGraph *graph, const char *args);
    AVFilterContext* get_hflip_filter(AVFilterContext *link_filter_context, AVFilterGraph *graph);
    AVFilterContext* get_vflip_filter(AVFilterContext *link_filter_context, AVFilterGraph *graph);
    AVFilterContext* get_rotate_filter(AVFilterContext *link_filter_context, AVFilterGraph *graph, double theta);
    AVFilterContext* get_scale_filter(AVFilterContext *link_filter_context, AVFilterGraph *graph, int width, int height);
    int configure_input_video_filter(FilterGraph *graph, AVFilterInOut *in);
    int configure_input_audio_filter(FilterGraph *graph, AVFilterInOut *in);
    int configure_output_video_filter(FilterGraph *graph, AVFilterInOut *out);
    int configure_output_audio_filter(FilterGraph *graph, AVFilterInOut *out);
};


#endif //FFMPEG_FILTERS_H
