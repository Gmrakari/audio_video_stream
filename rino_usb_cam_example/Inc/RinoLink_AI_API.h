
#ifndef _RINOLINK_AI_API_H_
#define _RINOLINK_AI_API_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(1)
typedef struct { 
    char language[16]; //中文:zh-CN 英文 en-US
    char channel_name[32]; //调用start会返回
    char voice_type[16];//男声:male 女声:female
    char graph_name[64]; //可为空
} RinolinkStartWorker;
#pragma pack(0)

#pragma pack(1)
typedef struct {
    char channel_name[32];
} RinolinkStopWorker;
#pragma pack(0)


/**
 *  启动ai，成功后返回channel_name，在调用Rinolink_AI_Stopworker时需要传入该channel_name
 */
int Rinolink_AI_Startworker(RinolinkStartWorker *request);

/**
 * 停止ai
 */
int Rinolink_AI_Stopworker(RinolinkStopWorker *request);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
