#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include <libavcodec/avcodec.h>
#include "libavutil/avutil.h"
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <glob.h>
#include <unistd.h>

#define USB_CAMERA_DEFAULT_V_WIDTH                 640
#define USB_CAMERA_DEFAULT_V_HEIGHT                480
#define USB_CAMERA_DEFAULT_DEVICE_PORT             "/dev/video3"
#define USB_CAMERA_DEFAULT_VIDEO_SIZE              "640x480"
#define USB_CAMERA_DEFAULT_FRAMERATE_STRING        "25"             // 
#define USB_CAMERA_DEFAULT_PIXEL_FORMAT            "yuyv422"        // 
#define USB_CAMERA_DEFAULT_TIME_BASE               (25)             // 视频帧时间记住，每帧的时间间隔
#define USB_CAMERA_DEFAULT_FRAMERATE_INT           (25)             // 视频帧率，表示每秒钟播放的帧数
#define USB_CAMERA_DEFAULT_GOP_SIZE                (10)             // 关键帧间的最大帧数，即每隔多少帧插入一个关键帧（I帧）
#define USB_CMAERA_DEFAULT_MAX_B_FRAMES            (1)              // 最大双向预测帧数，即允许使用多少个 B 帧
#define USB_CAMERA_DEFAULT_BIT_RATE                (600 * 1000)     // 视频的比特率，表示每秒钟传输的数据量
#define DEFAULT_RECORD_CAMERA_TIME                  (12)            // 12s
#define DEFAULT_H264_OUTPUT_FILE_PATH              "./video.h264"

typedef struct {
    const char *key;
    const char *value;
} av_val_cfg_t;

static AVFormatContext *open_dev(const char *device, const av_val_cfg_t *opts, int opt_count) {
    int ret;
    char errors[1024] = {0};
    AVFormatContext *fmt_ctx = NULL;
    AVDictionary *options = NULL;
    const AVInputFormat *iformat = av_find_input_format("video4linux2");

    avdevice_register_all();

    for (int i = 0;i < opt_count; i++) {
        av_dict_set(&options, opts[i].key, opts[i].value, 0);
    }

    ret = avformat_open_input(&fmt_ctx, device, iformat, &options);
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
    (*enc_ctx)->time_base = (AVRational){1, USB_CAMERA_DEFAULT_TIME_BASE};
    (*enc_ctx)->framerate = (AVRational){USB_CAMERA_DEFAULT_FRAMERATE_INT, 1};
    (*enc_ctx)->gop_size = USB_CAMERA_DEFAULT_GOP_SIZE;
    (*enc_ctx)->max_b_frames = USB_CMAERA_DEFAULT_MAX_B_FRAMES;
    (*enc_ctx)->bit_rate = USB_CAMERA_DEFAULT_BIT_RATE;
    (*enc_ctx)->pix_fmt = AV_PIX_FMT_YUV420P;               // encoder 支持yuv420p

    ret = avcodec_open2(*enc_ctx, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open codec: %s!\n", av_err2str(ret));
        exit(1);
    }

    return ;
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

static int encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *newpkt, FILE *outfile) {
    int ret;

    if (frame) {
        printf("send frame to encoder, pts=%ld\n", frame->pts);
    }

    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error, Failed to send a frame for encoding: %s\n", av_err2str(ret));
        return -1;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, newpkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return -1;
        } else if (ret < 0) {
            fprintf(stderr, "Error, Failed to encode: %s\n", av_err2str(ret));
            return -1;
        }

        fwrite(newpkt->data, 1, newpkt->size, outfile);
        av_packet_unref(newpkt);
    }

    return 0;
}

int rec_video(const char *device) {
    if (!device) 
        return -1;

    int ret, base = 0, count = 0;
    AVPacket pkt;
    AVCodecContext *enc_ctx = NULL;
    AVFrame *frame = create_frame(USB_CAMERA_DEFAULT_V_WIDTH, USB_CAMERA_DEFAULT_V_HEIGHT);
    AVPacket *newpkt = av_packet_alloc();
    struct SwsContext *sws_ctx = NULL;

    av_val_cfg_t opt[] = {
        {"video_size", USB_CAMERA_DEFAULT_VIDEO_SIZE},
        {"framerate", USB_CAMERA_DEFAULT_FRAMERATE_STRING},
        {"pixel_format", USB_CAMERA_DEFAULT_PIXEL_FORMAT}
    };

    int opt_count = sizeof(opt) / sizeof(opt[0]);

    AVFormatContext *fmt_ctx = open_dev(device, opt, opt_count);

    av_log_set_level(AV_LOG_DEBUG);
    FILE *outfile = fopen(DEFAULT_H264_OUTPUT_FILE_PATH, "wb+");
    if (!outfile) {
        fprintf(stderr, "Could not open output file\n");
        return -1;
    }

    open_encoder(USB_CAMERA_DEFAULT_V_WIDTH, USB_CAMERA_DEFAULT_V_HEIGHT, &enc_ctx);

    // 创建 SwsContext 用于格式转换
    sws_ctx = sws_getContext(USB_CAMERA_DEFAULT_V_WIDTH, USB_CAMERA_DEFAULT_V_HEIGHT, AV_PIX_FMT_YUYV422,
                             USB_CAMERA_DEFAULT_V_WIDTH, USB_CAMERA_DEFAULT_V_HEIGHT, AV_PIX_FMT_YUV420P,
                             SWS_BILINEAR, NULL, NULL, NULL);
    if (!sws_ctx) {
        fprintf(stderr, "Could not initialize the conversion context\n");
        return -1;
    }

    int video_time = 100 * (DEFAULT_RECORD_CAMERA_TIME / 4); // 100帧 -> 4秒

    while ((ret = av_read_frame(fmt_ctx, &pkt)) >= 0 && count++ < video_time) {
        if (pkt.stream_index == 0) {
            AVFrame *tmp_frame = av_frame_alloc();
            if (!tmp_frame) {
                fprintf(stderr, "Could not allocate temporary frame\n");
                break;
            }

            tmp_frame->width = USB_CAMERA_DEFAULT_V_WIDTH;
            tmp_frame->height = USB_CAMERA_DEFAULT_V_HEIGHT;
            tmp_frame->format = AV_PIX_FMT_YUYV422;

            ret = av_image_fill_arrays(tmp_frame->data, tmp_frame->linesize, pkt.data,
                                       AV_PIX_FMT_YUYV422, USB_CAMERA_DEFAULT_V_WIDTH, USB_CAMERA_DEFAULT_V_HEIGHT, 1);
            if (ret < 0) {
                fprintf(stderr, "Could not fill image arrays\n");
                av_frame_free(&tmp_frame);
                break;
            }

            // 转换格式从 YUYV422 到 YUV420P
            sws_scale(sws_ctx, (const uint8_t * const *)tmp_frame->data, tmp_frame->linesize,
                      0, USB_CAMERA_DEFAULT_V_HEIGHT, frame->data, frame->linesize);

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

    return 0;
}


static void print_help(const char *program_name) {
    printf("Usage: %s [-d <device>]\n", program_name);
    printf("Options:\n");
    printf("  -d <device>   Specify the video device (default: %s)\n", USB_CAMERA_DEFAULT_DEVICE_PORT);
}

static int is_device_available(const char *device) {

    FILE *fp = fopen(device, "r");
    if (fp) {
        fclose(fp);
        return 1;
    }
    return 0;
}

static int parse_command_line(int argc, char *argv[], char **device) {
    int opt;
    while ((opt = getopt(argc, argv, "d:h")) != -1) {
        switch (opt) {
            case 'd':
                *device = optarg; // Use the device specified by the user
                break;
            case 'h':
                print_help(argv[0]);
                exit(EXIT_SUCCESS);
            default:
                print_help(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {

    char *device = USB_CAMERA_DEFAULT_DEVICE_PORT;  // 默认设备参数
    int rc = 0;

    glob_t glob_result;
    if (glob("/dev/video*", 0, NULL, &glob_result) == 0) {
        printf("Found video devices:\n");
        for (size_t i = 0; i < glob_result.gl_pathc; i++) {
            printf("  %s\n", glob_result.gl_pathv[i]);
        }
        globfree(&glob_result);
    } else {
        printf("No video devices found.\n");
    }

    parse_command_line(argc, argv, &device);

    printf("use device: %s\r\n", device);

   if (!is_device_available(device)) {
        fprintf(stderr, "Error: Specified device '%s' does not exist.\n", device);
        exit(EXIT_FAILURE);
    }

    rec_video(device);

    return rc;
}