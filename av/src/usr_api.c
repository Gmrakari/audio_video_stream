#include "usr_api.h"
#include "cJSON.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tsbuilder.h"

#include "pthread.h"

#include <bits/types.h>
#include <time.h>

#define TS_FILEPATH_MAX_LENGTH          (256)

typedef struct {
    char filepath[TS_FILEPATH_MAX_LENGTH];
    void *handle;
    pthread_mutex_t w_lock;
} write_ts_private_t;

typedef enum {
    GM_VIDEO_PFRAME = 0,                                    /**< 0 */
    GM_VIDEO_IFRAME = 1,
    GM_AUDIO_FRAME = 2,
}Gmlink_FrameType_e;

typedef enum {
    GM_VIDEO_H264 = 0,                                        /**< 0 */
    GM_VIDEO_H265 = 1,
    GM_VIDEO_YUV  = 2,
    GM_VIDEO_JPEG = 3,

    GM_AUDIO_PCM = 4,
    GM_AUDIO_AAC = 5,
    GM_AUDIO_G711A = 6,
    GM_AUDIO_G711U = 7,
}Gmlink_MeidaFormat_e;

#pragma pack(1)
typedef struct {

    unsigned int                u32Width;          //分辨率宽度
    unsigned int                u32Height;      //分辨率高度
    unsigned int                u32Framerate;     //码率大小
    Gmlink_MeidaFormat_e        eFormat;          //编码格式
}Gmlink_VideoInfo_t;
#pragma pack()

#pragma pack(1)
typedef struct {
    unsigned int                 u32Samplerate;  //采样频率
    Gmlink_MeidaFormat_e         eFormat;          //编码格式
} Gmlink_AudioInfo_t;
#pragma pack()

#pragma pack(1)
typedef struct {
    
    char *                      pDataBuf;        //数据
    unsigned int                u32BufSize;     //数据大小
    Gmlink_FrameType_e        eType;          //帧类型
    unsigned int                u32Seq;           //帧序号    
    unsigned long long          u64Timestamp;      //时间戳
    union 
    {
       Gmlink_VideoInfo_t     stVideoInfo;
       Gmlink_AudioInfo_t     stAudioInfo;
    } Media_Packet_Info;    
    unsigned int                nchannel;       //对讲通道号,用于处理多路设备的对讲，例如nvr设备
} Gmlink_MediaPacket_t;
#pragma pack()

static write_ts_private_t ts_priv;

#include "h265_test_data_360x640.h"
#include "g711_data.h"

long long LocalTime_ms(void);

long long LocalTime_ms(void) // 返回标准UTC时间，由APP跟据不同的时区来显示
{
    struct timeval tv;
    long long ms = 0;

    gettimeofday(&tv, NULL);// UTC基准的秒, 同time()
    ms = (long long)tv.tv_sec * (long long)1000 + (long long)tv.tv_usec / (long long)1000;

    return ms;
}

static void* stream_pull_video_proc(void *args)
{    
    Gmlink_MediaPacket_t  stMediaPacket;

    unsigned int frame_count = 0;

    long videono = 0;

    char *fremabuff = malloc(256*1024);
    memset(fremabuff, 0x00, 256*1024);

    uint64_t start_time = 0;
    
    while (1) {
        int num_frames = sizeof(test_video_frames) / sizeof(test_video_frames[0]);
        int i = (frame_count++ % num_frames);

        memset(&stMediaPacket, 0, sizeof(Gmlink_MediaPacket_t));
        stMediaPacket.u32Seq = videono++;
        stMediaPacket.u64Timestamp = LocalTime_ms();
        stMediaPacket.Media_Packet_Info.stVideoInfo.u32Framerate = 15;
        stMediaPacket.Media_Packet_Info.stVideoInfo.u32Width = 360;
        stMediaPacket.Media_Packet_Info.stVideoInfo.u32Height = 640;
        stMediaPacket.Media_Packet_Info.stVideoInfo.eFormat = GM_VIDEO_H265;

        if (test_video_frames[i].len > 10000) {
            stMediaPacket.eType = GM_VIDEO_IFRAME;
        } else {
            stMediaPacket.eType = GM_VIDEO_IFRAME;
        }

        memcpy(fremabuff, test_video_frames[i].data, test_video_frames[i].len);
        stMediaPacket.u32BufSize = test_video_frames[i].len;
        stMediaPacket.pDataBuf = fremabuff;

        if (!ts_priv.handle) {
            ts_priv.handle = gm_ts_start(ts_priv.filepath, GM_VIDEO_H265, GM_AUDIO_G711U);
            start_time = stMediaPacket.u64Timestamp;
        }

        pthread_mutex_lock(&ts_priv.w_lock);

        gm_ts_write(ts_priv.handle, stMediaPacket.eType, stMediaPacket.u64Timestamp, stMediaPacket.pDataBuf, stMediaPacket.u32BufSize);

        pthread_mutex_unlock(&ts_priv.w_lock);

        if (stMediaPacket.u64Timestamp - start_time >= (10 * 1000)){ 
            gm_ts_stop(ts_priv.handle);
            ts_priv.handle = NULL;
            exit(0);
            break;
        }
        usleep(44*1000);
    }

    if (fremabuff) {
        free(fremabuff);
        fremabuff = NULL;
    }
}

#include "g711_data.h"
static void* stream_pull_audio_proc(void *args)
{    
    Gmlink_MediaPacket_t  stMediaPacket;
    long audiono = 0;

    char *fremabuff = malloc(1024);

    memset(fremabuff, 0x00, 1024);

    int ret = -1;

    int pcm_offset = 0;

    uint64_t start_time = 0;

    while (1) {
        memset(&stMediaPacket, 0, sizeof(Gmlink_MediaPacket_t)); 
        stMediaPacket.u32Seq = audiono++;
        stMediaPacket.u64Timestamp = LocalTime_ms();
        stMediaPacket.eType = GM_AUDIO_FRAME;

        memset(fremabuff, 0x00, 1024);

		memcpy(fremabuff, (uint8_t *)pcm_test_data + pcm_offset, 160);

		pcm_offset += 160;

		if ((pcm_offset + 160) > sizeof(pcm_test_data)) {
			pcm_offset = 0;
		}

        stMediaPacket.u32BufSize = 160;
		stMediaPacket.pDataBuf = fremabuff;

        if (!ts_priv.handle) {
			start_time = stMediaPacket.u64Timestamp;
		}

        pthread_mutex_lock(&ts_priv.w_lock);

        gm_ts_write(ts_priv.handle, stMediaPacket.eType, stMediaPacket.u64Timestamp, stMediaPacket.pDataBuf, stMediaPacket.u32BufSize);

		pthread_mutex_unlock(&ts_priv.w_lock);

        usleep(22*1000);
    }

    if (fremabuff) {
        free(fremabuff);
        fremabuff = NULL;
    }

}

int usr_api_app(void)
{
    int ret = 0;
        

    int ts_ret = 0;
    memset(ts_priv.filepath, 0x00, sizeof(ts_priv.filepath));

    char filepath[] = "111.ts";

    strncpy(ts_priv.filepath, filepath, sizeof(ts_priv.filepath));

    printf("[%s][%d]filepath:%s\r\n", __func__, __LINE__, ts_priv.filepath);

    pthread_mutex_init(&ts_priv.w_lock, NULL);

    int vid_fmt = GM_VIDEO_H265;
    int audio_fmt = GM_AUDIO_G711U;

    pthread_t video_pid;
    ret = pthread_create(&video_pid, NULL, stream_pull_video_proc, NULL);    
    if (ret != 0) {
        printf("pthread_create error: %d\n", ret);
        return -1;
    }

    pthread_t audio_pid;
    ret = pthread_create(&audio_pid, NULL, stream_pull_audio_proc, NULL);    
    if (ret != 0) {
        printf("pthread_create error: %d\n", ret);
        return -1;
    }

    pthread_join(video_pid, NULL);
    pthread_join(audio_pid, NULL);

    return ret;
}
