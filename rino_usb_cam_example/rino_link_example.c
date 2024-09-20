#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h> 
#include <sys/time.h>   // gettimeofday
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>  	// linux gethostbyname
#include <net/if.h>
#include <sched.h>
#include <stdarg.h>  // va_start va_end
#include <dirent.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>

//#include "p2p_serve_interface.h"
#include "cJSON.h"
#include "RinoLink_API.h"
// #include "RinoLink_AI_API.h"

#include "RinoDeviceinfo.h"
#include "ntp_client.h"
#include "RinoLogUploader.h"
#define     TWO_CHANNEL      2

#ifndef __isinfl
int __isinfl(long double x) {
 return isinf(x);
}
#endif

#ifndef __isnanl
int __isnanl(long double x) {
 return isnan(x);
}
#endif

typedef struct {
	
    unsigned int 	    nChannels;  //采样频率
    unsigned int     	nstreanChanels;  		//编码格式
} streamInfo_t;


char test_token[128] = {0};

int close_mqtt(){
	sleep(5);
	Rinolink_DeInitialize();
}


long long LocalTime_ms(void) // 返回标准UTC时间，由APP跟据不同的时区来显示
{
	struct timeval tv;
	long long ms = 0;

 	gettimeofday(&tv, NULL);// UTC基准的秒, 同time()
	 ms = (long long)tv.tv_sec * (long long)1000 + (long long)tv.tv_usec / (long long)1000;

 	return ms;
}

static void* RinoServer_SendEvent_Porc(void *args)
{   
	
    char pr_name[64];
    memset(pr_name,0,sizeof(pr_name));
    sprintf(pr_name,"RinoServer_KeyEvent_Listen_Porc");

    while (1)
	{
		//上报一个按键事件触发录像
		Rinolink_EventInfo_t event_info;
		memset(&event_info,0,sizeof(Rinolink_EventInfo_t));
		event_info.event_pts = LocalTime_ms();//CommLib_LocalTime_ms();
		printf("LocalTime_ms:%lld \n",LocalTime_ms());
		event_info.mainevent_type = RINO_EVENT_TYPE_ALARM; //主事件EVENT_TYPE
		event_info.subevent_type = RINO_TYPE_MOTION;  //子事件LED_CONTROL_EVENT，HARDTRIGGER_TYPE,STORAGE_STATUS，OTA_STATUS
	    // sprintf(event_info.event_data,"/tmp/mmcblk0p1/1080.jpg");
	    sprintf(event_info.event_data,"./res/test_alarm.jpeg");
		event_info.is_offline = 0;
		Rinolink_EventPushData(0,&event_info);
		usleep(30*1000*1000);
    }
    
    pthread_exit(0);
}

typedef struct
{
	unsigned long long  frame_pts;
	unsigned int        frame_size;
	unsigned short      frame_type;   //0:P,1:I
	unsigned short      frame_Format; //0:264,1:265,2:AAC,3:G711a
	char                *frame_Buff;

}FrameInfo_t;

typedef struct __Audio_head_t
{
	unsigned int        len;
	unsigned int        width;
	unsigned int        Height;
	unsigned int        Video_Format;
}Audio_head_t;

FILE *pf = NULL;
static void* stream_pullAudio_Porc(void *args)
{   
    Audio_head_t frameInfo = {0};
    char *fremabuff = malloc(256*1024);
        Rinolink_MediaPacket_t  stMediaPacket;
        streamInfo_t *streaminfo = (streamInfo_t *)args;
        FILE *fp = NULL;
        long videono = 0;
        long audiono = 0;
        int ret = -1;

        printf("==========stream_pullAudio_Porc====stream_pullAudio_Porc===============\n");
again_a:        
    fp = fopen("./res/send_audio_8k_1ch.g711u", "rb");
        if (fp == NULL)
        {
                perror("Failed to open file");  // 使用perror打印错误
                printf("==========stream_pullAudio_Porc=======fp == NULL===================\n");
                return NULL;
        }

    while (1)
        {
 
       			ret = fread(fremabuff, 1, 160, fp);
                if (ret <= 0)
                {
                if (feof(fp)) 
                        {
                                printf("Reached end of file.\n");
                        }
                        else if (ferror(fp)) 
                        {
                                perror("Failed to read from file");
                        }
                        printf("=========stream_pullAudio_Porc====Read FrameInfo_tfail!!!===================\n");
						fclose(fp);
						fp = NULL;
                        goto again_a;
                }


                if (ret > 256*1024 || ret <= 0 )
                {
                        printf("==========stream_pullAudio_Porc======frameInfo.len > 256*1024  read file end!========frameInfo.len:%d===================\n",frameInfo.len);
                        fclose(fp);
                        fp = NULL;
                        goto again_a;
                }

                //ret = fread(fremabuff,frm.len,1,fp);
                if (ret <= 0)
                {
                        printf("======stream_pullAudio_Porc==========RecordReader_GetOneFrame fread(g_OneFrameBuff,frameInfo->frame_size,1,mp4_Private->fp) fail!!!=====ret:%d  ,%d==============\n",ret,frameInfo.len);
                         fclose(fp);
                        fp = NULL;
                        goto again_a;
                }
                else
                {

                        memset(&stMediaPacket, 0, sizeof(Rinolink_MediaPacket_t)); 
                        stMediaPacket.u32BufSize = ret;
                        stMediaPacket.u32Seq = audiono++;
                        stMediaPacket.u64Timestamp = LocalTime_ms();
                        stMediaPacket.eType = RINO_AUDIO_FRAME;
                        stMediaPacket.pDataBuf = fremabuff; 
    //  stMediaPacket.Media_Packet_Info.stAudioInfo.eFormat = RINO_AUDIO_PCM;
                        stMediaPacket.Media_Packet_Info.stAudioInfo.u32Samplerate = 8000;
                        stMediaPacket.Media_Packet_Info.stAudioInfo.eFormat = RINO_AUDIO_G711U;
               // printf("=========stream_pull_Porc Audio  stMediaPacket.u32BufSize:%d=====\n",stMediaPacket.u32BufSize);

                        //把音频推送倒实时流通道这里如果设备需要支持亚马逊音箱需要输入pcm格式音频
                        int result = Rinolink_AVPushData(0, 0, &stMediaPacket);
                        if (result < 0)
                        {
                                printf("======stream_pullAudio_Porc===stream_pull_Porc Audio fail!!=====\n");
                                break;
                        }

                        result = Rinolink_AVPushData(0, 1, &stMediaPacket);
                        if (result < 0)
                        {
                                printf("======stream_pullAudio_Porc===stream_pull_Porc Audio fail!!=====\n");
                                break;
                        }

                        //把音频推送倒录像流通道
                        // result = Rinolink_AVPushData(0, 1, &stMediaPacket);
#if TWO_CHANNEL
                        /**双流*/
                   		 result = Rinolink_AVPushData(1, 0, &stMediaPacket);
                        if (result < 0)
                        {
                                printf("======stream_pullAudio_Porc===stream_pull_Porc Audio fail!!=====\n");
                                break;
                        }

                         result = Rinolink_AVPushData(1, 1, &stMediaPacket);
                        if (result < 0)
                        {
                                printf("======stream_pullAudio_Porc===stream_pull_Porc Audio fail!!=====\n");
                                break;
                        }
#endif
            usleep(1000*20);
                }
        }
        
    return 0;
}

 

static void* stream_pullVideo_Porc(void *args)
{   
	
    FrameInfo_t frameInfo = {0};
    char *fremabuff = malloc(256*1024);
	Rinolink_MediaPacket_t  stMediaPacket;
	streamInfo_t *streaminfo = (streamInfo_t *)args;
	
	FILE *fp = NULL;
	long videono = 0;
	long audiono = 0;
	int ret = -1;
	printf("=========stream_pullVideo_Porc=====stream_pullVideo_Porc===============\n");
again_:	
	fp = fopen("./res/mediafile.data", "rb");
	if (fp == NULL)
	{
		printf("========stream_pullVideo_Porc=========fp == NULL===================\n");
		return NULL;
	}

    while (1)
	{

		ret = fread(&frameInfo,sizeof(FrameInfo_t),1,fp);
		if (ret <= 0)
		{
			printf("=======stream_pullVideo_Porc======Read FrameInfo_tfail!!!===================\n");
		    goto again_;
		}

		if (frameInfo.frame_size > 256*1024 || frameInfo.frame_size <= 0 )
		{
			printf("=======stream_pullVideo_Porc=====stream_pullVideo_Porc=======RecordReader_GetOneFrame  read file end!===========================\n");
			fclose(fp);
			fp = NULL;
			goto again_;
		}

		ret = fread(fremabuff,frameInfo.frame_size,1,fp);
		if (ret <= 0)
		{
			printf("=======stream_pullVideo_Porc=========RecordReader_GetOneFrame fread(g_OneFrameBuff,frameInfo->frame_size,1,mp4_Private->fp) fail!!!=====ret:%d  ,%d==============\n",ret,frameInfo.frame_size);
		    fclose(fp);
			fp = NULL;
			goto again_;
		}
		else
		{
			if (frameInfo.frame_Format < 2)
            {
				memset(&stMediaPacket, 0, sizeof(Rinolink_MediaPacket_t)); 
				stMediaPacket.u32BufSize = frameInfo.frame_size;
				stMediaPacket.u32Seq = videono++;
				stMediaPacket.u64Timestamp = LocalTime_ms();
                stMediaPacket.Media_Packet_Info.stVideoInfo.u32Framerate = 15;
				stMediaPacket.Media_Packet_Info.stVideoInfo.u32Width = 1920;
				stMediaPacket.Media_Packet_Info.stVideoInfo.u32Height = 1080;
				stMediaPacket.Media_Packet_Info.stVideoInfo.eFormat = RINO_VIDEO_H264;
				if (frameInfo.frame_type == 1)
				{
					//printf("==================stream_pull_Porc I_FRAME_TYPE======================\n");
					stMediaPacket.eType = RINO_VIDEO_IFRAME;
				}
				else
				{
					stMediaPacket.eType = RINO_VIDEO_PFRAME;
				}
				//printf("u32BufSize:%d, eType:%d, u32Seq:%d, u64Timestamp:%lld\n",stMediaPacket.u32BufSize,stMediaPacket.eType,stMediaPacket.u32Seq, stMediaPacket.u64Timestamp);
				stMediaPacket.pDataBuf = fremabuff;  
       
				//把视频推送倒实时流通道
				int result = Rinolink_AVPushData(0, 0, &stMediaPacket);
				if (result < 0)
				{
					printf("=====stream_pullVideo_Porc====stream_pull_Porc fail!!=====\n");
					break;
				}

				result = Rinolink_AVPushData(0, 1, &stMediaPacket);
				if (result < 0)
				{
					printf("=====stream_pullVideo_Porc====stream_pull_Porc fail!!=====\n");
					break;
				}

                //把视频推送倒录像流通道
				// result = Rinolink_AVPushData(0,1, &stMediaPacket); 
#if TWO_CHANNEL
				/**双流*/
		  	    result = Rinolink_AVPushData(1, 0, &stMediaPacket);
				if (result < 0)
				{
					printf("=====stream_pullVideo_Porc====stream_pull_Porc fail!!=====\n");
					break;
				}

				result = Rinolink_AVPushData(1, 1, &stMediaPacket);
				if (result < 0)
				{
					printf("=====stream_pullVideo_Porc====stream_pull_Porc fail!!=====\n");
					break;
				}
#endif
			}
		}
		usleep(66*1000);
	}
	
    return 0;
}

int sdk_StatusCallback(const Rinolink_Status_e Rinolink_status,char *data,int datalen)
{
    printf("===========sdk_StatusCallback:%d===========\n", Rinolink_status);
    if (Rinolink_status == RINOLINK_UNBIND)
    {
        printf("===========Device RINOLINK_UNBIND===========\n");
        Rinolink_BindDevice(test_token);
    }
    else if (Rinolink_status == RINOLINK_BINDED)
    {
        printf("===========Device RINOLINK_BINDED OK===========\n");
        // Rinolink_BindDevice("4umbvm");
    }
    else if (Rinolink_status == RINOLINK_DEVICEONLINE)
    {
        printf("===========Device Online===========\n");
        // Rinolink_BindDevice("4umbvm");
    }
    else if (Rinolink_status == RINOLINK_RESET)
    {
        // 解绑设备，清除设备配置，清除配网信息
        printf("===========Device Reset ===========\n");
        cJSON *propertiesData = (cJSON *)data;
        if (propertiesData != NULL)
        {
            cJSON *clearData = cJSON_GetObjectItem(propertiesData, "clearData");
            printf("clearData=%d\n", clearData->valueint);
        }
        // Rinolink_BindDevice("4umbvm");
    }
    else if (Rinolink_status == RINOLINK_CLEAN_NETWORK)
    {
        // 不解绑设备，不清除设备配置，清除配网信息
        printf("===========Device clean network===========\n");
    }
    else if (Rinolink_status == RINOLINK_CLEAN_DATA)
    {
        // 不解绑设备，清除设备配置，不清除配网信息
        printf("===========Device clean data===========\n");
    }
    else if (Rinolink_status == RINOLINK_CLOUD_CONNECT)
    {
        printf("===========Cloud Connect ===========\n");
        // Rinolink_BindDevice("4umbvm");
    }
    else if (Rinolink_status == RINOLINK_CLOUD_DISCONNECT)
    {
        printf("===========Cloud DISConnect ===========\n");
        // Rinolink_BindDevice("4umbvm");
    }
    else if (Rinolink_status == RINOLINK_OTA)
    {
        printf("==========RINOLINK_OTA===========\n");
        cJSON *propertiesData = (cJSON *)data;
        if (propertiesData == NULL)
        {
            printf("propertiesData == NULL.\n");
            return -1;
        }
        // 处理事件
        printf("OTA OTA OTA .\n");
        cJSON *url = NULL;
        cJSON *version = NULL;
        cJSON *options = NULL;
        cJSON *stable = NULL;
        cJSON *firmwareType = NULL;

        url = cJSON_GetObjectItem(propertiesData, "url");
        if (url == NULL)
        {
            printf("url == NULL\n");
            return -1;
        }

        version = version = cJSON_GetObjectItem(propertiesData, "version");
        if (version == NULL)
        {
            printf("version == NULL\n");
            return -1;
        }

        firmwareType = cJSON_GetObjectItem(propertiesData, "firmwareType");
        if (firmwareType == NULL)
        {
            printf("firmwareType == NULL\n");
            return -1;
        }

        options = cJSON_GetObjectItem(propertiesData, "options");
        if (options == NULL)
        {
            printf("options == NULL\n");
            return -1;
        }

        stable = cJSON_GetObjectItem(options, "stable");
        if (stable == NULL)
        {
            printf("stable == NULL\n");
            return -1;
        }

        Rinolink_DownloadOtaFile(url->valuestring, "/tmp/ota.bin");
    }
    return 0;
}




/**
  {
        "version":"1.0.0",//主控版本
        "wifiVersion":"0.0.0",//wifi版本  wifi类型设备时回复
        "cardNo":"5678956789",//4g卡号  4g类型设备时回复
        "authStatus":0,//授权状态 0：未授权；1：已授权
        "signal4G":66, //4G信号强度  4g类型设备时回复
        "signalWifis":[ //wifi信号强度，wifi类型设备时回复
            {
                "signal":100,//信号强度
                "name":"kddev"//wifi名称
            },
            {
                "signal":95,//信号强度
                "name":"kddev2"//wifi名称
            },
        ],
        "uuid":"xxx"   //授权uuid  已授权时回复
        "secret":"授权密钥",  //授权密钥  已授权时回复
        "mac":"授权mac地址",  //授权mac地址  已授权时回复        
    }  
*/
int  factory_test_up_settings(){ 
	    //上报设备数据   异步 
		cJSON *root = cJSON_CreateObject();   
		cJSON_AddStringToObject(root, "version", "1.0.0"); //版本号
		cJSON_AddStringToObject(root, "wifiVersion", "0.0.0"); 
		// cJSON_AddStringToObject(root, "cardNo", "5678956789");
		
		// cJSON_AddNumberToObject(root, "signal4G", 66);

		//WIFI列表
		cJSON *signalWifis = cJSON_CreateArray();
		cJSON *wifi1 = cJSON_CreateObject();
		cJSON_AddNumberToObject(wifi1, "signal", 100);
		cJSON_AddStringToObject(wifi1, "name", "kddev");
		cJSON_AddItemToArray(signalWifis, wifi1);
		cJSON *wifi2 = cJSON_CreateObject();
		cJSON_AddNumberToObject(wifi2, "signal", 95);
		cJSON_AddStringToObject(wifi2, "name", "kddev2");
		cJSON_AddItemToArray(signalWifis, wifi2);
		cJSON_AddItemToObject(root, "signalWifis", signalWifis);

		 
	    char pid[120] = {0};
		char uuid[120] = {0};
		char secret[120] = {0};
		char mac[120] = {0};
	    readDeviceinfoValueByKey("uuid",uuid,sizeof(uuid));
	    readDeviceinfoValueByKey("secret",secret,sizeof(secret));
        readDeviceinfoValueByKey("mac",mac,sizeof(mac));
	    readDeviceinfoValueByKey("pid",pid,sizeof(pid)); 


		cJSON_AddStringToObject(root, "pid", pid);
		cJSON_AddStringToObject(root, "uuid", uuid);
		cJSON_AddStringToObject(root, "secret", secret);
		cJSON_AddStringToObject(root, "mac", mac);

		if (uuid[0] == '\0'|| secret[0] == '\0'|| mac[0] == '\0'|| pid[0] == '\0') { //如果配置文件里面有空的，说明没有烧录，默认数据为空
		     cJSON_AddNumberToObject(root, "authStatus", 0); 
		}else{
			cJSON_AddNumberToObject(root, "authStatus", 1);
		}
		 
		char *jsonString = cJSON_Print(root); 
	    int ret = Rinolink_UploadFactoryTestSettings(jsonString,strlen(jsonString)); 
		free(jsonString);
		cJSON_Delete(root);
		return ret;
}


 int fatctory_test_up_prop(){
   //上报设备数据   异步 

   /**
	*   "properties":{
            "basic_device_volume":50,//音量：1~100
            "ptz_speed":10,//速度
            "ptz_x":10,//x步数值
            "ptz_y":10,//y步数值
            "basic_nightvision": 0,//红外灯 0:自动 1:关 2:开
            "nightvision_mode":"auto",//照明灯，auto,ir_mode,color_mode
            "ptz_calibration":true,//ptz校准，true / false
            "device_restart":true//设备重启
        }
   */
		cJSON *root = cJSON_CreateObject();   

 	
		cJSON *properties = cJSON_CreateObject();   
		cJSON_AddItemToObject(root, "properties",properties);
	    cJSON_AddNumberToObject(properties, "basic_device_volume", 50);
		cJSON_AddNumberToObject(properties, "ptz_speed", 10);
		cJSON_AddNumberToObject(properties, "ptz_x", 10);
		cJSON_AddNumberToObject(properties, "ptz_y", 10);
		cJSON_AddNumberToObject(properties, "basic_nightvision", 0);
		cJSON_AddStringToObject(properties, "nightvision_mode", "auto");
		cJSON_AddBoolToObject(properties, "ptz_calibration", true);
		// cJSON_AddBoolToObject(root, "device_restart", true);
 
		char *jsonString = cJSON_Print(root); 
	    int ret = Rinolink_UploadParamer(jsonString,strlen(jsonString)); 
		free(jsonString);
		cJSON_Delete(root);
	

 }

int fatctory_test_handler(char*cmdtype ,cJSON *data){
	if(strcmp(cmdtype, "settings")==0){
/** 
 *   "data":{
        "signal4G":50,//4G信号合格强度(1-100)
        "signalWifiLess":5,//WIFI信号强度合格数量不少于(1-10)
        "signalWifi":80,//WIFI合格信号强度(50-100)
        "sensorDirection":1,//sensor方向 1:正装；2：反装
        "lampPanelType":1,//灯板类型 1:红外;2:红外+暖光
        "pilotLamp":1,//指示灯个数
        "horizontalDirection":1,//水平电机方向 1:正向;2:反向
        "verticalDirection":1,//垂直电机方向 1:正向;2:反向
        "horizontalNum":275,//水平移动步数
        "verticalNum":180,//垂直移动步数
        "speed":5,//转速(1-10)
        "receiveVolume":8,//咪头接收音量(0-9)
        "lensModel":1//镜头型号 1:4mm-8mm;2:8mm-12mm
    }
*/ 
	   //保存设置参数
 	   cJSON *element = NULL;
   	   cJSON_ArrayForEach(element, data) {
                if (cJSON_IsNumber(element)) {
                    char valueStr[20];
                    snprintf(valueStr, sizeof(valueStr), "%d", element->valueint);
                    writeDeviceinfoKeyValue(element->string, valueStr);
                } else if (cJSON_IsString(element)) {
                    writeDeviceinfoKeyValue(element->string, element->valuestring);
                }  
       } 

		//属性上报
		int ret = fatctory_test_up_prop();
	   
	    //参数授权上报
	    ret = factory_test_up_settings();

		
	  

	 
		return ret;  
		 
	}else if(strcmp(cmdtype, "burn")==0){
	    /**
		 * 
	   "data":{
			"uuid":"",//uuid
			"secret":"",//secret
			"mac":""//mac
			"pid":"" //产品ID
			"mqtturl":"" //mqtt服务器地址
   		 }
		*/
	   cJSON *element = NULL;
   	   cJSON_ArrayForEach(element, data) {
        	 if (cJSON_IsNumber(element)) {
                    char valueStr[20];
                    snprintf(valueStr, sizeof(valueStr), "%d", element->valueint);
                    writeDeviceinfoKeyValue(element->string, valueStr);
      		 } else if (cJSON_IsString(element)) {
                    writeDeviceinfoKeyValue(element->string, element->valuestring);
        	 }  
       }  

	   factory_test_up_settings();//重新上报属性

	}else if(strcmp(cmdtype, "erase")==0){ //擦写三元组信息
		writeDeviceinfoKeyValue("uuid","");
		writeDeviceinfoKeyValue("secret","");
		writeDeviceinfoKeyValue("mac","");
		writeDeviceinfoKeyValue("pid","");
		writeDeviceinfoKeyValue("mqtturl","");  
        factory_test_up_settings();//重新上报属性
	}else if(strcmp(cmdtype, "testing")==0){ 

		//测试步骤：1：设备自检；2：TF卡测试；3：复位键测试；  4：4G信号测试；5：wifi信号测试；6：有线网测试  
		//模拟测试 
		Rinolink_UploadFactoryTestResult(1,"success",""); 
		sleep(1);
 
		Rinolink_UploadFactoryTestResult(2,"success",""); 
		sleep(1);
 
		Rinolink_UploadFactoryTestResult(3,"success",""); 
		sleep(1);
 
		Rinolink_UploadFactoryTestResult(4,"success",""); 
		sleep(1);
 
		Rinolink_UploadFactoryTestResult(5,"success",""); 
	 	sleep(1); 
		Rinolink_UploadFactoryTestResult(6,"success","");  
	}
 

	return OPRT_OK;



}


//定义控制信令接受回调
int sdk_CmdDataCallback(const char *cmdjData,char *response,int *responselen)
{
	printf("sdk_CmdDataCallback:%s\n",cmdjData);
	if (strcmp(cmdjData,CMD_TYPE_REG_IFRAME) == 0)
	{
		printf("==================req_key_frame=====================\n");
	}
	else if (strcmp(cmdjData,CMD_TYPE_STARTTALK) == 0)
	{
		printf("==================开始语音对讲=====================\n");
	}	
	else if (strcmp(cmdjData,CMD_TYPE_STOPTALK) == 0)
	{
		printf("==================结束语音对讲=====================\n");
	}	
	else if (strcmp(cmdjData,CMD_TYPE_BITRATE) == 0)
	{
		printf("=======================================\n");
	}	
	else if (strcmp(cmdjData,CMD_TYPE_STARTSTREAM) == 0)
	{
		printf("==================开始音视频传输(包括实时视频和录像回放)=====================\n");
	}
	else if (strcmp(cmdjData,CMD_TYPE_ENDSTREAM) == 0)
	{
		printf("==================结束音视频传输=====================\n");
	}
	else if (strcmp(cmdjData,CMD_TYPE_USER_JOIN_CHANNEL) == 0)
	{
		printf("==================用户加入通道=====================%s\n", response);
	}
	else if (strcmp(cmdjData,CMD_TYPE_USER_LEAVE_CHANNEL) == 0)
	{
		printf("==================用户离开通道=====================%s\n", response);
	}
	else if (strcmp(cmdjData,CMD_TYPE_USER_MUTE_AUDIO) == 0)
	{
		printf("==================远端用户是否推音频流=====================%s\n", response);
	}
	else if (strcmp(cmdjData,CMD_TYPE_USER_MUTE_VIDEO) == 0)
	{
		printf("==================远端用户是否推视频流=====================%s\n", response);
	}
	else//下面是处理设备厂商定义的db控制点，以osd和图像反转为例子
	{
		cJSON *cmdJson =  cJSON_Parse(cmdjData);
		if(cmdJson!=NULL)
		{
			cJSON *code = cJSON_GetObjectItem(cmdJson,"code"); 
			if(code!=NULL)
			{
				if(strcmp(code->valuestring, "factory_test")==0)
				{  //产测模式
					printf("==================factory_test=====================\n");
					cJSON *cmdtype = cJSON_GetObjectItem(cmdJson,"cmdtype");
					cJSON *data = cJSON_GetObjectItem(cmdJson,"data");
					int ret = fatctory_test_handler(cmdtype->valuestring,data);
					
					if(ret==OPRT_OK&&response!=NULL){
						strcpy(response,"OK"); 
						*responselen=strlen(response);
					}else{
						printf("==================factory_error:%d=====================\n",ret);
					}
				}

				if(strcmp(code->valuestring, "log_switch")==0)
				{ //日志设置
					printf("==================log_switch=====================\n"); 
					cJSON *data = cJSON_GetObjectItem(cmdJson,"data");
					int enable = cJSON_GetInt(data,"enable",0); //0 关闭  1打开
					int type = cJSON_GetInt(data,"type",1); //enable=1时必传，上报方式：1单次上报；2：循环上报；  
					int intervalTime = cJSON_GetInt(data,"intervalTime",1);//enable=1时必传，上传周期；分钟；1~360分钟 
					rino_handleLogUpload(enable,type,intervalTime); 
				}

				if(strcmp(code->valuestring, "time")==0)
				{ //日志设置
					printf("==================time=====================\n"); 
					cJSON *data = cJSON_GetObjectItem(cmdJson,"data");
					char* str = cJSON_PrintUnformatted(data);
					printf("===================time====================\n:%s\n", str);
				}

				if(strcmp(code->valuestring, "start_record")==0)
				{ //协同布防触发开始录制
					printf("==================start_record=====================\n"); 
					cJSON *data = cJSON_GetObjectItem(cmdJson,"data");
					char* str = cJSON_PrintUnformatted(data);
					printf("===================start_record====================\n:%s\n", str);
				}

				if(strcmp(code->valuestring, "ping")==0)
				{ //云端主动ping
					printf("==================start_ping=====================\n"); 
					cJSON *data = cJSON_GetObjectItem(cmdJson,"data");
					char* str = cJSON_PrintUnformatted(data);
					printf("===================start_ping====================\n:%s\n", str);
					char* ping_resp = "{\"networkType\":1,\"signal\":1,\"signalValue\":100}";
					Rinolink_UploadNetworkStatus(ping_resp, strlen(ping_resp));
				}
			} 
            
			//下面是处理设备厂商定义的db控制点，以osd和图像反转为例子
			cJSON *data  = NULL;
			cJSON *propertiesData  = NULL;
			data = cJSON_GetObjectItem(cmdJson,"data");   
			propertiesData = cJSON_GetObjectItem(data,"properties");
			if(propertiesData != NULL)
			{
				//控制图像反转
				cJSON *basic_flip  = NULL;
				basic_flip = cJSON_GetObjectItem(propertiesData,"basic_flip");
				if (basic_flip != NULL)
				{
				}
		
				//控制osd
				cJSON *basic_osd  = NULL;
				basic_osd = cJSON_GetObjectItem(propertiesData,"basic_osd");
				if (basic_osd != NULL)
				{
				
				}
			}
			
		} 
	    cJSON_Delete(cmdJson);

        
		//需要回放就在response填入OK
		if (response != NULL)
		{
			strcpy(response,"OK");
		}
	}

	return 0;
}

FILE *outputFile = NULL;

// 打开文件
void openOutputFile(const char *filename) {
    outputFile = fopen(filename, "wb");
    if (!outputFile) {
        perror("Failed to open file for writing");
        exit(EXIT_FAILURE);
    }
}

// 关闭文件
void closeOutputFile() {
    if (outputFile) {
        fclose(outputFile);
        outputFile = NULL;
    }
}

//定义控制对讲音频数据回调
int sdk_PushTalkDataCallback(Rinolink_MediaPacket_t *talkdata)
{
	// printf("=============sdk_PushTalkDataCallback =====len %zu\n", talkdata->u32BufSize);
	// if (outputFile && talkdata && talkdata->pDataBuf && talkdata->u32BufSize > 0) {
    //     size_t written = fwrite(talkdata->pDataBuf, 1, talkdata->u32BufSize, outputFile);
    //     if (written != talkdata->u32BufSize) {
    //         perror("Failed to write full data to file");
    //         return -1; 
    //     }
    //     printf("Written %zu bytes to output file\n", written);
    // } else {
    //     printf("No data to write or file not open\n");
    // }
	// return 0;
}



int RinoServer_Start(void)
{
    int s32Ret = 0;   

	Rinolink_SnParam_t pstSNParam = {0};
	Rinolink_SdkEnvVar_t pstSdkEnvVar = {0};
    char pid[128] = {0};
	char uuid[128] = {0};
	char secret[128] = {0};
	char mac[128] = {0};
	char storagePath[128] = {0};
    char mqtturl[128] = {0};
	char factoryPath[128] = {0};
	printAllKeyValuePairs();


	readdeviceinfo(pid,uuid,secret,mac,storagePath);

	readDeviceinfoValueByKey("token",test_token,sizeof(test_token));

    readDeviceinfoValueByKey("mqtturl",mqtturl,sizeof(mqtturl));
    readDeviceinfoValueByKey("factoryPath",factoryPath,sizeof(factoryPath));

	// storagePath[strlen(storagePath)-1] = '\0';
    memcpy(pstSNParam.pid, pid, strlen(pid));
	memcpy(pstSNParam.uuid, uuid, strlen(uuid));
	memcpy(pstSNParam.secret, secret, strlen(secret));
	memcpy(pstSNParam.mac, mac, strlen(mac));
    printf("================ssss=======rino_recordserverStart Start===============\n");
	//memcpy(pstSNParam.pid, "yozjOum31oAc5v", strlen("yozjOum31oAc5v"));
	//memcpy(pstSNParam.uuid, "rn0130D1DEEDc52f", strlen("rn0130D1DEEDc52f"));
	//memcpy(pstSNParam.secret, "6f1b49bb340f4b34921a27410ebde775", strlen("6f1b49bb340f4b34921a27410ebde775"));
	//memcpy(pstSNParam.mac, "444AD7001045", strlen("444AD7001045")); 
	//strcpy(pstSdkEnvVar.mqttserverurl,"mqtt.rinoiot.com");//服务器的地址

    pstSdkEnvVar.devType = 1; /**< 设备类型0表示ipc， 1表示bpi, 2表示nvr, 3表示基站 参考 IvyDevType_e 类型定义 */
    pstSdkEnvVar.maxChannels = 1; /**< 最大支持的通道数 */
	pstSdkEnvVar.streanChanels = 2;/**< 通道下支持的码流数量 1:单码流，2:双码流 */
    pstSdkEnvVar.upload_timeout = 3; /* 上传超时时间 3~60秒 */

	pstSdkEnvVar.isCloudBindStatus = 0; //是否依赖于云端绑定状态，一般4g设备可能需要设置为1
	pstSdkEnvVar.isMultiFrameRate = 0; //在多目情况下，帧率是否一致。

	// pstSdkEnvVar.encryptionRules = 1;

#if TWO_CHANNEL
	/**双摄像头*/ 
    pstSdkEnvVar.maxChannels = 2; /**< 最大支持的通道数 */
	pstSdkEnvVar.streanChanels = 2;/**< 通道下支持的码流数量 1:单码流，2:双码流 */
#endif	


	pstSdkEnvVar.maxdataspace = 256*3*1024;/*最小不低于512*1024*/  
	pstSdkEnvVar.maxframesize = 128*1024;/*最大单帧大小*/   
	
	memcpy(pstSdkEnvVar.country, "86", strlen("86"));


	pstSdkEnvVar.deviceparamterbuff="{\"properties\":{\"basic_device_volume\":44},\"sw_version\":\"1.0.1\"}"; 

	memcpy(pstSdkEnvVar.mqttserverurl,mqtturl, strlen(mqtturl)); 
	
    memcpy(pstSdkEnvVar.configfilePath,storagePath,strlen(storagePath));/**< SDK内部配置文件存放路径，要求可读写，掉电不丢失,不要和storagePath设置为同路径 */
	memcpy(pstSdkEnvVar.storagePath,storagePath,strlen(storagePath));/**< 存储设备路径，用于本地录像存储，如果为空表示不支持本地存储设备 */

	// memcpy(pstSdkEnvVar.factoryPath,factoryPath,strlen(factoryPath));
	printf("%s\n",pstSNParam.pid);
	printf("%s\n",pstSNParam.uuid);
	printf("%s\n",pstSNParam.secret);
	printf("%s\n",pstSNParam.mac);
    printf("%s\n",pstSdkEnvVar.storagePath);
    printf("%s\n",test_token);

	// rino_init_log_upload_config("/tmp/mmcblk0p1/app.log",1);//初始化日志上报配置
	openOutputFile("output.pcm");
    Rinolink_RegisterStatusCallback(sdk_StatusCallback);
	Rinolink_RegisterCmdCallback(sdk_CmdDataCallback);
	Rinolink_RegisterPushTalkDataCallback(sdk_PushTalkDataCallback);

    Rinolink_Initialize(&pstSNParam,&pstSdkEnvVar);

	
    usleep(5*1000*1000);

    s32Ret = 0;
	pthread_t keyThread;
	s32Ret = pthread_create(&keyThread, NULL, RinoServer_SendEvent_Porc, NULL);
	if (0 != s32Ret) {
        printf("pthread_create error:%s \n", strerror(errno));
    }
	pthread_detach(keyThread);

    streamInfo_t *streaminfo1 = malloc(sizeof(streamInfo_t));
	streamInfo_t *streaminfo2 = malloc(sizeof(streamInfo_t));
	streaminfo1->nChannels = 0;
	streaminfo1->nstreanChanels = 0;
    s32Ret = 0;
	pthread_t streamThread1;
	s32Ret = pthread_create(&streamThread1, NULL, stream_pullVideo_Porc, streaminfo1);
	if (0 != s32Ret) {
        printf("pthread_create error:%s \n", strerror(errno));
    }
	pthread_detach(streamThread1);


	streaminfo2->nChannels = 0;
	streaminfo2->nstreanChanels = 0;
    s32Ret = 0;
	pthread_t streamThread2;
	s32Ret = pthread_create(&streamThread2, NULL, stream_pullAudio_Porc, streaminfo2);
	if (0 != s32Ret) {
        printf("pthread_create error:%s \n", strerror(errno));
    }
    pthread_detach(streamThread2);

	
	//启动录像模块
	Rino_LocalRecordInfo_t recordserverinfo = {0};
	recordserverinfo.isaudiomute = 0;
	recordserverinfo.isencrypt = 0;
	recordserverinfo.recordtpye = 2;//事件录像
	recordserverinfo.Channelcount = 1;
#if TWO_CHANNEL	
	/**双流*/
	recordserverinfo.Channelcount = 2;
#endif
	strcpy(recordserverinfo.storagePath,storagePath);
	// strcpy(recordserverinfo.storagePath,"/tmp/mmcblk0p1");
	// strcpy(recordserverinfo.storagePath,"/opt/code/ext/rino_iot_embeded_sdk/RinoLink_SDK/res");
    printf("rino_recordserverStart Start\n");
	int ret = Rinolink_StartLocalRecord(&recordserverinfo);
	if (ret < 0)
	{
		printf("rino_recordserverStart  Fail !!!\n");
	}	

   // s32Ret = 0;
	//pthread_t streamThread2;
	//streaminfo2->nChannels = 0;
	//streaminfo2->nstreanChanels = 1;
	//s32Ret = pthread_create(&streamThread2, NULL, stream_pull_Porc, streaminfo2);
	//if (0 != s32Ret) {
   	//printf("pthread_create error:%s \n", strerror(errno));
    //}
	// pthread_t close_mqtt_p;
	// s32Ret = pthread_create(&close_mqtt_p, NULL, close_mqtt, NULL);
	// RinolinkStartWorker start;
	// memset(&start, 0, sizeof(RinolinkStartWorker));
	// Rinolink_AI_Startworker(&start);
	// sleep(20);
	// RinolinkStopWorker stop;
	// memset(&stop, 0, sizeof(RinolinkStopWorker));
	// strncpy(stop.channel_name, start.channel_name, strlen(start.channel_name));
	// Rinolink_AI_Stopworker(&stop);

}


int RinoServer_Stop()
{

	return 0;
}

#include <sys/epoll.h> // for epoll_create1()

int test_epoll()
{
    int epoll_fd = epoll_create1(0);

    if (epoll_fd == -1)
    {
        printf("Failed to create epoll file descriptor\n");
        return 1;
    }

    if (close(epoll_fd))
    {
        printf("Failed to close epoll file descriptor\n");
        return 1;
    }

    return 0;
}

int main(int argc, char **argv)
{
	test_epoll();
	// NtpClient_Start();
    RinoServer_Start();
	while (1)
	{
		sleep(1); 
	}

	return 0; 
}
