#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include "RinoRecordPlayBack.h"
#include "RinoLink_API.h"




#pragma pack(1)
typedef struct
{
    unsigned long      filelencount;
	unsigned int       I_Frame_Count;
	unsigned int       I_Frame_interval;
    unsigned int       video_rate;
    unsigned long      I_Frame_Offset[256];
	unsigned short     video_width;
    unsigned short     video_heigh;
    unsigned short     video_format; 
    unsigned short     video_gop;

    unsigned short     audio_sample;//8000,16000,32000
    unsigned short     audio_rate;//8,16
    unsigned short     audio_channel;//1,2
    unsigned short     audio_format;//MP4_AUDIO_FORMAT
}P_File_Info_t;
#pragma pack()

#pragma pack(1)
typedef struct
{
    P_File_Info_t fileinfo;
	void *fp;
    long long audio_pts;
    long long video_pts;
}P_File_t;
#pragma pack()

#pragma pack(1)
typedef struct
{
	unsigned long long  frame_pts;
	unsigned int        frame_size;
	unsigned short      frame_type;   //0:P,1:I
	unsigned short      frame_Format; //0:264,1:265,2:AAC,3:G711a
	char                *frame_Buff;

}FrameInfo_t;
#pragma pack()

#pragma pack(1)
typedef struct __Record_MediaInfo_t
{
    unsigned short     video_width;   // 对齐
    unsigned short     video_heigh;
    unsigned short     video_format;  //MP4_VIDEO_FORMAT  
    unsigned short     video_rate;  

    unsigned short     audio_sample;  //8000,16000,32000
    unsigned short     audio_rate;    //8,16
    unsigned short     audio_channel; //1,2
    unsigned short     audio_format;  //MP4_AUDIO_FORMAT
}R_MediaInfo_t;
#pragma pack()

#pragma pack(1)
typedef struct __R_Time_t 
{
	unsigned short tm_sec;
	unsigned short tm_min;
	unsigned short tm_hour;
	unsigned short tm_mday;
	unsigned short tm_mon;
	unsigned short tm_year;
	long long      pts;
}R_Time_t;
#pragma pack()


#pragma pack(1)
typedef struct __RR_FileInfo_t
{
	R_Time_t            starttime;		//开始时间
	R_Time_t            endtime;		//结束时间
	unsigned char            recordtype;	//录像类型
	unsigned char            eventtype;		//事件类型
	unsigned short           timesCount;	//录像长度
	char                     filename[128];	//录像文件名
	R_MediaInfo_t       mediainfo;		//记录对应录像文件的视频和音频的信息
	unsigned short           fileindex;		//这个文件在索引文件列表的序号
	unsigned char            isencrypt;  	//是否进行加密
	char                     rsv3[5];		//预留
}R_FileInfo_t;
#pragma pack()

void Rinolink_QueryRecordByMonth(int channel,char *month,char *result)
{
   printf("=======Rinolink_QueryRecordByMonth  month:%s=======================\n",month);
   if (result == NULL)
   {
	   return -1;
   }

    char indexPath[128] = {0};
    char dateBuffer[9]; // 用于从date.index文件中提取日期，如"20230823\0"
    int i, len;
    int count = 0;

    // 构造date.index文件的路径
    sprintf(indexPath, "%s/EventRecord/chan_%d/date.index", "/tmp/mmcblk0p1", channel);

    // 打开文件
    FILE *fp = fopen(indexPath, "r");
    if (fp == NULL) {
        perror("Error opening file");
        return -1;
    }

    // 读取文件并检查每个日期
   while (!feof(fp)) 
   {
		if (fread(dateBuffer, 1, 8, fp) == 8) {
			dateBuffer[8] = '\0'; // 终止字符串
			if (strncmp(dateBuffer, month, 6) == 0) { // 判断日期是否匹配给定的月份
				if (count == 0) {
					sprintf(result, "%s", dateBuffer+6); // 只取日部分
					count++;
				} else {
					char day[4] = {0};
					strncpy(day, dateBuffer+6, 2);
					strcat(result, ",");
					strcat(result, day);
				}
			}
		}
	}	


    fclose(fp);
    printf("rino_QueryRecordByMonth %s\n", result);
}


void Rinolink_QueryRecordByday(int channel,char *day,char *result)
{
   printf("=======Rinolink_QueryRecordByday  day:%s=======================\n",day);
   if (result == NULL || day == NULL)
   {
	   return -1;
   }

  	FILE *pf = NULL;
	char recordpath[128] = {0};
	int  data[32];
	int  i = 0;
	int count = 0;
    int ret = 0;
	memset(recordpath,0,128);
	sprintf(recordpath,"%s/EventRecord/chan_%d/%s/indexfile.bin","/tmp/mmcblk0p1",channel,day);
	printf("rino_QueryRecordByday recordpath:%s\n",recordpath);
    pf = fopen(recordpath,"r");
    R_FileInfo_t fileInfo = {0};
  
	if (pf != NULL)
	{
		while (1)
		{
			ret = readRecordFromFile(pf,&fileInfo);
			if (ret < 0)
			{
				if (ret == -2)
				{
					break;
				}
				
				continue;;
			}
			else
			{
				int offset = fileInfo.endtime.pts - fileInfo.starttime.pts;//处理时间跨度大的非法数据
				if (fileInfo.starttime.pts/1000 <= 0 || fileInfo.endtime.pts/1000 <= 0 || access(fileInfo.filename, F_OK) != 0 || offset/1000 > 240)//文件不存在
				{
					continue;
				}
				
				if (count == 0)
				{
					sprintf(result,"%lld~%lld~%d",fileInfo.starttime.pts/1000,fileInfo.endtime.pts/1000,fileInfo.eventtype);
					count++;
				}
				else
				{
					char day[64] = {0};
					sprintf(day,"|%lld~%lld~%d",fileInfo.starttime.pts/1000,fileInfo.endtime.pts/1000,fileInfo.eventtype);
					strcat(result,day);
				}
			}
		}

		fclose(pf);
	}
	else
	{
		printf("rino_QueryRecordByday pf == NULL\n");
	}
}

int DeleteRecordByTimes(int channel,unsigned long long starttimes,unsigned long long endtimes)
{ 
	FILE *pf = NULL;
	char recordpath[128] = {0};
	int  data[32];
	int  i = 0;
	int count = 0;
    int ret = 0;
	memset(recordpath,0,128);
	struct tm *pts;
	time_t ptstime = starttimes;
	pts = localtime(&ptstime);
	sprintf(recordpath,"%s/EventRecord/chan_%d/%d%02d%02d/indexfile.bin","/tmp/mmcblk0p1",channel,pts->tm_year+1900,pts->tm_mon+1,pts->tm_mday);
	printf("rino_DeleteRecordByTimes recordpath:%s\n",recordpath);
    pf = fopen(recordpath,"r");
    R_FileInfo_t fileInfo = {0};
  
	if (pf != NULL)
	{
		while (1)
		{
			ret = readRecordFromFile(pf,&fileInfo);
			if (ret < 0)
			{
				if (ret == -2)
				{
					break;
				}
				
				continue;;
			}
			else
			{
				long long filestarttime = fileInfo.starttime.pts/1000;
				//printf("========starttimes:%lld,filestarttime:%lld,endtimes:%lld============%lld===============\n",starttimes,filestarttime,endtimes,fileInfo.starttime.pts);
				if (starttimes-2 <= filestarttime && filestarttime <= endtimes+2)
				{
					//strcpy(filepath,fileInfo.filename);
					printf("delete recordpath:%s\n",fileInfo.filename);
					remove(fileInfo.filename);
					continue;
				}
			}
		}
	    fclose(pf);
	}
   
	return 0;
}
typedef struct {
    long long start;
    long long end;
} rtime_t;
void Rinolink_DeleteRecordByTimes(int channel,char *deleteinfo)
{
	printf("=======Rinolink_DeleteRecordByTimes  deleteinfo:%s=======================\n",deleteinfo);
	if (strlen(deleteinfo) <= 0)
	{
        return -1;	    
	}
	
	char *token;
    rtime_t recordtime[256]; // 假设不超过10个参数

    int param_count = 0;
    token = strtok(deleteinfo, ",");
    
    while (token != NULL) 
	{
        long long start, end;
        sscanf(token, "%lld-%lld", &start, &end);
        recordtime[param_count].start = start;
        recordtime[param_count].end = end;
        param_count++;
		if (param_count >= 255)
		{
			break;
		}
        token = strtok(NULL, ",");
    }
    
	int i = 0 ;
	for (i = 0; i < param_count; i++) 
	{
       DeleteRecordByTimes(channel,recordtime[i].start,recordtime[i].end);
    }
}

int QueryRecordByTimes(int channel,unsigned long long times,char *filepath)
{ 
	FILE *pf = NULL;
	char recordpath[128] = {0};
	int  data[32];
	int  i = 0;
	int count = 0;
    int ret = 0;
	memset(recordpath,0,128);
	struct tm *pts;
	time_t ptstime = times;
	pts = localtime(&ptstime);
	sprintf(recordpath,"%s/EventRecord/chan_%d/%d%02d%02d/indexfile.bin","/tmp/mmcblk0p1",channel,pts->tm_year+1900,pts->tm_mon+1,pts->tm_mday);
	printf("rino_QueryRecordByTimes recordpath:%s\n",recordpath);
    pf = fopen(recordpath,"r");
    R_FileInfo_t fileInfo = {0};
  
	if (pf != NULL) 
	{
		while (1)
		{
			ret = readRecordFromFile(pf,&fileInfo);
			if (ret < 0)
			{
				if (ret == -2)
				{
					break;
				}
				
				continue;;
			}
			else
			{
				long long starttime = fileInfo.starttime.pts/1000;
				long long endtime   = fileInfo.endtime.pts/1000;
				long long querytime = times;
				if (querytime >= starttime-2 && querytime <= endtime+2)
				{
					strcpy(filepath,fileInfo.filename);
					break;
				}
			}
		}
	    fclose(pf);
	}
   
	printf("rino_QueryRecordByTimes %s\n",filepath);
	return 0;
}


void Rinolink_PlayBackStart(int channel,void *playhandle,int playtime)
{
	printf("=======Rinolink_PlayBackStart  playtime:%d=======================\n",playtime);
    char filename[256] = {0};
    QueryRecordByTimes(channel,playtime,filename);
	P_File_t *mp4_Private = NULL;
	mp4_Private =  malloc(sizeof(P_File_t));
	memset(mp4_Private,0,sizeof(P_File_t));
	mp4_Private->fp = fopen(filename, "rb");
	if (mp4_Private->fp == NULL)
	{
		free(mp4_Private);
		playhandle = playhandle;
		return ;
	}
	
	int offset = sizeof(P_File_t);
	fseek(mp4_Private->fp,-offset,SEEK_END);
	fread(&(mp4_Private->fileinfo),offset,1,mp4_Private->fp);
    fseek(mp4_Private->fp,0,SEEK_SET);
	//g_nMaxpts = (mp4_Private->fileinfo.I_Frame_interval/mp4_Private->fileinfo.video_rate)*mp4_Private->fileinfo.filelencount*1000;
    printf("= mp4_Private->fileinfo.video_rate:%d,mp4_Private->fileinfo.I_Frame_interval:%d\n",mp4_Private->fileinfo.video_rate,mp4_Private->fileinfo.I_Frame_interval);
	playhandle = mp4_Private;

}


void Rinolink_PlayBackStop(int channel,void *playhandle)
{
	 printf("=======Rinolink_PlayBackStop=======================\n");
	 if (playhandle == NULL)
	 {
		return;
	 }
	P_File_t *mp4_Private = (P_File_t *)playhandle;
	fclose(mp4_Private->fp);
	mp4_Private->fp = NULL;
    free(playhandle);
    playhandle = NULL;
	printf(" RecordReader_StopReader \n");
}


void Rinolink_PlayBacSeek(int channel,void *playhandle,int playtime)
{
	 printf("=======Rinolink_PlayBacSeek playtime:%d=======================\n",playtime);
}


void Rinolink_PlayBacGetNextOneFrame(int channel,void *playhandle,Rinolink_MediaPacket_t *frame)
{
	 printf("=======Rinolink_PlayBacGetNextOneFrame======================\n");
	 int ret = -1;
    if (playhandle == NULL || frame == NULL)
	{
		printf(" MP4Lib_Private_StopReader \n");
		return -1;
	}
	P_File_t *privatefile = (P_File_t *)playhandle;
    if (privatefile->fp == NULL)
	{
		return -1;
	}
    FrameInfo_t frameInfo = {0};

	ret = fread(&frameInfo,sizeof(FrameInfo_t),1,privatefile->fp);
	if (ret <= 0)
	{
		printf("Rinolink_PlayBacGetNextOneFrame fread(frameInfo,sizeof(PrivateFile_FrameInfo_t),1,mp4_Private->fp) fail!!! \n");
		return -2;
	}

	
	if (frameInfo.frame_size > 512*1024 || frameInfo.frame_size <= 0 )
	{
		printf(" Rinolink_PlayBacGetNextOneFrame  read file end! \n");
		return 1;//读到文件结束
	}

	ret = fread(frame->pDataBuf,frameInfo.frame_size,1,privatefile->fp);
	if (ret <= 0)
	{
		printf(" Rinolink_PlayBacGetNextOneFrame fread(g_OneFrameBuff,frameInfo.frame_size,1,privatefile.fp) fail!!! ret:%d ,%d \n",ret,frameInfo.frame_size);
		return -2;//读文件失败
	}	

	if (frameInfo.frame_type >= RINO_AUDIO_PCM)
	{
		frame->u32BufSize = frameInfo.frame_size;
		frame->u64Timestamp = frameInfo.frame_pts;
		frame->eType = RINO_AUDIO_FRAME;
		frame->Media_Packet_Info.stAudioInfo.u32Samplerate = privatefile->fileinfo.audio_sample;
		frame->Media_Packet_Info.stAudioInfo.eFormat = privatefile->fileinfo.audio_format;
	}
	else
	{
		frame->u32BufSize = frameInfo.frame_size;
		frame->u64Timestamp = frameInfo.frame_pts;
		frame->Media_Packet_Info.stVideoInfo.u32Framerate = privatefile->fileinfo.video_rate;
		frame->Media_Packet_Info.stVideoInfo.u32Width  = privatefile->fileinfo.video_width;
		frame->Media_Packet_Info.stVideoInfo.u32Height = privatefile->fileinfo.video_heigh;
		frame->Media_Packet_Info.stVideoInfo.eFormat   = frameInfo.frame_Format;
		if (frameInfo.frame_type == RINO_VIDEO_IFRAME)
		{
			//RINO_LOGD("PlayBackStream I_FRAME_TYPE\n");
			frame->eType = RINO_VIDEO_IFRAME;
		}
		else
		{				
			frame->eType = RINO_VIDEO_PFRAME;
		}
	}
	return 0;	
}


int rino_init_recordplayback()
{
    //注册录像回放相关回调函数
	Rinolink_RecordPlayBack_Handler_t  recordplaybackstatecallback;
	recordplaybackstatecallback.Rinolink_QueryRecordByMonth = Rinolink_QueryRecordByMonth;
	recordplaybackstatecallback.Rinolink_QueryRecordByday = Rinolink_QueryRecordByday;
	recordplaybackstatecallback.Rinolink_DeleteRecordByTimes = Rinolink_DeleteRecordByTimes;
	recordplaybackstatecallback.Rinolink_PlayBackStart = Rinolink_PlayBackStart;
	recordplaybackstatecallback.Rinolink_PlayBackStop = Rinolink_PlayBackStop;
	recordplaybackstatecallback.Rinolink_PlayBacSeek = Rinolink_PlayBacSeek;
	recordplaybackstatecallback.Rinolink_PlayBacGetNextOneFrame = Rinolink_PlayBacGetNextOneFrame;
	Rinolink_RegisterRecordPlayBackCallback(&recordplaybackstatecallback);

    return 0;
}