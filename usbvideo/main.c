#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include <libavcodec/avcodec.h>
#include "libavutil/avutil.h"
#include <libswscale/swscale.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define V_WIDTH 640
#define V_HEIGHT 480
#define USB_DEVICE "/dev/video1"

static AVFormatContext *open_dev() {
    int ret;
    char errors[1024] = {0};
    AVFormatContext *fmt_ctx = NULL;
    AVDictionary *options = NULL;
    const AVInputFormat *iformat = av_find_input_format("video4linux2");

    avdevice_register_all();
    av_dict_set(&options, "video_size", "640x480", 0);
    av_dict_set(&options, "framerate", "20", 0);
    av_dict_set(&options, "pixel_format", "yuyv422", 0);

    ret = avformat_open_input(&fmt_ctx, USB_DEVICE, iformat, &options);
    if (ret < 0) {
        av_strerror(ret, errors, sizeof(errors));
        fprintf(stderr, "Failed to open video device, [%d] %s\n", ret, errors);
        return NULL;
    }

    return fmt_ctx;
}

static void open_encoder(int width, int height, AVCodecContext **enc_ctx) {
    int ret;
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);

    if (!codec) {
        fprintf(stderr, "Codec libx264 not found\n");
        exit(1);
    }

    *enc_ctx = avcodec_alloc_context3(codec);
    if (!(*enc_ctx)) {
        fprintf(stderr, "Could not allocate video codec context!\n");
        exit(1);
    }

    (*enc_ctx)->width = width;
    (*enc_ctx)->height = height;
    (*enc_ctx)->time_base = (AVRational){1, 20};
    (*enc_ctx)->framerate = (AVRational){20, 1};
    (*enc_ctx)->gop_size = 10;
    (*enc_ctx)->max_b_frames = 1;
    (*enc_ctx)->bit_rate = 600000;
    (*enc_ctx)->pix_fmt = AV_PIX_FMT_YUV420P;

    ret = avcodec_open2(*enc_ctx, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open codec: %s!\n", av_err2str(ret));
        exit(1);
    }
}

static AVFrame *create_frame(int width, int height) {
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Error, No Memory!\n");
        return NULL;
    }

    frame->width = width;
    frame->height = height;
    frame->format = AV_PIX_FMT_YUV420P;  // 改为 YUV420P 格式

    if (av_frame_get_buffer(frame, 32) < 0) {
        fprintf(stderr, "Error, Failed to alloc buffer for frame!\n");
        av_frame_free(&frame);
        return NULL;
    }

    return frame;
}

static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *newpkt, FILE *outfile) {
    int ret;

    if (frame) {
        printf("send frame to encoder, pts=%ld\n", frame->pts);
    }

    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error, Failed to send a frame for encoding: %s\n", av_err2str(ret));
        return;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, newpkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if (ret < 0) {
            fprintf(stderr, "Error, Failed to encode: %s\n", av_err2str(ret));
            return;
        }

        fwrite(newpkt->data, 1, newpkt->size, outfile);
        av_packet_unref(newpkt);
    }
}

void rec_video() {
    int ret, base = 0, count = 0;
    AVPacket pkt;
    AVFormatContext *fmt_ctx = open_dev();
    AVCodecContext *enc_ctx = NULL;
    AVFrame *frame = create_frame(V_WIDTH, V_HEIGHT);
    AVPacket *newpkt = av_packet_alloc();
    struct SwsContext *sws_ctx = NULL;

    av_log_set_level(AV_LOG_DEBUG);
    FILE *outfile = fopen("./video.h264", "wb+");
    if (!outfile) {
        fprintf(stderr, "Could not open output file\n");
        return;
    }

    open_encoder(V_WIDTH, V_HEIGHT, &enc_ctx);

    // 创建 SwsContext 用于格式转换
    sws_ctx = sws_getContext(V_WIDTH, V_HEIGHT, AV_PIX_FMT_YUYV422,
                             V_WIDTH, V_HEIGHT, AV_PIX_FMT_YUV420P,
                             SWS_BILINEAR, NULL, NULL, NULL);
    if (!sws_ctx) {
        fprintf(stderr, "Could not initialize the conversion context\n");
        return;
    }

    while ((ret = av_read_frame(fmt_ctx, &pkt)) >= 0 && count++ < 100) {
        if (pkt.stream_index == 0) {
            AVFrame *tmp_frame = av_frame_alloc();
            if (!tmp_frame) {
                fprintf(stderr, "Could not allocate temporary frame\n");
                break;
            }

            tmp_frame->width = V_WIDTH;
            tmp_frame->height = V_HEIGHT;
            tmp_frame->format = AV_PIX_FMT_YUYV422;

            ret = av_image_fill_arrays(tmp_frame->data, tmp_frame->linesize, pkt.data,
                                       AV_PIX_FMT_YUYV422, V_WIDTH, V_HEIGHT, 1);
            if (ret < 0) {
                fprintf(stderr, "Could not fill image arrays\n");
                av_frame_free(&tmp_frame);
                break;
            }

            // 转换格式从 YUYV422 到 YUV420P
            sws_scale(sws_ctx, (const uint8_t * const *)tmp_frame->data, tmp_frame->linesize,
                      0, V_HEIGHT, frame->data, frame->linesize);

            frame->pts = base++;

            encode(enc_ctx, frame, newpkt, outfile);
            av_frame_free(&tmp_frame);
        }

        av_packet_unref(&pkt);
    }

    encode(enc_ctx, NULL, newpkt, outfile);
    fclose(outfile);
    avformat_close_input(&fmt_ctx);
    av_frame_free(&frame);
    av_packet_free(&newpkt);
    avcodec_free_context(&enc_ctx);
    sws_freeContext(sws_ctx);
}

int main(int argc, char *argv[]) {
    rec_video();
    return 0;
}