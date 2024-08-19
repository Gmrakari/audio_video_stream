
#ifndef TSBUILDER__h
#define TSBUILDER__h

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct __rion_ts_buff_t 
{
	void *buff; 
	int  buffoffset;
}rion_ts_buff_t;

typedef struct __rion_ts_info_t 
{
	void *tshandle; 
	void *fp;  
	int  audio_id;
	int  video_id;
	int  writetype;//1:写文件,2:写缓存
	rion_ts_buff_t buffinfo;
	long long audiopts;
	long long videopts;
	int audiosamplerate;
	int videoframerate;
} gm_ts_info_t;

int gm_ts_stop(void *handle);

int gm_ts_write(void *handle,int freamtype,long long pts,char *data,int datalen);

void *gm_ts_start(char *filepath,int videoformat,int audioformat);

#ifdef __cplusplus
}
#endif

#endif
