#include "mpeg-ps.h"
#include "mpeg-ts.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "tsbuilder.h"

static void* ts_alloc(void* param, size_t bytes)
{
	static char s_buffer[188];
	assert(bytes <= sizeof(s_buffer));
	return s_buffer;
}

static void ts_free(void* param, void* packet)
{
	return;
}

static int ts_write(void* param, const void* packet, size_t bytes)
{
	return 1 == fwrite(packet, bytes, 1, (FILE*)param) ? 0 : ferror((FILE*)param);
}

static int ts_writebuff(void* param, const void* packet, size_t bytes)
{

   // __rion_ts_buff_t *buffinfo = (__rion_ts_buff_t *)param;
	//memcpy(buffinfo.buff+buffinfo.buffoffset,packet,bytes);
	//buffinfo.buffoffset = buffinfo.buffoffset + bytes;
    return 0;
}


int gm_ts_write(void *handle,int freamtype,long long pts,char *data,int datalen)
{
	if (handle == NULL)
	{
		return -1;
	}

    gm_ts_info_t *tsinfo = NULL;
	tsinfo = (gm_ts_info_t *)handle;
    
	int id = 0;
	int flagsvalue = -1;
	if (freamtype == 1)
	{
		flagsvalue = 0x0001;
		id = tsinfo->video_id;
	}
	else if (freamtype == 0)
	{
		flagsvalue = 0x8000;
		id = tsinfo->video_id;
	}
	else
	{
		flagsvalue = 0x0001;
		id = tsinfo->audio_id;
	}

    long long dts = 0;

	long long tmppts = pts;//pFrameHead->pts;
    tmppts = pts*90;
    dts = tmppts;
    return mpeg_ts_write(tsinfo->tshandle,id, flagsvalue, dts, dts, data, datalen);
	
return 0;
}

int gm_ts_stop(void *handle)
{
    if (handle == NULL)
	{
		return -1;
	}

    gm_ts_info_t *tsinfo = NULL;
	tsinfo = (gm_ts_info_t *)handle;
	mpeg_ts_destroy(tsinfo->tshandle);

	fclose((FILE*)tsinfo->fp);
	tsinfo->fp = NULL;
	
    free(tsinfo);
	tsinfo = NULL;

	return 0;
}


typedef enum {
	GM_TS_VIDEO_H264 = 0,										/**< 0 */
    GM_TS_VIDEO_H265 = 1,
	GM_TS_VIDEO_YUV  = 2,
	GM_TS_VIDEO_JPEG = 3,

	GM_TS_AUDIO_PCM = 4,
	GM_TS_AUDIO_AAC = 5,
    GM_TS_AUDIO_G711A = 6,
    GM_TS_AUDIO_G711U = 7,
}Gmlink_TS_MeidaFormat_e;

void *gm_ts_start(char *filepath,int videoformat,int audioformat)
{
	if (!filepath) {
		printf("[%s][%d]invalid param!\r\n", __func__, __LINE__);
		return NULL;
	}

	gm_ts_info_t *tsinfo = NULL;
	tsinfo = malloc(sizeof(gm_ts_info_t));
	if (tsinfo == NULL)
	{
		return NULL;
	}

	memset(tsinfo,0,sizeof(gm_ts_info_t));

	// if (filepath == NULL)
	// {
	// 	free(tsinfo);
	// 	return NULL;
	// }
	tsinfo->fp = fopen(filepath, "wb");
	if (tsinfo->fp == NULL)
	{
		free(tsinfo);
		tsinfo = NULL;
		return NULL;
	}
	
	struct mpeg_ts_func_t tshandler;
	tshandler.alloc = ts_alloc;
	tshandler.write = ts_write;
	tshandler.free = ts_free;

	tsinfo->tshandle = mpeg_ts_create(&tshandler, tsinfo->fp);
#if GM_FDK_AAC
    tsinfo->audio_id = mpeg_ts_add_stream(tsinfo->tshandle, PSI_STREAM_AAC, NULL, 0);
#else
    if (audioformat == GM_TS_AUDIO_G711A)
    {
        tsinfo->audio_id = mpeg_ts_add_stream(tsinfo->tshandle, PSI_STREAM_AUDIO_G711A, NULL, 0);
    }
    else if (audioformat == GM_TS_AUDIO_G711U)
    {

        tsinfo->audio_id = mpeg_ts_add_stream(tsinfo->tshandle, PSI_STREAM_AUDIO_G711U, NULL, 0);
    }
    else
    {
        tsinfo->audio_id = mpeg_ts_add_stream(tsinfo->tshandle, PSI_STREAM_AAC, NULL, 0);
    }
#endif
	
	if(videoformat == GM_TS_VIDEO_H264)//h264
	{
		tsinfo->video_id = mpeg_ts_add_stream(tsinfo->tshandle, PSI_STREAM_H264, NULL, 0);
	}
	else//h265
	{
        tsinfo->video_id = mpeg_ts_add_stream(tsinfo->tshandle, PSI_STREAM_H265, NULL, 0);
	}

    return tsinfo;
}
